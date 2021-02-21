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
#include <ndrx_ddr.h>
#include <ndrxd.h>
#include "expr_range.tab.h"
/* Handle locations */
int ndrx_G_ddrcolumn = 1;
#define YY_USER_ACTION ddrlloc.first_column = ndrx_G_ddrcolumn; ddrlloc.last_column = ndrx_G_ddrcolumn+ddrleng-1; \
                                               ndrx_G_ddrcolumn +=ddrleng;

%}

%%
"MIN"                   { 
                            return MIN; 
                        }
"MAX"                   { 
                            return MAX; 
                        }
"*"                     { 
                            return DEFAULT; 
                        }
"-"                     { 
                            return MINUS; 
                        }
":"                     { 
                            return COLON; 
                        }
","                     { 
                            return COMMA; 
                        }
[^\*\-:,\n\t \']+        {
                            ddrlval.val = NDRX_STRDUP(ddrtext);
                            if (NULL==ddrlval.val)
                            {
                                ddrerror("Failed to copy string - OOM?"); 
                                return EOL;
                            }
                            return RANGEVAL;
                        }
"'"                     {
                            BEGIN STRING;
                            ndrx_growlist_free(&ndrx_G_ddrp.stringbuffer);
                        }
<STRING>\\\\'|\\'          {
                                char c='\'';
                                if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_ddrp.stringbuffer, &c))
                                {
                                    ddrerror("Failed to append text to string buffer"); 
                                    return EOL;
                                }
                        }
<STRING>[^'\n]           {
                            /* add string */
                            if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_ddrp.stringbuffer, ddrtext))
                            {
                                ddrerror("Failed to append text to string buffer"); 
                                return EOL;
                            }
                        }
<STRING>[\n]           {
                            ddrerror("Unexpected and of the string"); 
                            return EOL;
                       }
<STRING>'              {
                            char c='\0';
                            if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_ddrp.stringbuffer, &c))
                            {
                                ddrerror("Failed to append text to string buffer"); 
                                return EOL;
                            }

                            BEGIN INITIAL;
                            return STRVAL;
                        }
[ \t \n]                   {
                        }

%%

/* yylex_destroy() is missing on older bison versions */
#if !defined(DDR_FLEX_MAJOR_VERSION) || DDR_FLEX_MAJOR_VERSION < 2             \
|| (DDR_FLEX_MAJOR_VERSION == 2                                                \
        && (!defined(DDR_FLEX_MINOR_VERSION) || DDR_FLEX_MINOR_VERSION < 5     \
                || (DDR_FLEX_MINOR_VERSION == 5                                \
                      && (!defined(DDR_FLEX_SUBMINOR_VERSION)                  \
                              || DDR_FLEX_SUBMINOR_VERSION < 9))))

#define ddrlex_destroy() ddr_delete_buffer(YY_CURRENT_BUFFER)

#endif


/* vim: set ts=4 sw=4 et smartindent: */
