/* -*- coding: utf-8 -*-
###############################################################################
# Author: Alain Cady <alain.cady@bull.net>
# Created on: Nov 17, 2014
# Contributors:
#              
###############################################################################
# Copyright (C) 2014  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################
*/
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* Cunit */
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

/* BXI Api */
#include <bxi/base/str.h>
#include <bxi/base/log.h>
#include <bxi/util/vector.h>
#include <bxi/util/misc.h>

/* KeVaLa */
union YYSTYPE
{
    /* Types used in the lexer: can't be modified */
    long *num;
    char *str;
    bxivector_p tuple;
};
#include <bxi/util/kvl.h>

/* Logging facility */
SET_LOGGER(_LOGGER, KVP_LOG_NAME"_unittest");

#ifndef VTEST
    #undef DEBUG
    #define DEBUG(...)
    #define log_level BXILOG_ALERT
#else
    #define log_level BXILOG_LOWEST
#endif



/* Yacc utils */
YYSTYPE *_yylval;
YYLTYPE *_yylloc;

/* Helper */
#define bxivector_compare(a, b, type) do {                                                  \
    size_t min_size = BXIMISC_MIN(bxivector_get_size(a), bxivector_get_size(b));            \
        for (size_t i=0; i<min_size; ++i) {                                                 \
            CU_ASSERT(*(type*)bxivector_get_elem(a, i) == *(type*)bxivector_get_elem(b, i));\
        }                                                                                   \
        CU_ASSERT(bxivector_get_size(a) == bxivector_get_size(b));                          \
    } while(0)

/******************************** Lexer ***********************************************/
/* Lexer test suite initialization function. */
int init_lexerSuite(void) {
    _yylval = bximem_calloc(sizeof(YYSTYPE));
    _yylloc = bximem_calloc(sizeof(YYLTYPE));

    return 0;
}

/* Lexer test suite cleanup function. */
int clean_lexerSuite(void) {
    BXIFREE(_yylloc);
    BXIFREE(_yylval);

    return 0;
}

#define test_digit(buf, expected_value) do {        \
        CU_ASSERT(NUM == _lex_me(buf));             \
        DEBUG(_LOGGER, "num %ld", *_yylval->num);   \
        CU_ASSERT(expected_value == *_yylval->num); \
    } while (0)

#define _test_str(buf, expected_tok, expected_value) do {       \
        CU_ASSERT(expected_tok == _lex_me(buf));                \
        CU_ASSERT_STRING_EQUAL(expected_value, _yylval->str);   \
    } while (0)

#define test_tuple(buf, expected_value) do {                    \
        CU_ASSERT(TUPLE == _lex_me(buf));                       \
        bxivector_compare(expected_value, _yylval->tuple, long);\
    } while (0)

#define test_prefix(buf, expected_value) _test_str(buf, PREFIX, expected_value)
#define test_key(buf, expected_value) _test_str(buf, KEY, expected_value)
#define test_string(buf, expected_value) _test_str(buf, STR, expected_value)

#define test_eof(buf) CU_ASSERT(0 == _lex_me(buf))
#define test_eol(buf) CU_ASSERT(EOL == _lex_me(buf))
#define test_err(buf, expected_mistoken) CU_ASSERT(expected_mistoken == _lex_me(buf))

/* Simple token stringification */
char *tok2str(enum yytokentype tok) {
    switch(tok) {
        case PREFIX:  return "PREFIX";
        case KEY:  return "KEY";
        case NUM: return "NUM";
        case STR: return "STR";
        case TUPLE: return "TUPLE";
        case EOL: return "EOL";
        default: return "/!\\UNKNOWN/*\\";
    }
}

enum yytokentype _lex_me(char *buf) {
    FILE *fakefile = fmemopen(buf, BXISTR_BYTES_NB(buf), "r");
    assert(fakefile != NULL);

    yyscan_t _scanner = kvl_init_from_fd(fakefile, "lex_unit_t");

    enum yytokentype token = yylex(_yylval, _yylloc, _scanner);
    DEBUG(_LOGGER, "%s: %s", buf, tok2str(token));

    kvl_finalize(_scanner);

    fclose(fakefile);

    return token;
}

void lexSpecial(void) {
    test_eof(" \0");
    test_eol("\n");
}

void lexKey(void) {
    /* case */
    test_key("kirikou", "kirikou");

    /* underscore */
    test_key("ki_ik_u", "ki_ik_u");
    test_err("_underscored", '_');

    /* dash */
    // Will be caught by grammar: test_err("dash-", '-');
    // Will be caught by grammar: test_err("da-sh", '-');
    test_err("--dash", '-');
}

void lexPrefix(void) {
    /* case */
    test_prefix("KIRIKOU", "KIRIKOU");
    test_prefix("Kirikou", "Kirikou");

    /* underscore */
    test_prefix("KI_IK_U", "KI");
}

void lexDigit(void) {
    test_digit("0", 0);
    test_digit("-3", -3);

    /* We do not handle float, yet */
    test_err(".414", '.');

    /* The answer to the ultimate question of life, the universe and everything */
    test_digit("42", 42);

    /* UCHAR_MAX = 255 */
    test_digit(bxistr_new("%d", UCHAR_MAX), UCHAR_MAX);
    test_digit(bxistr_new("%d", UCHAR_MAX+1), UCHAR_MAX+1);

    /* USHRT_MAX = 65535 */
    test_digit(bxistr_new("%d", USHRT_MAX), USHRT_MAX);
    test_digit(bxistr_new("%d", USHRT_MAX+1), USHRT_MAX+1);

    /* UINT_MAX = 4294967295U */
    test_digit(bxistr_new("%u", UINT_MAX), UINT_MAX);
    test_digit(bxistr_new("%lu", (long unsigned int)(UINT_MAX)+1U), (long unsigned int)(UINT_MAX)+1U);

    /* LONG_MAX = 9223372036854775807L */
    test_digit(bxistr_new("%ld", LONG_MAX), LONG_MAX);


    // those one overflow...
    test_digit(bxistr_new("%lu", (unsigned long)(LONG_MAX)+1UL), (unsigned long)((LONG_MAX)));
    /* ULONG_MAX = 18446744073709551615UL */
    test_digit(bxistr_new("%lu", ULONG_MAX), (long)LONG_MAX);
    // FIXME: errno?
    test_digit(bxistr_new("%llu", (unsigned long long)(ULONG_MAX)+1ULL), 0L);
}

void lexString(void) {
    /* Empty string */
    test_string("''", "");
    test_string("\"\"", "");

    /* Simple string */
    test_string("'ehlo'", "ehlo");
    test_string("\"ehlo\"", "ehlo");

    /* Separator */
    test_string("' inner space     do not               count'", " inner space     do not               count");
    test_string("\"inner quote shouldn't count...\"", "inner quote shouldn't count...");
    test_string("'\"'", "\"");
    test_string("\"'\"", "'");

    /* Error */
    /* yylex returns 0 when EOF is reached */
    test_err("'enclose mismatch\"", 0);
    test_err("\"enclose mismatch'", 0);

    test_err("'multi\nline str'", EOL);

    test_err("\"", '\0');
    test_err("'", '\0');
}

bxivector_p mk_ulong_tuple(size_t size, ...) {
    va_list ap;
    bxivector_p tuple = bxivector_new(0, NULL);

    va_start(ap, size);
    for(size_t i=0; i<size; ++i){
        long digit = va_arg(ap, long);

        bxivector_push(tuple, &digit);
    }
    va_end(ap);

    return tuple;
}

void lexTuple(void) {
    bxivector_p ttuple = bxivector_new(0, NULL);
    /* Empty tuple */
    test_tuple("()", ttuple);
    test_tuple("{}", ttuple);

    /* Error */
    test_err(")", ')');
    test_err("}", '}');

    test_err("(", '\0');
    test_err("{", '\0');

    test_err("(,", ',');
    test_err("{,", ',');

    test_err("(2,", '\0');
    test_err("{3,", '\0');

    test_err("{)", ')');
    test_err("{2,)", ')');

    test_err("(}", '}');
    test_err("(3,}", '}');


    /* single */
    long quarante_deux = 42;
    bxivector_push(ttuple, &quarante_deux);
    test_tuple("(42)",      ttuple);
    test_tuple("{42}",      ttuple);
    test_tuple("(42 )",     ttuple);
    test_tuple("( 42)",     ttuple);
    test_tuple("{ 42 }",    ttuple);
    test_tuple("{42,}",     ttuple);
    test_tuple("(42, )",    ttuple);
    test_tuple("( 42,   )", ttuple);

    /* double */
    long dix_huit = -18;
    bxivector_push(ttuple, &dix_huit);
    test_tuple("(42,-18)",       ttuple);
    test_tuple("(42, -18)",      ttuple);
    test_tuple("(42,-18 )",      ttuple);
    test_tuple("( 42,-18,)",     ttuple);
    test_tuple("( 42,-18, )",    ttuple);

    /* limits */
    long zero = 0;
    bxivector_p zerotuple = bxivector_new(0, NULL);
    bxivector_push(zerotuple, &zero);
    test_tuple("(0)",      zerotuple);
}

