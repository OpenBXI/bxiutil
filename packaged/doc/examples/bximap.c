/* -*- coding: utf-8 -*-
###############################################################################
# Author:  Quintin Jean-Noël <jean-noel.quintin@bull.net>
# Created on: 2015-07-09
# Contributors:
###############################################################################
# Copyright (C) 2015  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################
*/

#include "bxi/base/err.h"
#include "bxi/base/log.h"
#include "bxi/util/map.h"

// *********************************************************************************
// ********************************** Defines **************************************
// *********************************************************************************

// *********************************************************************************
// ********************************** Types ****************************************
// *********************************************************************************

// *********************************************************************************
// ********************************** Static Functions  ****************************
// *********************************************************************************

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************

SET_LOGGER(MAIN_LOGGER, "main");


// *********************************************************************************
// ********************************** Implementation           *********************
// *********************************************************************************


// *********************************************************************************
// ********************************** Static Functions Implementation  *************
// *********************************************************************************
/**! [INNER LOOP] */
/*
 * Inner loop code
 *
 * - start first iteration which should be done
 * - end last iteration (end should not be done)
 * - thread identifier of the thread
 * - data provided by user to each call (shared between threads).
 *
 * - return BXIERR_OK is every thing goes well
 *   In our example we return an error if the thread number isn't modulo 2
 */
bxierr_p test_function(bximap_task_idx_t start,
                       bximap_task_idx_t end,
                       bximap_thrd_idx_t thread,
                       void *usr_data) {
    char * str = (char *)usr_data;
    OUT(MAIN_LOGGER,
        "Thread "THRD_IDX_FMT" work on range "
        "["TASK_IDX_FMT", "TASK_IDX_FMT"[ %s",
        thread, start, end, str);
    if (thread % 2 == 0) {
        return BXIERR_OK;
    } else {
        return bxierr_gen("Thread "THRD_IDX_FMT" no modulo 2", thread);
    }
}
/**! [INNER LOOP] */

// *********************************************************************************
// ********************************** MAIN *****************************************
// *********************************************************************************
int main(int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);

    /**! [CPU BINDING] */
    //Bind the process only on cpu one
    bxierr_p err = bximap_set_cpumask("0");
    if (bxierr_isko(err)) {
        BXILOG_REPORT(MAIN_LOGGER, BXILOG_WARNING, err,
                      "Can't map on the cpu 0");
    }
    /**! [CPU BINDING] */

    /**! [BALANCE LOAD] */
    //Initialize 10 threads
    bximap_thrd_idx_t thread_nb = 10;
    err = bximap_init(&thread_nb);
    if (bxierr_isko(err)) BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);


    //Generate the work to be done
    bximap_ctx_p ctx = NULL;
    char * usr_str = "Got the data";
    err = bximap_new(0, 10, 1, &test_function, usr_str, &ctx);
    if (bxierr_isko(err)) BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);

    //Execute the work contains inside the context
    err = bximap_execute(ctx);
    if (bxierr_isko(err)) BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    /**! [BALANCE LOAD] */

    /**! [CHECK RESULT] */
    //Check the result of the iteration execution
    bximap_thrd_idx_t n = 0;
    bxierr_p * errors = NULL;
    err = bximap_get_error(ctx, &n, &errors);
    if (bxierr_isko(err)) BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    // Half of the iteration doesn't return BXIERR_OK
    if (n != 5) BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    for (bximap_thrd_idx_t i = 0; i < n; i++) {
        BXILOG_REPORT(MAIN_LOGGER, BXILOG_WARNING, errors[i],
                      "Got error from some iterations");
    }
    /**! [CHECK RESULT] */

    /**! [STOP THREADS] */
    //Stop the threads
    err = bximap_finalize();
    if (bxierr_isko(err)) BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    /**! [STOP THREADS] */
    return 0;
}
