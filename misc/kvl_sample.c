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
#include <bxi/util/kvl.h>

/* Yacc utils */
union YYSTYPE
{
    /* Key/value lexer base types */
    #include <bxi/util/kvl_types.h>

    /* One can add Parser specific types */
    // ...
};

/* Simple token stringification */
char *tok2str(enum yytokentype tok) {
    switch(tok) {
        case 0 ... 255: return (char*)&tok;
        case PREFIX:  return "PREFIX";
        case KEY:  return "KEY";
        case NUM: return "NUM";
        case STR: return "STR";
        case TUPLE: return "TUPLE";
        case EOL: return "EOL";
        case END_OF_FILE: return "EOF";
        default: return "/!\\UNKNOWN/*\\";
    }
}

int main(int argc, char **argv) {
    YYSTYPE _yylval;
    YYLTYPE _yylloc;

    /* Scanner initialization */
    yyscan_t _scanner = kvl_init_from_fd(NULL, "-", NULL);

    enum yytokentype token;
    do {
        token = yylex(&_yylval, &_yylloc, _scanner);
        printf("%s ", tok2str(token));
    } while (token && token != END_OF_FILE) ;

    /* Clean-up */
    kvl_finalize(_scanner);

    return 0;
}

