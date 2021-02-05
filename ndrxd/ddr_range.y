/**
 * @brief UBF Expression Evaluator.
 *
 * @file ddr_range.y
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

%define api.prefix {ddr}

%{
#include <stdio.h>
#include <stdlib.h>
#include <ndrx_ddr.h>

extern int ddrlex (void);
%}

%union {
    char *val;
}

%locations

/* declare tokens */
%token EOL
%token MINUS        /**< min/max split   */
%token COLON        /**< group after     */
%token COMMA        /**< new ranage      */
%token MIN          /**< min value, flag */
%token MAX          /**< max value, flag */
%token DEFAULT      /**< default symbol  */
%token <val> RANGEVAL /*< range value    */

%start group_expression

%%

/*
 * Note that %% and !% works only for string vs string or field vs string.
 */
group_expression:
            range_expr COLON RANGEVAL   { if (ndrx_G_ddrp.error || EXSUCCEED!=ndrx_ddr_add_group($1, $3)) DDRERROR; }
          | range_expr COLON DEFAULT    { if (ndrx_G_ddrp.error || EXSUCCEED!=ndrx_ddr_add_group($1, NULL)) DDRERROR; }
          | range_expr COLON MIN        { if (ndrx_G_ddrp.error || EXSUCCEED!=ndrx_ddr_add_group($1, "MIN")) DDRERROR; }
          | range_expr COLON MAX        { if (ndrx_G_ddrp.error || EXSUCCEED!=ndrx_ddr_add_group($1  "MAX")) DDRERROR; }
	  ;

/* get the range variants */
range_expr:
          range_val MINUS range_val     { $$ = ndrx_ddr_new_rangeexpr($1, $3);    if (!$$|| ndrx_G_ddrp.error) DDRERROR;}
          | MIN MINUS range_val         { $$ = ndrx_ddr_new_rangeexpr(NULL, $3);  if (!$$|| ndrx_G_ddrp.error) DDRERROR;}
          | range_val MINUS MAX         { $$ = ndrx_ddr_new_rangeexpr($1, NULL);  if (!$$|| ndrx_G_ddrp.error) DDRERROR;}
          | DEFAULT                     { $$ = ndrx_ddr_new_rangeexpr(NULL, NULL);if (!$$|| ndrx_G_ddrp.error) DDRERROR;}
/* get the range variants val */
range_val:
          RANGEVAL                      {$$ = ndrx_ddr_new_rangeval($1, 0);      if (!$$|| ndrx_G_ddrp.error) DDRERROR; }
          | MINUS RANGEVAL              {$$ = ndrx_ddr_new_rangeval($1, EXTRUE); if (!$$|| ndrx_G_ddrp.error) DDRERROR; }
%%

group_expression: /* nothing */
    group_expression COMMA {}
  | group_expression EOL {}
 ;
 
/* vim: set ts=4 sw=4 et smartindent: */