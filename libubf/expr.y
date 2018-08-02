/* 
 * @brief UBF Expression Evaluator.
 *
 * @file expr.y
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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

%{
#include <stdio.h>
#include <stdlib.h>
#include "ndebug.h"
#include "expr.h"

extern int yylex (void);
%}

%union {
  struct ast *a;
  double d;
  long l;
  struct symbol *s;		/* which symbol */
  struct symlist *sl;
  int fn;			/* which function */
  bfldid_t fld;
  char strval[MAX_TEXT+1]; /* String value */
  char  funcname[MAX_FUNC_NAME]; /* Function name */
}

%locations

/* declare tokens */
%token EOL
%token OR
%token AND
%token XOR
%token <fn> EQOP
%token <fn> EQOP_REG
%token <fn> RELOP
%token <fn> ADDOP
%token <fn> MULTOP
%token <fn> UNARYOP
%token <d> UFLOAT
%token <l> ULONG
%token <strval> STRING
%token <fld> FLDREF
%token <funcname> FUNCREF
%token META
%token OP
%token CP

%type <a> boolean logical_and xor_expr equality_expr 
	relational_expr additive_expr multiplicative_expr
	unary_expr primary_expr
	unsigned_constant unsigned_number string_constant field_ref func_ref

%start calclist

%%
/**
 * $1 - left arg
 * $2 - operation in the middle.
 * $3 - right arg
 * Like:
 * 4!=5 will be: $1->4, $2->value populated, $3->
 */
/*
 * Note that %% and !% works only for string vs string or field vs string.
 */
boolean: logical_and
	    | boolean OR logical_and {$$ = newast(NODE_TYPE_OR, SUB_OR_OP, $1, $3); if (!$$ || G_error) YYERROR; }
	    ;
logical_and: xor_expr
	    | logical_and AND xor_expr {$$ = newast(NODE_TYPE_AND, SUB_AND_OP, $1, $3); if (!$$ || G_error) YYERROR; }
	    ;
xor_expr: equality_expr
	    | xor_expr XOR equality_expr {$$ = newast(NODE_TYPE_XOR, SUB_XOR_OP, $1, $3); if (!$$ || G_error) YYERROR; }
	    ;
equality_expr: relational_expr
		| string_constant EQOP_REG string_constant {$$ = newast(NODE_TYPE_EQOP, $2, $1, $3); if (!$$ || G_error) YYERROR; }
		| field_ref EQOP_REG string_constant {$$ = newast(NODE_TYPE_EQOP, $2, $1, $3); if (!$$ || G_error) YYERROR; }
	    | equality_expr EQOP relational_expr {$$ = newast(NODE_TYPE_EQOP, $2, $1, $3); if (!$$ || G_error) YYERROR; }
	    ;
relational_expr: additive_expr
	    | relational_expr RELOP additive_expr {$$ = newast(NODE_TYPE_RELOP, $2, $1, $3); if (!$$ || G_error) YYERROR; }
	    ;
additive_expr: multiplicative_expr
	    | additive_expr ADDOP multiplicative_expr {$$ = newast(NODE_TYPE_ADDOP, $2, $1, $3); if (!$$ || G_error) YYERROR; }
	    ;
multiplicative_expr: unary_expr
	    | multiplicative_expr MULTOP unary_expr {$$ = newast(NODE_TYPE_MULTOP, $2, $1, $3); if (!$$ || G_error) YYERROR; }
	    ;
unary_expr: primary_expr
	    | UNARYOP primary_expr {$$ = newast(NODE_TYPE_UNARY, $1, NULL, $2); if (!$$ || G_error) YYERROR; }
	    | ADDOP primary_expr {$$ = newast(NODE_TYPE_UNARY, $1, NULL, $2);  if (!$$ || G_error) YYERROR; }
	    ;
primary_expr: unsigned_constant
	    | OP boolean CP {$$ = $2;}
	    | field_ref
        | func_ref
	    ;
unsigned_constant: unsigned_number
	    | string_constant
	    ;
unsigned_number: UFLOAT {$$ = newfloat($1); if (!$$ || G_error) YYERROR; }
	    | ULONG {$$ = newlong($1); if (!$$ || G_error) YYERROR; }
	    ;
string_constant: STRING {$$ = newstring($1); if (!$$ || G_error) YYERROR; }
		;
field_ref: FLDREF {$$ = newfld($1); if (!$$ || G_error) YYERROR; }
		;
func_ref: FUNCREF {$$ = newfunc($1); if (!$$ || G_error) YYERROR; }
		;

calclist: /* nothing */
  | calclist boolean EOL {
	G_p_root_node = $2;
    }
 ;
%%

 
