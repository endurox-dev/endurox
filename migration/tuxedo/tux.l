/**
 * @brief Tuxedo config lexer
 *
 * @file tux.l
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
%option prefix="tux"
%option noyywrap nodefault yylineno
/* Context for reading field */
%x STRING

%{
#include <ndebug.h>
#include <stdlib.h>
#include <string.h>
#include <nstdutil.h>
#include "tux.h"
#include "tux.tab.h"
/* Handle locations */
int ndrx_G_tuxcolumn = 1;
int ndrx_G_tuxline = 1;
#define YY_USER_ACTION tuxlloc.first_column = ndrx_G_tuxcolumn; tuxlloc.last_column = ndrx_G_tuxcolumn+tuxleng-1; \
                                               ndrx_G_tuxcolumn +=tuxleng;

%}

%%


^[ \t\r]*#.*$           { 
                        }
                        
[ \t\r]+                { 
                        }
"DEFAULT:"              { 
    
                            tuxlval.val = NDRX_STRDUP("DEFAULT:");
                            if (NULL==tuxlval.val)
                            {
                                tuxerror("Failed to copy string - OOM?"); 
                                return EOL;
                            }
                            return DEFAULT; 
                        }

"="                     { 
                            return EQUAL; 
                        }

","                     { 
                            return COMMA; 
                        }

"*RESOURCES"            {
                            return RESOURCES;
                        }

\*[A-Za-z0-9]+          { 
                            tuxlval.val = NDRX_STRDUP(tuxtext);
                            if (NULL==tuxlval.val)
                            {
                                tuxerror("Failed to copy string - OOM?");
                                return EOL;
                            }
                            return SECTION; 
                        }

[A-Za-z0-9_]+           {
                            tuxlval.val = NDRX_STRDUP(tuxtext);
                            if (NULL==tuxlval.val)
                            {
                                tuxerror("Failed to copy string - OOM?"); 
                                return EOL;
                            }
                            return OPTION;
                        }

"\""                    {
                            BEGIN STRING;
                            ndrx_growlist_free(&ndrx_G_tuxp.stringbuffer);
                        }

<STRING>\\\\            {
                                char c='\\';
                                if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_tuxp.stringbuffer, &c))
                                {
                                    tuxerror("Failed to append text to string buffer"); 
                                    return EOL;
                                }
                        }

<STRING>\\\"            {
                                char c='\"';
                                if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_tuxp.stringbuffer, &c))
                                {
                                    tuxerror("Failed to append text to string buffer"); 
                                    return EOL;
                                }
                        }

<STRING>[^\n\"]         {
                            /* add string, char by char? */
                            if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_tuxp.stringbuffer, tuxtext))
                            {
                                tuxerror("Failed to append text to string buffer"); 
                                return EOL;
                            }
                        }
                        
<STRING>\n              {
                            /* add string, char by char? */
                            if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_tuxp.stringbuffer, tuxtext))
                            {
                                tuxerror("Failed to append text to string buffer"); 
                                return EOL;
                            }
                            ndrx_G_tuxline++;
                        }

<STRING>\"              {
                            char c='\0';
                            if (EXSUCCEED!=ndrx_growlist_append(&ndrx_G_tuxp.stringbuffer, &c))
                            {
                                tuxerror("Failed to append text to string buffer"); 
                                return EOL;
                            }
                            tuxlval.val=NDRX_STRDUP(ndrx_G_tuxp.stringbuffer.mem);
                            BEGIN INITIAL;
                            return OPTION;
                        }

[ \t\r]                 {
                        }

[ \n]                   {
                            /* count the lines for the error */
                            ndrx_G_tuxline++;
                        }

.                       {
                            tuxerror("Invalid character [%s]", tuxtext); 
                            return EOL;
                        }

%%

/* . { printf("lex Unknown character = '%s'\n", ddrtext); ddrerror("lex Unknown character"); }*/

/* yylex_destroy() is missing on older bison versions */
#if !defined(TUX_FLEX_MAJOR_VERSION) || TUX_FLEX_MAJOR_VERSION < 2             \
|| (TUX_FLEX_MAJOR_VERSION == 2                                                \
        && (!defined(TUX_FLEX_MINOR_VERSION) || TUX_FLEX_MINOR_VERSION < 5     \
                || (TUX_FLEX_MINOR_VERSION == 5                                \
                      && (!defined(TUX_FLEX_SUBMINOR_VERSION)                  \
                              || TUX_FLEX_SUBMINOR_VERSION < 9))))

#define tuxlex_destroy() tux_delete_buffer(YY_CURRENT_BUFFER)

#endif


/* vim: set ts=4 sw=4 et smartindent: */
