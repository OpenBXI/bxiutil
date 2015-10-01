/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Pierre Vigneras <pierre.vigneras@bull.net>
 # Created on: May 21, 2013
 # Contributors:
 ###############################################################################
 # Copyright (C) 2012  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */
#define _GNU_SOURCE

#include <CUnit/Basic.h>
#include <CUnit/Console.h>
#include <CUnit/Automated.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <libgen.h>
#include <time.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

// This is required to actually test the implementation

#include "misc.c"
#include "map.c"
#include "vector.c"
#include "rng.c"

SET_LOGGER(TEST_LOGGER, "bxiutil.test");
char ** ARGV = NULL;

// Actual test functions are separated into their respective test file.
// However they must be included here.

#include "test_rng.c"
#include "test_vector.c"
#include "test_stretch.c"
#include "test_map.c"
#include "test_misc.c"

#include "test_kvl.c"

/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void) {
    errno = 0;
    init_lexerSuite();
    return 0;
}

/* The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void) {
    clean_lexerSuite();
    return 0;
}

/* Test if the test have been run correctly */
int _handle_rc_code(){
    if(CU_get_error()!=0){
        return 99;
    } else {
        fprintf(stderr, "CU_get_number_of_suites_failed %d, "
                "CU_get_number_of_tests_failed %d,"
                "  CU_get_number_of_failures %d\n",
                CU_get_number_of_suites_failed(),
                CU_get_number_of_tests_failed(),
                CU_get_number_of_failures());
        if(CU_get_number_of_failures() != 0){
            return 1;
        }
    }
    return 0;
}



/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int main(int argc, char * argv[]) {
    UNUSED(argc);
    ARGV = argv;
    char * fullprogname = strdup(argv[0]);
    char * progname = basename(fullprogname);
    char * filename = bxistr_new("%s%s", progname, ".bxilog");
    char cwd[PATH_MAX];
    char * res = getcwd(cwd, PATH_MAX);
    assert(NULL != res);
    char * fullpathname = bxistr_new("%s/%s", cwd, filename);

    bxierr_p err = bxilog_init(bxilog_unit_test_config(progname, fullpathname, false));
    bxierr_abort_ifko(err);

//    bxilog_install_sighandler();

    fprintf(stderr, "Logging to file: %s\n", fullpathname);

    BXIFREE(fullprogname);
    BXIFREE(filename);
    BXIFREE(fullpathname);

    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("BXI_TestSuite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return (CU_get_error());
    }

    /* add the tests to the suite */
    if (false
        || (NULL == CU_add_test(pSuite, "test strto", test_misc_strto))
        || (NULL == CU_add_test(pSuite, "test bitarray", test_bitarray))

        || (NULL == CU_add_test(pSuite, "test vector", test_vector))
        || (NULL == CU_add_test(pSuite, "test stretch", test_stretch))

        || (NULL == CU_add_test(pSuite, "test map", test_map))
        || (NULL == CU_add_test(pSuite, "test map scheduler", test_scheduler))
        || (NULL == CU_add_test(pSuite, "test map fork", test_mapper_fork))

        || (NULL == CU_add_test(pSuite, "test rng", test_rng))

        || (NULL == CU_add_test(pSuite, "test misc_tuple2str", test_misc_tuple2str))
        || (NULL == CU_add_test(pSuite, "test min/max", test_min_max))
        || (NULL == CU_add_test(pSuite, "test mktemp", test_mktemp))
        || (NULL == CU_add_test(pSuite, "test getfilename", test_getfilename))

        || false) {
        CU_cleanup_registry();
        return (CU_get_error());
    }

    /* Run all tests using the automated interface */
    CU_set_output_filename("./report/cunit");
    CU_automated_run_tests();
    CU_list_tests_to_file();

    int rc = _handle_rc_code();

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    printf("\n");
    CU_basic_show_failures(CU_get_failure_list());
    printf("\n\n");

    int rc2 = _handle_rc_code();
    rc = rc > rc2 ? rc : rc2;

    CU_cleanup_registry();
    err = bxilog_finalize(true);
    bxierr_abort_ifko(err);

    return rc;
}
