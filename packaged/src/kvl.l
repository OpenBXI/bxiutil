/* -*- coding: utf-8 -*-
###############################################################################
# Author: Alain Cady <alain.cady@bull.net>
# Created on: Nov 17, 2014
# Contributors:
#              
###############################################################################
# Copyright (C) 2018 Bull S.A.S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P. 68, 78340 Les Clayes-sous-Bois
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
            %option never-interactive
*/
#include <string.h> // strdup
#include <libgen.h> // used 4 basename()

#include <bxi/util/vector.h>
union YYSTYPE
{
    #include <bxi/util/kvl_types.h>
};
#include <bxi/util/kvl.h>

/* BXI LOG */
SET_LOGGER(_LOGGER, KVL_LOG_NAME);

#define YY_TRACK_NEWLINE do {                           \
        yylloc->first_line = ++yylloc->last_line;       \
        yylloc->first_column = yylloc->last_column = 1; \
    } while (0)

#define YY_USER_ACTION do {                         \
        yylloc->first_column = yylloc->last_column; \
        yylloc->last_column += (int)yyleng;         \
    } while (0);

#define YY_FATAL_ERROR(msg) CRITICAL(_LOGGER, "%s", msg)

#define e_buf (yyextra->buf)
#define e_buf_fp (yyextra->buf_fp)
#define e_buf_len (yyextra->buf_len)
#define e_enclosing (yyextra->enclosing)
#define e_tuple (yyextra->tuple)
#define e_kwlookup (yyextra->kw_lookup)
%}

    /* Paranoïd mode */
%option warn nodefault

    /* Enforce default options */
%option pointer

    /* Use a "pure" scanner: without any global var */
%option reentrant

    /* Skip not used function generation */
%option nounput noinput
%option noyywrap

    /* Properly interact with Bison */
%option bison-bridge bison-locations

    /* We'll use BXI custom allocators */
%option noyyalloc noyyrealloc noyyfree



    /* Scanner States */
%x          INSIMPLEQUOTE
%x          INDOUBLEQUOTE
%x          INBRACE

    /* Regular expressions */
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
                    e_enclosing = yytext[0];
                    if (e_enclosing == '"') {
                        BEGIN(INDOUBLEQUOTE);
                    } else {
                        BEGIN(INSIMPLEQUOTE);
                    }
                    errno = 0;
                    e_buf_fp = open_memstream(&e_buf, &e_buf_len);
                    if (!e_buf_fp) {
                        bxierr_p err = bxierr_errno("open_memstream");
                        BXILOG_REPORT(_LOGGER, BXILOG_ERROR, err,
                                      "open_memstream failed while openning quote");
                        bxierr_destroy(&err);
                        return (enum yytokentype)yytext[0];
                    }
                }

<INSIMPLEQUOTE>'  |
<INDOUBLEQUOTE>\" {
                    BEGIN(INITIAL);
                    fclose(e_buf_fp);
                    e_buf_fp = NULL;
                    yylval->str = e_buf;

                    return STR;
                }

<INSIMPLEQUOTE>[^'\n]+  {
                    fwrite(yytext, sizeof(char), yyleng, e_buf_fp);
                }

<INDOUBLEQUOTE>[^\"\n]+ {
                    fwrite(yytext, sizeof(char), yyleng, e_buf_fp);
                }

<INSIMPLEQUOTE,INDOUBLEQUOTE>{
    /* Multilines str ain't allowed */
\n              {
                    YY_TRACK_NEWLINE;
                    BEGIN(INITIAL);
                    ERROR(_LOGGER, "String are single line only!");
                    fclose(e_buf_fp);
                    if (NULL != e_buf) BXIFREE(e_buf);
                }

<<EOF>>         {
                    BEGIN(INITIAL);
                    ERROR(_LOGGER, "Unfinished quote; %c missing?", e_enclosing);
                    fclose(e_buf_fp);
                    if (NULL != e_buf) BXIFREE(e_buf);
                    return (enum yytokentype)yytext[0];
                }
} // End of <INSTR>

    /* Tuple */
{oTuple}        {
                    BEGIN(INBRACE);
                    e_enclosing = (char) (('[' == yytext[0]) ? ']' : ( '(' == yytext[0] ? ')' : '}')) ;
                    e_tuple = bxivector_new(0, NULL);
                }

<INBRACE>{
{digit}[[:blank:]]*{comma}? {
                    long *digit = bximem_calloc(sizeof(long));
                    bxierr_p rc = bximisc_strtol(yytext, 10, digit);
                    if (bxierr_isko(rc) && BXIMISC_REMAINING_CHAR != rc->code) {
                        BXILOG_REPORT(_LOGGER, BXILOG_ERROR, rc, 
                                      "Error while parsing in brace digit");
                        bxierr_destroy(&rc);
                        return (enum yytokentype)yytext[0];
                    }
                    bxierr_destroy(&rc);
                    bxivector_push(e_tuple, digit);
                }

{cTuple}        {
                    BEGIN(INITIAL);
                    yylval->tuple = e_tuple;
                    if (yytext[0] != e_enclosing) {
                        ERROR(_LOGGER, "Tuple enclosing do not match, expecting %c", e_enclosing);
                        bxivector_destroy(&e_tuple, (void (*)(void **))bximem_destroy);
                        return (enum yytokentype)yytext[0];
                    }
                    return TUPLE;
                }
    /* in Tuple error */
<<EOF>>         {
                    BEGIN(INITIAL);
                    ERROR(_LOGGER, "EOF encountered while in Tuple");
                    bxivector_destroy(&e_tuple, (void (*)(void **))bximem_destroy);
                    return (enum yytokentype)yytext[0];
                }
(\n)            {
                    YY_TRACK_NEWLINE;
                    BEGIN(INITIAL);
                    ERROR(_LOGGER, "New line encountered while in Tuple!");
                    bxivector_destroy(&e_tuple, (void (*)(void **))bximem_destroy);
                }
.               {
                    BEGIN(INITIAL);
                    ERROR(_LOGGER, "Got unexpected <%c>, while in Tuple!", yytext[0]);
                    bxivector_destroy(&e_tuple, (void (*)(void **))bximem_destroy);
                    return (enum yytokentype)yytext[0];
                }
} // End of <INBRACE>

    /* Int */
{digit}         {
                    long *digit = bximem_calloc(sizeof(long));
                    bxierr_p rc = bximisc_strtol(yytext, 10, digit);
                    if (bxierr_isko(rc) && BXIMISC_REMAINING_CHAR != rc->code) {
                        BXILOG_REPORT(_LOGGER, BXILOG_ERROR, rc,
                                      "Error while parsing digit");
                        bxierr_destroy(&rc);
                        return (enum yytokentype)yytext[0];
                    }
                    bxierr_destroy(&rc);
                    yylval->num = digit;
                    return NUM;
                }


{prefix}        {
                    enum yytokentype kw = e_kwlookup(yytext);
                    if (kw) return kw;

                    yylval->str = bxistr_new("%s", yytext);
                    return PREFIX;
                }

{key}           {
                    enum yytokentype kw = e_kwlookup(yytext);
                    if (kw) return kw;

                    yylval->str = bxistr_new("%s", yytext);
                    return KEY;
                }

{equal}         { return (enum yytokentype)'='; }

    /* Separators */
\n              { 
                    YY_TRACK_NEWLINE;

                    /* Consume new_line */
                }

:               { return (enum yytokentype)':'; }
\.              { return (enum yytokentype)'.'; }
<<EOF>>         { return END_OF_FILE; }

.               { ERROR(_LOGGER, "Unexpected char: 0x%x `%c`", yytext[0], yytext[0]); return (enum yytokentype)yytext[0]; }

%%

/* Use BXI custom allocator. */
void *yyalloc(size_t bytes, void *yyscanner) {
    UNUSED(yyscanner);
    return bximem_calloc(bytes);
}

void *yyrealloc(void *ptr, size_t bytes, void *yyscanner) {
    UNUSED(yyscanner);
    return bximem_realloc(ptr, 0, bytes);
}

void yyfree(void *ptr, void *yyscanner) {      
    UNUSED(yyscanner);
    BXIFREE(ptr);
}

/* Always returns 0 lookup failback function */
enum yytokentype nokw(char *token_buffer) {
    UNUSED(token_buffer);
    return (enum yytokentype) 0;
}

/* Scanner initialization from file descriptor */
yyscan_t kvl_init_from_fd(FILE *file_in, char *fname, enum yytokentype (*kw_lookup_fnt)(char*)) {
    yyscan_t scanner;
    yyextra_data_s* extra = bximem_calloc(sizeof(yyextra_data_s));
    if (NULL == kw_lookup_fnt) kw_lookup_fnt = nokw;
    extra->kw_lookup = kw_lookup_fnt;
    extra->filename = (NULL == fname ? strdup("from fd") : strdup(fname));
    yylex_init_extra(extra, &scanner);
    yyset_in(file_in, scanner);

    return scanner;
}

/* Scanner initialization from file name */
yyscan_t kvl_init(char *fname, enum yytokentype (*kw_lookup_fnt)(char*)) {
    FILE* file_in = NULL;

    /* Scanner initialization */
    if (NULL == fname || '-' == fname[0]) {
        fname = strdup("stdin");
    } else {
        errno = 0;
        file_in = fopen(fname, "r");
        if (NULL == file_in) {
            bxierr_p err = bxierr_errno("Calling fopen(\"%s\", \"r\"): failed", fname);
            BXILOG_REPORT(_LOGGER, BXILOG_ERROR,
                          err,
                          "Can't initialize kvlexer!");
            return NULL;
        }
    }

    return kvl_init_from_fd(file_in, basename(fname), kw_lookup_fnt);
}

/* Scanner clean-up */
void kvl_finalize(yyscan_t scanner) {
    if (NULL == scanner) {
        bxierr_p err = bxierr_new(18, NULL, NULL, NULL, NULL, "NULL scanner can't be freed!");
        BXILOG_REPORT(_LOGGER, BXILOG_CRITICAL, err, "can't finalize properly");
        return;
    }
    /* Clean-up */
    yyextra_data_s *extra = yyget_extra(scanner);

    BXIFREE(extra->filename);
    BXIFREE(extra);

    yylex_destroy(scanner);
}
