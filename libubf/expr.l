/* 
 * @brief recognize tokens for UBF evaluator
 *
 * @file expr.l
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */

%option noyywrap nodefault yylineno
/* Context for reading field */
%x FLDNM 

%{
#include "expr.h"
#include "expr.tab.h"
#include <ndebug.h>
#include <string.h>

 /* Handle locations */
 int yycolumn = 1;
 #define YY_USER_ACTION yylloc.first_column = yycolumn; yylloc.last_column = yycolumn+yyleng-1; \
						yycolumn +=yyleng;
%}

/* float exponent */
EXP	([Ee][-+]?[0-9]+)
FLD	([_a-zA-Z][_a-zA-Z0-9]*)
FUNC ([_a-zA-Z][_a-zA-Z0-9]*"()")
%%
 /* single character ops */
"("				{ return OP; }
")"				{ return CP; }

 /* Logical OR */
"||"			{ return OR; }
 /* Logical AND */
"&&"			{ return AND; }
 /* Logical XOR */
"^"				{ return XOR; }
  /* Equality Operations */
"=="			{ yylval.fn = EQOP_EQUAL; 		return EQOP; }
"!="			{ yylval.fn = EQOP_NOT_EQUAL; 	return EQOP; }
"%%"			{ yylval.fn = EQOP_REGEX_EQUAL; 	return EQOP_REG; }
"!%"			{ yylval.fn = EQOP_REGEX_NOT_EQUAL; return EQOP_REG; }
 /* Rel ops */
"<"				{ yylval.fn = RELOP_LESS; 		return RELOP; }
"<="			{ yylval.fn = RELOP_LESS_EQUAL; return RELOP; }
">"				{ yylval.fn = RELOP_GREATER; 	return RELOP; }
">="			{ yylval.fn = RELOP_GREATER_EQUAL; 	return RELOP; }
 /* Add ops */
"+"				{ yylval.fn = ADDOP_PLUS;  return ADDOP; }
"-"				{ yylval.fn = ADDOP_MINUS; return ADDOP; }
 /* Those bellow we are not going to work as ADDOP! */
"~"				{ yylval.fn = UNARY_CMPL;  return ADDOP; }
"!"				{ yylval.fn = UNARY_INV;   return ADDOP; }
 /* Mult ops */
"*"				{ yylval.fn = MULOP_DOT;   return MULTOP; }
"/"				{ yylval.fn = MULOP_DIV;   return MULTOP; }
"\%"			{ yylval.fn = MULOP_MOD;   return MULTOP; }
'[^']*'			{
					{
						int len = strlen(yytext);
						if (len>MAX_TEXT)
						{
							yyerror("String constant too long!"); yyterminate();
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
[0-9]+			{
					yylval.l = atol(yytext);
					return ULONG;
				}
[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}? {
 
					yylval.d = atof(yytext); return UFLOAT;
				}
 /* Custom function */
{FUNC}          {
                    {
						int len = strlen(yytext);
                        if (len>MAX_FUNC_NAME)
                        {
                            yyerror("Function name too long!"); yyterminate();
                        }
                        else
                        {
                            strncpy(yylval.funcname, yytext, BF_LENGTH);
                            yylval.funcname[BF_LENGTH]='\0';
                            UBF_LOG(log_debug, "got function (%s)\n",
                                                yylval.funcname);
                        }
                    }
					return FUNCREF;
                }
 /* Field names */
{FLD}			{
					strncpy(yylval.fld.fldnm, yytext, BF_LENGTH);
					yylval.fld.fldnm[BF_LENGTH]='\0';
					yylval.fld.occ=0;
					UBF_LOG(log_debug, "got FLD (%s[%d])\n",
										yylval.fld.fldnm, yylval.fld.occ);
					return FLDREF;
				}
 /* Parse out field name */
{FLD}\[			{
					{
						int len = strlen(yytext);
						/* Copy only first `actual' characters from field name */
						if (len>BF_LENGTH)
						{
							strncpy(yylval.fld.fldnm, yytext, BF_LENGTH);
							yylval.fld.fldnm[BF_LENGTH]='\0';
							UBF_LOG(log_debug, "got FLD %s stripped "
									"off - was too long!\n", yylval.fld.fldnm);
						}
						else  /* Extract out the name with/out trailing [*/
						{
							strncpy(yylval.fld.fldnm, yytext, len-1);
							yylval.fld.fldnm[len-1]='\0';
						};
					}
					BEGIN FLDNM;
				}
 /* Parse out occurrance */
<FLDNM>\?\]		{
					yylval.fld.occ=OCC_ANY;
					UBF_LOG(log_debug, "got FLD (%s[%d])",
								yylval.fld.fldnm, yylval.fld.occ);
					BEGIN INITIAL;
					yyerror("Any occurrance [?] - not yet supported!");
					yyterminate();

					return FLDREF;
				}
<FLDNM>[0-9]*\]	{
					yylval.fld.occ=atoi(yytext);
					UBF_LOG(log_debug, "got FLD (%s[%d])",
							yylval.fld.fldnm, yylval.fld.occ);
					BEGIN INITIAL;
					return FLDREF;
				}
 /* Fix out bad occurrance */
<FLDNM>.|\n		{
					{
						yyerror("Bad occurrance - [%64.64s]", yytext);
						yyterminate();
					}
				}
[ \t]   /* ignore white space */ 

"\n"			{
					return EOL;
				}
.				{
					yyerror("Unknown character [%c]", *yytext); yyterminate();
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

