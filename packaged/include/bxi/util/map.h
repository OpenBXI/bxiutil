/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Jean-Noel Quintin <jean-noel.quintin@bull.net>
 # Created on: Nov 29, 2013
 # Contributors:
 ###############################################################################
 # Copyright (C) 2012  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */

#ifndef BXIMAP_H_
#define BXIMAP_H_

#ifndef BXICFFI
#include <stddef.h>
#include <bxi/base/err.h>
#endif



/**
 * @file    map.h
 * @brief   Independent tasks parallel execution framework
 *
 * ### Overview
 * This module implements a framework for the parallel execution of independent tasks
 *
 * To have the best performance with thread the first issue is to map each of them on cpus.
 * This could be done as:
 * @snippet bximap.c CPU BINDING
 *
 * This module allows an easy way to balance the load between the threads for
 * the execution of a loop over the threads.
 * The following code shows how to initialize the threads the work to be done
 * and execute the different iterations:
 * @snippet bximap.c BALANCE LOAD
 *
 * The work to be done should be factorize inside a function with a specific signature:
 * @snippet bximap.c INNER LOOP
 *
 * Then when the work is finished the result of the iterations could be checked:
 * @snippet bximap.c CHECK RESULT
 *
 * Finally ending the threads:
 * @snippet bximap.c STOP THREADS
 *
 * ### Full Running Examples
 * - @link bximap.c map a loop iteration on several threads. @endlink
 */

// *********************************************************************************
// ********************************** Define  **************************************
// *********************************************************************************


#define BXIMAP_NEGATIVE_INTEGER 36471317    // Leet speak of .EGATI.EI.T
#define BXIMAP_INTERVAL_ERROR 173241322     // Leet speak of I.TER.ALERR

/**
 * The error code returned when the module has already been initialized.
 */
#define BXIMAP_INITIALIZE 1171413           // Leet speak of I.ITIA.I.E


/**
 * The error code returned when the module has not been initialized.
 */
#define BXIMAP_NOT_INITIALIZED 11714130     // Leet speak of I.ITIA.I.ED

/**
 * The error returned when a NULL context is provided as an argument.
 */
#define BXIMAP_NO_CONTEXT 9007387            // Leet speak of .O.O.TEXT
                                             // Prefixed with a '9' so it does not
                                             // start with a '0' (octal meaning!)

/**
 * The error returned when the module is already running.
 */
#define BXIMAP_RUNNING 216                   // Leet speak of R...I.G

/**
 * The error returned when some arguments are invalid.
 */
#define BXIMAP_ARG_ERROR 42632202           // Leet speak of ARGERROR


// *********************************************************************************
// ********************************** Types   **************************************
// *********************************************************************************
/**
 * The bximap context abstract data type
 */
typedef struct bximap_ctx_s_t * bximap_ctx_p;

/* Type which can hold a number of threads (or index thereof),
 * or a number of errors (or index thereof).
 * We set a theoretical limit of 2^15-1 threads/errors for this purpose.
 * We do not need to optimize for space
 */
typedef int bximap_thrd_idx_t;
#define THRD_IDX_FMT "%d"

/* Type which can hold a number of tasks (or index thereof),
 * as well as the start/end values described by each task.
 * We choose to allow integers as large as possible
 * with which we can dereference an array
 */
typedef long long bximap_task_idx_t;
#define TASK_IDX_FMT "%lld"

/* CPU indices are specified with type int,
 * like in the example of sched_setaffinity's man page
 */
typedef int bximap_cpu_idx_t;
#define CPU_IDX_FMT "%d"

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************


// *********************************************************************************
// ********************************** Interface ************************************
// *********************************************************************************


/**
 * Initialize the bximap library
 *
 *
 * if nb_threads == NULL or *nb_threads is equal to 0,
 *     the BXIMAP_NB_THREADS environment variable is used.
 * if BXIMAP_NB_THREADS isn't defined or is equal to 0,
 *     the number of physical processors is used.
 *
 * If nb_threads != NULL && *nb_threads == 0,
 *     the number of threads used is written back into *nb_threads.
 *
 * This function should be called before bximap_task
 *
 * @param[inout] nb_threads a pointer to the number of
 *               threads to use (can be NULL)
 * @return BXIERR_OK on error, anything else on error
 *
 */
bxierr_p bximap_init(bximap_thrd_idx_t * nb_threads);

/**
 * Clean all resources allocated by the library
 *
 * @return BXIERR_OK on error, anything else on error
 */
bxierr_p bximap_finalize();

/**
 * Create a new mapping
 * Map the iteration from start to end over the threads
 * each task has grain iterations to do
 * if the number of iterations isn't proportional to the grain:
 *      if the number of tasks is proportional to the number of threads
 *          the remaining work is split between the task
 *      else
 *          one task is added with the remaining work.
 *  if gain is equal to 0 the optimal size is used
 *
 * @param[in] start the first iteration to start the computation with
 * @param[in] end the last iteration to end the computation with
 * @param[in] granularity the number of iteration each thread must do
 * @param[in] func the function to use by each thread
 * @param[in] usr_data the data to pass to the function
 * @param[out] ctx_p a pointer on the created context
 *
 * @return BXIERR_OK on success, anything else on error
 */
bxierr_p bximap_new(bximap_task_idx_t start,
                    bximap_task_idx_t end,
                    bximap_task_idx_t granularity,
                    bxierr_p       (* func)(bximap_task_idx_t start,
                                            bximap_task_idx_t end,
                                            bximap_thrd_idx_t thread,
                                            void * usr_data),
                    void            * usr_data,
                    bximap_ctx_p    * ctx_p);

/**
 * Release the given bximap context.
 *
 * @param[inout] ctx_p a pointer on the bximap context to destroy
 *
 * @return BXIERR_OK on success, anything else on error
 */
bxierr_p bximap_destroy(bximap_ctx_p * ctx_p);

/**
 * Execute the work describe by the context over the initialized threads
 *
 *  WARNING bximap_init_threads should be called before
 *
 *  @param[in] context the bximap context to use
 *
 *  @return BXIERR_OK on success, anything else on error
 */
bxierr_p bximap_execute(bximap_ctx_p context);

/**
 * Return the error and the number of error
 *
 * @param[in] context the bximap context to use
 * @param[out] n a pointer on the number of errors
 * @param[out] err_p a pointer on an array of errors.
 *
 * @return BXIERR_OK on success, anything else on error.
 */
bxierr_p bximap_get_error(bximap_ctx_p context,
                          bximap_thrd_idx_t * n,
                          bxierr_p ** err_p);

/**
 * Bind the current thread on the provided cpu index.
 *
 * @param[in] cpu
 *
 * @returns BXIERR_OK when the mapping is possible,
 *          if the index is larger than the number of cpu an error is returned.
 */
bxierr_p bximap_on_cpu(bximap_cpu_idx_t cpu);

/**
 * Set a cpu mask to be used by bximap threads. This mapping should be
 * provided before calling bximap_init_threads().
 * The string could be compound of a single cpu separated by comma, or range
 * of cpu as for taskset command.
 * Example 0,4-6 will use the processors 0,4,5,6.
 *
 * @param[in] cpus
 *
 * @returns   BXIERR_OK if the string is well formed and the cpus are available
 */
bxierr_p bximap_set_cpumask(char * cpus);

/**
 * @example bximap.c
 * An example on how to use the module map.
 *
 */

#endif /* BXILOOP_H_ */
