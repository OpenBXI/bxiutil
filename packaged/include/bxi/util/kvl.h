
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

#include <stdio.h>
#include <limits.h>

#include <bxi/base/str.h>
#include <bxi/base/log.h>
#include <bxi/util/misc.h>
#include <bxi/util/vector.h>

/* Bison/Flex binding */
#ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
        typedef void* yyscan_t;
    #endif
    
#ifndef YY_TYPEDEF_YY_EXTRA_T
    #define YY_TYPEDEF_YY_EXTRA_T
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
     EOL = 263
   };
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
# define YYSTYPE_IS_DECLARED 1
typedef union YYSTYPE YYSTYPE;
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
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

/* Flex utils */
enum yytokentype yylex(YYSTYPE *, YYLTYPE*, yyscan_t);
#define YY_DECL enum yytokentype yylex \
                (YYSTYPE *yylval_param, YYLTYPE *yylloc_param, yyscan_t yyscanner)

yyscan_t kvl_init(char *fname, enum yytokentype (*kw_lookup_fnt)(char*));
yyscan_t kvl_init_from_fd(FILE *file_in, char *fname, enum yytokentype (*kw_lookup_fnt)(char*));
void kvl_finalize(yyscan_t scanner);
YY_EXTRA_TYPE yyget_extra(yyscan_t scanner);

#define KVL_LOG_NAME "kvl"

#endif // __kvl_lexer_h__

