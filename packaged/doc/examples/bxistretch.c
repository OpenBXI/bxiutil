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

#include <unistd.h>

#include "bxi/base/err.h"
#include "bxi/base/log.h"
#include "bxi/util/stretch.h"

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

// *********************************************************************************
// ********************************** MAIN *****************************************
// *********************************************************************************
int main(int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);

    /**! [INIT STRETCH]*/
    //Allocate an extensible array for int
    //with chunk of 10 elements
    //and initialized with 10 elements
    bxistretch_p sarray = bxistretch_new(10, sizeof(int), 20);
    bxierr_p err = BXIERR_OK;

    if (sarray == NULL) {
        err=  bxierr_gen("Can't allocate a stretched array");
        BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    }
    /**! [INIT STRETCH]*/

    /**! [GET STRETCH]*/
    //Access to an allocated element
    int * test = bxistretch_get(sarray, 0);
    int * first_element;
    if (test == NULL) {
        err=  bxierr_gen("Can't get access to the first element of a stretched array");
        BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    }

    *test = 10;
    first_element = test;
    //Get the last allocated element
    test = bxistretch_get(sarray, 19);
    if (test == NULL) {
        err=  bxierr_gen("Can't get access to the first element of a stretched array");
        BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    }
    test = bxistretch_get(sarray, 20);
    if (test != NULL) {
        err=  bxierr_gen("Get return non null for a not allocated element");
        BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    }
    if (*first_element != 10) {
        err=  bxierr_gen("first element changed");
        BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    }
    /**! [GET STRETCH]*/
    test = bxistretch_get(sarray, 0);
    if (*test != 10) {
        err=  bxierr_gen("An element doesn't keep its value between two access");
        BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    }

    /**! [HIT STRETCH]*/
    //Doesn't change the number of allocated element (19 is already present)
    test = bxistretch_hit(sarray, 19);
    if (test == NULL) {
        err=  bxierr_gen("Can't get access to an allocated element with bxistretch_hit");
        BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    }
    //Extended the array to hit the 20th element
    test = bxistretch_hit(sarray, 20);
    if (test == NULL) {
        err=  bxierr_gen("Can't get access to an element with bxistretch_hit");
        BXIEXIT(EXIT_FAILURE, err, MAIN_LOGGER, BXILOG_CRITICAL);
    }
    /**! [HIT STRETCH]*/

    /**! [FREE STRETCH]*/
    bxistretch_destroy(&sarray);
    /**! [FREE STRETCH]*/

    return 0;
}
