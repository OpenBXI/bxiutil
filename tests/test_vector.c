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


void test_vector(void) {
    int a = 987, b = 654, c = 321, d = 0;

    int * test[] = {&a, &b, &c, &d};
    bxivector_p array = bxivector_new(ARRAYLEN(test), (void **)test);
    CU_ASSERT_PTR_NOT_NULL(array);

    CU_ASSERT_EQUAL(bxivector_get_size(array), 4);

    int ** direct_array = (int **)bxivector_get_array(array);
    CU_ASSERT_PTR_NOT_NULL(direct_array);
    CU_ASSERT_EQUAL(direct_array[0], test[0]);
    CU_ASSERT_EQUAL(*direct_array[0], *test[0]);

    int * poped = (int *) bxivector_pop(array);
    CU_ASSERT_EQUAL(poped, test[3]);
    CU_ASSERT_EQUAL(bxivector_get_size(array), 3);

    poped = (int *) bxivector_pop(array);
    CU_ASSERT_EQUAL(poped, test[2]);
    CU_ASSERT_EQUAL(bxivector_get_size(array), 2);

    poped = (int *) bxivector_pop(array);
    CU_ASSERT_EQUAL(poped, test[1]);
    CU_ASSERT_EQUAL(bxivector_get_size(array), 1);

    poped = (int *) bxivector_pop(array);
    CU_ASSERT_EQUAL(poped, test[0]);
    CU_ASSERT_EQUAL(bxivector_get_size(array), 0);

    bxivector_push(array, test[0]);
    CU_ASSERT_EQUAL(bxivector_get_size(array), 1);

    bxivector_push(array, NULL);
    CU_ASSERT_EQUAL(bxivector_get_size(array), 2);

    bxivector_destroy(&array, NULL);
    CU_ASSERT_PTR_NULL(array);

    array = bxivector_new(0, NULL);
    CU_ASSERT_PTR_NOT_NULL(array);
    CU_ASSERT_EQUAL(bxivector_get_size(array), 0);

    size_t MAX = 100;
    for (size_t i = 0; i < MAX; i++){
        size_t * allocated = bximem_calloc(sizeof(*allocated));
        *allocated = i;
        bxivector_push(array, allocated);
        CU_ASSERT_EQUAL(bxivector_get_size(array), i+1);
    }

    for (size_t i = MAX; i != 0; i--) {
        size_t * poped = bxivector_pop(array);
        CU_ASSERT_PTR_NOT_NULL(poped);
        CU_ASSERT_EQUAL(*poped, i-1);
        BXIFREE(poped);
        CU_ASSERT_EQUAL(bxivector_get_size(array), i-1);
    }

    for (size_t i = 0; i < MAX; i++){
        size_t * allocated = bximem_calloc(sizeof(*allocated));
        *allocated = i;
        bxivector_push(array, allocated);
        CU_ASSERT_EQUAL(bxivector_get_size(array), i+1);
    }

    for (size_t i = 0; i < MAX; i++) {
        size_t * get = bxivector_get_elem(array, i);
        CU_ASSERT_PTR_NOT_NULL(get);
        CU_ASSERT_EQUAL(*get, i);
        CU_ASSERT_EQUAL(bxivector_get_size(array), MAX);
    }

    bxivector_destroy(&array, (void (*)(void **))bximem_destroy);
    CU_ASSERT_PTR_NULL(array);
}


