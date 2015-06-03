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
#ifndef __kvl_lexer_h__
#define __kvl_lexer_h__

#ifndef BXICFFI
#include <stdio.h>
#include <limits.h>
#include <bxi/base/str.h>
#include <bxi/base/log.h>
#endif

#include <bxi/util/misc.h>
#include <bxi/util/vector.h>

/**
 * @file    kvl.h
 * @brief   a Key/Value lexer.
 *
 * TODO: document.
 */


/* Bison/Flex binding */
#ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
        typedef void* yyscan_t;
    #endif

#ifndef YY_TYPEDEF_YY_EXTRA_T
    #define YY_TYPEDEF_YY_EXTRA_T
        /**
         * Custom structure to store extra information such as filename of the current fd.
         *
         * It also holds helpers such as pointers to internal buffer / functions used while lexing.
         */
        typedef struct yyextra_data_t_s {
            char *filename;
            char *buf;
            FILE *buf_fp;
            size_t buf_len;
            char enclosing;
            bxivector_p tuple;
            enum yytokentype (*kw_lookup)(char *token_buffer);
        } yyextra_data_s;
    #endif

#ifndef YYTOKENTYPE
#define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     PREFIX = 258,
     KEY = 259,
     NUM = 260,
     STR = 261,
     TUPLE = 262,
     EOL = 263,
     END_OF_FILE = 264
   };
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
# define YYSTYPE_IS_DECLARED 1
typedef union YYSTYPE YYSTYPE;
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
/**
 * Structure to keep track of tokens' textual locations.
 *
 * yylex fills it while tokenizing, to be used by the parser, e.g. on error reporting.
 */
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
#endif

// extra data used in yylex
#define YY_EXTRA_TYPE yyextra_data_s*
/* End of Bison/Flex binding */

/**
 * Flex lexing function
 *
 * @param[out] yylval_param lexed value
 * @param[out] yylloc_param token' textual locations
 * @param[in] yyscanner the scanner to extract next token from
 *
 * @return Token ID
 * @see yytokentype
 */
enum yytokentype yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param, yyscan_t yyscanner);
#define YY_DECL enum yytokentype yylex \
                (YYSTYPE *yylval_param, YYLTYPE *yylloc_param, yyscan_t yyscanner)

/**
 * Scanner initialization from file name
 *
 * @param[in] fname file path of the file to tokenize
 * @param[in] kw_lookup_fnt fuction to define your own token id
 *
 * @return a scanner object to pass to yylex function
 */
yyscan_t kvl_init(char *fname, enum yytokentype (*kw_lookup_fnt)(char*));

/**
 * Scanner initialization from file descriptor
 *
 * @param[in] file_in file descriptor to tokenize
 * @param[in] fname optional filename, used for error message generation
 * @param[in] kw_lookup_fnt fuction to define your own token id
 *
 * @return a scanner object to pass to yylex function
 */
yyscan_t kvl_init_from_fd(FILE *file_in, char *fname, enum yytokentype (*kw_lookup_fnt)(char*));

/**
 * Scanner proper clean-up
 *
 * @param[in] scanner The scanner to clean-up
 */
void kvl_finalize(yyscan_t scanner);

/**
 * Return the extra structure associated with the given scanner.
 *
 * @param[in] scanner The considered scanner
 *
 * @return The structure containing scanner specific extra data
 * @see yyextra_data_t_s
 */
YY_EXTRA_TYPE yyget_extra(yyscan_t scanner);

#define KVL_LOG_NAME "kvl"

#endif // __kvl_lexer_h__

