/* see copyright notice in pscript.h */
#include <pscript.h>
#include <math.h>
#include <stdlib.h>
#include <psstdmath.h>

#define SINGLE_ARG_FUNC(_funcname) static PSInteger math_##_funcname(HPSCRIPTVM v){ \
    PSFloat f; \
    ps_getfloat(v,2,&f); \
    ps_pushfloat(v,(PSFloat)_funcname(f)); \
    return 1; \
}

#define TWO_ARGS_FUNC(_funcname) static PSInteger math_##_funcname(HPSCRIPTVM v){ \
    PSFloat p1,p2; \
    ps_getfloat(v,2,&p1); \
    ps_getfloat(v,3,&p2); \
    ps_pushfloat(v,(PSFloat)_funcname(p1,p2)); \
    return 1; \
}

static PSInteger math_srand(HPSCRIPTVM v)
{
    PSInteger i;
    if(PS_FAILED(ps_getinteger(v,2,&i)))
        return ps_throwerror(v,_SC("invalid param"));
    srand((unsigned int)i);
    return 0;
}

static PSInteger math_rand(HPSCRIPTVM v)
{
    ps_pushinteger(v,rand());
    return 1;
}

static PSInteger math_abs(HPSCRIPTVM v)
{
    PSInteger n;
    ps_getinteger(v,2,&n);
    ps_pushinteger(v,(PSInteger)abs((int)n));
    return 1;
}

SINGLE_ARG_FUNC(sqrt)
SINGLE_ARG_FUNC(fabs)
SINGLE_ARG_FUNC(sin)
SINGLE_ARG_FUNC(cos)
SINGLE_ARG_FUNC(asin)
SINGLE_ARG_FUNC(acos)
SINGLE_ARG_FUNC(log)
SINGLE_ARG_FUNC(log10)
SINGLE_ARG_FUNC(tan)
SINGLE_ARG_FUNC(atan)
TWO_ARGS_FUNC(atan2)
TWO_ARGS_FUNC(pow)
SINGLE_ARG_FUNC(floor)
SINGLE_ARG_FUNC(ceil)
SINGLE_ARG_FUNC(exp)

#define _DECL_FUNC(name,nparams,tycheck) {_SC(#name),math_##name,nparams,tycheck}
static const PSRegFunction mathlib_funcs[] = {
    _DECL_FUNC(sqrt,2,_SC(".n")),
    _DECL_FUNC(sin,2,_SC(".n")),
    _DECL_FUNC(cos,2,_SC(".n")),
    _DECL_FUNC(asin,2,_SC(".n")),
    _DECL_FUNC(acos,2,_SC(".n")),
    _DECL_FUNC(log,2,_SC(".n")),
    _DECL_FUNC(log10,2,_SC(".n")),
    _DECL_FUNC(tan,2,_SC(".n")),
    _DECL_FUNC(atan,2,_SC(".n")),
    _DECL_FUNC(atan2,3,_SC(".nn")),
    _DECL_FUNC(pow,3,_SC(".nn")),
    _DECL_FUNC(floor,2,_SC(".n")),
    _DECL_FUNC(ceil,2,_SC(".n")),
    _DECL_FUNC(exp,2,_SC(".n")),
    _DECL_FUNC(srand,2,_SC(".n")),
    _DECL_FUNC(rand,1,NULL),
    _DECL_FUNC(fabs,2,_SC(".n")),
    _DECL_FUNC(abs,2,_SC(".n")),
    {NULL,(PSFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

PSRESULT psstd_register_mathlib(HPSCRIPTVM v)
{
    PSInteger i=0;
    while(mathlib_funcs[i].name!=0) {
        ps_pushstring(v,mathlib_funcs[i].name,-1);
        ps_newclosure(v,mathlib_funcs[i].f,0);
        ps_setparamscheck(v,mathlib_funcs[i].nparamscheck,mathlib_funcs[i].typemask);
        ps_setnativeclosurename(v,-1,mathlib_funcs[i].name);
        ps_newslot(v,-3,PSFalse);
        i++;
    }
    ps_pushstring(v,_SC("RAND_MAX"),-1);
    ps_pushinteger(v,RAND_MAX);
    ps_newslot(v,-3,PSFalse);
    ps_pushstring(v,_SC("PI"),-1);
    ps_pushfloat(v,(PSFloat)M_PI);
    ps_newslot(v,-3,PSFalse);
    return PS_OK;
}
