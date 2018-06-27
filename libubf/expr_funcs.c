/* 
** UBF library
** UBF expression functions
**
** @file expr_funcs.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "expr.h"
#include "ndebug.h"
#include "ferror.h"
#include <ubf.h>
#include <errno.h>
#include <sys/types.h>
#include <regex.h>
#include <expr.tab.h>
#include <exhash.h>
/*---------------------------Externs------------------------------------*/
/* make llvm silent.. */
extern void yy_scan_string (char *yy_str  );
extern void _free_parser(void);
extern int yyparse (void);
/*---------------------------Macros-------------------------------------*/
#define FREE_UP_UB_BUF(v) \
if (v->dyn_alloc && NULL!=v->strval)\
{\
free(v->strval);\
v->strval=NULL;\
v->dyn_alloc=0;\
}

#ifdef UBF_DEBUG
#define DUMP_VALUE_BLOCK(TEXT, V) if (EXSUCCEED==ret) dump_val(TEXT, V)
#else
#define DUMP_VALUE_BLOCK(TEXT, V) {}
#endif

/** Compare the the float to test against is it 0 or not */
#define IS_FLOAT_0(X) (X < 0.000000001 && X > -0.000000001)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/* Compile time stuff only: */
__thread struct ast *G_p_root_node=NULL;
__thread int G_node_count=0;
__thread int G_error;
static __thread struct list_node *M_cur_mem;
static __thread struct list_node *M_first_mem;
/* Compile time stuff only, end */

/* Hash list for function callback pointers */
struct func_hash {
    char name[MAX_FUNC_NAME+1];/* key (string is WITHIN the structure) */
    functionPtr_t fptr;        /* Pointer to function                  */             
    EX_hash_handle hh;         /* makes this structure hashable        */
};
typedef struct func_hash func_hash_t;

func_hash_t *M_func_hash = NULL;    /* Hash of customer functions regi */
/*---------------------------Statics------------------------------------*/

/*
 * Listing of types
 */
char *M_nodetypes[] =
{
    "[OR    (0) ]",
    "[AND   (1) ]",
    "[XOR   (2) ]",
    "[EQOP  (3) ]",
    "[RELOP (4) ]",
    "[ADDOP (5) ]",
    "[MULTOP(6) ]",
    "[UNARY (7) ]",
    "[FLD   (8) ]",
    "[STR   (9) ]",
    "[FLOAT (10)]",
    "[LONG  (11)]",
    "[FUNC  (12)]"
};

/*
 * Listing of sub-types
 */
char *M_subtypes[] =
{
    "[DF(0) ]",
    "[==(1) ]",
    "[!=(2) ]",
    "[%%(3) ]",
    "[!%(4) ]",
    "[< (5) ]",
    "[<=(6) ]",
    "[> (7) ]",
    "[>=(8) ]",
    "[+ (9) ]",
    "[- (10)]",
    "[~ (11)]",
    "[! (12)]",
    "[* (13)]",
    "[/ (14)]",
    "[% (15)]",
    "[||(16)]",
    "[&&(17)]",
    "[^ (18)]"
};

/*
 * Listing of sub-types ( only operation sign for tree printing.)
 */
char *M_subtypes_sign_only[] =
{
    " ",
    " == ",
    " != ",
    " %% ",
    " !% ",
    " < ",
    " <= ",
    " > ",
    " >= ",
    "+",
    "-",
    "~",
    "!",
    "*",
    "/",
    "%",
    " || ",
    " && ",
    " ^ "
};

/*---------------------------Prototypes---------------------------------*/
expublic void _Btreefree_no_recurse (char *tree);
int get_bfldid(bfldid_t *p_fl);
int is_float_val(value_block_t *v);
/*
 * ERROR RECOVERY AND ALLOCATED RESOUCES.
 * Not sure is there any other better way (using bison's destructor maybe?)
 * but I tried and it does not work out. So I have created simple linked list
 * that will track allocated nodes. If parse finds out to be OK, then only list
 * it self is removed by remove_resouce_list(). If parse fails, then by using the list
 * all allocated ASTs are removed and then list by it self by function remove_resouces().
 */

/**
 * Put error here!
 * @param s
 * @param ...
 */
void yyerror(char *s, ...)
{
    /* Log only first error! */
    if (EXFAIL!=G_error)
    {
        va_list ap;
        va_start(ap, s);
        char errbuf[2048];

        sprintf(errbuf, ". Near of %d-%d: ", yylloc.first_column, yylloc.last_column);
        vsprintf(errbuf+strlen(errbuf), s, ap);

        if (ndrx_Bis_error())
        {
          /* append message */
          ndrx_Bappend_error_msg(errbuf);
        }
        else
        {
          /* Set the error from fresh! (+2 for remvoing [. ]) */
          ndrx_Bset_error_msg(BSYNTAX, errbuf+2);
        }

        G_error = EXFAIL;
    }
}

/**
 * Not sure are there any better way to clean up the AST... somehow does not
 * work for me!
 * @param p
 * @return
 */
int add_resource(char *p)
{

    struct list_node *tmp = M_cur_mem;
    M_cur_mem = malloc (sizeof(struct list_node));

    if (NULL==M_cur_mem)
            return EXFAIL; /* <<< RETURN FAIL! */

    if (NULL!=tmp)
        tmp->next = M_cur_mem;
    
    M_cur_mem->mem = p;
    M_cur_mem->next = NULL;

    if (NULL==M_first_mem)
    {
        M_first_mem=M_cur_mem;
    }

    return EXSUCCEED;
}

/**
 * In normal case just destroy the resource list!
 * @return
 */
void remove_resouce_list(void)
{
    struct list_node *p = M_first_mem;
    while (p)
    {
        struct list_node *tmp = p;
        p = p->next;
        NDRX_FREE(tmp); /* Delete current */
        UBF_LOG(6, "List node free-up!");
    }
}

/**
 * Destroy the AST and then remove resouce list
 * @return
 */
void remove_resouces(void)
{
    struct list_node *p = M_first_mem;
    while (p)
    {
        /* Free up AST */
        _Btreefree_no_recurse(p->mem);
        p = p->next;
    }
    /* Remove list by it self */
    remove_resouce_list();
}

/**
 * Same as Btreefree, but no recusion!
 * This is for error recovery.
 * This must be maintained to gether with Btreefree()!
 * @param tree
 */
expublic void _Btreefree_no_recurse (char *tree)
{
    struct ast *a = (struct ast *)tree;
    struct ast_string *a_string = (struct ast_string *)tree;

    if (NULL==tree)
        return; /* <<<< RETURN! Nothing to do! */

    UBF_LOG(6, "Free up nodeid=%d nodetype=%d", a->nodeid, a->nodetype);
    switch (a->nodetype)
    {
        case NODE_TYPE_FLD:
            /* nothing to do */
            break;
        case NODE_TYPE_STR:
            /* Free up internal resources (if have such)? */
            if (NULL!=a_string->str)
                NDRX_FREE(a_string->str);

            /* check regexp, maybe needs to clean up? */
            if (a_string->regex.compiled)
                regfree(&(a_string->regex.re));
            break;
        case NODE_TYPE_FLOAT:
            /* nothing to do */
            break;
        case NODE_TYPE_LONG:
            /* nothing to do */
            break;
        default:
            /* nothing to do */
            break;
    }
    /* delete self */
    NDRX_FREE(tree);
}

/**
 * Standard AST node type W/O data. (i.e. by it self it is +/-, etc...)
 * @param nodetype - node type
 * @param sub_type - sub type
 * @param l - left node
 * @param r - right node
 * @return ptr to AST node.
 */
struct ast * newast(int nodetype, int sub_type, struct ast *l, struct ast *r)
{
    struct ast *a = NDRX_MALLOC(sizeof(struct ast));
    memset(a, 0, sizeof(struct ast));

    if(!a) {
      yyerror("out of space");
      ndrx_Bset_error_msg(BMALLOC, "out of memory for new ast");
      return NULL;
    }
    else
    {
        if (EXSUCCEED!=add_resource((char *)a))
        {
            yyerror("out of space");
            ndrx_Bset_error_msg(BMALLOC, "out of memory for resource list");
            return NULL;
        }
    }

    a->nodetype = nodetype;
    a->sub_type = sub_type;
    a->nodeid = G_node_count;
    a->l = l;
    a->r = r;

    G_node_count++;

    UBF_LOG(log_debug, "adding newast: nodeid: %d, nodetype: %d, type: %s, sub-type:%s "
                              "value: [N/A] l=%p r=%p",
                          a->nodeid, a->nodetype,
                          M_nodetypes[a->nodetype],
                          M_subtypes[a->sub_type],
                          l, r);

    return a;
}

struct ast * newfld(bfldid_t f)
{
    struct ast_fld *a = NDRX_MALLOC(sizeof(struct ast_fld));
    memset(a, 0, sizeof(struct ast_fld));

    if(!a) {
      yyerror("out of space");
      ndrx_Bset_error_msg(BMALLOC, "out of memory for new ast_fld");
      return NULL;
    }
    else
    {
        if (EXSUCCEED!=add_resource((char *)a))
        {
            yyerror("out of space");
            ndrx_Bset_error_msg(BMALLOC, "out of memory for resource list");
            return NULL;
        }
    }

    a->nodetype = NODE_TYPE_FLD;
    a->sub_type = NODE_SUB_TYPE_DEF;
    a->nodeid = G_node_count;
    a->fld = f;
    a->fld.bfldid = BBADFLDID;

    G_node_count++;

    /* Resolve field id */
    if (BBADFLDID==get_bfldid(&(a->fld)))
    {
        yyerror("Bad field Id");
        ndrx_Bset_error_fmt(BBADNAME, "Bad field name for [%s]", a->fld.fldnm);
        /* Not sure is this good? If we do free get stack coruption/double free */
        goto out;
    }
    UBF_LOG(log_debug, "adding newfld: id: %02d, type: %s, sub-type:%s "
                              "value: [fld: [%s] occ: [%d] bfldid: [%d]]",
                          a->nodeid,
                          M_nodetypes[a->nodetype],
                          M_subtypes[a->sub_type],
                          a->fld.fldnm, a->fld.occ,
                          a->fld.bfldid);
out:
    return (struct ast *)a;

}

/*
 * Get Function name
 */
functionPtr_t get_func(char *funcname)
{
    func_hash_t *r = NULL;
    EXHASH_FIND_STR( M_func_hash, funcname, r);
    if (NULL!=r)
        return r->fptr;
    else
        return NULL;
}

/*
 * Set function pointer by user.
 * This is API function. If function ptr is NULL, func is removed from hash.
 */
int set_func(char *funcname, functionPtr_t functionPtr)
{
    int ret=EXSUCCEED;
    func_hash_t *tmp;
    
    UBF_LOG(log_warn, "registering callback: [%s]:%p", 
                        funcname, functionPtr);
    
    if (NULL==functionPtr)
    {
        func_hash_t *r = NULL;
        EXHASH_FIND_STR( M_func_hash, funcname, r);
        if (NULL!=r)
        {
            EXHASH_DEL(M_func_hash, r);
            NDRX_FREE(r);
        }
    }
    else
    {
        tmp = NDRX_MALLOC(sizeof(func_hash_t));

        if (NULL==tmp)
        {
            yyerror("out of space");
            ndrx_Bset_error_msg(BMALLOC, "out of memory for new func_hash_t");
            ret=EXFAIL;
            goto out;
        }

        strcpy(tmp->name, funcname);
        tmp->fptr = functionPtr;
        EXHASH_ADD_STR( M_func_hash, name, tmp );
    }
    
out:
	return ret;
}

/* Function call-back */
struct ast * newfunc(char *funcname)
{
  int len;
  struct ast_func *a = NDRX_MALLOC(sizeof(struct ast_func));
  memset(a, 0, sizeof(struct ast_func));

  if(!a)
  {
    yyerror("out of space");
    ndrx_Bset_error_msg(BMALLOC, "out of memory for new ast_func");
    return NULL;
  }
  else
  {
      if (EXSUCCEED!=add_resource((char *)a))
      {
          yyerror("out of space");
          ndrx_Bset_error_msg(BMALLOC, "out of memory for resource list");
          return NULL;
      }
  }

  a->nodetype = NODE_TYPE_FUNC;
  a->sub_type = NODE_SUB_TYPE_DEF;
  a->nodeid = G_node_count;
  
  len = strlen(funcname);
  
  /* Resolve field function */
  if (len<3)
  {
      yyerror("Function name too short!");
      ndrx_Bset_error_fmt(BBADNAME, "Full function name too short [%s]", funcname);
      /* Not sure is this good? If we do free get stack coruption/double free */
      goto out;
  }
  
  NDRX_STRNCPY(a->funcname, funcname, len-2);
  a->funcname[len-2] = EXEOS;
  
  G_node_count++;

  /* Resolve field function */
  if (NULL==(a->f=get_func(a->funcname)))
  {
      yyerror("Bad function name");
      ndrx_Bset_error_fmt(BBADNAME, "Bad function name for [%s]", a->funcname);
      /* Not sure is this good? If we do free get stack coruption/double free */
      goto out;
  }
  
  UBF_LOG(log_debug, "ast_func id: %02d, type: %s, sub-type:%s "
                            "value: [func: [%s]]",
                        a->nodeid,
                        M_nodetypes[a->nodetype],
                        M_subtypes[a->sub_type],
                        a->funcname);
  out:
  return (struct ast *)a;
}

struct ast * newstring(char *str)
{
    struct ast_string *a = NDRX_MALLOC(sizeof(struct ast_string));
    memset(a, 0, sizeof(struct ast_string));

    a->str = malloc (strlen(str)+1);

    if(!a || !a->str) {
        yyerror("out of space");
        ndrx_Bset_error_msg(BMALLOC, "out of memory for new ast_string or a->str");
        return NULL;
    }
    else
    {
      if (EXSUCCEED!=add_resource((char *)a))
      {
          yyerror("out of space");
          ndrx_Bset_error_msg(BMALLOC, "out of memory for resource list");
          return NULL;
      }
    }

    a->nodetype = NODE_TYPE_STR;
    a->sub_type = NODE_SUB_TYPE_DEF;
    a->nodeid = G_node_count;
    strcpy(a->str, str);
    G_node_count++;

    UBF_LOG(log_debug, "adding newstr: id: %02d, type: %s, sub-type:%s "
                    "value: [%s]",
                    a->nodeid,
                    M_nodetypes[a->nodetype],
                    M_subtypes[a->sub_type],
                    a->str);

    return (struct ast *)a;
}

struct ast * newfloat(double d)
{
    struct ast_float *a = NDRX_MALLOC(sizeof(struct ast_float));
    memset(a, 0, sizeof(struct ast_float));

    if(!a) {
        yyerror("out of space");
        ndrx_Bset_error_msg(BMALLOC, "out of memory for new newfloat");
        return NULL;
    }
    else
    {
      if (EXSUCCEED!=add_resource((char *)a))
      {
          yyerror("out of space");
          ndrx_Bset_error_msg(BMALLOC, "out of memory for resource list");
          return NULL;
      }
    }
    
    a->nodetype = NODE_TYPE_FLOAT;
    a->sub_type = NODE_SUB_TYPE_DEF;
    a->nodeid = G_node_count;
    a->d = d;
    G_node_count++;
    
    UBF_LOG(log_debug, "adding newflt: id: %02d, type: %s, sub-type:%s "
            "value: [%0.13f]",
            a->nodeid,
            M_nodetypes[a->nodetype],
            M_subtypes[a->sub_type],
            a->d);

    return (struct ast *)a;
}

struct ast * newlong(long l)
{
    struct ast_long *a = NDRX_MALLOC(sizeof(struct ast_long));
    memset(a, 0, sizeof(struct ast_long));

    if(!a) {
        yyerror("out of space");
        ndrx_Bset_error_msg(BMALLOC, "out of memory for new newfloat");
        return NULL;
    }
    else
    {
      if (EXSUCCEED!=add_resource((char *)a))
      {
          yyerror("out of space");
          ndrx_Bset_error_msg(BMALLOC, "out of memory for resource list");
          return NULL;
      }
    }
    
    a->nodetype = NODE_TYPE_LONG;
    a->sub_type = NODE_SUB_TYPE_DEF;
    a->nodeid = G_node_count;
    a->l = l;
    G_node_count++;

    UBF_LOG(log_debug, "adding newlng: id: %02d, type: %s, sub-type:%s "
                "value: [%ld]",
                a->nodeid,
                M_nodetypes[a->nodetype],
                M_subtypes[a->sub_type],
                a->l);

    return (struct ast *)a;
}
/*
 * Convert value block to the string
 * Or hmm... maybe we need to convert string to float and then do check?
 * That would cause better effect in case of trailing zeros...
 */
int get_float(value_block_t *v)
{
	int ret=EXSUCCEED;
    if (VALUE_TYPE_FLOAT==v->value_type)
    {
        /* do nothing. */
    }
    else if (VALUE_TYPE_LONG==v->value_type)
    {
        v->value_type = VALUE_TYPE_FLOAT;
        v->floatval= (double) v->longval;
    }
    else if (VALUE_TYPE_STRING==v->value_type || VALUE_TYPE_FLD_STR==v->value_type)
    {
        v->value_type = VALUE_TYPE_FLOAT;
        v->floatval = atof(v->strval);
    }
    else
    {
        UBF_LOG(log_error, "get_float: Unknown value type %d\n",
                                    v->value_type);
        ret=EXFAIL;
    }
	
    return ret;
}
/**
 * Dump value block
 * @param v - ptr to initialized value block
 */
void dump_val(char *text, value_block_t *v)
{
    switch(v->value_type)
    {
        case VALUE_TYPE_LONG:
            UBF_LOG(log_info, "%s:ret long          : %ld", text, v->longval);
            break;
        case VALUE_TYPE_FLOAT:
            UBF_LOG(log_info, "%s:ret float         : %.13lf", text, v->floatval);
            break;
        case VALUE_TYPE_STRING:
            UBF_LOG(log_info, "%s:ret const string  : [%s]", text, v->strval);
            break;
        case VALUE_TYPE_FLD_STR:
            UBF_LOG(log_info, "%s:ret fld string    : [%s]", text, v->strval);
            break;
        default:
            UBF_LOG(log_error,   "%s:Error: unknown type value block", text, v->strval);
            break;
    }
    UBF_LOG(log_debug,       "%s:ret bool          : %d", text, v->boolval);
}
/**
 * Convert value block to string.
 * @param buf - pointer to buffer space where to put string value.
 * @param v
 * @return SUCCEED
 */
int conv_to_string(char *buf, value_block_t *v)
{
    int ret=EXSUCCEED;

    v->strval = buf;
    
    if (VALUE_TYPE_LONG==v->value_type)
    {
    v->value_type = VALUE_TYPE_STRING;
            sprintf(v->strval, "%ld", v->longval);
    }
    else if (VALUE_TYPE_FLOAT==v->value_type)
    {
    v->value_type = VALUE_TYPE_STRING;
            sprintf(v->strval, "%.13lf", v->floatval);
    }
    else
    {
    UBF_LOG(log_error, "conv_to_string: Unknown value type %d\n",
                            v->value_type);
    ret=EXFAIL;
    }

    return ret;
}

/**
 * Convert value block to long.
 * @param v
 * @return SUCCEED
 */
int conv_to_long(value_block_t *v)
{
    int ret=EXSUCCEED;

    if (VALUE_TYPE_STRING==v->value_type || VALUE_TYPE_FLD_STR==v->value_type)
    {
        v->longval=atof(v->strval);
    }
    else if (VALUE_TYPE_FLOAT==v->value_type)
    {
        v->longval=(long)v->floatval;
    }
    else
    {
        UBF_LOG(log_error,"conv_to_long: Unknown value type %d\n",
                                v->value_type);
        ret=EXFAIL;
    }

    return ret;
}

/**
 * Compare (and do other math ops) on two value blocks
 * @param type
 * @param sub_type
 * @param lval
 * @param rval
 * @param v - returned value block
 * @return SUCCEED/FAIL
 */
int op_equal_float_cmp(int type, int sub_type, value_block_t *lval, value_block_t *rval, value_block_t *v)
{
    int ret=EXSUCCEED;

    v->value_type = VALUE_TYPE_LONG;

    if (VALUE_TYPE_FLOAT!=lval->value_type && EXSUCCEED!=get_float(lval))
    {
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret && VALUE_TYPE_FLOAT!=rval->value_type &&
                            EXSUCCEED!=get_float(rval))
    {
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret)
    {
        UBF_LOG(log_debug, "flt CMP (%s/%s): [%lf] vs [%lf]",
                        M_nodetypes[type], M_subtypes[sub_type], lval->floatval, rval->floatval);

        if (NODE_TYPE_EQOP==type)
        {
            v->longval = v->boolval = (fabs(lval->floatval-rval->floatval)<DOUBLE_EQUAL);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_LESS==sub_type) /* RELOP_LESS -> [x < y]*/
        {
            v->longval = v->boolval = (lval->floatval<rval->floatval?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_LESS_EQUAL==sub_type) /* RELOP_LESS_EQUAL -> [x <= y]*/
        {
            v->longval = v->boolval = (lval->floatval<=rval->floatval?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_GREATER==sub_type) /* RELOP_GREATER -> [x > y]*/
        {
            v->longval = v->boolval = (lval->floatval>rval->floatval?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_GREATER_EQUAL==sub_type) /* RELOP_GREATER_EQUAL -> [x >= y]*/
        {
            v->longval = v->boolval = (lval->floatval>=rval->floatval?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_ADDOP==type || NODE_TYPE_MULTOP==type)
        {
            v->value_type = VALUE_TYPE_FLOAT;

            if (NODE_TYPE_ADDOP==type && ADDOP_PLUS==sub_type)
            {
                v->floatval=lval->floatval+rval->floatval;
            }
            else if (NODE_TYPE_ADDOP==type && ADDOP_MINUS==sub_type)
            {
                v->floatval=lval->floatval-rval->floatval;
            }
            else if (NODE_TYPE_MULTOP==type && MULOP_DOT==sub_type)
            {
                v->floatval=lval->floatval*rval->floatval;
            }
            else if (NODE_TYPE_MULTOP==type && MULOP_DIV==sub_type)
            {
                if (IS_FLOAT_0(rval->floatval)) /* Original system specifics... */
                    rval->floatval = 0;
                else
                    v->floatval=lval->floatval/rval->floatval;
            }
            else if (NODE_TYPE_MULTOP==type && MULOP_MOD==sub_type)
            {
                /* Make these cases as false - original system do this such */
                v->value_type = VALUE_TYPE_LONG;
                UBF_LOG(log_warn, "ERROR! No mod support for floats!");
                v->longval = v->boolval = EXFALSE;
            }
            
            /* Set TRUE/FALSE flag */
            if (!IS_FLOAT_0(v->floatval))
                v->boolval=EXTRUE;
            else
                v->boolval=EXFALSE;
        }
    } /*if (SUCCEED==ret)*/
    
    /* Dump out the final value */
    DUMP_VALUE_BLOCK("op_equal_float_cmp", v);

    return ret;
}

/**
 * Do compare and math op's on left/rigt and put result back in v
 * @param type
 * @param sub_type
 * @param lval
 * @param rval
 * @param v
 * @return SUCCEED/FAIL
 */
int op_equal_long_cmp(int type, int sub_type, value_block_t *lval, value_block_t *rval, value_block_t *v)
{
    int ret=EXSUCCEED;
    v->value_type = VALUE_TYPE_LONG;

    if ((VALUE_TYPE_LONG!=lval->value_type) && EXSUCCEED!=conv_to_long(lval))
    {
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret && (VALUE_TYPE_LONG!=rval->value_type) &&
            EXSUCCEED!=conv_to_long(rval))
    {
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret)
    {
        UBF_LOG(log_debug, "lng CMP (%s/%s): [%ld] vs [%ld]",
                        M_nodetypes[type], M_subtypes[sub_type], lval->longval, rval->longval);

        if (NODE_TYPE_EQOP==type)
        {
            v->longval = v->boolval = (lval->longval==rval->longval);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_LESS==sub_type) /* RELOP_LESS -> [x < y]*/
        {
            v->longval = v->boolval = (lval->longval<rval->longval?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_LESS_EQUAL==sub_type) /* RELOP_LESS_EQUAL -> [x <= y]*/
        {
            v->longval = v->boolval = (lval->longval<=rval->longval?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_GREATER==sub_type) /* RELOP_GREATER -> [x > y]*/
        {
            v->longval = v->boolval = (lval->longval>rval->longval?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_GREATER_EQUAL==sub_type) /* RELOP_GREATER_EQUAL -> [x >= y]*/
        {
            v->longval = v->boolval = (lval->longval>=rval->longval?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_ADDOP==type || NODE_TYPE_MULTOP==type)
        {
            v->value_type = VALUE_TYPE_LONG;

            if (NODE_TYPE_ADDOP==type && ADDOP_PLUS==sub_type)
            {
                v->longval=lval->longval+rval->longval;
            }
            if (NODE_TYPE_ADDOP==type && ADDOP_MINUS==sub_type)
            {
                v->longval=lval->longval-rval->longval;
            }
            else if (NODE_TYPE_MULTOP==type && MULOP_DOT==sub_type)
            {
                v->longval=lval->longval*rval->longval;
            }
            else if (NODE_TYPE_MULTOP==type && MULOP_DIV==sub_type)
            {
                /* Original system specifics... */
                if (0==rval->longval)
                    v->longval=0;
                else
                    v->longval=lval->longval/rval->longval;
            }
            else if (NODE_TYPE_MULTOP==type && MULOP_MOD==sub_type)
            {
                if (0==rval->longval)
                    v->longval=lval->longval;
                else
                    v->longval=lval->longval%rval->longval;
            }
            /* Set TRUE/FALSE flag */
            if (v->longval)
                v->boolval=EXTRUE;
            else
                v->boolval=EXFALSE;
        }
    }

    /* Dump out the final value */
    DUMP_VALUE_BLOCK("op_equal_long_cmp", v);

    return ret;
}

int op_equal_str_cmp(int type, int sub_type, value_block_t *lval, value_block_t *rval, value_block_t *v)
{
    int ret=EXSUCCEED;
    int result;
    char lval_buf[64]; /* should be enought for all data types */
    char rval_buf[64]; /* should be enought for all data types */
    v->value_type = VALUE_TYPE_LONG;

    if (VALUE_TYPE_FLD_STR!=lval->value_type &&
            VALUE_TYPE_STRING!=lval->value_type &&
            EXSUCCEED!=conv_to_string(lval_buf, lval))
    {
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret && VALUE_TYPE_FLD_STR!=rval->value_type &&
            VALUE_TYPE_STRING!=rval->value_type &&
            EXSUCCEED!=conv_to_string(rval_buf, rval))
    {
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret)
    {
        result=strcmp(lval->strval, rval->strval);

        UBF_LOG(log_debug, "str CMP (%s/%s): [%s] vs [%s]",
                        M_nodetypes[type], M_subtypes[sub_type], lval->strval, rval->strval);

        if (NODE_TYPE_EQOP==type)
        {
            v->longval = v->boolval = result==0?EXTRUE:EXFALSE;
        }
        else if (NODE_TYPE_RELOP==type && RELOP_LESS==sub_type) /* RELOP_LESS -> [x < y]*/
        {
            v->longval = v->boolval = (result<0?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_LESS_EQUAL==sub_type) /* RELOP_LESS_EQUAL -> [x <= y]*/
        {
            v->longval = v->boolval = (result<=0?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_GREATER==sub_type) /* RELOP_GREATER -> [x > y]*/
        {
            v->longval = v->boolval = (result>0?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_RELOP==type && RELOP_GREATER_EQUAL==sub_type) /* RELOP_GREATER_EQUAL -> [x >= y]*/
        {
            v->longval = v->boolval = (result>=0?EXTRUE:EXFALSE);
        }
        else if (NODE_TYPE_ADDOP==type || NODE_TYPE_MULTOP==type)
        {
            /* Make these cases as false - original system do this such */
            UBF_LOG(log_warn, "ERROR! No math support for strings!");
            v->longval = v->boolval = EXFALSE;
        }

        UBF_LOG(log_debug, "Result bool: %d long:%ld", v->boolval, rval->longval);
    }

    /* Dump out the final value */
    DUMP_VALUE_BLOCK("op_equal_str_cmp", v);
    
    return ret;
}

/**
 * As by specification ' ' VS NUM, we convert NUM to str
 * if FLD(STR) VS NUM, we convert FDL to NUM
 */
int op_equal(UBFH *p_ub, int type, int sub_type, struct ast *l, struct ast *r, value_block_t *v)
{
    int ret=EXSUCCEED;
    value_block_t lval, rval;
    memset(&lval, 0, sizeof(lval));
    memset(&rval, 0, sizeof(rval));
    /* Get the values out of the child stuff */
    if (EXSUCCEED!=eval(p_ub, l, &lval))
    {
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret && EXSUCCEED!=eval(p_ub, r, &rval))
    {
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret)
    {
        if (lval.is_null || rval.is_null)
        {
            /* Not equal... */
            UBF_LOG(log_debug, "LVAR or LVAL is NULL => False");
            v->longval = v->boolval = EXFALSE;
            goto out;
        }
	
        if (( (VALUE_TYPE_STRING==lval.value_type &&
            VALUE_TYPE_STRING==rval.value_type)
            ||
            (VALUE_TYPE_FLD_STR==lval.value_type &&
            VALUE_TYPE_FLD_STR==rval.value_type)
            ||
            (VALUE_TYPE_STRING==lval.value_type &&
            VALUE_TYPE_FLD_STR==rval.value_type)
            ||
            (VALUE_TYPE_FLD_STR==lval.value_type &&
            VALUE_TYPE_STRING==rval.value_type)) &&
                !(type==NODE_TYPE_ADDOP || type==NODE_TYPE_MULTOP) /* do not run math ops */
                )
        {
            ret=op_equal_str_cmp(type, sub_type, &lval, &rval, v);

        }
        else if ((VALUE_TYPE_STRING==lval.value_type ||
            VALUE_TYPE_STRING==rval.value_type) && !(type==NODE_TYPE_ADDOP || 
                type==NODE_TYPE_MULTOP))
        {
            ret=op_equal_str_cmp(type, sub_type, &lval, &rval, v);
        }
        /* if both are longs, then compare them */
        else if (VALUE_TYPE_LONG==lval.value_type && VALUE_TYPE_LONG==rval.value_type)
        {
            ret=op_equal_long_cmp(type, sub_type, &lval, &rval, v);
        }
        else /* limit the scope for is_float_val call */
        {
            int is_lval_float = is_float_val(&lval);
            int is_rval_float = is_float_val(&rval);
#if 0
            /* If any is long and other is not containing float symbols - convert longs & cmp*/
            if (VALUE_TYPE_LONG==lval.value_type && !is_rval_float ||
                     VALUE_TYPE_LONG==rval.value_type && !is_lval_float)
            {
                ret=op_equal_long_cmp(type, sub_type, &lval, &rval, v);
            }
            /* If both strings are not floats, then do the long cmp */
            else
#endif      /* mode (%) we will process as long. */
            if ((!is_lval_float && !is_rval_float) || 
                    (NODE_TYPE_MULTOP==type && MULOP_MOD==sub_type))
            {
                ret=op_equal_long_cmp(type, sub_type, &lval, &rval, v);
            }
            else /* Nothing to do:- downgrate to float comparsation */
            {
                ret=op_equal_float_cmp(type, sub_type, &lval, &rval, v);
            } /* else */
        }

    } /* main SUCCEED==ret */

out:
    /* Ensure that we clean up dynamically allocated FB resources! */
    FREE_UP_UB_BUF((&lval));
    FREE_UP_UB_BUF((&rval));

    return ret;
}

/*
 * Read and cache field id
 */
int get_bfldid(bfldid_t *p_fl)
{
    BFLDID ret=BBADFLDID;

    UBF_LOG(log_debug, "About to get info for [%s]\n", p_fl->fldnm);

    /* Cache this lookup */
    if (BBADFLDID==p_fl->bfldid && BBADFLDID==(p_fl->bfldid=Bfldid(p_fl->fldnm)))
    {
        UBF_LOG(log_error, "Failed to lookup data type for [%s]\n", p_fl->fldnm);
        ret=BBADFLDID;
    }
    else
    {
        ret=p_fl->bfldid;
    }

    return ret;
}

/* This will not use recursion on eval. We will take data out of AST by selves! */
int regexp_eval(UBFH *p_ub, struct ast *l, struct ast *r, value_block_t *v)
{
    int ret=EXSUCCEED;
    char l_buf[MAX_TEXT+1];
    char *p_l=NULL;
    char *p_r=NULL;
    BFLDLEN len=sizeof(l_buf);

    struct ast_string *ls = (struct ast_string *)l;
    struct ast_fld *lf = (struct ast_fld *)l;
    struct ast_func *lfunc = (struct ast_func *)l;
    struct ast_string *rs = (struct ast_string *)r;

    if (NODE_TYPE_FLD==l->nodetype)
    {
        /* Get the value of field */
        if (EXSUCCEED==ret && EXSUCCEED!=(ret=CBget(p_ub, lf->fld.bfldid, lf->fld.occ,
                                    l_buf, &len, BFLD_STRING)))
        {
            if (BNOTPRES==Berror)
            {
                ndrx_Bunset_error(); /* Clear error, because this is not error case! */
                /* This is OK */
                UBF_LOG(log_warn, "Field not present [%s]", lf->fld.fldnm);
                v->value_type=VALUE_TYPE_LONG;
                v->longval=v->boolval=EXFALSE;
                v->is_null=EXTRUE; /* Cannot do compare! */

                return EXSUCCEED; /* <<< RETURN!!! Nothing to do. */
            }
            else
            {
                UBF_LOG(log_warn, "Failed to get [%s] - %s",
                                        lf->fld.fldnm, Bstrerror(Berror));
                ret=EXFAIL;
            }

        }
        else if (EXSUCCEED==ret)
        {
            p_l = l_buf;
        }
    }
    else if (NODE_TYPE_STR==l->nodetype)
    {
        /* Set the pointer to AST node */
        p_l = ls->str;
    }
    else
    {
        /* cannot handle other items, but should be already handled by parser! */
        ndrx_Bset_error_msg(BSYNTAX, "Left side of regex must be const string or FB field");
        ret=EXFAIL;
    }

    /* Right string must be quoted const string! */
    if (EXSUCCEED==ret && NODE_TYPE_STR==r->nodetype)
    {
            p_r = rs->str;
    } /* We do not have correct right side - FAIL */
    else if (EXSUCCEED==ret)
    {
    /* We must handle this by parser */
            UBF_LOG(log_error, "Right side of regexp must be const string! "
                                    "But got node type [%d]\n", r->nodetype);
    ndrx_Bset_error_msg(BSYNTAX, "Right side of regex must be const string");
    }

    if (EXSUCCEED==ret)
    {
        regex_t *re = &(rs->regex.re);
        int err;
        UBF_LOG(log_debug, "Regex left  [%s]", p_l);
        UBF_LOG(log_debug, "Regex right [%s]", p_r);

        /* Now see do we need to compile  */
        if (!rs->regex.compiled)
        {
            UBF_LOG(log_debug, "Compiling regex");
            if (EXSUCCEED!=(err=regcomp(re, p_r, REG_EXTENDED | REG_NOSUB)))
            {
                ndrx_report_regexp_error("regcomp", err, re);
                ret=EXFAIL;
            }
            else
            {
                UBF_LOG(log_debug, "REGEX: Compiled OK");
                rs->regex.compiled = 1;
            }
        }

        if (EXSUCCEED==ret && EXSUCCEED==regexec(re, p_l, (size_t) 0, NULL, 0))
        {
            v->value_type=VALUE_TYPE_LONG;
            v->longval=v->boolval=EXTRUE;
            UBF_LOG(log_debug, "REGEX: matched!");
        }
        else if (EXSUCCEED==ret)
        {
            v->value_type=VALUE_TYPE_LONG;
            v->longval=v->boolval=EXFALSE;
            UBF_LOG(log_debug, "REGEX: NOT matched!");
        }
    }

    /* Dump out the final value */
    DUMP_VALUE_BLOCK("regexp_eval", v);

    return ret;
}

/**
 * This reads FB. But takes out no string value.
 */
int read_unary_func(UBFH *p_ub, struct ast *a, value_block_t * v)
{
    int ret=EXSUCCEED;
    struct ast_func *func = (struct ast_func *)a;
    char *fn = "read_unary_func";

    UBF_LOG(log_debug, "Entering %s func: [%s]",
                                    fn, func->funcname);
    
    /* Call the function... */
    v->value_type=VALUE_TYPE_LONG;
    v->longval=func->f(p_ub, func->funcname);

    if (v->longval)
        v->boolval=EXTRUE;
    else
        v->boolval=EXFALSE;
    
    /* Dump out the final value */
    DUMP_VALUE_BLOCK("read_unary_fb", v);

    UBF_LOG(log_debug, "return %s %d", fn, ret);
    
    return ret;
}
/**
 * This reads FB. But takes out no string value.
 * This reads long, float/double.
 * All others are assumed as 
 */
int read_unary_fb(UBFH *p_ub, struct ast *a, value_block_t * v)
{
    int ret=EXSUCCEED;
    struct ast_fld *fld = (struct ast_fld *)a;
    BFLDID bfldid;
    BFLDOCC occ;
    int fld_type;
    char fn[] = "read_unary_fb()";
    /* Must be already found! */
    bfldid = fld->fld.bfldid;
    occ = fld->fld.occ;

    UBF_LOG(log_debug, "Entering %s fldnm [%s] bfldid=%d occ=%d",
                                    fn, fld->fld.fldnm, bfldid, occ);
	/*
	 * Now read field type.
	 * If string or char, then it becomes as long/bool.
	 * float/double->float (double)
	 * short/long -> long
	 * all true if field present.
	 */
	if (EXSUCCEED==ret)
	{
            fld_type=Bfldtype(bfldid);

            if (!Bpres(p_ub, bfldid, occ))
            {
                UBF_LOG(log_debug, "Field [%s] not present in fb",
                                                    fld->fld.fldnm);
                v->value_type = VALUE_TYPE_LONG;
                v->longval=v->boolval=EXFALSE;
                v->is_null=EXTRUE;
            }
            /* In this case it is just a TRUE. */
            else if (BFLD_STRING==fld_type || BFLD_CARRAY==fld_type || BFLD_CHAR==fld_type)
            {
                BFLDLEN len = MAX_TEXT+1;

                if (NULL==(v->strval=NDRX_MALLOC(len)))
                {
                    UBF_LOG(log_error, "Error malloc fail!");
                    ndrx_Bset_error_fmt(BMALLOC, "Error malloc fail! (cannot allocate %d)", len);
                    ret=EXFAIL;
                }
                else
                {
                    v->dyn_alloc = 1; /* ensure that FREE_UP_UB_BUF can capture these */
                }

                if (EXSUCCEED==ret && EXSUCCEED!=CBget(p_ub, bfldid, occ,
                                (char *)v->strval, &len, BFLD_STRING))
                {
                    if (BNOTPRES==Berror)
                    {
                        ndrx_Bunset_error(); /* clear error */
                        UBF_LOG(log_warn, "Failed to get [%s] as str"
                                                     " - downgrade to FALSE!",
                                                     fld->fld.fldnm);
                        v->value_type = VALUE_TYPE_FLD_STR;
                        v->longval=v->boolval=EXFALSE;
                        v->is_null=EXTRUE;
                    }
                    else /* on all other errors we are going to FAIL! */
                    {
                        UBF_LOG(log_warn, "Failed to get [%s] - %s",
                            fld->fld.fldnm, Bstrerror(Berror));
                        ret=EXFAIL;
                    }
                    /* Lets free memory right right here, why not? */

                    NDRX_FREE(v->strval);
                    v->dyn_alloc = 0;
                    v->strval = NULL;
                }
                else if (EXSUCCEED==ret)
                {
                    v->value_type = VALUE_TYPE_FLD_STR;
                    v->boolval=EXTRUE;
                }

            }
            else if (BFLD_SHORT==fld_type || BFLD_LONG==fld_type)
            {
                if (EXSUCCEED!=CBget(p_ub, bfldid, occ, 
                                (char *)&v->longval, NULL, BFLD_LONG))
                {
                    if (BNOTPRES==Berror)
                    {
                        ndrx_Bunset_error(); /* clear error */
                        UBF_LOG(log_warn, "Failed to get [%s] as long"
                                " - downgrade to FALSE!",
                                fld->fld.fldnm);
                        v->value_type = VALUE_TYPE_LONG;
                        v->longval=v->boolval=EXFALSE;
                        v->is_null=EXTRUE;
                    }
                    else /* on all other errors we are going to FAIL! */
                    {
                        UBF_LOG(log_warn, "Failed to get [%s] - %s",
                            fld->fld.fldnm, Bstrerror(Berror));
                        ret=EXFAIL;
                    }
                }
                else
                {
                        v->value_type = VALUE_TYPE_LONG;
                        v->boolval=EXTRUE;
                }
            }
            else if (BFLD_FLOAT==fld_type || BFLD_DOUBLE==fld_type)
            {
                if (EXSUCCEED!=CBget(p_ub, bfldid, occ, 
                                (char *)&v->floatval, NULL, BFLD_DOUBLE))
                {
                    if (BNOTPRES==Berror)
                    {
                        ndrx_Bunset_error(); /* clear error */
                        UBF_LOG(log_warn, "Failed to get [%s] as double"
                                " - downgrade to FALSE!",
                                fld->fld.fldnm);
                        v->value_type = VALUE_TYPE_LONG;
                        v->longval=v->boolval=EXFALSE;
                        v->is_null=EXTRUE;
                    }
                    else /* on all other errors we are going to FAIL! */
                    {
                        UBF_LOG(log_warn, "Failed to get [%s] - %s",
                            fld->fld.fldnm, Bstrerror(Berror));
                        ret=EXFAIL;
                    }
                }
                else
                {
                    v->value_type = VALUE_TYPE_FLOAT;
                    v->boolval=EXTRUE;
                }
            }
	}

    /* Dump out the final value */
    DUMP_VALUE_BLOCK("read_unary_fb", v);

    UBF_LOG(log_debug, "return %s %d", fn, ret);

    return ret;
}
/**
 * Detect is string looking like float?
 * @param s
 * @return
 */
int is_float(char *s)
{
    if (strpbrk(s, ".,eE")) /* this will work faster than multiple strchrs */
    {
        return EXTRUE;
    }
    return EXFALSE;
}
/**
 * Detect is float value type block.
 * @param v
 * @return
 */
int is_float_val(value_block_t *v)
{
    if ( VALUE_TYPE_FLOAT==v->value_type )
        return EXTRUE;

    if ( VALUE_TYPE_FLD_STR==v->value_type || VALUE_TYPE_STRING==v->value_type)
        return is_float(v->strval);
    
    return EXFALSE;
}

/**
 * Process unary operation.
 * This may have some differences from orginal system when operating
 * with FB fields.
 * @param p_ub - pointer to FB
 * @param op - unary operation (ADDOP_PLUS, ADDOP_MINUS, UNARY_CMPL, UNARY_INV)
 * @param a - tree node containing unary operation (left side node contains the value of)
 * @param v - return value block
 * @return SUCCEED/FAIL
 */
int process_unary(UBFH *p_ub, int op, struct ast *a, value_block_t *v)
{
    int ret=EXSUCCEED;
    value_block_t pri;
    /* Data keepers */
    double f;
    long  l;
    int is_long=EXTRUE;
    char fn[] = "process_unary()";

    memset(&pri, 0, sizeof(pri));
	
    UBF_LOG(log_debug, "Entering %s", fn);

    if (EXSUCCEED==eval(p_ub, a->r, &pri))
    {

        if (VALUE_TYPE_FLD_STR==pri.value_type || 
                VALUE_TYPE_STRING==pri.value_type)
        {
            if (is_float(pri.strval))
            {
                f = atof(pri.strval);
                is_long = EXFALSE;
                UBF_LOG(log_warn, "Treating unary field as "
                                    "float [%f]!", f);
            }
            else
            {
                l = atol(pri.strval);
                is_long = EXTRUE;
                UBF_LOG(log_warn, "Treating unary "
                        "field as long [%ld]", l);
            }
        }
        else if (VALUE_TYPE_FLOAT==pri.value_type)
        {
            is_long=EXFALSE;
            f = pri.floatval;
        }
        else if (VALUE_TYPE_LONG==pri.value_type)
        {
            /* it must be long */
            is_long=EXTRUE;
            l = pri.longval;
        } /* Bug #325 */
        else if (VALUE_TYPE_BOOL!=pri.value_type)
        {
            UBF_LOG(log_warn, "Unknown value type %d op: %d", 
                                pri.value_type, op);
            return EXFAIL;
        }
        
#if 0
        - Bug #325
        if (((op==UNARY_CMPL || op==UNARY_INV) && !is_long) 
                && VALUE_TYPE_BOOL!=pri.value_type)
        {
            /* Convert to long */
            UBF_LOG(log_warn, "! or ~ converting double to long!");
            l = (long) f;
        }
#endif
        
        v->boolval = pri.boolval;

        switch (op)
        {
            case ADDOP_PLUS:
                /* actually do nothing here! */
                if (is_long)
                {
                    v->value_type=VALUE_TYPE_LONG;
                    v->longval = l;
                    
                    if (v->longval)
                        v->boolval=EXTRUE;
                    else
                        v->boolval=EXFALSE;
                    
                }
                else /* float */
                {
                    v->value_type=VALUE_TYPE_FLOAT;
                    v->floatval = f;
                    if (!IS_FLOAT_0(v->floatval))
                        v->boolval=EXTRUE;
                    else
                        v->boolval=EXFALSE;
                }
                break;
            case ADDOP_MINUS:
                /* actually do nothing here! */
                if (is_long)
                {
                    v->value_type=VALUE_TYPE_LONG;
                    v->longval = -l;
                    
                    if (v->longval)
                        v->boolval=EXTRUE;
                    else
                        v->boolval=EXFALSE;
                    
                }
                else /* float */
                {
                    v->value_type=VALUE_TYPE_FLOAT;
                    v->floatval = -f;
                    
                    if (!IS_FLOAT_0(v->floatval))
                        v->boolval=EXTRUE;
                    else
                        v->boolval=EXFALSE;
                }
                break;
            case UNARY_CMPL:
                /* Works only on longs! */
                v->value_type=VALUE_TYPE_LONG;
                v->boolval = ~pri.boolval;
                /* Assuming long as final bool*/
                v->longval = v->boolval;
                break;
            case UNARY_INV:
                v->value_type=VALUE_TYPE_LONG;
                v->boolval = !pri.boolval;
                v->longval = v->boolval;
                break;
        }
    }
    else
    {
        ret=EXFAIL;
    }
    /* Ensure that we clean up dynamically allocated FB resources! */
    FREE_UP_UB_BUF((&pri));
    /* Dump out the final value */
    DUMP_VALUE_BLOCK("process_unary", v);
    UBF_LOG(log_debug, "Return %s %d", fn, ret);
    return ret;
}

/**
 * Doing evaluation recursively.
 * There is specific strategy for FB string buffer hadling.
 * FB strings are put in dynamically allocated buffers. Meaning that these buffers
 * needs to be freed after field wents out of the stack (i.e. on function return)
 *
 * @param p_ub - pointer to FB
 * @param a - root of the AST
 * @param v - value to return.
 * @return SUCCEED/FAIL
 */
int eval(UBFH *p_ub, struct ast *a, value_block_t *v)
{
    int ret=EXSUCCEED;
    value_block_t l, r;
    char fn[] = "eval";
    memset(v, 0, sizeof(*v));
    memset(&l, 0, sizeof(l));
    memset(&r, 0, sizeof(r));

    if (!v)
    {
        ndrx_Bset_error_msg(BNOTFLD, "internal error, null ret value");
            return EXFAIL; /* <<< RETURN HERE! */
    }

    if(!a)
    {
        ndrx_Bset_error_msg(BNOTFLD, "internal error, null eval");
        return EXFAIL; /* <<< RETURN HERE! */
    }
    
    UBF_LOG(log_debug, "%s: id: %02d type: %s sub-type %s", fn,
                    a->nodeid, M_nodetypes[a->nodetype], M_subtypes[a->sub_type]);
	/* Reset that we are successful. */
	
    switch (a->nodetype)
    {
        case NODE_TYPE_OR:
            ret=eval(p_ub, a->l, &l);
            /* So that do we really need to evaluate other side 
             * to keep c style working!
             */
            if (EXSUCCEED==ret && !l.boolval)
                    ret=eval(p_ub, a->r, &r);

            if (EXSUCCEED==ret)
            {
                v->value_type=VALUE_TYPE_LONG;
                v->longval = v->boolval = (l.boolval || r.boolval)?EXTRUE:EXFALSE;
            }
            DUMP_VALUE_BLOCK("NODE_TYPE_OR", v);
            break;
        case NODE_TYPE_AND:
            ret=eval(p_ub, a->l, &l);
            /* Right side needs only if left is true
             * keeping c style
             */
            if (EXSUCCEED==ret && l.boolval)
            {
                ret=eval( p_ub, a->r, &r);
            }

            if (EXSUCCEED==ret)
            {
                v->value_type=VALUE_TYPE_LONG;

                v->longval = v->boolval = (l.boolval && r.boolval)?EXTRUE:EXFALSE;
            }
            DUMP_VALUE_BLOCK("NODE_TYPE_AND", v);
            break;
        case NODE_TYPE_XOR:
            ret = eval( p_ub, a->l, &l);

            if (EXSUCCEED==ret)
            {
                ret = eval( p_ub, a->r, &r);
            }

            if (EXSUCCEED==ret)
            {
                v->value_type=VALUE_TYPE_LONG;

                if ((l.boolval && !r.boolval) || (!l.boolval && r.boolval))
                    v->boolval=EXTRUE;
                else
                    v->boolval=EXFALSE;
            }
            DUMP_VALUE_BLOCK("NODE_TYPE_XOR", v);
            break;
        case NODE_TYPE_EQOP:
            switch (a->sub_type)
            {
                case EQOP_EQUAL:
                    ret = op_equal(p_ub, NODE_TYPE_EQOP, NODE_SUB_TYPE_DEF, a->l, a->r, v);
                    break;
                case EQOP_NOT_EQUAL:
                    ret = op_equal(p_ub, NODE_TYPE_EQOP, NODE_SUB_TYPE_DEF, a->l, a->r, v);
                    if (EXSUCCEED==ret)
                    {
                        /* Inverse the result. */
                        v->boolval = !v->boolval;
                        v->longval = !v->longval;
                    }
                    DUMP_VALUE_BLOCK("EQOP_NOT_EQUAL", v);
                    break;
                case EQOP_REGEX_EQUAL:
                    ret=regexp_eval(p_ub, a->l, a->r, v);
                    break;
                case EQOP_REGEX_NOT_EQUAL:
                    ret=regexp_eval(p_ub, a->l, a->r, v);
                    if (EXSUCCEED==ret)
                    {
                        /* Inverse the result. */
                        v->boolval = !v->boolval;
                        v->longval = !v->longval;
                    }
                    DUMP_VALUE_BLOCK("EQOP_REGEX_NOT_EQUAL", v);
                    break;
            }
            break;
        case NODE_TYPE_RELOP:
            ret = op_equal(p_ub, NODE_TYPE_RELOP, a->sub_type, a->l, a->r, v);
            break;
        case NODE_TYPE_ADDOP:
            ret = op_equal(p_ub, NODE_TYPE_ADDOP, a->sub_type, a->l, a->r, v);
            break;
        case NODE_TYPE_MULTOP:
            ret = op_equal(p_ub, NODE_TYPE_MULTOP, a->sub_type, a->l, a->r, v);
            break;
        case NODE_TYPE_UNARY:
            ret = process_unary(p_ub, a->sub_type, a, v);
            break;
        case NODE_TYPE_FLD:
            /* read the field */
            ret=read_unary_fb(p_ub, a, v);
            break;
        case NODE_TYPE_FUNC:
            /* Get func unary... */
            ret=read_unary_func(p_ub, a, v);
            break;
        case NODE_TYPE_STR:
            /* In this case we assume it is TRUE 
             * We do not use string value block so that we do not get
             * stack full of those.
             */
            v->value_type = VALUE_TYPE_STRING;
            v->boolval = EXTRUE; 
            {
                struct ast_string *s = (struct ast_string *)a;
                /* Make pointer to point to the AST str value. */
                /* strcpy(v->strval, s->str); */
                v->strval = s->str;
            }
            /* dump the final value */
            DUMP_VALUE_BLOCK("NODE_TYPE_STR", v);
            break;
        case NODE_TYPE_FLOAT:
            v->value_type = VALUE_TYPE_FLOAT;
            /* Copy off the string value */
            {
                struct ast_float * a_f = (struct ast_float*)a;
                v->floatval = a_f->d;
                /* Set the boolean value of this stuff */
                if (!IS_FLOAT_0(v->floatval))
                    v->boolval = EXTRUE;
                else
                    v->boolval = EXFALSE;
            }
            /* dump the final value */
             DUMP_VALUE_BLOCK("VALUE_TYPE_FLOAT", v);
            break;
        case NODE_TYPE_LONG:
            v->value_type = VALUE_TYPE_LONG;
            /* Copy off the string value */
            {
                struct ast_long * a_long = (struct ast_long*)a;
                v->longval = a_long->l;
                /* Set the boolean value of this stuff */
                if (v->longval)
                    v->boolval = EXTRUE;
                else
                    v->boolval = EXFALSE;
            }
            /* dump the final value */
            DUMP_VALUE_BLOCK("VALUE_TYPE_LONG", v);
            break;
    }
    /* Free up resources that may be allocated! */
    FREE_UP_UB_BUF((&l));
    FREE_UP_UB_BUF((&r));
    return ret;
}
/* ===========================================================================*/
/* =========================API FUNCTIONS=====================================*/
/* ===========================================================================*/

expublic char * ndrx_Bboolco (char * expr)
{

    char *ret=NULL;
    char *fn = "Bboolco";
    extern  int yycolumn;
    char *expr_int;
    int buf_len = strlen(expr)+2;
    UBF_LOG(log_debug, "%s: Compiling expression [%s]", fn, expr);

    /* We have some trouble with flex! For marking the end of the line
     * So we will put there newline... not the best practice, but it will work. */
    expr_int = NDRX_MALLOC(buf_len);

    if (NULL==expr_int)
    {
        ndrx_Bset_error_fmt(BMALLOC, "cannot allocate %d bytes for expression copy",
                                    buf_len);
    }
    else
    {
        /* All OK */
        NDRX_STRNCPY_SAFE(expr_int, expr, buf_len);
        strcat(expr_int, "\n");

        yy_scan_string(expr_int);
        G_p_root_node = NULL;
        G_node_count = 0;
        G_error = 0;
        yycolumn = 1;

        M_first_mem=NULL;
        M_cur_mem=NULL;

        if (EXSUCCEED==yyparse() && NULL!=G_p_root_node && EXFAIL!=G_error)
        {
            ret=(char *)G_p_root_node;
            remove_resouce_list();
        }
        else
        {
            remove_resouces();
        }

	/* wrapper call to yylex_destroy */
        _free_parser();

        NDRX_FREE(expr_int);
    }
    UBF_LOG(log_debug, "%s: return %p", fn, ret);
    return ret;
}

expublic int ndrx_Bboolev (UBFH * p_ub, char *tree)
{
    int ret=EXSUCCEED;
    value_block_t v;
    struct ast *a = (struct ast *)tree;
    memset(&v, 0, sizeof(v));

    if (NULL==tree)
    {
        ndrx_Bset_error_msg(BEINVAL, "NULL tree passed for eval!");
        return EXFAIL; /* <<< RETURN */
    }
    /*
     * Evaluate the parse tree
     */
    if (EXSUCCEED==eval(p_ub, a, &v))
    {
        if (v.boolval)
        {
            ret=EXTRUE;
        }
        else
        {
            ret=EXFALSE;
        }
    }
    else
    {
        ret=EXFAIL;
    }

    FREE_UP_UB_BUF((&v));

    return ret;
}

/**
 * This function is not tested very hardly but it shows up that it should work fine
 * @param p_ub
 * @param tree
 * @return
 */
expublic double ndrx_Bfloatev (UBFH * p_ub, char *tree)
{
    double ret=0.0;
    value_block_t v;
    struct ast *a = (struct ast *)tree;
    memset(&v, 0, sizeof(v));

    if (NULL==tree)
    {
        ndrx_Bset_error_msg(BEINVAL, "NULL tree passed for eval!");
        return EXFAIL; /* <<< RETURN */
    }
    /*
     * Evaluate the parse tree
     */
    if (EXSUCCEED==eval(p_ub, a, &v))
    {
        if (VALUE_TYPE_FLOAT!=v.value_type)
            get_float(&v);
        ret=v.floatval;
    }
    else
    {
        ret=EXFAIL;
    }

    FREE_UP_UB_BUF((&v));

    return ret;
}
/**
 * Free up used tree.
 * This recursively frees up resources. Should be called once compiled tree
 * is not needed.
 * @param tree
 */
expublic void ndrx_Btreefree (char *tree)
{
    struct ast *a = (struct ast *)tree;
    struct ast_string *a_string = (struct ast_string *)tree;

    if (NULL==tree)
        return; /* <<<< RETURN! Nothing to do! */

    UBF_LOG(6, "Free up buffer %p nodeid=%d nodetype=%d", tree, a->nodeid, a->nodetype);
    switch (a->nodetype)
    {
        case NODE_TYPE_FUNC:
            /* nothing to do */
            break;
        case NODE_TYPE_FLD:
            /* nothing to do */
            break;
        case NODE_TYPE_STR:
            /* Free up internal resources (if have such)? */
            if (NULL!=a_string->str)
                NDRX_FREE(a_string->str);

            /* check regexp, maybe needs to clean up? */
            if (a_string->regex.compiled)
                regfree(&(a_string->regex.re));
            break;
        case NODE_TYPE_FLOAT:
            /* nothing to do */
            break;
        case NODE_TYPE_LONG:
            /* nothing to do */
            break;
        default:
            if (a->l)
            {
                ndrx_Btreefree ((char *)a->l);
            }
            if (a->r)
            {
                ndrx_Btreefree ((char *)a->r);
            }
            break;
    }
    /* delete self */
    NDRX_FREE(tree);

}

/**
 * Print expression tree to file
 * @param tree - evaluation tree
 * @param outf - file to print to 
 */
expublic void ndrx_Bboolpr (char * tree, FILE *outf)
{
    int ret=EXSUCCEED;

    struct ast *a = (struct ast *)tree;
    struct ast_string *a_string = (struct ast_string *)tree;

    if (NULL==tree)
        return; /* <<<< RETURN! Nothing to do! */


    if (ferror(outf))
    {
        return;
    }

    switch (a->nodetype)
    {
        case NODE_TYPE_FUNC:
            {
                /* print func */
                struct ast_func *a_func = (struct ast_func *)tree;
                fprintf(outf, "%s()", a_func->funcname);
            }
            break;
        case NODE_TYPE_FLD:
            {
                /* print field */
                struct ast_fld *a_fld = (struct ast_fld *)tree;
                fprintf(outf, "%s", a_fld->fld.fldnm);
            }
            break;
        case NODE_TYPE_STR:
            
            /* print string value */
            if (NULL!=a_string->str)
                fprintf(outf, "'%s'", a_string->str);
            else
                fprintf(outf, "<null>");
            
            break;
        case NODE_TYPE_FLOAT:
            {
                /* Print float value */
                struct ast_float *a_float = (struct ast_float *)tree;
                /* print field */
                fprintf(outf, "%.*lf", DOUBLE_RESOLUTION, a_float->d);
            }
            break;
        case NODE_TYPE_LONG:
            {
                /* Print long value */
                struct ast_long *a_long = (struct ast_long *)tree;
                fprintf(outf, "%ld", a_long->l);
            }
            break;
        default:
            fprintf(outf, "(");
            if (a->l)
            {
                ndrx_Bboolpr ((char *)a->l, outf);
            }
            fprintf(outf, "%s", M_subtypes_sign_only[a->sub_type]);
            if (a->r)
            {
                ndrx_Bboolpr ((char *)a->r, outf);
            }
            fprintf(outf, ")");
            break;
    }
}

/*
 * Set callback function
 */
expublic int ndrx_Bboolsetcbf (char *funcname, 
        long (*functionPtr)(UBFH *p_ub, char *funcname))
{

    int ret=EXSUCCEED;
    char *fn = "_Bsetcbfunc";
    int len;
    
    UBF_LOG(log_debug, "%s: setting callback function [%s]:%p", fn, 
            funcname, functionPtr);

    if (NULL==funcname || (len=strlen(funcname)) < 3 || len > MAX_FUNC_NAME-2)
    {
        ndrx_Bset_error_fmt(BBADNAME, "Bad function name passed [%s]", funcname);
        ret=EXFAIL;
        goto out;
    }
    
    ret = set_func(funcname, functionPtr);
    
out:
    UBF_LOG(log_debug, "%s: return %p", fn, ret);
    return ret;
}

