/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Pierre Vigneras <pierre.vigneras@bull.net>
 # Created on: Oct 2, 2014
 # Contributors:
 ###############################################################################
 # Copyright (C) 2012  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */

#include <stdio.h>
bxierr_p  test_function(size_t start, size_t end, size_t thread, void *usr_data){
    int * test = (int*) usr_data;

    UNUSED(thread);
    bximap_ctx_p task = NULL;
    bxierr_p rc = bximap_new(1, 9, 1, &test_function, test, &task);
    CU_ASSERT_TRUE(rc == BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIMAP_RUNNING);
    CU_ASSERT_PTR_NOT_NULL(task);
    bximap_destroy(&task);
        //fprintf(stderr, "start: %d end %d\n",start,end);
    for(size_t i = start; i < end; i++){
        test[i]++;
        //fprintf(stderr, "i: %d -> %d\n", i, test[i]);
    }
    return BXIERR_OK;
}

bxierr_p  test_map_schedule(size_t start, size_t end, size_t thread, void *usr_data){
    UNUSED(start);
    UNUSED(end);
    //printf("%zu should equal %d\n", (size_t)usr_data + thread, sched_getcpu());
    CU_ASSERT_EQUAL((size_t)usr_data + thread, sched_getcpu());
    return BXIERR_OK;
}

bxierr_define(TEST_ERR, 10, "test");
bxierr_p  test_function2(size_t start, size_t end, size_t thread_id, void *usr_data){
    UNUSED(thread_id);
    int * test = (int*) usr_data;
        //fprintf(stderr, "start: %d end %d\n",start,end);
    for(size_t i = start; i < end; i++){
        test[i]++;
        //fprintf(stderr, "i: %d -> %d\n", i, test[i]);
    }
    return TEST_ERR;
}


void test_scheduler(void) {
    size_t max_nb_cpu = (size_t)sysconf(_SC_NPROCESSORS_ONLN);;

    bxierr_p err = bximap_on_cpu(65356);
    CU_ASSERT_TRUE(bxierr_isko(err));
    bxierr_destroy(&err);

    err = bximap_on_cpu(max_nb_cpu);
    CU_ASSERT_TRUE(bxierr_isko(err));
    bxierr_destroy(&err);

    for (size_t i=0; i < max_nb_cpu; i++) {
        bxierr_p err = bximap_on_cpu(i);
        CU_ASSERT_TRUE(bxierr_isok(err));
        CU_ASSERT_EQUAL(i, sched_getcpu());
    }

    err = bximap_set_cpumask(NULL);
    CU_ASSERT_TRUE(bxierr_isok(err));
    bxierr_destroy(&err);

    err = bximap_set_cpumask("qsq");
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_NODIGITS_ERR);
    bxierr_destroy(&err);

    err = bximap_set_cpumask("-1");
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMAP_NEGATIVE_INTERGER);
    bxierr_destroy(&err);

    err = bximap_set_cpumask("3-1");
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMAP_INTERVAL_ERROR);
    bxierr_destroy(&err);

    err = bximap_set_cpumask("0");
    CU_ASSERT_TRUE(bxierr_isok(err));
    size_t threads_nb = 1;

    CU_ASSERT_EQUAL(0, sched_getcpu());
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);

    if (max_nb_cpu == 1) {
        WARNING(TEST_LOGGER, "Only one core is available all the tests cannot be run");
        return;
    }

    bxirng_p rnd = bxirng_new(bxirng_new_seed());
    size_t min_cpu = bxirng_nextint(rnd, 0, (uint32_t)max_nb_cpu - 2);
    size_t max_cpu = bxirng_nextint(rnd, (uint32_t)min_cpu + 1, (uint32_t)max_nb_cpu -1);
    threads_nb = max_cpu - min_cpu + 1;
    char * cpus = bxistr_new("%zu-%zu", min_cpu, max_cpu);
    bxirng_destroy(&rnd);
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isok(err));
    BXIFREE(cpus);

    CU_ASSERT_EQUAL(min_cpu, sched_getcpu());
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);

    bximap_ctx_p task = NULL;
    err = bximap_new(1, 10, 0, &test_map_schedule, (void*)min_cpu, &task);
    CU_ASSERT_TRUE(bxierr_isok(err));
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);


    cpus = bxistr_new("%zu,%zu", min_cpu, max_cpu);
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isko(err));

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isok(err));
    BXIFREE(cpus);
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);

    CU_ASSERT_EQUAL(min_cpu, sched_getcpu());

    cpus = bxistr_new("%zu-%zu,%zu", min_cpu, max_cpu-1, max_cpu);
    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isok(err));
    BXIFREE(cpus);
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);

    CU_ASSERT_EQUAL(min_cpu, sched_getcpu());
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);

    cpus = bxistr_new("%zu-%zu,%zu", min_cpu, max_cpu, max_cpu);
    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isok(err));
    BXIFREE(cpus);
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);

    CU_ASSERT_EQUAL(min_cpu, sched_getcpu());
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    bximap_destroy(&task);


}

void test_map(void) {

    CU_ASSERT_EQUAL(bximap_finalize(), BXIMAP_NOT_INITIALIZED);
    bximap_ctx_p null_tasks = NULL;
    bxierr_p rc = bximap_new(1, 5, 2, NULL, NULL, &null_tasks);
    CU_ASSERT_TRUE(rc == BXIMAP_ARG_ERROR);
    CU_ASSERT_EQUAL(null_tasks, NULL);
    rc = bximap_new(10, 5, 2, &test_function, NULL, &null_tasks);
    CU_ASSERT_TRUE(rc == BXIMAP_ARG_ERROR);
    CU_ASSERT_EQUAL(null_tasks, NULL);
    bximap_ctx_p task2 = NULL;
    rc = bximap_new(1, 5, 0, &test_function, NULL, &task2);
    CU_ASSERT_TRUE(rc == BXIERR_OK);
    CU_ASSERT_PTR_NOT_NULL(task2);
    bximap_destroy(&task2);

    size_t threads_nb = 0;
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
    CU_ASSERT_NOT_EQUAL(threads_nb, 0);
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIMAP_INITIALIZE);
    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_init(NULL), BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);

    CU_ASSERT_EQUAL(bximap_execute(NULL), BXIMAP_NO_CONTEXT);
    int * test = bximem_calloc(10 * sizeof(*test));
    bximap_ctx_p task = NULL;
    rc = bximap_new(1, 10, 0, &test_function, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIMAP_NOT_INITIALIZED);


    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
    size_t n = 0;
    bxierr_p * errors = NULL;
    CU_ASSERT_EQUAL(bximap_get_error(task, &n, &errors), BXIERR_OK);
    CU_ASSERT_EQUAL(n, 0);
    CU_ASSERT_EQUAL(errors, NULL);

    CU_ASSERT_EQUAL(test[0], 0);
    for(size_t i = 1; i <10; i++){
        CU_ASSERT_EQUAL(test[i], 1);
    }

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    bximap_destroy(&task);
    BXIFREE(test);

    threads_nb = 4;
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 0, &test_function, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);

    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
    CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for(size_t i = 1; i < 9; i++){
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    bximap_destroy(&task);
    BXIFREE(test);
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 0, &test_function, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
    CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for(size_t i = 1; i < 9; i++){
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    bximap_destroy(&task);
    BXIFREE(test);
    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);

    CU_ASSERT_EQUAL(bximap_init(NULL), BXIERR_OK);
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 1, &test_function, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
    CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for(size_t i = 1; i < 9; i++){
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    bximap_destroy(&task);
    BXIFREE(test);

    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 5, &test_function, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
    CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for(size_t i = 1; i < 9; i++){
        //fprintf(stderr, "test i: %zu -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    bximap_destroy(&task);
    BXIFREE(test);

    // Case of spread the work among the tasks
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
    test = bximem_calloc(53 * sizeof(*test));
    rc = bximap_new(1, 48, 10, &test_function, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
    CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for(size_t i = 1; i < 48; i++){
        //fprintf(stderr, "test i: %zu -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    for(size_t i = 48; i < 53; i++){
        CU_ASSERT_EQUAL(test[i], 0);
    }

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    bximap_destroy(&task);
    BXIFREE(test);

    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 1, &test_function2, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
    n = 0;
    errors = NULL;
    CU_ASSERT_EQUAL(bximap_get_error(task, &n, &errors), BXIERR_OK);
    CU_ASSERT_EQUAL(n, 8);
    CU_ASSERT_PTR_NOT_NULL(errors);
    CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for(size_t i = 1; i < 9; i++){
        CU_ASSERT_EQUAL(errors[i-1],TEST_ERR);
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    bximap_destroy(&task);
    BXIFREE(test);

    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(2, 4, 0, &test_function2, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);
    CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
    CU_ASSERT_EQUAL(test[0], 0);
    CU_ASSERT_EQUAL(bximap_get_error(task, &n, &errors), BXIERR_OK);
    CU_ASSERT_EQUAL(n, 2);
    CU_ASSERT_PTR_NOT_NULL(errors);
    CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for(size_t i = 2; i < 4; i++){
        CU_ASSERT_EQUAL(errors[i-2],TEST_ERR);
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    bximap_destroy(&task);
    BXIFREE(test);
    BXIFREE(null_tasks);
}

void test_mapper_fork(void) {
    DEBUG(TEST_LOGGER, "Starting test");

    size_t threads_nb = 4;
    CU_ASSERT_EQUAL(shared_info.state, MAPPER_UNSET);
    CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
    int * test = bximem_calloc(10 * sizeof(*test));
    bximap_ctx_p task = NULL;
    bxierr_p rc = bximap_new(2, 4, 0, &test_function2, test, &task);
    CU_ASSERT_EQUAL(rc, BXIERR_OK);

    CU_ASSERT_EQUAL(shared_info.state, MAPPER_INITIALIZED);
    DEBUG(TEST_LOGGER, "Forking a child");
    errno = 0;
    pid_t cpid = fork();
    switch(cpid) {
    case -1: {
        BXIEXIT(EXIT_FAILURE,
                bxierr_errno("Can't fork()"),
                TEST_LOGGER, BXILOG_CRITICAL);
        break;
    }
    case 0: { // In the child
        CU_ASSERT_EQUAL(shared_info.state, MAPPER_UNSET);
        CU_ASSERT_EQUAL(bximap_execute(task), BXIMAP_NOT_INITIALIZED);
        CU_ASSERT_EQUAL(bximap_init(&threads_nb), BXIERR_OK);
        CU_ASSERT_EQUAL(shared_info.state, MAPPER_INITIALIZED);
        CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);
        CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
        for(size_t i = 2; i < 4; i++){
            //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
            CU_ASSERT_EQUAL(test[i], 1);
        }
        for(size_t i = 5; i < 9; i++){
            CU_ASSERT_EQUAL(test[9], 0);
        }
        CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
        CU_ASSERT_EQUAL(shared_info.state, MAPPER_UNSET);
        BXIFREE(test);
        bximap_destroy(&task);

        exit(EXIT_SUCCESS);
        break;
    }
    default: {  // In the parent
        CU_ASSERT_EQUAL(shared_info.state, MAPPER_INITIALIZED);
        CU_ASSERT_EQUAL(bximap_execute(task), BXIERR_OK);

        CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
        for(size_t i = 2; i < 4; i++){
            //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
            CU_ASSERT_EQUAL(test[i], 1);
        }
        for(size_t i = 5; i < 9; i++){
            CU_ASSERT_EQUAL(test[9], 0);
        }

        TRACE(TEST_LOGGER, "Parent: One log line");
        int status;
        errno = 0;
        pid_t w = waitpid(cpid, &status, WUNTRACED);
        if (-1 == w) {
            BXIEXIT(EXIT_FAILURE,
                    bxierr_errno("Can't wait()"),
                    TEST_LOGGER, BXILOG_CRITICAL);
        }
        CU_ASSERT_TRUE(WIFEXITED(status));
        CU_ASSERT_EQUAL_FATAL(WEXITSTATUS(status), EXIT_SUCCESS);
        DEBUG(TEST_LOGGER, "Child %d terminated", cpid);
        break;
    }
    }
    CU_ASSERT_EQUAL(bximap_finalize(), BXIERR_OK);
    CU_ASSERT_EQUAL(shared_info.state, MAPPER_UNSET);
    BXIFREE(test);
    bximap_destroy(&task);
}

