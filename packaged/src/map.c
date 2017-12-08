/* -*- coding: utf-8 -*-
###############################################################################
# Author: Jean-Noël Quintin <jean-noel.quintin@bull.net>
# Created on: Nov 29, 2013
# Contributors: Pierre Vignèras <pierre.vigneras@bull.net>
###############################################################################
# Copyright (C) 2012  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################
*/
//#define ZMQ
//#define FADD
#define _GNU_SOURCE
#include <sched.h>

#include <stdlib.h> // getenv
#include <unistd.h> // sysconf
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <bxi/base/log.h>


#include <bxi/base/mem.h>
#include <bxi/base/str.h>
#include <bxi/base/err.h>
#include <bxi/base/time.h>

#include <bxi/util/misc.h>

#include <bxi/util/vector.h>
#ifdef ZMQ
#include <bxi/base/zmq.h>
#endif

#include "bxi/util/map.h"

// *********************************************************************************
// ********************************** Defines **************************************
// *********************************************************************************

#define INITIALIZE_MSG "bximap already initialized"
#define NOT_INITIALIZED_MSG "bximap has not been initialized"
#define RUNNING_MSG "bximap is already running"
#define NO_CONTEXT_MSG "bximap got NULL context"
#define NULL_PTR_MSG "bximap got NULL context pointer"
#define ARG_ERROR_MSG "Argument Error"

typedef enum {
    MAPPER_UNSET,
    MAPPER_INITIALIZED,
    MAPPER_FORKED,
    MAPPER_FOLLOW_FORKED,
} _state_mapper;

// *********************************************************************************
// ********************************** Types ****************************************
// *********************************************************************************

typedef struct bximap_ctx_s_t{
    size_t      start;
    size_t      end;
    size_t      granularity;
    size_t      id;
    bxierr_p    (*func)(size_t start, size_t end, size_t thread, void *usr_data);
    void *      usr_data;
    bxierr_p *  tasks_error;
    size_t      next_error;
} bximap_ctx_s;


typedef struct{
    bximap_ctx_p    global_task;
    bximap_ctx_s*   tasks;
    size_t          nb_threads;
    size_t          nb_tasks;
    size_t          ended;
    pthread_t   *   threads_id;
    size_t *        threads_args;
    _state_mapper   state;
#ifndef ZMQ
    size_t          next_task;
#else
    void *          context;
    void *          zocket_pub;
    void *          zocket_tasks;
    void *          zocket_result;
#endif
} _intern_info;

// *********************************************************************************
// **************************** Static function declaration ************************
// *********************************************************************************
static void _mapper_parent_before_fork(void);
static void _mapper_parent_after_fork(void);
static void _mapper_once(void);
static bxierr_p _do_job(bximap_ctx_p task, size_t thread_id);
static void * _start_function(void* arg);
static bxierr_p _fill_vector_with_cpu(intptr_t first_cpu, intptr_t last_cpu,
                                      bxivector_p vcpu);

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************


SET_LOGGER(MAPPER_LOGGER, BXILOG_LIB_PREFIX "bxiutil.map");
pthread_once_t mapper_once_control = PTHREAD_ONCE_INIT;


#ifndef ZMQ
#ifdef FADD
pthread_mutex_t cond_mutex;
pthread_cond_t wait_work;
#else
pthread_barrier_t barrier;
#endif
#else
pthread_barrier_t zmq_barrier;
#define MAP_TASK_ZMQ_URL "inproc://map_tasks"
#define MAP_RESULT_ZMQ_URL "inproc://map_results"
#define MAP_PUB_ZMQ_URL "inproc://map_pub"
#endif


struct bximap_ctx_s_t last_task = {
    .start = 0,
    .end = 0,
    .granularity = 0,
    .func = NULL,
    .usr_data = NULL};


_intern_info shared_info = {
    NULL,
    .state = MAPPER_UNSET,
    .nb_threads = 0,
};

bxivector_p vcpus = NULL;


/* Initialize a new mapping
 * Map the iteration from start to end over the threads
 * each thread has grain iteration to do (only the last one could be shorter)
 *  if gain is equal to 0 the optimal size is used */
bxierr_p bximap_new(size_t start,
                    size_t end,
                    size_t granularity,
                    bxierr_p   (*func)(size_t start, size_t end,
                                       size_t thread,
                                       void *usr_data),
                    void * usr_data,
                    bximap_ctx_p * task_p) {

    bxiassert(NULL != task_p);
    bxiassert(start <= end && NULL != func);

    if (*task_p == NULL) *task_p = bximem_calloc(sizeof(**task_p));
    bximap_ctx_p task = *task_p;
    task->start = start;
    task->end = end;
    task->func = func;
    task->granularity = granularity;
    task->usr_data = usr_data;
    return BXIERR_OK;
}

bxierr_p bximap_destroy(bximap_ctx_p *ctx) {

    for (size_t i = 0; i < (*ctx)->next_error; i++) {
        bxierr_p err = (*ctx)->tasks_error[i];
        bxierr_destroy(&err);
    }
    BXIFREE((*ctx)->tasks_error);
    BXIFREE(*ctx);
    return BXIERR_OK;
}

bxierr_p bximap_get_error(bximap_ctx_p context, size_t *n, bxierr_p **err_p) {
    bxiassert(NULL != context);
    bxiassert(NULL != n  && NULL != err_p);

    *n = context->next_error;
    if (*n != 0) *err_p = context->tasks_error;
    return BXIERR_OK;
}

/* execute the work describe by the context */
bxierr_p bximap_execute(bximap_ctx_p context) {
    int rc = 0;
    UNUSED(rc);
    bxiassert(NULL != context);

    if (shared_info.state != MAPPER_INITIALIZED) {
        return bxierr_simple(BXIMAP_NOT_INITIALIZED, NOT_INITIALIZED_MSG);
    }

    if (shared_info.global_task != &last_task
        && shared_info.global_task != NULL) {
        return bxierr_new(BXIMAP_RUNNING,
                          NULL, NULL, NULL, NULL,
                          RUNNING_MSG);
    }

    struct timespec mapping_time;
    bxitime_get(CLOCK_MONOTONIC, &mapping_time);
    double running_duration = 0, mapping_duration = 0, tmp = 0;
    UNUSED(running_duration);
    UNUSED(tmp);

    // Split the work between the threads.
    shared_info.global_task = context;
    size_t cur_start = context->start;
    size_t granularity = context->granularity;
    if (granularity == 0){
        granularity = (context->end - context->start) / (shared_info.nb_threads);
        granularity /= 10;
        if (granularity == 0) {
            granularity++;
        }
    }
    shared_info.nb_tasks = (context->end - context->start) / granularity;
    if (shared_info.global_task->tasks_error != NULL) {
        BXIFREE(shared_info.global_task->tasks_error);
    }

    TRACE(MAPPER_LOGGER, "tasks_error allocate %zu error", shared_info.nb_tasks);
    shared_info.global_task->tasks_error = bximem_calloc(shared_info.nb_tasks *
                                                         sizeof(*shared_info.global_task->tasks_error));
    shared_info.global_task->next_error = 0;

    size_t remaining_work = (context->end - context->start) % granularity;
    if (remaining_work != 0) {
        // With this granularity some work remain
        if (shared_info.nb_tasks % shared_info.nb_threads != 0) {
            // Considering that each iteration takes the same time
            // if the number of task isn't proportional to the number of threads
            // the remaining work is done in an additional task
            // this task will be done in parallel of other larger tasks
            shared_info.nb_tasks++;
            remaining_work = 0;
        }
    }

    shared_info.tasks = bximem_calloc((size_t)shared_info.nb_tasks * sizeof(*shared_info.tasks));
    for (size_t i = 0; i < shared_info.nb_tasks; i++) {

        // Spread the remaining work among all
        // First tasks could have less
        // It's mandatory to have less work inside some tasks
        size_t additional_work = remaining_work / (shared_info.nb_tasks - i);
        if (remaining_work % (shared_info.nb_tasks - i) != 0) {
            additional_work++;
        }
        remaining_work -= additional_work;

        shared_info.tasks[i].granularity = granularity + additional_work;

        shared_info.tasks[i].func = shared_info.global_task->func;
        shared_info.tasks[i].usr_data = shared_info.global_task->usr_data;
        shared_info.tasks[i].start = cur_start;
        shared_info.tasks[i].end = cur_start + shared_info.tasks[i].granularity;
        if (shared_info.tasks[i].end > shared_info.global_task->end) {
            shared_info.tasks[i].end = shared_info.global_task->end;
        }

        shared_info.tasks[i].id = i;
        TRACE(MAPPER_LOGGER, "Task %zu start %zu end %zu granularity %zu"
              " granularity requested %zu",
              i, shared_info.tasks[i].start, shared_info.tasks[i].end,
              shared_info.tasks[i].granularity, granularity);
        cur_start += shared_info.tasks[i].granularity;
    }

    TRACE(MAPPER_LOGGER, "last task end: %zu global task end: %zu"
          " task granularity %zu nb tasks: %zu",
          shared_info.tasks[shared_info.nb_tasks - 1].end,
          shared_info.global_task->end,
          shared_info.tasks[shared_info.nb_tasks - 1].granularity,
          shared_info.nb_tasks);


#ifndef ZMQ
    shared_info.next_task  = shared_info.nb_threads;
    shared_info.ended  = 0;
#ifdef FADD
    //start the task with all the threads
    bxierr_p perr = BXIERR_OK;
    rc = 0;
    errno = 0;
    rc = pthread_mutex_lock(&cond_mutex);
    if (0 != rc) {
        perr = bxierr_errno("Error on pthread mutex lock");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, perr, "Error");
    }
    errno = 0;
    rc = pthread_cond_broadcast(&wait_work);
    if (0 != rc) {
        perr = bxierr_errno("Error on pthread cond broadcast");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, perr, "Error");
    }
    errno = 0;
    rc = pthread_mutex_unlock(&cond_mutex);
    if (0 != rc) {
        perr = bxierr_errno("Error on pthread mutex unlock");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, perr, "Error");
    }
#else
    __sync_synchronize();
    errno = 0;
    TRACE(MAPPER_LOGGER, "Enter barrier")
    rc = pthread_barrier_wait(&barrier);
    TRACE(MAPPER_LOGGER, "Leave barrier")
    if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
        bxierr_p perr = bxierr_errno("Error on pthread barrier wait");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, perr, "Error");
    }
#endif

    struct timespec running_time;

    size_t nb_iterations = shared_info.tasks[0].end
                           - shared_info.tasks[0].start;

    bxierr_p err = bxitime_get(CLOCK_MONOTONIC, &running_time);
    bxierr_p task_err = _do_job( &shared_info.tasks[0], 0);
    bxierr_p err2 = bxitime_duration(CLOCK_MONOTONIC, running_time, &tmp);
    BXIERR_CHAIN(err, err2);
    running_duration += tmp;
    if (bxierr_isko(task_err)) {
        size_t next_error = __sync_fetch_and_add(&shared_info.global_task->next_error, 1);
        TRACE(MAPPER_LOGGER, "thread:%d next_error %zu", 0, next_error);
        shared_info.global_task->tasks_error[next_error] = task_err;
    }

    size_t next_task = __sync_fetch_and_add (&shared_info.next_task, 1);
    while (next_task < shared_info.nb_tasks) {
        nb_iterations += shared_info.tasks[next_task].end
                         - shared_info.tasks[next_task].start;
        err2 = bxitime_get(CLOCK_MONOTONIC, &running_time);
        task_err = _do_job(&shared_info.tasks[next_task], 0);
        BXIERR_CHAIN(err, err2);
        double tmp_duration;
        err2 = bxitime_duration(CLOCK_MONOTONIC, running_time, &tmp_duration);
        BXIERR_CHAIN(err, err2);
        running_duration += tmp_duration;
        if (bxierr_isko(task_err)) {
            size_t next_error = __sync_fetch_and_add(&shared_info.global_task->next_error, 1);
            TRACE(MAPPER_LOGGER, "thread:%d next_error %zu", 0, next_error);
            shared_info.global_task->tasks_error[next_error] = task_err;
        }
        next_task = __sync_fetch_and_add (&shared_info.next_task, 1);
    }
    INFO(MAPPER_LOGGER, "Timing thread:%zd worked %f seconds for %zu iterations",
         (size_t)0, running_duration, nb_iterations);

#ifdef FADD
    while (shared_info.ended < shared_info.nb_threads - 1) {
        __sync_synchronize();
    }
#else
    __sync_synchronize();
    errno = 0;
    TRACE(MAPPER_LOGGER, "Enter barrier")
    rc = pthread_barrier_wait(&barrier);
    TRACE(MAPPER_LOGGER, "Leave barrier")
    if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
        err2 = bxierr_errno("Error on pthread barrier wait");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
#endif
#else

    struct timespec sending_time, receiving_time;
    double sending_duration, receiving_duration;
    bxierr_p err = bxitime_get(CLOCK_MONOTONIC, &sending_time);
    for (size_t i = 0; i < shared_info.nb_tasks; i++) {
        bxierr_p err2 = bxizmq_data_snd_zc(&shared_info.tasks[i],
                                           sizeof(shared_info.tasks[i]),
                                           shared_info.zocket_tasks, 0, 10, 10000,
                                           NULL, NULL);
        BXIERR_CHAIN(err, err2);
    }
    bxierr_p err2 = bxitime_duration(CLOCK_MONOTONIC,
                                     sending_time,
                                     &sending_duration);
    BXIERR_CHAIN(err, err2);
    err2 = bxitime_get(CLOCK_MONOTONIC, &receiving_time);
    BXIERR_CHAIN(err, err2);
    for (size_t i = 0; i < shared_info.nb_tasks; i++) {
        bxierr_p task_err = NULL;
        bxierr_p * task_err_p = &task_err;
        size_t received_size = 0;
        err2 = bxizmq_data_rcv((void **)&task_err_p, sizeof(task_err),
                               shared_info.zocket_result, 0, false,
                               &received_size);
        BXIERR_CHAIN(err, err2);
        if (bxierr_isko(task_err)) {
            size_t next_error = shared_info.global_task->next_error++;

            shared_info.global_task->tasks_error[next_error] = task_err;
        }
    }
    err2 = bxitime_duration(CLOCK_MONOTONIC,
                            receiving_time,
                            &receiving_duration);
    BXIERR_CHAIN(err, err2);
    INFO(MAPPER_LOGGER,
         "Timing ZMQ send %f seconds, recv %f seconds",
         sending_duration, receiving_duration);
#endif
    err2 = bxitime_duration(CLOCK_MONOTONIC,
                            mapping_time,
                            &mapping_duration);
    BXIERR_CHAIN(err, err2);
    INFO(MAPPER_LOGGER, "Global map timing %f seconds", mapping_duration);

    shared_info.global_task = NULL;
    BXIFREE(shared_info.tasks);
    return err;
}

/* Initialize the nb_threads threads
 * if nb_threads is equal to 0 then
 *      test the BXIMAP_NB_THREADS environnement variable
 *      if the variable isn't valid or is equal to 0
 *      the number of physical cpu will be used
 */
bxierr_p bximap_init(size_t * nb_threads) {
    if (shared_info.state == MAPPER_INITIALIZED) {
        return bxierr_simple(BXIMAP_INITIALIZE, INITIALIZE_MSG);
    }
    size_t thr_nb = nb_threads == NULL ? 0 : *nb_threads;

    struct timespec creation_time;
    bxierr_p err = BXIERR_OK;
    bxierr_p err2 = bxitime_get(CLOCK_MONOTONIC, &creation_time);
    BXIERR_CHAIN(err, err2);
    if (shared_info.state == MAPPER_FOLLOW_FORKED
        && shared_info.nb_threads != 0) {
        thr_nb = shared_info.nb_threads;
    }

    if (thr_nb == 0) {
        char * nb_threads_s = getenv("BXIMAP_NB_THREADS");
        long sys_cpu = 0;
        if (nb_threads_s != NULL){
            bxierr_p err2 = bximisc_strtol(nb_threads_s, 10, &sys_cpu);
            BXIERR_CHAIN(err, err2);
            if (bxierr_isko(err)) return err;
            TRACE(MAPPER_LOGGER,
                  "Mapper getenv returned: %s, bximisc_strtol: %ld",
                  nb_threads_s, sys_cpu);
        }
        if (sys_cpu <= 0) {
            sys_cpu = sysconf(_SC_NPROCESSORS_CONF);
            if (sys_cpu < 1) {
                WARNING(MAPPER_LOGGER,
                        "Can't detect the number of processors only"
                        " one thread will be used");
                sys_cpu = 1;
            }
            TRACE(MAPPER_LOGGER, "Mapper sysconf returned: %ld", sys_cpu);
        }
        thr_nb = (size_t)sys_cpu;
    }
    if (nb_threads != NULL) *nb_threads = thr_nb;
    INFO(MAPPER_LOGGER, "Mapper initialized %zu threads", thr_nb);

    shared_info.threads_args = bximem_calloc((size_t)thr_nb * sizeof(*shared_info.threads_args));

    shared_info.threads_id = bximem_calloc((size_t)thr_nb * sizeof(*shared_info.threads_id));
    shared_info.nb_threads = thr_nb;
    shared_info.ended = 0;
    size_t first = 0;
    int rc = 0;

#ifndef ZMQ
    first++;
    //condition use between to set of task to avoid keep the thread wake up
#ifdef FADD
    rc = 0;
    errno = 0;
    rc = pthread_mutex_init(&cond_mutex, NULL);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread mutex init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
    errno = 0;
    rc = pthread_cond_init(&wait_work, NULL);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread cond init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
#else
    errno = 0;
    rc = pthread_barrier_init(&barrier, NULL, (unsigned)thr_nb);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread barrier init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
#endif
#else
    TRACE(MAPPER_LOGGER, "Master start zmq");
    errno = 0;
    rc = pthread_barrier_init(&zmq_barrier, NULL, (unsigned) (thr_nb + 1));
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread barrier init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
    err2 = bxizmq_context_new(&shared_info.context);
    shared_info.zocket_pub = NULL;
    shared_info.zocket_tasks = NULL;
    shared_info.zocket_result = NULL;
    BXIERR_CHAIN(err, err2);
    err2 = bxizmq_zocket_bind(shared_info.context, ZMQ_PUB, MAP_PUB_ZMQ_URL,
                              NULL, &shared_info.zocket_pub);
    BXIERR_CHAIN(err, err2);
    err2 = bxizmq_zocket_bind(shared_info.context, ZMQ_PUSH, MAP_TASK_ZMQ_URL,
                              NULL, &shared_info.zocket_tasks);
    BXIERR_CHAIN(err, err2);
    err2 = bxizmq_zocket_bind(shared_info.context, ZMQ_PULL, MAP_RESULT_ZMQ_URL,
                              NULL, &shared_info.zocket_result);
    BXIERR_CHAIN(err, err2);
#endif

    for (size_t i = first; i < shared_info.nb_threads; i++) {
        shared_info.threads_args[i] = i;

        errno = 0;
        int rc = pthread_create(&shared_info.threads_id[i], NULL,
                                &_start_function,
                                &shared_info.threads_args[i]);
        if (0 != rc) {
            err2 = bxierr_errno("Error on pthread create");
            BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
        }
        TRACE(MAPPER_LOGGER, "Creation of one thread:%zu", i);
    }

#ifndef ZMQ
#ifdef FADD
    while (shared_info.ended < thr_nb - 1) __sync_synchronize();
#else
    __sync_synchronize();
    errno = 0;
    TRACE(MAPPER_LOGGER, "Enter barrier")
    rc = pthread_barrier_wait(&barrier);
    TRACE(MAPPER_LOGGER, "Leave barrier")
    if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
        err2 = bxierr_errno("Error on pthread barrier wait");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
#endif
#else
    errno = 0;
    TRACE(MAPPER_LOGGER, "Enter barrier")
    rc = pthread_barrier_wait(&zmq_barrier);
    TRACE(MAPPER_LOGGER, "Leave barrier")
    if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
        err2 = bxierr_errno("Error on pthread barrier wait");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
#endif

    rc = pthread_once( &mapper_once_control , _mapper_once);
    BXIASSERT(MAPPER_LOGGER, rc == 0);
    TRACE(MAPPER_LOGGER, "Initialization done.");

    shared_info.state = MAPPER_INITIALIZED;

    double duration;
    err2 = bxitime_duration(CLOCK_MONOTONIC, creation_time, &duration);
    BXIERR_CHAIN(err, err2);
    INFO(MAPPER_LOGGER, "Map initialization %f seconds", duration);
    return err;
}

/* clean properly the threads and liberate the memory */
bxierr_p bximap_finalize() {
    int rc = 0;
    if (shared_info.state != MAPPER_INITIALIZED) {
        return bxierr_simple(BXIMAP_NOT_INITIALIZED, NOT_INITIALIZED_MSG);
    }
    struct timespec stop_time;
    bxierr_p err = BXIERR_OK;
    bxierr_p err2 = bxitime_get(CLOCK_MONOTONIC, &stop_time);
    BXIERR_CHAIN(err, err2);

    shared_info.global_task = &last_task;
    size_t first = 0;

#ifndef ZMQ
    first++;
#ifdef FADD
    while (shared_info.ended < shared_info.nb_threads - 1) __sync_synchronize();
    rc = 0;
    errno = 0;
    rc = pthread_mutex_lock(&cond_mutex);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread cond init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
    errno = 0;
    rc = pthread_cond_broadcast(&wait_work);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread cond init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
    errno = 0;
    rc = pthread_mutex_unlock(&cond_mutex);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread cond init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
    errno = 0;
    rc = pthread_mutex_destroy(&cond_mutex);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread cond init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
    errno = 0;
    rc = pthread_cond_destroy(&wait_work);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread cond init");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
#else
    errno = 0;
    TRACE(MAPPER_LOGGER, "Enter barrier")
    rc = pthread_barrier_wait(&barrier);
    TRACE(MAPPER_LOGGER, "Leave barrier")
    if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
        err2 = bxierr_errno("Error on pthread barrier wait");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
    errno = 0;
    rc = pthread_barrier_destroy(&barrier);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread barrier destroy");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
#endif
#else
    TRACE(MAPPER_LOGGER, "Master sends last task");
    for (size_t i = 0; i < shared_info.nb_threads; i++) {
        err2 = bxizmq_data_snd_zc(&last_task,  sizeof(last_task),
                                  shared_info.zocket_pub,
                                  0, 10, 10000, NULL, NULL);
        BXIERR_CHAIN(err, err2);
    }
#endif

    for (size_t i = first; i < shared_info.nb_threads; i++) {
        void * retval;
        TRACE(MAPPER_LOGGER, "Master joins thread:%zu", i);
        errno = 0;
        rc = pthread_join(shared_info.threads_id[i], &retval);
        if (0 != rc) {
            err2 = bxierr_errno("Error on pthread barrier destroy");
            BXIERR_CHAIN(err, err2);
        } else {
            err2 = (bxierr_p)retval;

            if (bxierr_isko(err2)) {
                TRACE(MAPPER_LOGGER, "thread:%zu return BIXERR OK: %ld", i, (long)retval);
            } else {
                TRACE(MAPPER_LOGGER, "thread:%zu return error: %ld", i, (long)retval);
            }
            BXIERR_CHAIN(err, err2);
        }
    }

    BXIFREE(shared_info.threads_id);
    BXIFREE(shared_info.threads_args);
    shared_info.state = MAPPER_UNSET;

#ifdef ZMQ
    TRACE(MAPPER_LOGGER, "Master cleans zmq");
    err2 = bxizmq_zocket_destroy(shared_info.zocket_pub);
    BXIERR_CHAIN(err, err2);
    err2 = bxizmq_zocket_destroy(shared_info.zocket_tasks);
    BXIERR_CHAIN(err, err2);
    err2 = bxizmq_zocket_destroy(shared_info.zocket_result);
    BXIERR_CHAIN(err, err2);
    errno = 0;
    rc = pthread_barrier_destroy(&zmq_barrier);
    if (0 != rc) {
        err2 = bxierr_errno("Error on pthread barrier destroy");
        BXIERR_CHAIN(err, err2);
    }
    TRACE(MAPPER_LOGGER, "Master destroy ctx");
    do{
        rc = zmq_ctx_destroy(shared_info.context);
    } while (rc == -1 && zmq_errno() == EINTR);
#endif

    if (vcpus != NULL) {
        bxivector_destroy(&vcpus, NULL);
    }

    double duration;
    bxitime_duration(CLOCK_MONOTONIC, stop_time, &duration);
    BXIERR_CHAIN(err, err2);
    INFO(MAPPER_LOGGER, "Map stop %f seconds", duration);
    return BXIERR_OK;
}

bxierr_p bximap_on_cpu(size_t cpu) {
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(cpu, &cpu_mask);
    errno = 0;
    if (sched_setaffinity(0, 1, &cpu_mask) != 0) {
        return bxierr_errno("Process binding on the cpu failed (sched_setaffinity)");
    }
    return BXIERR_OK;
}

bxierr_p bximap_translate_cpumask(const char * cpus, bxivector_p * vcpus) {
    BXIASSERT(MAPPER_LOGGER, vcpus != NULL);

    *vcpus = bxivector_new(0, NULL);

    char * next_int = (char *)cpus;
    intptr_t previous_cpu = -1;
    intptr_t cpu = -1;
    while (*next_int != '\0') {
        char * int_str = next_int;
        errno = 0;
        cpu = strtol(int_str, &next_int, 10);
        if (0 != errno) {
            bxivector_destroy(vcpus, NULL);
            return bxierr_errno("Error while parsing number: '%s'", int_str);
        }
        if (next_int == int_str) {
            bxivector_destroy(vcpus, NULL);
            return bxierr_new(BXIMISC_NODIGITS_ERR,
                              strdup(int_str),
                              free,
                              NULL,
                              NULL,
                              "No digit found in '%s'",
                              int_str);
        }

        if ('-' == *next_int) {
            next_int++;
            previous_cpu = cpu;
        } else {
            if (cpu < 0) {
                bxivector_destroy(vcpus, NULL);
                return bxierr_new(BXIMAP_NEGATIVE_INTEGER, strdup(int_str),
                                  free, NULL, NULL, "Negative cpu number found %s",
                                  int_str);
            }
            if (',' == *next_int) next_int++;
            bxierr_p err = _fill_vector_with_cpu(previous_cpu, cpu, *vcpus);
            if (bxierr_isko(err)) {
                bxivector_destroy(vcpus, NULL);
                return err;
            }
            previous_cpu = -1;
        }
    }

    return BXIERR_OK;
}

bxierr_p bximap_set_cpumask(char * cpus) {
    if (shared_info.state == MAPPER_INITIALIZED) {
        return bxierr_simple(BXIMAP_INITIALIZE, INITIALIZE_MSG);
    }
    if (cpus == NULL || strcmp(cpus, "") == 0) {
        if (vcpus != NULL) {
            bxivector_destroy(&vcpus, NULL);
        }
        return BXIERR_OK;
    }

    bxierr_p err = bximap_translate_cpumask(cpus, &vcpus);
    if (vcpus != NULL && 0 < bxivector_get_size(vcpus)) {
        char * cpus_str = bxistr_new("%zd", (intptr_t)bxivector_get_elem(vcpus, 0));
        for (size_t i = 1; i < bxivector_get_size(vcpus); i++) {
            char * next_cpus = bxistr_new("%s,%zd", cpus_str,
                                          (intptr_t)bxivector_get_elem(vcpus, i));
            BXIFREE(cpus_str);
            cpus_str = next_cpus;
        }
        TRACE(MAPPER_LOGGER,
              "Conversion of %s into %zu element: [%s]",
              cpus, bxivector_get_size(vcpus), cpus_str);
        BXIFREE(cpus_str);

        size_t cpu = (size_t)bxivector_get_elem(vcpus, 0);
        TRACE(MAPPER_LOGGER,"Schedule on cpu=\"%zu\"", cpu);
        bxierr_p next = bximap_on_cpu(cpu);
        if (bxierr_isko(err)) {
            BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, next,
                          "Can't be mapped on cpu %zu", cpu);
        }
    }

    return err;
}

// *********************************************************************************
// ********************************** Static Functions  ****************************
// *********************************************************************************

void _mapper_parent_before_fork(void) {
    TRACE(MAPPER_LOGGER, "%s state:%d", __func__, shared_info.state);
    bxierr_p err = bximap_finalize();
    if (BXIMAP_NOT_INITIALIZED != err->code) {
        shared_info.state = MAPPER_FORKED;
    }
}

void _mapper_once(void) {
    TRACE(MAPPER_LOGGER, "%s state:%d", __func__, shared_info.state);
    pthread_atfork(_mapper_parent_before_fork, _mapper_parent_after_fork, NULL);
}

void _mapper_parent_after_fork(void) {
    TRACE(MAPPER_LOGGER, "%s state:%d", __func__, shared_info.state);
    if (shared_info.state != MAPPER_FORKED) return;
    shared_info.state = MAPPER_FOLLOW_FORKED;
    TRACE(MAPPER_LOGGER, "%s state:%d call init thread", __func__, shared_info.state);
    bxierr_p rc = bximap_init(0);
    BXIASSERT(MAPPER_LOGGER, rc == BXIERR_OK);
}



bxierr_p _do_job(bximap_ctx_p task, size_t thread_id){
    size_t start = task->start;
    size_t end = task->end ;
    TRACE(MAPPER_LOGGER, "start %zu, end %zu, thread_id %zu", start, end, thread_id);
    bxierr_p err = task->func(start, end, thread_id, task->usr_data);
    return err;
}

void * __start_function(void *arg) {
    size_t thread_id = *(size_t*) arg;
    bximap_ctx_p current_task = NULL;
    bxierr_p err = BXIERR_OK, err2;
    struct timespec starting_time;
    double working_time = 0;
    size_t nb_iterations = 0;
    int rc = 0;
    TRACE(MAPPER_LOGGER, "thread:%zu start", thread_id);
    if (vcpus != NULL) {
        size_t nb_cpus = bxivector_get_size(vcpus);
        size_t my_cpu = thread_id % nb_cpus;
        size_t cpu = (size_t)bxivector_get_elem(vcpus, my_cpu);
        TRACE(MAPPER_LOGGER,"Schedule on cpu=\"%zu\" thread_id=\"%zu\"", cpu, thread_id);
        bxierr_p err = bximap_on_cpu(cpu);
        if (bxierr_isko(err)) {
            BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err, "Can't be mapped on cpu %zu", cpu);
        }
    }
#ifdef ZMQ
    TRACE(MAPPER_LOGGER, "thread:%zu start zmq", thread_id);
    void *  zocket_sub_end = NULL;
    err2 = bxizmq_zocket_connect(shared_info.context, ZMQ_SUB, MAP_PUB_ZMQ_URL,
                                 &zocket_sub_end);
    BXIERR_CHAIN(err, err2);
    err2 = bxizmq_zocket_setopt(zocket_sub_end, ZMQ_SUBSCRIBE, "", 0);
    BXIERR_CHAIN(err, err2);
    void *  zocket_pull_tasks = NULL;
    err2 = bxizmq_zocket_connect(shared_info.context, ZMQ_PULL, MAP_TASK_ZMQ_URL,
                                 &zocket_pull_tasks);
    BXIERR_CHAIN(err, err2);
    void *  zocket_push_result = NULL;
    err2 = bxizmq_zocket_connect(shared_info.context, ZMQ_PUSH, MAP_RESULT_ZMQ_URL,
                                 &zocket_push_result);
    BXIERR_CHAIN(err, err2);
    zmq_pollitem_t items [] = { {zocket_pull_tasks, 0, ZMQ_POLLIN, 0 },
        {zocket_sub_end, 0, ZMQ_POLLIN, 0 }};
    errno = 0;
    TRACE(MAPPER_LOGGER, "Enter barrier")
    rc = pthread_barrier_wait(&zmq_barrier);
    TRACE(MAPPER_LOGGER, "Leave barrier")
    if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
        err = bxierr_errno("Error on pthread barrier wait");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err, "Error");
    }
#else
#ifndef FADD
    errno = 0;
    TRACE(MAPPER_LOGGER, "Enter barrier")
    rc = pthread_barrier_wait(&barrier);
    TRACE(MAPPER_LOGGER, "Leave barrier")
    if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
        err2 = bxierr_errno("Error on pthread barrier wait");
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
    }
#endif
#endif

    TRACE(MAPPER_LOGGER, "started");

    while (true) {

#ifndef ZMQ
#ifdef FADD
        errno = 0;
        rc = pthread_mutex_lock(&cond_mutex);
        if (0 != rc) {
            err2 = bxierr_errno("Error on pthread mutex lock");
            BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
        }
        shared_info.ended++;
        __sync_synchronize();
        errno = 0;
        rc = pthread_cond_wait(&wait_work, &cond_mutex);
        if (0 != rc) {
            err2 = bxierr_errno("Error on pthread cond wait");
            BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
        }
        errno = 0;
        rc = pthread_mutex_unlock(&cond_mutex);
        if (0 != rc) {
            err2 = bxierr_errno("Error on pthread mutex unlock");
            BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
        }
#else
        errno = 0;
        TRACE(MAPPER_LOGGER, "Enter barrier")
        rc = pthread_barrier_wait(&barrier);
        TRACE(MAPPER_LOGGER, "Leave barrier")
        if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
            err2 = bxierr_errno("Error on pthread barrier wait");
            BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
        }
#endif
#else
        int rc = zmq_poll(items, 2, -1); // -1 -> wait infinitely
        if (rc == -1) {
            if (zmq_errno() == EINTR) continue;
            return bxierr_errno("Calling zmq_poll failed.");
        }
        if (items [0].revents & ZMQ_POLLIN) {
            zmq_msg_t zmsg;
            errno = 0;
            int rc = zmq_msg_init(&zmsg);
            if (rc == -1) {
                return bxierr_errno("Calling zmq_msg_init() failed.");
            }

            rc = zmq_msg_recv(&zmsg, zocket_pull_tasks, 0);
            if (rc == -1) {
                while (rc == -1 && zmq_errno() == EINTR) {
                    rc = zmq_msg_recv(&zmsg, zocket_pull_tasks, 0);
                }
                if (rc == -1) {
                    return bxierr_errno("Can't receive a msg through zsocket: %p",
                                        zocket_pull_tasks);
                }
            }
            current_task = zmq_msg_data(&zmsg);
            zmq_msg_close(&zmsg);
        }
        // Any waiting controller command acts as 'KILL'
        if (items[1].revents & ZMQ_POLLIN) {
            break; // Exit loop
        }
#endif

        if (shared_info.global_task == &last_task) {
            TRACE(MAPPER_LOGGER, "thread:%zu got last task", thread_id);
            break;
        }
        if (shared_info.global_task == NULL) {
            TRACE(MAPPER_LOGGER, "thread:%zu got null pointer", thread_id);
            break;
        }

#ifndef ZMQ
        working_time = 0;
        nb_iterations = 0;
        if (thread_id < shared_info.nb_tasks) {
            current_task = &shared_info.tasks[thread_id];
            TRACE(MAPPER_LOGGER, "thread:%zu start task:%zu", thread_id, current_task->id);
            err2 = bxitime_get(CLOCK_MONOTONIC, &starting_time);
            BXIERR_CHAIN(err, err2);
            bxierr_p task_err = _do_job(current_task, thread_id);
            double duration;
            err2 = bxitime_duration(CLOCK_MONOTONIC, starting_time, &duration);
            BXIERR_CHAIN(err, err2);
            working_time += duration;
            nb_iterations += (current_task->end - current_task->start);
            if (bxierr_isko(task_err)) {
                size_t next_error = __sync_fetch_and_add(&shared_info.global_task->next_error, 1);
                TRACE(MAPPER_LOGGER, "thread:%zu next_error %zu", thread_id, next_error);
                shared_info.global_task->tasks_error[next_error] = task_err;
            }
        }

        size_t next_task = __sync_fetch_and_add (&shared_info.next_task, 1);
        while (next_task < shared_info.nb_tasks){
            current_task = &shared_info.tasks[next_task];
#endif
            TRACE(MAPPER_LOGGER,
                  "thread:%zu start task:%zu", thread_id, current_task->id);
            err2 = bxitime_get(CLOCK_MONOTONIC, &starting_time);
            BXIERR_CHAIN(err, err2);
            bxierr_p task_err = _do_job(current_task, thread_id);
            double tmp;
            err2 = bxitime_duration(CLOCK_MONOTONIC, starting_time, &tmp);
            BXIERR_CHAIN(err, err2);
            working_time += tmp;
            nb_iterations += (current_task->end - current_task->start);
#ifndef ZMQ
            if (bxierr_isko(task_err)) {
                size_t next_error = __sync_fetch_and_add(&shared_info.global_task->next_error, 1);
                shared_info.global_task->tasks_error[next_error] = task_err;
            }
            next_task = __sync_fetch_and_add(&shared_info.next_task, 1);
        }
        DEBUG(MAPPER_LOGGER, "Timing thread:%zu worked %f seconds for %zu iterations",
              thread_id, working_time, nb_iterations);
#ifndef FADD
        errno = 0;
        TRACE(MAPPER_LOGGER, "Enter barrier")
        rc = pthread_barrier_wait(&barrier);
        TRACE(MAPPER_LOGGER, "Leave barrier")
        if (0 != rc && PTHREAD_BARRIER_SERIAL_THREAD != rc) {
            err2 = bxierr_errno("Error on pthread barrier wait");
            BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err2, "Error");
        }
#endif
#else
        err2 = bxizmq_data_snd(&task_err, sizeof(task_err),
                               zocket_push_result, 0, 10, 10000);
        BXIERR_CHAIN(err, err2);
#endif

    }
#ifdef ZMQ
    TRACE(MAPPER_LOGGER, "thread:%zu clean zmq", thread_id);
    INFO(MAPPER_LOGGER, "Timing thread:%zu worked %f seconds for %zu iterations",
         thread_id, working_time, nb_iterations);
    err2 = bxizmq_zocket_destroy(zocket_sub_end);
    BXIERR_CHAIN(err, err2);
    err2 = bxizmq_zocket_destroy(zocket_pull_tasks);
    BXIERR_CHAIN(err, err2);
    err2 = bxizmq_zocket_destroy(zocket_push_result);
    BXIERR_CHAIN(err, err2);
#endif
    TRACE(MAPPER_LOGGER, "thread:%zu stop", thread_id);
    return err;
}

void * _start_function(void * arg) {
    bxierr_p err = __start_function(arg);
    if (bxierr_isko(err)) {
        BXILOG_REPORT(MAPPER_LOGGER, BXILOG_WARNING, err, "Error");
    }
    size_t thread_id = *(size_t *)arg;
    TRACE(MAPPER_LOGGER, "thread:%zu stop", thread_id);
    return err;
}



bxierr_p _fill_vector_with_cpu(intptr_t first_cpu, intptr_t last_cpu, bxivector_p vcpus) {
    TRACE(MAPPER_LOGGER, "first_cpu=\"%zd\" last_cpu=\"%zd\"",
          first_cpu, last_cpu);
    if (first_cpu > last_cpu) {
        return bxierr_new(BXIMAP_INTERVAL_ERROR, NULL, NULL, NULL, NULL,
                          "Interval with a greater first index %zu than last index %zu",
                          first_cpu, last_cpu);
    }
    if (-1 == first_cpu) first_cpu = last_cpu;
    for (intptr_t i = first_cpu; i <= last_cpu; i++) {
        bxivector_push(vcpus, (void *)i);
    }
    return BXIERR_OK;
}
