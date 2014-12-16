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
%{
/*
    TODO:
        # speed optim?
            %option align
            %option full
*/
#include "kvp.h"
void push_str(YYSTYPE*, char*);
void push_tuple(YYSTYPE*, bxivector_p);
void push_num(YYSTYPE*, long*);

/* BXI LOG */
SET_LOGGER(_LOGGER, KVP_LOG_NAME"_lexer");

#define YY_TRACK_NEWLINE do {                            \
        yylloc->first_line = ++yylloc->last_line;       \
        yylloc->first_column = yylloc->last_column = 1; \
    } while (0)

#define YY_USER_ACTION do {                         \
        yylloc->first_column = yylloc->last_column; \
        yylloc->last_column += yyleng;              \
    } while (0);

#define YY_FATAL_ERROR(msg) CRITICAL(_LOGGER, "%s", msg)

#define e_buf (yyextra->buf)
#define e_buf_fp (yyextra->buf_fp)
#define e_buf_len (yyextra->buf_len)
#define e_enclosing (yyextra->enclosing)
#define e_tuple (yyextra->tuple)
%}

    /* Paranoïd mode */
%option warn nodefault

    /* Enforce default options */
%option ansi-definitions ansi-prototypes
%option pointer

    /* Use a "pure" scanner: without any global var */
%option reentrant

    /* Skip not used function generation */
%option nounput noinput

    /* Properly interact with Bison */
%option bison-bridge bison-locations

    /* We'll use BXI custom allocators */
%option noyyalloc noyyrealloc noyyfree



    /* Scanner States */
%x          INSIMPLEQUOTE
%x          INDOUBLEQUOTE
%x          INBRACE

equal       (=|:=)
quote       \"|'
comma       \,|\.
oTuple      \{|\(|\[
cTuple      \}|\)|\]
space       [[:blank:]]+
prefix      [A-Z]+
key         [a-z][_a-z]+
comment     #|\/\/
digit       [-]?[[:digit:]]+


%%

    /* Skipped must go fist! */
<INITIAL,INBRACE>{space}                     /* skip space */;

    /* Comment */
{comment}.*     { /* Consume comment */ }

    /* Str */
{quote}         {
                    LOWEST(_LOGGER, "STR start <%c>", yytext[0]);
                    e_enclosing = yytext[0];
                    if (e_enclosing == '"') {
                        BEGIN(INDOUBLEQUOTE);
                    } else {
                        BEGIN(INSIMPLEQUOTE);
                    }
                    // FIXME: error checking!
                    e_buf_fp = open_memstream(&e_buf, &e_buf_len);
                }

<INSIMPLEQUOTE>'  |
<INDOUBLEQUOTE>\" {
                    BEGIN(INITIAL);
                    fclose(e_buf_fp);
                    TRACE(_LOGGER, "found str<%s>", e_buf);
                    push_str(yylval, e_buf);

                    return STR;
                }

<INSIMPLEQUOTE>[^'\n]+  {
                    LOWEST(_LOGGER, "substr<%s>", yytext);
                    fwrite(yytext, sizeof(char), yyleng, e_buf_fp);
                }

<INDOUBLEQUOTE>[^\"\n]+ {
                    LOWEST(_LOGGER, "substr<%s>", yytext);
                    fwrite(yytext, sizeof(char), yyleng, e_buf_fp);
                }

<INSIMPLEQUOTE,INDOUBLEQUOTE>{
    /* Multilines str ain't allowed */
\n              { YY_TRACK_NEWLINE; BEGIN(INITIAL); ERROR(_LOGGER, "String are single line only!"); return EOL; }

<<EOF>>         { BEGIN(INITIAL); ERROR(_LOGGER, "Unfinished quote; %c missing?", e_enclosing); return yytext[0]; }
} // End of <INSTR>

    /* Tuple */
{oTuple}        {
                    BEGIN(INBRACE);
                    LOWEST(_LOGGER, "TUPLE start <%c>", yytext[0]);
                    e_enclosing = ('[' == yytext[0]) ? ']' : ( '(' == yytext[0] ? ')' : '}') ;
                    e_tuple = bxivector_new(0, NULL);
                }

<INBRACE>{
{digit}[[:blank:]]*{comma}? {
                    long *digit = bximem_calloc(sizeof(long));
                    bximisc_strtol(yytext, 10, digit); // FIXME: error handling!
                    LOWEST(_LOGGER, "upl<%lu>", *digit);
                    bxivector_push(e_tuple, digit);
                }

{cTuple}        {
                    BEGIN(INITIAL);
                    push_tuple(yylval, e_tuple);
                    if (yytext[0] != e_enclosing) {
                        ERROR(_LOGGER, "Tuple enclosing do not match, expecting %c", e_enclosing);
                        return yytext[0];
                    }
                    return TUPLE;
                }

(.|EOF)         { BEGIN(INITIAL); ERROR(_LOGGER, "Tuple Err: <%s>", yytext); return yytext[0]; }
(\n)            { YY_TRACK_NEWLINE; BEGIN(INITIAL); ERROR(_LOGGER, "Tuple Err: <%s>", yytext); return EOL; }
} // End of <INBRACE>

    /* Int */
{digit}         {
                    TRACE(_LOGGER, "digit: <%s>", yytext);
                    long *digit = bximem_calloc(sizeof(long));
                    bximisc_strtol(yytext, 10, digit); // FIXME: error handling!
                    push_num(yylval, digit);
                    return NUM;
                }

{prefix}            {
                    TRACE(_LOGGER, "prefix: <%s>", yytext);
                    push_str(yylval, bxistr_new("%s", yytext));
                    return PREFIX;
                }

{key}            {
                    TRACE(_LOGGER, "key: <%s>", yytext);
                    push_str(yylval, bxistr_new("%s", yytext));
                    return KEY;
                }

{equal}         { TRACE(_LOGGER, "EQ"); return '='; }

    /* Separators */
\n              { YY_TRACK_NEWLINE; TRACE(_LOGGER, "EOL"); return EOL; }

:               { TRACE(_LOGGER, "<:>"); return ':'; }
\.              { TRACE(_LOGGER, "<.>"); return '.'; }

.               { ERROR(_LOGGER, "Unexpected <%c>", yytext[0]); return yytext[0]; }

%%

/* Use BXI custom allocator. */
void *yyalloc(size_t bytes, void *yyscanner) {
    UNUSED(yyscanner);
    return bximem_calloc(bytes);
}

void *yyrealloc(void *ptr, size_t bytes, void *yyscanner) {
    UNUSED(yyscanner);
    return bximem_realloc(ptr, bytes);
}

void yyfree(void *ptr, void *yyscanner) {      
    UNUSED(yyscanner);
    BXIFREE(ptr);
}

