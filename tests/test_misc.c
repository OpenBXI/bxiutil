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



void test_misc_tuple2str(void) {
    const uint8_t tuple[] = { 1, 2, 3, 0, 0 };
    char * str = bximisc_tuple_str(0, tuple, 0, '[', ',', ']');
    DEBUG(TEST_LOGGER, "Empty tuple: %s", str);
    CU_ASSERT_STRING_EQUAL(str, "[]");
    uint8_t dst[1024];
    uint8_t dim;
    bxierr_p err = bximisc_str_tuple(str, str + strlen(str) - 1, '[', ',', ']', &dim, dst);
    CU_ASSERT_TRUE(bxierr_isok(err));
    for (size_t i = 0; i < dim; i++) CU_ASSERT_EQUAL(tuple[i], dst[i]);
    BXIFREE(str);
    str = bximisc_tuple_str(1, tuple, 0, '[', ',', ']');
    DEBUG(TEST_LOGGER, "One element tuple: %s", str);
    CU_ASSERT_STRING_EQUAL(str, "[01]");
    err = bximisc_str_tuple(str, str + strlen(str) - 1, '[', ',', ']', &dim, dst);
    CU_ASSERT_TRUE(bxierr_isok(err));
    for (size_t i = 0; i < dim; i++) CU_ASSERT_EQUAL(tuple[i], dst[i]);
    BXIFREE(str);
    str = bximisc_tuple_str(2, tuple, 0, '[', ',', ']');
    DEBUG(TEST_LOGGER, "Two elements tuple: %s", str);
    CU_ASSERT_STRING_EQUAL(str, "[01,02]");
    err = bximisc_str_tuple(str, str + strlen(str) - 1, '[', ',', ']', &dim, dst);
    CU_ASSERT_TRUE(bxierr_isok(err));
    for (size_t i = 0; i < dim; i++) CU_ASSERT_EQUAL(tuple[i], dst[i]);
    BXIFREE(str);
    str = bximisc_tuple_str(3, tuple, 0, '\0', '.', '\0');
    DEBUG(TEST_LOGGER, "Three elements tuple: %s", str);
    CU_ASSERT_STRING_EQUAL(str, "01.02.03");
    err = bximisc_str_tuple(str, str + strlen(str) - 1, '\0', '.', '\0', &dim, dst);
    CU_ASSERT_TRUE(bxierr_isok(err));
    for (size_t i = 0; i < dim; i++) CU_ASSERT_EQUAL(tuple[i], dst[i]);
    BXIFREE(str);
    str = bximisc_tuple_str(3, tuple, 2, '\0', '.', '*');
    DEBUG(TEST_LOGGER, "Single element tuple with end mark: %s", str);
    CU_ASSERT_STRING_EQUAL(str, "01*");
    err = bximisc_str_tuple(str, str + strlen(str) - 1, '\0', '.', '*', &dim, dst);
    CU_ASSERT_TRUE(bxierr_isok(err));
    for (size_t i = 0; i < dim; i++) CU_ASSERT_EQUAL(tuple[i], dst[i]);
    BXIFREE(str);
    str = bximisc_tuple_str(3, tuple, 3, '*', '.', '\0');
    DEBUG(TEST_LOGGER, "Two elements tuple with end mark: %s", str);
    CU_ASSERT_STRING_EQUAL(str, "*01.02");
    err = bximisc_str_tuple(str, str + strlen(str) - 1, '*', '.', '\0', &dim, dst);
    CU_ASSERT_TRUE(bxierr_isok(err));
    for (size_t i = 0; i < dim; i++) CU_ASSERT_EQUAL(tuple[i], dst[i]);
    BXIFREE(str);
    str = bximisc_tuple_str(5, tuple, 11, '[', '.', ']');
    DEBUG(TEST_LOGGER, "All elements tuple with end mark and max reached: %s", str);
    CU_ASSERT_STRING_EQUAL(str, "[01.02.03.00.00]");
    err = bximisc_str_tuple(str, str + strlen(str) - 1, '[', ',', ']', &dim, dst);
    CU_ASSERT_TRUE(bxierr_isok(err));
    for (size_t i = 0; i < dim; i++) CU_ASSERT_EQUAL(tuple[i], dst[i]);
    BXIFREE(str);
}


void test_bitarray(void) {
    size_t MAX = 100;
    char bitset23[BITNSLOTS(MAX)];
    memset(bitset23, 0, BITNSLOTS(MAX));
    for (size_t i = 0; i < MAX; i++) {
        if (i % 2 == 0 || i % 3 == 0) {
            BITSET(bitset23, i);
        }
    }
    for (size_t i = 0; i < MAX; i++) {
        if (BITTEST(bitset23, i)) {
            CU_ASSERT_TRUE(i % 2 == 0 || i % 3 == 0);
        }
    }
    char * str = bximisc_bitarray_str(bitset23, MAX);
    DEBUG(TEST_LOGGER, "Multiple of 2 and 3: %s", str);
    BXIFREE(str);

    // Test another one
    char bitset5[BITNSLOTS(MAX)];
    memset(bitset5, 0, BITNSLOTS(MAX));
    for (size_t i = 0; i < MAX; i++) {
        if (i % 5 == 0) {
            BITSET(bitset5, i);
        }
    }
    for (size_t i = 0; i < MAX; i++) {
        if (BITTEST(bitset5, i)) {
            CU_ASSERT_TRUE(i % 5 == 0);
        }
    }

    // Test union
    char bitset_op[BITNSLOTS(MAX)];
    memset(bitset_op, 0, BITNSLOTS(MAX));
    for(size_t i = 0; i < BITNSLOTS(MAX); i++) {
        bitset_op[i] = bitset23[i] | bitset5[i];
    }
    str = bximisc_bitarray_str(bitset_op, MAX);
    DEBUG(TEST_LOGGER, "Union of 2, 3 and 5 multiples: %s", str);
    BXIFREE(str);
    for (size_t i = 0; i < MAX; i++) {
        if (BITTEST(bitset_op, i)) {
            CU_ASSERT_TRUE(i % 2 == 0 || i % 3 == 0 || i % 5 == 0);
        }
    }

    // Test complement
    memset(bitset_op, 0, BITNSLOTS(MAX));
    for(size_t i = 0; i < BITNSLOTS(MAX); i++) {
        bitset_op[i] = (char)~bitset5[i];
    }
    str = bximisc_bitarray_str(bitset_op, MAX);
    DEBUG(TEST_LOGGER, "Complement of multiple of 5: %s", str);
    BXIFREE(str);
    for (size_t i = 0; i < MAX; i++) {
        if (BITTEST(bitset_op, i)) {
            CU_ASSERT_FALSE(i % 5 == 0);
        }
    }

}

void test_min_max() {
    int a1 = 1, b1 = 2;
    CU_ASSERT_EQUAL(BXIMISC_MIN(a1,b1), a1);
    CU_ASSERT_EQUAL(BXIMISC_MAX(a1,b1), b1);

    float a2 = 0.1f, b2 = 0.2f;
    CU_ASSERT_EQUAL(BXIMISC_MIN(a2,b2), a2);
    CU_ASSERT_EQUAL(BXIMISC_MAX(a2,b2), b2);

    double a3 = 0.1, b3 = 0.2;
    CU_ASSERT_EQUAL(BXIMISC_MIN(a3,b3), a3);
    CU_ASSERT_EQUAL(BXIMISC_MAX(a3,b3), b3);
}
