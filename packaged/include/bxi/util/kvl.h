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
 * @brief   A flex generated Key/Value lexer
 *
 * ### Overview
 *
 * This module provides a Key/Value tokenizer able to deals with:
 *
 * - Single line comment: anything from # or // to end of line
 * - Prefix: at least two upper case chars
 * - Key: at least two lower case chars, (only first can't be an underscore)
 * - Equal symbol: `=` or `:=`
 * - Distinct colon  and dot separator
 * - Value:
 *
 *   - Single line quoted strings (single or double),
 *   - Number (long),
 *   - Tuple: (), {} or [] enclosed, comma separated longs
 *
 * ### Limitations:
 *
 * Identifiers (Prefix & key) are ASCII only!
 *
 * ### Notes:
 *
 * Full running example (compile with `-lbxiutil -lbxibase`):
 * ~~~
 * #include <bxi/util/kvl.h>
 * 
 * union YYSTYPE
 * {
 *     // Key/value lexer base types
 *     #include <bxi/util/kvl_types.h>
 * 
 *     // One can add Parser specific types
 *     // ...
 * };
 * 
 * // Helper to stringify token types
 * char *tok2str(enum yytokentype tok) {
 *     switch(tok) {
 *         case 0 ... 255:   return (char*)&tok;
 *         case PREFIX:      return "PREFIX";
 *         case KEY:         return "KEY";
 *         case NUM:         return "NUM";
 *         case STR:         return "STR";
 *         case TUPLE:       return "TUPLE";
 *         case EOL:         return "EOL";
 *         case END_OF_FILE: return "EOF";
 *         default:          return "/!\\UNKNOWN/!\\";
 *     }
 * }
 * 
 * int main(int argc, char **argv) {
 *     YYSTYPE _yylval;
 *     YYLTYPE _yylloc;
 * 
 *     // Scanner initialization
 *     yyscan_t _scanner = kvl_init_from_fd(NULL, "-", NULL);
 * 
 *     enum yytokentype token;
 *     do {
 *         token = yylex(&_yylval, &_yylloc, _scanner);
 *         printf("%s ", tok2str(token));
 *     } while (token && token != END_OF_FILE) ;
 * 
 *     // Clean-up
 *     kvl_finalize(_scanner);
 * 
 *     return 0;
 * }
 * ~~~
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

