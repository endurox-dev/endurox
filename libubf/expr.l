/**
 * @brief recognize tokens for UBF evaluator
 *
 * @file expr.l
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
 * -----------------------------------------------------------------------------
 * AGPL license:
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */

%option noyywrap nodefault yylineno
/* Context for reading field */
%x FUNCNM

%{
#include "expr.h"
#include "expr.tab.h"
#include <ndebug.h>
#include <stdlib.h>
#include <string.h>
#include <nstdutil.h>

 /* Handle locations */
 int yycolumn = 1;
 #define YY_USER_ACTION yylloc.first_column = yycolumn; yylloc.last_column = yycolumn+yyleng-1; \
						yycolumn +=yyleng;
%}

/* float exponent */
EXP	([Ee][-+]?[0-9]+)
FLD	([_a-zA-Z][_a-zA-Z0-9]*)
FUNC ([_a-zA-Z][_a-zA-Z0-9]*)\(

/* Recursive field syntax FIELD[OCC].FIELD2.FIELD3[OCC], etc.. 
 * Needs a parsers to build field sequence
 */
FLDREF    ([_a-zA-Z][_a-zA-Z0-9]*(\[[0-9\?]+\]){0,1}((\.[_a-zA-Z][_a-zA-Z0-9]*(\[[0-9\?]+\]){0,1})+)*)

%%
 /* single character ops */
"("                     { return OP; }
")"                     { return CP; }

 /* Logical OR */
"||"			{ return OR; }
 /* Logical AND */
"&&"			{ return AND; }
 /* Logical XOR */
"^"                     { return XOR; }
  /* Equality Operations */
"=="			{ yylval.fn = EQOP_EQUAL; 		return EQOP; }
"!="			{ yylval.fn = EQOP_NOT_EQUAL; 	return EQOP; }
"%%"			{ yylval.fn = EQOP_REGEX_EQUAL; 	return EQOP_REG; }
"!%"			{ yylval.fn = EQOP_REGEX_NOT_EQUAL; return EQOP_REG; }
 /* Rel ops */
"<"                     { yylval.fn = RELOP_LESS; 		return RELOP; }
"<="			{ yylval.fn = RELOP_LESS_EQUAL; return RELOP; }
">"                     { yylval.fn = RELOP_GREATER; 	return RELOP; }
">="			{ yylval.fn = RELOP_GREATER_EQUAL; 	return RELOP; }
 /* Add ops */
"+"                     { yylval.fn = ADDOP_PLUS;  return ADDOP; }
"-"                     { yylval.fn = ADDOP_MINUS; return ADDOP; }
 /* Those bellow we are not going to work as ADDOP! */
"~"                     { yylval.fn = UNARY_CMPL;  return UNARYOP; }
"!"                     { yylval.fn = UNARY_INV;   return UNARYOP; }
 /* Mult ops */
"*"                     { yylval.fn = MULOP_DOT;   return MULTOP; }
"/"                     { yylval.fn = MULOP_DIV;   return MULTOP; }
"\%"                    { yylval.fn = MULOP_MOD;   return MULTOP; }
'[^']*'			{
                            {
                                int len = strlen(yytext);
                                if (len>MAX_TEXT)
                                {
                                    yyerror("String constant too long!"); 
                                    yyterminate();
                                }
                                else
                                {
                                    /* stripping off leading ' */
                                    strncpy(yylval.strval, yytext+1, len-1);
                                    yylval.strval[len-2] = '\0';
                                }
                                return STRING;
                            }
                        }
 /* Numbers - float*/
[0-9]+                  {
                            yylval.l = atol(yytext);
                            return ULONG;
                        }
[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}?        {
                            /* TODO: fix sperator according to locale */
                            yylval.d = ndrx_atof(yytext); 

                            UBF_LOG(log_debug, "FLOAT: (%s): %lf %lf %lf %lf",
                                    yytext, yylval.d, atof(yytext), atof("1.3"), atof("1,3"));
                            return UFLOAT;
                        }
 /* Custom function */
{FUNC}        {
                    {
                        int len = strlen(yytext);
                        
                        /* -1 for (, +1 for EOS */
                        if (len>MAX_FUNC_NAME)
                        {
                            yyerror("Function name too long (max: %d)!", (MAX_FUNC_NAME-1)); 
                            yyterminate();
                        }
                        else
                        {    
                            yylval.funccall = ndrx_fpmalloc(sizeof(ndrx_symbfunc_t), 0);
                            
                            if (NULL==yylval.funccall)
                            {
                                yyerror("Failed to malloc funccall: %s", strerror(errno));
                                yyterminate();
                            }
                            
                            yylval.funccall->arg1[0]=EXEOS;
                            
                            /* strip off () at the end of func */
                            
                            ndrx_strncpy_eos(yylval.funccall->funcname, yytext, len-1,
                                    sizeof(yylval.funccall->funcname));
                            
                            UBF_LOG(log_debug, "got function call [%s]", 
                                    yylval.funccall->funcname);
                            
                        }
                    }
                    BEGIN FUNCNM;
                }

<FUNCNM>\)      {
                    /* pure func. */
                    BEGIN INITIAL;
                    return FUNCREF;
                }

<FUNCNM>\'[^']*\'\) {
        
                    int len = strlen(yytext);

                    /* -3 chars + 1 EOS */
                    if (len-2>MAX_TEXT)
                    {
                        yyerror("Function arg1 too long (max: %d)!", MAX_TEXT-1); 
                        yyterminate();
                    }
                    else
                    {
                        UBF_LOG(log_debug, "Arg value raw [%s]", yytext);
                        ndrx_strncpy_eos(yylval.funccall->arg1, yytext+1, 
                            strlen(yytext)-3, sizeof(yylval.funccall->arg1));
                        BEGIN INITIAL;
                        return FUNCREF;
                    }
                }
 /* Fix out bad occurrence */
<FUNCNM>.|\n		{
                            ndrx_fpfree(yylval.funccall);
                            yyerror("Incomplete function call", yytext);
                            yyterminate();
                        }

 /* Field names */
{FLDREF}                {
                            if (EXSUCCEED!=ndrx_ubf_rfldid_parse(yytext, &yylval.fld))
                            {
                                yyerror("Failed to parse field id [%s]", yytext);
                                yyterminate();
                            }
                            return FLDREF;
                        }
[ \t]   /* ignore white space */ 

"\n"                    {
                            return EOL;
                        }
.                       {
                            yyerror("Unknown character [%c]", *yytext); 
                            yyterminate();
                        }
%%

/* yylex_destroy() is missing on older bison versions */
#if !defined(YY_FLEX_MAJOR_VERSION) || YY_FLEX_MAJOR_VERSION < 2             \
|| (YY_FLEX_MAJOR_VERSION == 2                                               \
        && (!defined(YY_FLEX_MINOR_VERSION) || YY_FLEX_MINOR_VERSION < 5     \
                || (YY_FLEX_MINOR_VERSION == 5                               \
                      && (!defined(YY_FLEX_SUBMINOR_VERSION)                 \
                              || YY_FLEX_SUBMINOR_VERSION < 9))))

#define yylex_destroy() yy_delete_buffer(YY_CURRENT_BUFFER)

#endif


/* common function to free up parser */
void _free_parser(void)
{
    yylex_destroy();
}

/* vim: set ts=4 sw=4 et smartindent: */
