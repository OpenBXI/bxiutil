/* -*- coding: utf-8 -*-
###############################################################################
# Author:  Quintin Jean-Noël <jean-noel.quintin@bull.net>
# Created on: 2015-02-26
# Contributors:
###############################################################################
# Copyright (C) 2018 Bull S.A.S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P. 68, 78340 Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################
*/

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


// *********************************************************************************
// ********************************** Implementation           *********************
// *********************************************************************************

void test_stretch(void) {
    bxistretch_destroy(NULL);
    bxistretch_p sarray = bxistretch_new(10, sizeof(int), 20);
    CU_ASSERT_PTR_NOT_NULL(sarray);
    int * test = bxistretch_get(sarray, 0);
    int * first_element;
    CU_ASSERT_PTR_NOT_NULL(test);
    *test = 10;
    first_element = test;
    test = bxistretch_get(sarray, 19);
    CU_ASSERT_PTR_NOT_NULL(test);
    test = bxistretch_get(sarray, 20);
    CU_ASSERT_PTR_NULL(test);
    test = bxistretch_get(sarray, 0);
    CU_ASSERT_PTR_NOT_NULL(test);

    CU_ASSERT_EQUAL(*test, 10);
    test = bxistretch_hit(sarray, 19);
    CU_ASSERT_PTR_NOT_NULL(test);
    test = bxistretch_hit(sarray, 20);
    CU_ASSERT_PTR_NOT_NULL(test);
    test = bxistretch_hit(sarray, 0);
    CU_ASSERT_PTR_NOT_NULL(test);
    CU_ASSERT_EQUAL(*test, 10);
    CU_ASSERT_EQUAL(*first_element, 10);
    test = bxistretch_hit(sarray, 300);
    CU_ASSERT_PTR_NOT_NULL(test);
    CU_ASSERT_EQUAL(*first_element, 10);


    bxistretch_destroy(&sarray);
    CU_ASSERT_PTR_NULL(sarray);

    sarray = bxistretch_new(10, sizeof(int), 2);
    CU_ASSERT_PTR_NOT_NULL(sarray);

    test = bxistretch_get(sarray, 0);
    CU_ASSERT_PTR_NOT_NULL(test);
    *test = 10;
    test = bxistretch_get(sarray, 1);
    CU_ASSERT_PTR_NOT_NULL(test);
    test = bxistretch_get(sarray, 3);
    CU_ASSERT_PTR_NULL(test);
    test = bxistretch_get(sarray, 0);
    CU_ASSERT_PTR_NOT_NULL(test);
    CU_ASSERT_EQUAL(*test, 10);

    bxistretch_destroy(&sarray);
    CU_ASSERT_PTR_NULL(sarray);

    sarray = bxistretch_new(10, sizeof(int), 0);
    CU_ASSERT_PTR_NOT_NULL(sarray);
    bxistretch_destroy(&sarray);
    CU_ASSERT_PTR_NULL(sarray);
    sarray = bxistretch_new(0, sizeof(int), 0);
    CU_ASSERT_PTR_NOT_NULL(sarray);
    test = bxistretch_get(sarray, 0);
    CU_ASSERT_PTR_NULL(test);
    test = bxistretch_get(sarray, 3);
    CU_ASSERT_PTR_NULL(test);
    bxistretch_destroy(&sarray);
    CU_ASSERT_PTR_NULL(sarray);

}

// *********************************************************************************
// ********************************** Static Functions Implementation  *************
// *********************************************************************************

