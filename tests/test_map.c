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

bxierr_p test_function(bximap_task_idx_t start,
                       bximap_task_idx_t end,
                       bximap_thrd_idx_t thread,
                       void *usr_data) {
    int * test = (int *)usr_data;

    UNUSED(thread);
    bximap_ctx_p task = NULL;
    bxierr_p rc = bximap_new(1, 9, 1, &test_function, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    bxierr_p err = bximap_execute(task);
    CU_ASSERT_EQUAL(err->code, BXIMAP_RUNNING);
    CU_ASSERT_PTR_NOT_NULL(task);
    bximap_destroy(&task);
    bxierr_destroy(&err);
    //fprintf(stderr, "start: %d end %d\n",start,end);
    for (bximap_task_idx_t i = start; i < end; i++) {
        test[i]++;
        //fprintf(stderr, "i: %d -> %d\n", i, test[i]);
    }
    return BXIERR_OK;
}

bxierr_p test_map_schedule(bximap_task_idx_t start,
                           bximap_task_idx_t end,
                           bximap_thrd_idx_t thread,
                           void *usr_data) {
    UNUSED(start);
    UNUSED(end);
    bximap_cpu_idx_t first_cpu = (bximap_cpu_idx_t)(intptr_t)usr_data;
    DEBUG(TEST_LOGGER,
          "on thread "THRD_IDX_FMT", "
          "first cpu "CPU_IDX_FMT", "
          THRD_IDX_FMT" should equal %d\n",
          thread, first_cpu,
          (bximap_thrd_idx_t)first_cpu + thread,
          sched_getcpu());
    CU_ASSERT_EQUAL((bximap_thrd_idx_t)first_cpu + thread,
                    (bximap_thrd_idx_t)sched_getcpu());
    return BXIERR_OK;
}

#define TEST_ERR 10
bxierr_p test_function2(bximap_task_idx_t start,
                        bximap_task_idx_t end,
                        bximap_thrd_idx_t thread_id,
                        void *usr_data) {
    UNUSED(thread_id);
    int * test = (int *)usr_data;
        //fprintf(stderr, "start: %d end %d\n",start,end);
    for (bximap_task_idx_t i = start; i < end; i++) {
        test[i]++;
        //fprintf(stderr, "i: %d -> %d\n", i, test[i]);
    }
    return bxierr_simple(TEST_ERR, "test");
}


void test_scheduler(void) {
    DEBUG(TEST_LOGGER, "Starting test");
    bximap_cpu_idx_t max_nb_cpu = (bximap_cpu_idx_t)get_nprocs();

    bxierr_p err = bximap_on_cpu(65356);
    CU_ASSERT_TRUE(bxierr_isko(err));
    bxierr_destroy(&err);

    err = bximap_on_cpu(max_nb_cpu);
    CU_ASSERT_TRUE(bxierr_isko(err));
    bxierr_destroy(&err);
    errno = 0;
    if (sched_getcpu() == -1) {
        bxierr_p err = bxierr_errno("Getting the cpu calling sched_getcpu()");
        BXILOG_REPORT(TEST_LOGGER, BXILOG_WARNING, err, "Can't be call sched_getcpu");
        return;
    }

    for (bximap_cpu_idx_t i = 0; i < max_nb_cpu; i++) {
        bxierr_p err = bximap_on_cpu(i);
        CU_ASSERT_TRUE(bxierr_isok(err));
        CU_ASSERT_EQUAL(i, (bximap_cpu_idx_t)sched_getcpu());
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
    CU_ASSERT_EQUAL(err->code, BXIMAP_NEGATIVE_INTEGER);
    bxierr_destroy(&err);

    err = bximap_set_cpumask("3-1");
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMAP_INTERVAL_ERROR);
    bxierr_destroy(&err);

    err = bximap_set_cpumask("0");
    CU_ASSERT_TRUE(bxierr_isok(err));
    bximap_thrd_idx_t threads_nb = 1;

    CU_ASSERT_EQUAL(0, sched_getcpu());
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));

    if (max_nb_cpu == 1) {
        WARNING(TEST_LOGGER,
                "Only one core is available all the tests cannot be run");
        return;
    }

    bxirng_p rnd = bxirng_new(bxirng_new_seed());
    bximap_cpu_idx_t min_cpu = (bximap_cpu_idx_t)bxirng_nextint(rnd, 0, (uint32_t)max_nb_cpu - 2);
    bximap_cpu_idx_t max_cpu = (bximap_cpu_idx_t)bxirng_nextint(rnd, (uint32_t)min_cpu + 1, (uint32_t)max_nb_cpu -1);
    threads_nb = (bximap_thrd_idx_t)(max_cpu - min_cpu + 1);
    char * cpus = bxistr_new(CPU_IDX_FMT"-"CPU_IDX_FMT,
                             min_cpu, max_cpu);
    bxirng_destroy(&rnd);
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isok(err));
    BXIFREE(cpus);

    CU_ASSERT_EQUAL(min_cpu, (size_t)sched_getcpu());
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));

    bximap_ctx_p task = NULL;
    err = bximap_new(1, 10, 0, &test_map_schedule, (void*)(intptr_t)min_cpu, &task);
    CU_ASSERT_TRUE(bxierr_isok(err));
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));


    cpus = bxistr_new(CPU_IDX_FMT","CPU_IDX_FMT, min_cpu, max_cpu);
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isko(err));
    bxierr_destroy(&err);

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isok(err));
    BXIFREE(cpus);
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));

    CU_ASSERT_EQUAL(min_cpu, (bximap_cpu_idx_t)sched_getcpu());

    cpus = bxistr_new(CPU_IDX_FMT"-"CPU_IDX_FMT","CPU_IDX_FMT,
                      min_cpu, max_cpu-1, max_cpu);
    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isok(err));
    BXIFREE(cpus);
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));

    CU_ASSERT_EQUAL(min_cpu, (bximap_cpu_idx_t)sched_getcpu());
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));

    cpus = bxistr_new(CPU_IDX_FMT"-"CPU_IDX_FMT","CPU_IDX_FMT,
                      min_cpu, max_cpu, max_cpu);
    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    err = bximap_set_cpumask(cpus);
    CU_ASSERT_TRUE(bxierr_isok(err));
    BXIFREE(cpus);
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));

    CU_ASSERT_EQUAL(min_cpu, (bximap_cpu_idx_t)sched_getcpu());
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    bximap_destroy(&task);


    DEBUG(TEST_LOGGER, "End test");
}

void test_map(void) {
    DEBUG(TEST_LOGGER, "Starting test");

    bxierr_p err = bximap_finalize();
    CU_ASSERT_EQUAL(err->code, BXIMAP_NOT_INITIALIZED);
    bxierr_destroy(&err);

    bximap_ctx_p task2 = NULL;
    bxierr_p rc = bximap_new(1, 5, 0, &test_function, NULL, &task2);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    CU_ASSERT_PTR_NOT_NULL(task2);
    bximap_destroy(&task2);

    bximap_thrd_idx_t threads_nb = 0;
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
    CU_ASSERT_NOT_EQUAL(threads_nb, 0);
    err = bximap_init(&threads_nb);
    CU_ASSERT_EQUAL(err->code, BXIMAP_INITIALIZE);
    bxierr_destroy(&err);
    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(NULL)));
    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));

    int * test = bximem_calloc(10 * sizeof(*test));
    bximap_ctx_p task = NULL;
    rc = bximap_new(1, 10, 0, &test_function, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    err = bximap_execute(task);
    CU_ASSERT_EQUAL(err->code, BXIMAP_NOT_INITIALIZED);
    bxierr_destroy(&err);

    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
    bximap_thrd_idx_t n = 0;
    bxierr_p * errors = NULL;
    CU_ASSERT_TRUE(bxierr_isok(bximap_get_error(task, &n, &errors)));
    CU_ASSERT_EQUAL(n, 0);
    CU_ASSERT_EQUAL(errors, NULL);

    CU_ASSERT_EQUAL(test[0], 0);
    for (int i = 1; i < 10; i++) {
        CU_ASSERT_EQUAL(test[i], 1);
    }

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    bximap_destroy(&task);
    BXIFREE(test);

    threads_nb = 4;
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 0, &test_function, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));

    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
    CU_ASSERT_EQUAL(test[0], 0);
    //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for (int i = 1; i < 9; i++) {
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    bximap_destroy(&task);
    BXIFREE(test);
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 0, &test_function, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
    CU_ASSERT_EQUAL(test[0], 0);
    //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for (int i = 1; i < 9; i++) {
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    bximap_destroy(&task);
    BXIFREE(test);
    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));

    CU_ASSERT_TRUE(bxierr_isok(bximap_init(NULL)));
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 1, &test_function, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
    CU_ASSERT_EQUAL(test[0], 0);
    //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for (int i = 1; i < 9; i++) {
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    bximap_destroy(&task);
    BXIFREE(test);

    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 5, &test_function, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
    CU_ASSERT_EQUAL(test[0], 0);
    //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for (int i = 1; i < 9; i++) {
        //fprintf(stderr, "test i: %zu -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    bximap_destroy(&task);
    BXIFREE(test);

    // Case of spread the work among the tasks
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
    test = bximem_calloc(53 * sizeof(*test));
    rc = bximap_new(1, 48, 10, &test_function, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
    CU_ASSERT_EQUAL(test[0], 0);
    //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for (int i = 1; i < 48; i++) {
        //fprintf(stderr, "test i: %zu -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    for (int i = 48; i < 53; i++) {
        CU_ASSERT_EQUAL(test[i], 0);
    }

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    bximap_destroy(&task);
    BXIFREE(test);

    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(1, 9, 1, &test_function2, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
    n = 0;
    errors = NULL;
    CU_ASSERT_TRUE(bxierr_isok(bximap_get_error(task, &n, &errors)));
    CU_ASSERT_EQUAL(n, 8);
    CU_ASSERT_PTR_NOT_NULL(errors);
    CU_ASSERT_EQUAL(test[0], 0);
    //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for (int i = 1; i < 9; i++) {
        bxierr_p inerr = errors[i-1];
        CU_ASSERT_EQUAL(inerr->code,TEST_ERR);
        //bxierr_destroy(&inerr);
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    bximap_destroy(&task);
    BXIFREE(test);

    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
    test = bximem_calloc(10 * sizeof(*test));
    rc = bximap_new(2, 4, 0, &test_function2, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));
    CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
    CU_ASSERT_EQUAL(test[0], 0);
    CU_ASSERT_TRUE(bxierr_isok(bximap_get_error(task, &n, &errors)));
    CU_ASSERT_EQUAL(n, 2);
    CU_ASSERT_PTR_NOT_NULL(errors);
    CU_ASSERT_EQUAL(test[0], 0);
    //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
    for (int i = 2; i < 4; i++) {
        bxierr_p inerr = errors[i-2];
        CU_ASSERT_EQUAL(inerr->code, TEST_ERR);
        //bxierr_destroy(&inerr);
        //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
        CU_ASSERT_EQUAL(test[i], 1);
    }
    CU_ASSERT_EQUAL(test[9], 0);

    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    bximap_destroy(&task);
    BXIFREE(test);
    DEBUG(TEST_LOGGER, "End test");
}

void test_mapper_fork(void) {
    DEBUG(TEST_LOGGER, "Starting test");

    bximap_thrd_idx_t threads_nb = 4;
    CU_ASSERT_EQUAL(shared_info.state, MAPPER_UNSET);
    CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
    int * test = bximem_calloc(10 * sizeof(*test));
    bximap_ctx_p task = NULL;
    bxierr_p rc = bximap_new(2, 4, 0, &test_function2, test, &task);
    CU_ASSERT_TRUE(bxierr_isok(rc));

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
        bxierr_p err = bximap_execute(task);
        CU_ASSERT_EQUAL(err->code, BXIMAP_NOT_INITIALIZED);
        bxierr_destroy(&err);
        CU_ASSERT_TRUE(bxierr_isok(bximap_init(&threads_nb)));
        CU_ASSERT_EQUAL(shared_info.state, MAPPER_INITIALIZED);
        CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));
        CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
        for (int i = 2; i < 4; i++) {
            //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
            CU_ASSERT_EQUAL(test[i], 1);
        }
        for (int i = 5; i < 9; i++) {
            CU_ASSERT_EQUAL(test[9], 0);
        }
        CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
        CU_ASSERT_EQUAL(shared_info.state, MAPPER_UNSET);
        BXIFREE(test);
        bximap_destroy(&task);

        exit(EXIT_SUCCESS);
        break;
    }
    default: {  // In the parent
        CU_ASSERT_EQUAL(shared_info.state, MAPPER_INITIALIZED);
        CU_ASSERT_TRUE(bxierr_isok(bximap_execute(task)));

        CU_ASSERT_EQUAL(test[0], 0);
        //fprintf(stderr, "test i: %d -> %d and should be 0 \n", 0, test[0]);
        for (int i = 2; i < 4; i++) {
            //fprintf(stderr, "test i: %d -> %d and should be 1 \n", i, test[i]);
            CU_ASSERT_EQUAL(test[i], 1);
        }
        for (int i = 5; i < 9; i++) {
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
    CU_ASSERT_TRUE(bxierr_isok(bximap_finalize()));
    CU_ASSERT_EQUAL(shared_info.state, MAPPER_UNSET);
    BXIFREE(test);
    bximap_destroy(&task);
}

