/* 
 * @brief UBF library
 *   Declarations for a expression evaluator.
 *
 * @file expr.h
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

#ifndef __EXPR_H
#define	__EXPR_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ubf.h>
#include <ndrstandard.h>
#include <sys/types.h>
#include <regex.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#ifndef EXTRUE
#define EXTRUE	1
#endif 

#ifndef EXFALSE
#define EXFALSE	0
#endif 

#define MAX_TEXT	511
/* Currently this is not supported! */
#define OCC_ANY		-2
#define MAX_FUNC_NAME   66      /* Maximum function name len */
    
typedef long (*functionPtr_t)(UBFH *p_ub, char *funcname);


/***************** AST NODE TYPES **************/
#define NODE_TYPE_OR		0
#define NODE_TYPE_AND		1
#define NODE_TYPE_XOR		2
#define NODE_TYPE_EQOP		3
#define NODE_TYPE_RELOP		4
#define	NODE_TYPE_ADDOP		5
#define	NODE_TYPE_MULTOP	6
#define	NODE_TYPE_UNARY		7
#define	NODE_TYPE_FLD		8
#define	NODE_TYPE_STR		9
#define	NODE_TYPE_FLOAT		10
#define	NODE_TYPE_LONG		11
#define	NODE_TYPE_FUNC		12

/************* AST SUB-NODE TYPES **************/
/* Each sub-type we will have it's own identifier! */
/* Default node subtype/no subtype */
#define NODE_SUB_TYPE_DEF       0
/* Equality Operations subtypes */
#define EQOP_EQUAL              1
#define EQOP_NOT_EQUAL          2
#define EQOP_REGEX_EQUAL        3
#define EQOP_REGEX_NOT_EQUAL	4
/* Rel OPs */
#define RELOP_LESS              5
#define RELOP_LESS_EQUAL        6
#define RELOP_GREATER           7
#define RELOP_GREATER_EQUAL     8
/* Addition OPs */
#define ADDOP_PLUS              9
#define ADDOP_MINUS             10
/* Unary operations - IDs must be above ADDOP_* */
#define UNARY_CMPL              11
#define UNARY_INV               12
/* Multipliation OPs */
#define MULOP_DOT               13
#define MULOP_DIV               14
#define MULOP_MOD               15
/* Master Ops sub operation defs (for printing only) */
#define SUB_OR_OP               16
#define SUB_AND_OP              17
#define SUB_XOR_OP              18
/***********************************************/

/* Valute types for value block */
#define VALUE_TYPE_BOOL		0
#define VALUE_TYPE_LONG		1
#define VALUE_TYPE_FLOAT	2
/* Value exracted from FLD */
#define VALUE_TYPE_FLD_STR	3
/* String value in ' ' in expression */
#define VALUE_TYPE_STRING	4
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*
 * Field info
 */
typedef struct {
	char fldnm[BF_LENGTH+1];
	BFLDID bfldid;
	BFLDOCC occ;
} bfldid_t;

/* symbol table */
struct symbol {		/* a variable name */
  char *name;
  double value;
  struct ast *func;	/* stmt for the function */
  struct symlist *syms; /* list of dummy args */
  char strval[MAX_TEXT+1];
  bfldid_t fld;
  char  funcname[MAX_FUNC_NAME+1]; /* Function name */
};

/* nodes in the Abstract Syntax Tree */
/* all have common initial nodetype */

typedef struct {
	int compiled;
	regex_t re;
} regex_compl_t;

struct ast {
  int nodetype;
  int sub_type;
  int nodeid;
  struct ast *l;
  struct ast *r;
};

struct ast_fld {
	int nodetype;
	int sub_type;
    int nodeid;
	bfldid_t fld;
};

/* Pointer to function */
struct ast_func {
    int nodetype;
	int sub_type;
    int nodeid;
    functionPtr_t f;
    char  funcname[MAX_FUNC_NAME]; /* Function name */
};

struct ast_string {
	int nodetype;
	int sub_type;
    int nodeid;
	/*char str[MAX_TEXT+1]; */
    char *str;
	regex_compl_t regex;
};

struct ast_long {
	int nodetype;
	int sub_type;
    int nodeid;
	long l;
};

struct ast_float {
	int nodetype;
	int sub_type;
    int nodeid;
	double d;
};

typedef struct {
    short dyn_alloc;
	short value_type;
	int is_null;
	short boolval;
	long longval;
	double floatval;
	/* char strval[MAX_TEXT+1]; */
    char *strval;
} value_block_t;

/*************** Dynamic list for allocated resources ***********************/
struct list_node {
    char *mem;
    struct list_node *next;
};
/****************************************************************************/

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/* build an AST */
struct ast *newast(int nodetype, int sub_type, struct ast *l, struct ast *r);

struct ast *newfld(bfldid_t f);
struct ast *newstring(char *str);
struct ast *newfloat(double d);
struct ast *newlong(long l);
struct ast *newfunc(char *funcname);

/* evaluate an AST */
int eval(UBFH *p_ub, struct ast *a, value_block_t *v);

/* delete and free an AST */
void treefree(struct ast *);

/* interface to the lexer */
extern int yylineno; /* from lexer */
void yyerror(char *s, ...);

extern int debug;
void dumpast(struct ast *a, int level);

extern char * ndrx_Bboolco (char * expr);
extern int ndrx_Bboolev (UBFH * p_ub, char *tree);
extern double ndrx_Bfloatev (UBFH * p_ub, char *tree);
extern void ndrx_Btreefree (char *tree);

extern __thread struct ast *G_p_root_node;
extern __thread int G_node_count;
extern __thread int G_error;

#ifdef	__cplusplus
}
#endif

#endif	/* __EXPR_H */
