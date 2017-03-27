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


void test_misc_strto(void) {
    long vall;
    unsigned long valul;

    bxierr_p err = bximisc_strtol("", 10, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_NODIGITS_ERR);
    bxierr_destroy(&err);

    err = bximisc_strtoul("", 10, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_NODIGITS_ERR);
    bxierr_destroy(&err);

    err = bximisc_strtol("foo", 10, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_NODIGITS_ERR);
    bxierr_destroy(&err);

    err = bximisc_strtoul("bar", 10, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_NODIGITS_ERR);
    bxierr_destroy(&err);

    err = bximisc_strtol("2", 10, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isok(err));
    bxierr_destroy(&err);
    CU_ASSERT_EQUAL(vall, 2);

    err = bximisc_strtoul("4", 10, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isok(err));
    bxierr_destroy(&err);
    CU_ASSERT_EQUAL(valul, 4);

    err = bximisc_strtol("-8", 10, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isok(err));
    bxierr_destroy(&err);
    CU_ASSERT_EQUAL(vall, -8);

    err = bximisc_strtoul("-16", 10, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isok(err));
    CU_ASSERT_EQUAL(valul, (unsigned long) -16);
    bxierr_destroy(&err);

    err = bximisc_strtol("32 bytes", 10, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_REMAINING_CHAR);
    CU_ASSERT_EQUAL(*((char *) err->data), ' ');
    CU_ASSERT_EQUAL(vall, 32);
    bxierr_destroy(&err);

    err = bximisc_strtoul("64 bytes", 10, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_REMAINING_CHAR);
    CU_ASSERT_EQUAL(*((char *) err->data), ' ');
    CU_ASSERT_EQUAL(valul, 64);
    bxierr_destroy(&err);

    err = bximisc_strtol("-128 bytes", 10, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_REMAINING_CHAR);
    CU_ASSERT_EQUAL(*((char *) err->data), ' ');
    CU_ASSERT_EQUAL(vall, -128);
    bxierr_destroy(&err);

    err = bximisc_strtoul("-256 bytes", 10, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_REMAINING_CHAR);
    CU_ASSERT_EQUAL(*((char *) err->data), ' ');
    CU_ASSERT_EQUAL(valul, (unsigned long) -256);
    bxierr_destroy(&err);

    err = bximisc_strtol("FF hex", 16, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_REMAINING_CHAR);
    CU_ASSERT_EQUAL(*((char *) err->data), ' ');
    CU_ASSERT_EQUAL(vall, 255);
    bxierr_destroy(&err);

    err = bximisc_strtol("-FF hex", 16, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_REMAINING_CHAR);
    CU_ASSERT_EQUAL(*((char *) err->data), ' ');
    CU_ASSERT_EQUAL(vall, -255);
    bxierr_destroy(&err);

    err = bximisc_strtoul("FF hex (unsigned)", 16, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_REMAINING_CHAR);
    CU_ASSERT_EQUAL(*((char *) err->data), ' ');
    CU_ASSERT_EQUAL(valul, 255);
    bxierr_destroy(&err);

    err = bximisc_strtoul("-FF hex (unsigned)", 16, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, BXIMISC_REMAINING_CHAR);
    CU_ASSERT_EQUAL(*((char *) err->data), ' ');
    CU_ASSERT_EQUAL(valul, (unsigned long) -255);
    bxierr_destroy(&err);

    err = bximisc_strtol("FFFFFFFFFFFFFFFFF", 16, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, ERANGE);
    bxierr_destroy(&err);

    err = bximisc_strtol("-FFFFFFFFFFFFFFFFF", 16, &vall);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL(err->code, ERANGE);
    bxierr_destroy(&err);

    err = bximisc_strtoul("FFFFFFFFFFFFFFFFFF", 16, &valul);
    OUT(TEST_LOGGER, "Error msg is: %s", err->msg);
    CU_ASSERT_TRUE(bxierr_isko(err));
    CU_ASSERT_EQUAL_FATAL(err->code, ERANGE);
    bxierr_destroy(&err);
}

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
    BXILOG_REPORT(TEST_LOGGER, BXILOG_ERROR, err, "Error");
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
    err = bximisc_str_tuple(str, str + strlen(str) - 1, '[', '.', ']', &dim, dst);
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
    char * str = bximisc_bitarray_str(bitset23, MAX, "[", " ", "]");
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
        bitset_op[i] = (char)(bitset23[i] | bitset5[i]);
    }
    str = bximisc_bitarray_str(bitset_op, MAX, "[", " ", "]");
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
    str = bximisc_bitarray_str(bitset_op, MAX, "[", " ", "]");
    DEBUG(TEST_LOGGER, "Complement of multiple of 5: %s", str);
    BXIFREE(str);
    for (size_t i = 0; i < MAX; i++) {
        if (BITTEST(bitset_op, i)) {
            CU_ASSERT_FALSE(i % 5 == 0);
        }
    }

}

void test_mktemp() {
    char * tmp = NULL;
    char * res = NULL;
    char * tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL) {
        tmpdir= "/tmp";
    }

    bxierr_p err = bximisc_mkstemp(tmp, &res, NULL);
    CU_ASSERT_EQUAL(err, BXIERR_OK);
    CU_ASSERT_NOT_EQUAL_FATAL(res, NULL);
    CU_ASSERT_TRUE(strncmp(res, tmpdir, strlen(tmpdir)) == 0);
    unlink(res);
    BXIFREE(res);

    tmp = "tmp";
    err = bximisc_mkstemp(tmp, &res, NULL);
    CU_ASSERT_EQUAL(err, BXIERR_OK);
    CU_ASSERT_NOT_EQUAL_FATAL(res, NULL);
    unlink(res);
    BXIFREE(res);


    tmp = "tmp-XXXXXX";
    int fd = 0;
    err = bximisc_mkstemp(tmp, &res, &fd);
    CU_ASSERT_EQUAL(err, BXIERR_OK);
    CU_ASSERT_NOT_EQUAL_FATAL(res, NULL);
    CU_ASSERT_TRUE(strncmp(res, tmpdir, strlen(tmpdir)) == 0);
    CU_ASSERT_NOT_EQUAL_FATAL(fd, 0);
    close(fd);
    unlink(res);
    BXIFREE(res);

    tmp = NULL;
    res = NULL;

    err = bximisc_mkdtemp(tmp, &res);
    CU_ASSERT_EQUAL(err, BXIERR_OK);
    CU_ASSERT_NOT_EQUAL_FATAL(res, NULL);
    CU_ASSERT_TRUE(strncmp(res, tmpdir, strlen(tmpdir)) == 0);
    unlinkat(0, res, AT_REMOVEDIR);
    BXIFREE(res);

    tmp = "tmp";
    err = bximisc_mkdtemp(tmp, &res);
    CU_ASSERT_EQUAL(err, BXIERR_OK);
    CU_ASSERT_NOT_EQUAL_FATAL(res, NULL);
    CU_ASSERT_TRUE(strncmp(res, tmpdir, strlen(tmpdir)) == 0);
    unlinkat(0, res, AT_REMOVEDIR);
    BXIFREE(res);


    tmp = "tmp-XXXXXX";
    err = bximisc_mkdtemp(tmp, &res);
    CU_ASSERT_TRUE(bxierr_isok(err));
    CU_ASSERT_NOT_EQUAL_FATAL(res, NULL);
    CU_ASSERT_TRUE(strncmp(res, tmpdir, strlen(tmpdir)) == 0);

    char * addr;
    char * file = bxistr_new("%s/test", res);
    DEBUG(TEST_LOGGER, "Map on file %s", file);
    err = bximisc_file_map(file, 10, true, true, PROT_READ | PROT_WRITE, &addr);
    CU_ASSERT_TRUE(bxierr_isko(err));
    bxierr_destroy(&err);
    err = bximisc_file_map(file, 10, false, true, PROT_READ | PROT_WRITE, &addr);
    CU_ASSERT_TRUE(bxierr_isok(err));
    strcpy(addr, "OkWorking");
    munmap(addr, 10);
    err = bximisc_file_map(file, 10, true, true, PROT_READ | PROT_WRITE, &addr);
    CU_ASSERT_TRUE(bxierr_isok(err));
    CU_ASSERT_TRUE(strcmp(addr, "OkWorking") == 0);
    strcpy(addr, "OkWork   ");
    munmap(addr, 10);
    err = bximisc_file_map(file, 10, false, true, PROT_READ | PROT_WRITE, &addr);
    CU_ASSERT_TRUE(bxierr_isko(err));
    bxierr_destroy(&err);
    err = bximisc_file_map(file, 10, true, true, PROT_READ | PROT_WRITE, &addr);
    CU_ASSERT_TRUE(bxierr_isok(err));
    CU_ASSERT_FALSE(strcmp(addr, "OkWork   ") == 0);


    munmap(addr, 10);
    unlinkat(0, res, AT_REMOVEDIR);
    BXIFREE(res);
    BXIFREE(file);
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

void test_getfilename() {
    char * result;
    bxierr_p err = bximisc_get_filename(stdout, &result);
    CU_ASSERT_TRUE(bxierr_isok(err));
    OUT(TEST_LOGGER, "Filename for stdout: %s", result);
    BXIFREE(result);

    err = bximisc_get_filename(stderr, &result);
    CU_ASSERT_TRUE(bxierr_isok(err));
    OUT(TEST_LOGGER, "Filename for stderr: %s", result);
    BXIFREE(result);

    err = bximisc_get_filename(stdin, &result);
    CU_ASSERT_TRUE(bxierr_isok(err));
    OUT(TEST_LOGGER, "Filename for stdin: %s", result);
    BXIFREE(result);

    // Create a unique file
    char * tmp = "bximisc.test_getfilename";
    char * oldpath;
    int fd = 0;
    err = bximisc_mkstemp(tmp, &oldpath, &fd);
    CU_ASSERT_EQUAL(err, BXIERR_OK);
    close(fd);

    FILE * stream = fopen(oldpath, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(stream);
    err = bximisc_get_filename(stream, &result);
    CU_ASSERT_TRUE(bxierr_isok(err));
    OUT(TEST_LOGGER, "Filename for %s: %s", oldpath, result);
    CU_ASSERT_STRING_EQUAL(oldpath, result);
    fclose(stream);

    // Create a symlink
    char * newpath = bxistr_new("%s.link", oldpath);
    int rc = symlink(result, newpath);
    if (0 != rc) {
        bxierr_p err = bxierr_errno("Calling symlink(%s, %s) failed",
                                            result, newpath);
        BXILOG_REPORT(TEST_LOGGER, BXILOG_ERROR,
                      err,
                      "Non fatal error.");
    }

    stream = fopen(newpath, "r");
    if (NULL == stream) {
        BXIEXIT(EXIT_FAILURE,
                bxierr_errno("Calling fopen(%s, r) failed",
                             newpath),
                TEST_LOGGER, BXILOG_ERROR);
    }
    char * result2;
    err = bximisc_get_filename(stream, &result2);
    CU_ASSERT_TRUE(bxierr_isok(err));
    OUT(TEST_LOGGER, "Filename for %s: %s", newpath, result2);
    CU_ASSERT_STRING_EQUAL(result, result2);
    BXIFREE(result);
    BXIFREE(result2);
    fclose(stream);

    rc = unlink(newpath);
    if (0 != rc) {
        bxierr_p err = bxierr_errno("Calling unlink(%s) failed",
                                    newpath);
        BXILOG_REPORT(TEST_LOGGER, BXILOG_ERROR,
                      err,
                      "Non fatal error.");
    }
    rc = unlink(oldpath);
    if (0 != rc) {
        bxierr_p err = bxierr_errno("Calling unlink(%s) failed",
                                    oldpath);
        BXILOG_REPORT(TEST_LOGGER, BXILOG_ERROR,
                      err, "Non fatal error.");
    }

    BXIFREE(newpath);
    BXIFREE(oldpath);

}
