/**
 * @brief Migrate tuxedo ubb config, parser
 *
 * @file ubb.y
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

/* %define api.prefix {ubb}*/
%name-prefix "ubb"

%{
#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include "ubb.h"
    
extern int ubblex (void);
%}

%union {
    char *val;
}

/* declare tokens */
%token EOL
%token SECTION      /**< section name  	 */
%token EQUAL        /**< equal sign      */
%token COMMA        /**< comma in option */
%token OPTION       /**< option name     */
%token DEFAULT      /**< default symbol  */
%token RESOURCES    /**< resources sect  */

%start ubb_loop

%type <val> OPTION
%type <val> SECTION
%type <val> DEFAULT


%destructor { NDRX_FREE($$); } <*>

%locations

%%
res_opt_arg:
        OPTION                                  {if (EXSUCCEED!=ubb_add_val($1)) {YYERROR;}}
        | res_opt_arg COMMA OPTION              {if (EXSUCCEED!=ubb_add_val($3)) {YYERROR;} }
        ;

resource_loop:
        RESOURCES
        | resource_loop OPTION res_opt_arg      {if (EXSUCCEED!=ubb_add_res_parm($2)) {YYERROR;} }
        ;

opt_add:
        OPTION                                  {if (EXSUCCEED!=ubb_add_val($1)) {YYERROR;} }
        | opt_add COMMA OPTION                  {if (EXSUCCEED!=ubb_add_val($3)) {YYERROR;} }
        ;
opt:
        OPTION                                  {if (EXSUCCEED!=ubb_add_sect_parm($1)) {YYERROR;} }
        | DEFAULT                               {if (EXSUCCEED!=ubb_add_sect_parm($1)) {YYERROR;} }
        | OPTION EQUAL opt_add                  {if (EXSUCCEED!=ubb_add_sect_keyw($1)) {YYERROR;} }

section_loop:
        SECTION                                 {if (EXSUCCEED!=ubb_add_sect($1)) {YYERROR;} } 
        | section_loop opt
        ;
    
ubb_loop:
        resource_loop
        | ubb_loop section_loop
        | ubb_loop EOL
        ;
;

%%

/* vim: set ts=4 sw=4 et smartindent: */
