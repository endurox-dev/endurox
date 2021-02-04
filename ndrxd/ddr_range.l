/**
 * @brief Tokenize the range string
 *  We will use one additional global for string buffer
 *  
 *
 * @file ddr_range.l
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
%option prefix="ddr"
%option noyywrap nodefault yylineno
/* Context for reading field */
%x STRING

%{
#include <ndebug.h>
#include <stdlib.h>
#include <string.h>
#include <nstdutil.h>

 /* Handle locations */
 int ddrcolumn = 1;
 #define DDR_USER_ACTION ddrlloc.first_column = ddrcolumn; ddrlloc.last_column = ddrcolumn+ddrleng-1; \
						ddrcolumn +=ddrleng;
%}

/* float exponent */
EXP	([Ee][-+]?[0-9]+)

%%
 /* single character ops */
"*"                     
                        { 
                            return DEFAULT; 
                        }
"-"				
                        { 
                            return MINUS; 
                        }
":"				
                        { 
                            return COLON; 
                        }
","			
                        { 
                            return COMMA; 
                        }
/* get the normal text, will parse later the numbers */
[^\*-:,\n\t \']
                        {
                            yylval.val = yytext;
                            return RANGEVAL;
                        }

/* start the string */
"'"  
                        {
                            BEGIN STRING;
                        }
<STRING>\\'
                        {
                            /* just add ' -> legacy mode */
                        }
<STRING>\'
                        {
                            /* just add ' */
                        }
<STRING>.               {
                            /* add string */
                            if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_ddrp.stringbuffer, ddrtext))
                            {
                                ddrerror("Failed to append text: %s to string buffer", 
                                        ddrtext); 
                                ddrterminate();
                            }
                        }
<STRING>'               
                        {
                            /* terminate the string buffer ... TODO: how to clean up?*/

                        }

/* ignore white space */ 
[ \t]

/* terminate expression */
"\n"			{
                            return EOL;
                        }
.                       {
                            ddrerror("Unknown character [%c]", *ddrtext); ddrterminate();
                        }
%%

/* yylex_destroy() is missing on older bison versions */
#if !defined(DDR_FLEX_MAJOR_VERSION) || DDR_FLEX_MAJOR_VERSION < 2             \
|| (DDR_FLEX_MAJOR_VERSION == 2                                                \
        && (!defined(DDR_FLEX_MINOR_VERSION) || DDR_FLEX_MINOR_VERSION < 5     \
                || (DDR_FLEX_MINOR_VERSION == 5                                \
                      && (!defined(DDR_FLEX_SUBMINOR_VERSION)                  \
                              || DDR_FLEX_SUBMINOR_VERSION < 9))))

#define ddrlex_destroy() ddr_delete_buffer(DDR_CURRENT_BUFFER)

#endif


/* common function to free up parser */
void _free_parser(void)
{
    ddrlex_destroy();
}

/* vim: set ts=4 sw=4 et smartindent: */
