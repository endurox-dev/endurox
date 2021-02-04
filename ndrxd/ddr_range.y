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
            range_expr COLON RANGEVAL   { if (ndrx_G_ddrp.error || EXSUCCEED!=ndrx_ddr_add_group($3)) DDRERROR;  }
          | range_expr COLON DEFAULT    { ndrx_G_ddrp.flags|=NDRX_DDR_FLAG_DEFAULT_GRP; if (ndrx_G_ddrp.error || EXSUCCEED!=ndrx_ddr_add_group(NULL)) DDRERROR;}
	  ;

/* get the range variants */
range_expr:
          RANGEVAL MINUS RANGEVAL       { ndrx_G_ddrp.min = strdup($1); ndrx_G_ddrp.max = strdup($3); if (!ndrx_G_ddrp.min || !ndrx_G_ddrp.max || ndrx_G_ddrp.error) DDRERROR;}
          | MIN MINUS RANGEVAL          { ndrx_G_ddrp.flags|=NDRX_DDR_FLAG_MIN;  ndrx_G_ddrp.max = strdup($3); if (!ndrx_G_ddrp.max || ndrx_G_ddrp.error) DDRERROR;}
          | RANGEVAL MINUS MAX          { ndrx_G_ddrp.flags|=NDRX_DDR_FLAG_MAX;  ndrx_G_ddrp.min = strdup($1); if (!ndrx_G_ddrp.min || ndrx_G_ddrp.error) DDRERROR;}
          | DEFAULT                     { ndrx_G_ddrp.flags|=NDRX_DDR_FLAG_DEFAULT_VAL; if (ndrx_G_ddrp.error) DDRERROR;}
%%

group_expression: /* nothing */
  | group_expression EOL {}
 ;
 
/* vim: set ts=4 sw=4 et smartindent: */