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


typedef struct {
    bxirng_s * rnds;               // One rnd per thread
    uint32_t max;
    uint32_t * distribution;
} task_data_s;

//static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static bxierr_p distribute(bximap_task_idx_t start,
                           bximap_task_idx_t end,
                           bximap_thrd_idx_t thread,
                           void * usr_data) {
    UNUSED(thread);
    task_data_s * task_data = usr_data;
    bxirng_p rnd = &task_data->rnds[thread];
    INFO(TEST_LOGGER, "Using rnd #"THRD_IDX_FMT" at %p, seed: %u",
         thread, rnd, rnd->seed);
    for (bximap_task_idx_t i = start; i < end; i++) {
        // Fetch 2 numbers between 0 and max
        uint32_t s = bxirng_nextint(rnd, 0, task_data->max);
        uint32_t e = bxirng_nextint(rnd, 0, task_data->max);
        //uint32_t s = bxirng_nextint_tsd(0, task_data->max);
        //uint32_t e = bxirng_nextint_tsd(0, task_data->max);
        CU_ASSERT_TRUE(s < task_data->max);
        CU_ASSERT_TRUE(e < task_data->max);

        // Store the distribution
        __sync_fetch_and_add(task_data->distribution + s, 1);
        __sync_fetch_and_add(task_data->distribution + e, 1);
        //pthread_mutex_lock(&lock);
        //task_data->distribution[s]++;
        //task_data->distribution[e]++;
        //pthread_mutex_unlock(&lock);

        if (e == s) continue;

        // We also check that for random bounds,
        // bxirng_int works as expected
        if (e < s) {
            // Reorder, s, and e.
            uint32_t tmp = s;
            s = e;
            e = tmp;
        }
        // Fetch a number between random s and e
        uint32_t n = bxirng_nextint(rnd, s, e);
        // Check
        CU_ASSERT_TRUE(n >= s && n < e);
    }
    return BXIERR_OK;
}

void test_rng(void) {
//    unsigned int max = RAND_MAX / 10000;
    uint32_t max = 25;
    uint32_t distribution[max];
    memset(distribution, 0, max * sizeof(max));
    // Get the number of threads
    bximap_thrd_idx_t threads_nb = 0;
    bxierr_p rc = bximap_init(&threads_nb);
    CU_ASSERT_TRUE(rc == BXIERR_OK);
    // Generate a random seed for each threads
    bxirng_s * rnds = NULL;
    bxirng_new_rngs(bxirng_new_seed(), (uint32_t)threads_nb, &rnds);
    task_data_s task_data = {.distribution = distribution,
                             .max = max, // number of lines
                             .rnds = rnds,
    };
//    unsigned int tries = (unsigned int) 1000 * (RAND_MAX / 25);
    uint32_t tries = 1000 * max;

    bximap_ctx_p ctx = NULL;
    rc = bximap_new(0, tries, 0, distribute, &task_data, &ctx);
    CU_ASSERT_TRUE(rc == BXIERR_OK);

    bxierr_p err = bximap_execute(ctx);
    CU_ASSERT_EQUAL(err, BXIERR_OK);
    bximap_destroy(&ctx);
    err = bximap_finalize();
    CU_ASSERT_EQUAL(err, BXIERR_OK);
    BXIFREE(rnds);
    // Compute the max columns
    // Uniform distribution means distribution[i] = 2 * tries / max on average
    // (since we try start and end, hence 2 times the normal case)
    // So we give 45 columns for distribution[i] = 2 * tries / max
    double uniform_columns = 70.0;
    char * pattern;
    size_t dummy;
    FILE * stream = open_memstream(&pattern, &dummy);
    fprintf(stream, "bxirng_nextint() distribution: \n");
    uint32_t normal = 2 * tries / max;
    double q = uniform_columns / normal;
    for (size_t i = 0; i < max; i++) {
        size_t col = (size_t) (distribution[i]*q);
        for (size_t j = 0; j < col; j++) putc('#', stream);
        int diff = (int) distribution[i] - (int) normal;
        fprintf(stream, " %u (%+d, %g%%)",
                distribution[i], diff, (float) diff / (float) normal * 100.0);
        fprintf(stream, "\n");
    }
    fclose(stream);
    TRACE(TEST_LOGGER, "%s", pattern);
    bximisc_stats_s stats;
    bximisc_stats(max, distribution, &stats);
    OUT(TEST_LOGGER,
        "min: %u, max: %u, mean: %lf, stddev: %lf",
        stats.min, stats.max, stats.mean, stats.stddev);
    BXIFREE(pattern);
}
