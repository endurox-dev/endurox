/*
    see copyright notice in pscript.h
*/
#include "pspcheader.h"
#include "psvm.h"
#include "psstring.h"
#include "pstable.h"
#include "psarray.h"
#include "psfuncproto.h"
#include "psclosure.h"
#include "psclass.h"
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

static bool str2num(const PSChar *s,PSObjectPtr &res,PSInteger base)
{
    PSChar *end;
    const PSChar *e = s;
    bool iseintbase = base > 13; //to fix error converting hexadecimals with e like 56f0791e
    bool isfloat = false;
    PSChar c;
    while((c = *e) != _SC('\0'))
    {
        if (c == _SC('.') || (!iseintbase && (c == _SC('E') || c == _SC('e')))) { //e and E is for scientific notation
            isfloat = true;
            break;
        }
        e++;
    }
    if(isfloat){
        PSFloat r = PSFloat(scstrtod(s,&end));
        if(s == end) return false;
        res = r;
    }
    else{
        PSInteger r = PSInteger(scstrtol(s,&end,(int)base));
        if(s == end) return false;
        res = r;
    }
    return true;
}

static PSInteger base_dummy(HPSCRIPTVM PS_UNUSED_ARG(v))
{
    return 0;
}

#ifndef NO_GARBAGE_COLLECTOR
static PSInteger base_collectgarbage(HPSCRIPTVM v)
{
    ps_pushinteger(v, ps_collectgarbage(v));
    return 1;
}
static PSInteger base_resurectureachable(HPSCRIPTVM v)
{
    ps_resurrectunreachable(v);
    return 1;
}
#endif

static PSInteger base_getroottable(HPSCRIPTVM v)
{
    v->Push(v->_roottable);
    return 1;
}

static PSInteger base_getconsttable(HPSCRIPTVM v)
{
    v->Push(_ss(v)->_consts);
    return 1;
}


static PSInteger base_setroottable(HPSCRIPTVM v)
{
    PSObjectPtr o = v->_roottable;
    if(PS_FAILED(ps_setroottable(v))) return PS_ERROR;
    v->Push(o);
    return 1;
}

static PSInteger base_setconsttable(HPSCRIPTVM v)
{
    PSObjectPtr o = _ss(v)->_consts;
    if(PS_FAILED(ps_setconsttable(v))) return PS_ERROR;
    v->Push(o);
    return 1;
}

static PSInteger base_seterrorhandler(HPSCRIPTVM v)
{
    ps_seterrorhandler(v);
    return 0;
}

static PSInteger base_setdebughook(HPSCRIPTVM v)
{
    ps_setdebughook(v);
    return 0;
}

static PSInteger base_enabledebuginfo(HPSCRIPTVM v)
{
    PSObjectPtr &o=stack_get(v,2);

    ps_enabledebuginfo(v,PSVM::IsFalse(o)?PSFalse:PSTrue);
    return 0;
}

static PSInteger __getcallstackinfos(HPSCRIPTVM v,PSInteger level)
{
    PSStackInfos si;
    PSInteger seq = 0;
    const PSChar *name = NULL;

    if (PS_SUCCEEDED(ps_stackinfos(v, level, &si)))
    {
        const PSChar *fn = _SC("unknown");
        const PSChar *src = _SC("unknown");
        if(si.funcname)fn = si.funcname;
        if(si.source)src = si.source;
        ps_newtable(v);
        ps_pushstring(v, _SC("func"), -1);
        ps_pushstring(v, fn, -1);
        ps_newslot(v, -3, PSFalse);
        ps_pushstring(v, _SC("src"), -1);
        ps_pushstring(v, src, -1);
        ps_newslot(v, -3, PSFalse);
        ps_pushstring(v, _SC("line"), -1);
        ps_pushinteger(v, si.line);
        ps_newslot(v, -3, PSFalse);
        ps_pushstring(v, _SC("locals"), -1);
        ps_newtable(v);
        seq=0;
        while ((name = ps_getlocal(v, level, seq))) {
            ps_pushstring(v, name, -1);
            ps_push(v, -2);
            ps_newslot(v, -4, PSFalse);
            ps_pop(v, 1);
            seq++;
        }
        ps_newslot(v, -3, PSFalse);
        return 1;
    }

    return 0;
}
static PSInteger base_getstackinfos(HPSCRIPTVM v)
{
    PSInteger level;
    ps_getinteger(v, -1, &level);
    return __getcallstackinfos(v,level);
}

static PSInteger base_assert(HPSCRIPTVM v)
{
    if(PSVM::IsFalse(stack_get(v,2))){
        return ps_throwerror(v,_SC("assertion failed"));
    }
    return 0;
}

static PSInteger get_slice_params(HPSCRIPTVM v,PSInteger &sidx,PSInteger &eidx,PSObjectPtr &o)
{
    PSInteger top = ps_gettop(v);
    sidx=0;
    eidx=0;
    o=stack_get(v,1);
    if(top>1){
        PSObjectPtr &start=stack_get(v,2);
        if(type(start)!=OT_NULL && ps_isnumeric(start)){
            sidx=tointeger(start);
        }
    }
    if(top>2){
        PSObjectPtr &end=stack_get(v,3);
        if(ps_isnumeric(end)){
            eidx=tointeger(end);
        }
    }
    else {
        eidx = ps_getsize(v,1);
    }
    return 1;
}

static PSInteger base_print(HPSCRIPTVM v)
{
    const PSChar *str;
    if(PS_SUCCEEDED(ps_tostring(v,2)))
    {
        if(PS_SUCCEEDED(ps_getstring(v,-1,&str))) {
            if(_ss(v)->_printfunc) _ss(v)->_printfunc(v,_SC("%s"),str);
            return 0;
        }
    }
    return PS_ERROR;
}

static PSInteger base_error(HPSCRIPTVM v)
{
    const PSChar *str;
    if(PS_SUCCEEDED(ps_tostring(v,2)))
    {
        if(PS_SUCCEEDED(ps_getstring(v,-1,&str))) {
            if(_ss(v)->_errorfunc) _ss(v)->_errorfunc(v,_SC("%s"),str);
            return 0;
        }
    }
    return PS_ERROR;
}

static PSInteger base_compilestring(HPSCRIPTVM v)
{
    PSInteger nargs=ps_gettop(v);
    const PSChar *src=NULL,*name=_SC("unnamedbuffer");
    PSInteger size;
    ps_getstring(v,2,&src);
    size=ps_getsize(v,2);
    if(nargs>2){
        ps_getstring(v,3,&name);
    }
    if(PS_SUCCEEDED(ps_compilebuffer(v,src,size,name,PSFalse)))
        return 1;
    else
        return PS_ERROR;
}

static PSInteger base_newthread(HPSCRIPTVM v)
{
    PSObjectPtr &func = stack_get(v,2);
    PSInteger stksize = (_closure(func)->_function->_stacksize << 1) +2;
    HPSCRIPTVM newv = ps_newthread(v, (stksize < MIN_STACK_OVERHEAD + 2)? MIN_STACK_OVERHEAD + 2 : stksize);
    ps_move(newv,v,-2);
    return 1;
}

static PSInteger base_suspend(HPSCRIPTVM v)
{
    return ps_suspendvm(v);
}

static PSInteger base_array(HPSCRIPTVM v)
{
    PSArray *a;
    PSObject &size = stack_get(v,2);
    if(ps_gettop(v) > 2) {
        a = PSArray::Create(_ss(v),0);
        a->Resize(tointeger(size),stack_get(v,3));
    }
    else {
        a = PSArray::Create(_ss(v),tointeger(size));
    }
    v->Push(a);
    return 1;
}

static PSInteger base_type(HPSCRIPTVM v)
{
    PSObjectPtr &o = stack_get(v,2);
    v->Push(PSString::Create(_ss(v),GetTypeName(o),-1));
    return 1;
}

static PSInteger base_callee(HPSCRIPTVM v)
{
    if(v->_callsstacksize > 1)
    {
        v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
        return 1;
    }
    return ps_throwerror(v,_SC("no closure in the calls stack"));
}

static const PSRegFunction base_funcs[]={
    //generic
    {_SC("seterrorhandler"),base_seterrorhandler,2, NULL},
    {_SC("setdebughook"),base_setdebughook,2, NULL},
    {_SC("enabledebuginfo"),base_enabledebuginfo,2, NULL},
    {_SC("getstackinfos"),base_getstackinfos,2, _SC(".n")},
    {_SC("getroottable"),base_getroottable,1, NULL},
    {_SC("setroottable"),base_setroottable,2, NULL},
    {_SC("getconsttable"),base_getconsttable,1, NULL},
    {_SC("setconsttable"),base_setconsttable,2, NULL},
    {_SC("assert"),base_assert,2, NULL},
    {_SC("print"),base_print,2, NULL},
    {_SC("error"),base_error,2, NULL},
    {_SC("compilestring"),base_compilestring,-2, _SC(".ss")},
    {_SC("newthread"),base_newthread,2, _SC(".c")},
    {_SC("suspend"),base_suspend,-1, NULL},
    {_SC("array"),base_array,-2, _SC(".n")},
    {_SC("type"),base_type,2, NULL},
    {_SC("callee"),base_callee,0,NULL},
    {_SC("dummy"),base_dummy,0,NULL},
#ifndef NO_GARBAGE_COLLECTOR
    {_SC("collectgarbage"),base_collectgarbage,0, NULL},
    {_SC("resurrectunreachable"),base_resurectureachable,0, NULL},
#endif
    {NULL,(PSFUNCTION)0,0,NULL}
};

void ps_base_register(HPSCRIPTVM v)
{
    PSInteger i=0;
    ps_pushroottable(v);
    while(base_funcs[i].name!=0) {
        ps_pushstring(v,base_funcs[i].name,-1);
        ps_newclosure(v,base_funcs[i].f,0);
        ps_setnativeclosurename(v,-1,base_funcs[i].name);
        ps_setparamscheck(v,base_funcs[i].nparamscheck,base_funcs[i].typemask);
        ps_newslot(v,-3, PSFalse);
        i++;
    }

    ps_pushstring(v,_SC("_versionnumber_"),-1);
    ps_pushinteger(v,PSCRIPT_VERSION_NUMBER);
    ps_newslot(v,-3, PSFalse);
    ps_pushstring(v,_SC("_version_"),-1);
    ps_pushstring(v,PSCRIPT_VERSION,-1);
    ps_newslot(v,-3, PSFalse);
    ps_pushstring(v,_SC("_charsize_"),-1);
    ps_pushinteger(v,sizeof(PSChar));
    ps_newslot(v,-3, PSFalse);
    ps_pushstring(v,_SC("_intsize_"),-1);
    ps_pushinteger(v,sizeof(PSInteger));
    ps_newslot(v,-3, PSFalse);
    ps_pushstring(v,_SC("_floatsize_"),-1);
    ps_pushinteger(v,sizeof(PSFloat));
    ps_newslot(v,-3, PSFalse);
    ps_pop(v,1);
}

static PSInteger default_delegate_len(HPSCRIPTVM v)
{
    v->Push(PSInteger(ps_getsize(v,1)));
    return 1;
}

static PSInteger default_delegate_tofloat(HPSCRIPTVM v)
{
    PSObjectPtr &o=stack_get(v,1);
    switch(type(o)){
    case OT_STRING:{
        PSObjectPtr res;
        if(str2num(_stringval(o),res,10)){
            v->Push(PSObjectPtr(tofloat(res)));
            break;
        }}
        return ps_throwerror(v, _SC("cannot convert the string"));
        break;
    case OT_INTEGER:case OT_FLOAT:
        v->Push(PSObjectPtr(tofloat(o)));
        break;
    case OT_BOOL:
        v->Push(PSObjectPtr((PSFloat)(_integer(o)?1:0)));
        break;
    default:
        v->PushNull();
        break;
    }
    return 1;
}

static PSInteger default_delegate_tointeger(HPSCRIPTVM v)
{
    PSObjectPtr &o=stack_get(v,1);
    PSInteger base = 10;
    if(ps_gettop(v) > 1) {
        ps_getinteger(v,2,&base);
    }
    switch(type(o)){
    case OT_STRING:{
        PSObjectPtr res;
        if(str2num(_stringval(o),res,base)){
            v->Push(PSObjectPtr(tointeger(res)));
            break;
        }}
        return ps_throwerror(v, _SC("cannot convert the string"));
        break;
    case OT_INTEGER:case OT_FLOAT:
        v->Push(PSObjectPtr(tointeger(o)));
        break;
    case OT_BOOL:
        v->Push(PSObjectPtr(_integer(o)?(PSInteger)1:(PSInteger)0));
        break;
    default:
        v->PushNull();
        break;
    }
    return 1;
}

static PSInteger default_delegate_tostring(HPSCRIPTVM v)
{
    if(PS_FAILED(ps_tostring(v,1)))
        return PS_ERROR;
    return 1;
}

static PSInteger obj_delegate_weakref(HPSCRIPTVM v)
{
    ps_weakref(v,1);
    return 1;
}

static PSInteger obj_clear(HPSCRIPTVM v)
{
    return ps_clear(v,-1);
}


static PSInteger number_delegate_tochar(HPSCRIPTVM v)
{
    PSObject &o=stack_get(v,1);
    PSChar c = (PSChar)tointeger(o);
    v->Push(PSString::Create(_ss(v),(const PSChar *)&c,1));
    return 1;
}



/////////////////////////////////////////////////////////////////
//TABLE DEFAULT DELEGATE

static PSInteger table_rawdelete(HPSCRIPTVM v)
{
    if(PS_FAILED(ps_rawdeleteslot(v,1,PSTrue)))
        return PS_ERROR;
    return 1;
}


static PSInteger container_rawexists(HPSCRIPTVM v)
{
    if(PS_SUCCEEDED(ps_rawget(v,-2))) {
        ps_pushbool(v,PSTrue);
        return 1;
    }
    ps_pushbool(v,PSFalse);
    return 1;
}

static PSInteger container_rawset(HPSCRIPTVM v)
{
    return ps_rawset(v,-3);
}


static PSInteger container_rawget(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_rawget(v,-2))?1:PS_ERROR;
}

static PSInteger table_setdelegate(HPSCRIPTVM v)
{
    if(PS_FAILED(ps_setdelegate(v,-2)))
        return PS_ERROR;
    ps_push(v,-1); // -1 because ps_setdelegate pops 1
    return 1;
}

static PSInteger table_getdelegate(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_getdelegate(v,-1))?1:PS_ERROR;
}

const PSRegFunction PSSharedState::_table_default_delegate_funcz[]={
    {_SC("len"),default_delegate_len,1, _SC("t")},
    {_SC("rawget"),container_rawget,2, _SC("t")},
    {_SC("rawset"),container_rawset,3, _SC("t")},
    {_SC("rawdelete"),table_rawdelete,2, _SC("t")},
    {_SC("rawin"),container_rawexists,2, _SC("t")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("clear"),obj_clear,1, _SC(".")},
    {_SC("setdelegate"),table_setdelegate,2, _SC(".t|o")},
    {_SC("getdelegate"),table_getdelegate,1, _SC(".")},
    {NULL,(PSFUNCTION)0,0,NULL}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

static PSInteger array_append(HPSCRIPTVM v)
{
    return ps_arrayappend(v,-2);
}

static PSInteger array_extend(HPSCRIPTVM v)
{
    _array(stack_get(v,1))->Extend(_array(stack_get(v,2)));
    return 0;
}

static PSInteger array_reverse(HPSCRIPTVM v)
{
    return ps_arrayreverse(v,-1);
}

static PSInteger array_pop(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_arraypop(v,1,PSTrue))?1:PS_ERROR;
}

static PSInteger array_top(HPSCRIPTVM v)
{
    PSObject &o=stack_get(v,1);
    if(_array(o)->Size()>0){
        v->Push(_array(o)->Top());
        return 1;
    }
    else return ps_throwerror(v,_SC("top() on a empty array"));
}

static PSInteger array_insert(HPSCRIPTVM v)
{
    PSObject &o=stack_get(v,1);
    PSObject &idx=stack_get(v,2);
    PSObject &val=stack_get(v,3);
    if(!_array(o)->Insert(tointeger(idx),val))
        return ps_throwerror(v,_SC("index out of range"));
    return 0;
}

static PSInteger array_remove(HPSCRIPTVM v)
{
    PSObject &o = stack_get(v, 1);
    PSObject &idx = stack_get(v, 2);
    if(!ps_isnumeric(idx)) return ps_throwerror(v, _SC("wrong type"));
    PSObjectPtr val;
    if(_array(o)->Get(tointeger(idx), val)) {
        _array(o)->Remove(tointeger(idx));
        v->Push(val);
        return 1;
    }
    return ps_throwerror(v, _SC("idx out of range"));
}

static PSInteger array_resize(HPSCRIPTVM v)
{
    PSObject &o = stack_get(v, 1);
    PSObject &nsize = stack_get(v, 2);
    PSObjectPtr fill;
    if(ps_isnumeric(nsize)) {
        if(ps_gettop(v) > 2)
            fill = stack_get(v, 3);
        _array(o)->Resize(tointeger(nsize),fill);
        return 0;
    }
    return ps_throwerror(v, _SC("size must be a number"));
}

static PSInteger __map_array(PSArray *dest,PSArray *src,HPSCRIPTVM v) {
    PSObjectPtr temp;
    PSInteger size = src->Size();
    for(PSInteger n = 0; n < size; n++) {
        src->Get(n,temp);
        v->Push(src);
        v->Push(temp);
        if(PS_FAILED(ps_call(v,2,PSTrue,PSFalse))) {
            return PS_ERROR;
        }
        dest->Set(n,v->GetUp(-1));
        v->Pop();
    }
    return 0;
}

static PSInteger array_map(HPSCRIPTVM v)
{
    PSObject &o = stack_get(v,1);
    PSInteger size = _array(o)->Size();
    PSObjectPtr ret = PSArray::Create(_ss(v),size);
    if(PS_FAILED(__map_array(_array(ret),_array(o),v)))
        return PS_ERROR;
    v->Push(ret);
    return 1;
}

static PSInteger array_apply(HPSCRIPTVM v)
{
    PSObject &o = stack_get(v,1);
    if(PS_FAILED(__map_array(_array(o),_array(o),v)))
        return PS_ERROR;
    return 0;
}

static PSInteger array_reduce(HPSCRIPTVM v)
{
    PSObject &o = stack_get(v,1);
    PSArray *a = _array(o);
    PSInteger size = a->Size();
    if(size == 0) {
        return 0;
    }
    PSObjectPtr res;
    a->Get(0,res);
    if(size > 1) {
        PSObjectPtr other;
        for(PSInteger n = 1; n < size; n++) {
            a->Get(n,other);
            v->Push(o);
            v->Push(res);
            v->Push(other);
            if(PS_FAILED(ps_call(v,3,PSTrue,PSFalse))) {
                return PS_ERROR;
            }
            res = v->GetUp(-1);
            v->Pop();
        }
    }
    v->Push(res);
    return 1;
}

static PSInteger array_filter(HPSCRIPTVM v)
{
    PSObject &o = stack_get(v,1);
    PSArray *a = _array(o);
    PSObjectPtr ret = PSArray::Create(_ss(v),0);
    PSInteger size = a->Size();
    PSObjectPtr val;
    for(PSInteger n = 0; n < size; n++) {
        a->Get(n,val);
        v->Push(o);
        v->Push(n);
        v->Push(val);
        if(PS_FAILED(ps_call(v,3,PSTrue,PSFalse))) {
            return PS_ERROR;
        }
        if(!PSVM::IsFalse(v->GetUp(-1))) {
            _array(ret)->Append(val);
        }
        v->Pop();
    }
    v->Push(ret);
    return 1;
}

static PSInteger array_find(HPSCRIPTVM v)
{
    PSObject &o = stack_get(v,1);
    PSObjectPtr &val = stack_get(v,2);
    PSArray *a = _array(o);
    PSInteger size = a->Size();
    PSObjectPtr temp;
    for(PSInteger n = 0; n < size; n++) {
        bool res = false;
        a->Get(n,temp);
        if(PSVM::IsEqual(temp,val,res) && res) {
            v->Push(n);
            return 1;
        }
    }
    return 0;
}


static bool _sort_compare(HPSCRIPTVM v,PSObjectPtr &a,PSObjectPtr &b,PSInteger func,PSInteger &ret)
{
    if(func < 0) {
        if(!v->ObjCmp(a,b,ret)) return false;
    }
    else {
        PSInteger top = ps_gettop(v);
        ps_push(v, func);
        ps_pushroottable(v);
        v->Push(a);
        v->Push(b);
        if(PS_FAILED(ps_call(v, 3, PSTrue, PSFalse))) {
            if(!ps_isstring( v->_lasterror))
                v->Raise_Error(_SC("compare func failed"));
            return false;
        }
        if(PS_FAILED(ps_getinteger(v, -1, &ret))) {
            v->Raise_Error(_SC("numeric value expected as return value of the compare function"));
            return false;
        }
        ps_settop(v, top);
        return true;
    }
    return true;
}

static bool _hsort_sift_down(HPSCRIPTVM v,PSArray *arr, PSInteger root, PSInteger bottom, PSInteger func)
{
    PSInteger maxChild;
    PSInteger done = 0;
    PSInteger ret;
    PSInteger root2;
    while (((root2 = root * 2) <= bottom) && (!done))
    {
        if (root2 == bottom) {
            maxChild = root2;
        }
        else {
            if(!_sort_compare(v,arr->_values[root2],arr->_values[root2 + 1],func,ret))
                return false;
            if (ret > 0) {
                maxChild = root2;
            }
            else {
                maxChild = root2 + 1;
            }
        }

        if(!_sort_compare(v,arr->_values[root],arr->_values[maxChild],func,ret))
            return false;
        if (ret < 0) {
            if (root == maxChild) {
                v->Raise_Error(_SC("inconsistent compare function"));
                return false; // We'd be swapping ourselve. The compare function is incorrect
            }

            _Swap(arr->_values[root],arr->_values[maxChild]);
            root = maxChild;
        }
        else {
            done = 1;
        }
    }
    return true;
}

static bool _hsort(HPSCRIPTVM v,PSObjectPtr &arr, PSInteger PS_UNUSED_ARG(l), PSInteger PS_UNUSED_ARG(r),PSInteger func)
{
    PSArray *a = _array(arr);
    PSInteger i;
    PSInteger array_size = a->Size();
    for (i = (array_size / 2); i >= 0; i--) {
        if(!_hsort_sift_down(v,a, i, array_size - 1,func)) return false;
    }

    for (i = array_size-1; i >= 1; i--)
    {
        _Swap(a->_values[0],a->_values[i]);
        if(!_hsort_sift_down(v,a, 0, i-1,func)) return false;
    }
    return true;
}

static PSInteger array_sort(HPSCRIPTVM v)
{
    PSInteger func = -1;
    PSObjectPtr &o = stack_get(v,1);
    if(_array(o)->Size() > 1) {
        if(ps_gettop(v) == 2) func = 2;
        if(!_hsort(v, o, 0, _array(o)->Size()-1, func))
            return PS_ERROR;

    }
    return 0;
}

static PSInteger array_slice(HPSCRIPTVM v)
{
    PSInteger sidx,eidx;
    PSObjectPtr o;
    if(get_slice_params(v,sidx,eidx,o)==-1)return -1;
    PSInteger alen = _array(o)->Size();
    if(sidx < 0)sidx = alen + sidx;
    if(eidx < 0)eidx = alen + eidx;
    if(eidx < sidx)return ps_throwerror(v,_SC("wrong indexes"));
    if(eidx > alen || sidx < 0)return ps_throwerror(v, _SC("slice out of range"));
    PSArray *arr=PSArray::Create(_ss(v),eidx-sidx);
    PSObjectPtr t;
    PSInteger count=0;
    for(PSInteger i=sidx;i<eidx;i++){
        _array(o)->Get(i,t);
        arr->Set(count++,t);
    }
    v->Push(arr);
    return 1;

}

const PSRegFunction PSSharedState::_array_default_delegate_funcz[]={
    {_SC("len"),default_delegate_len,1, _SC("a")},
    {_SC("append"),array_append,2, _SC("a")},
    {_SC("extend"),array_extend,2, _SC("aa")},
    {_SC("push"),array_append,2, _SC("a")},
    {_SC("pop"),array_pop,1, _SC("a")},
    {_SC("top"),array_top,1, _SC("a")},
    {_SC("insert"),array_insert,3, _SC("an")},
    {_SC("remove"),array_remove,2, _SC("an")},
    {_SC("resize"),array_resize,-2, _SC("an")},
    {_SC("reverse"),array_reverse,1, _SC("a")},
    {_SC("sort"),array_sort,-1, _SC("ac")},
    {_SC("slice"),array_slice,-1, _SC("ann")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("clear"),obj_clear,1, _SC(".")},
    {_SC("map"),array_map,2, _SC("ac")},
    {_SC("apply"),array_apply,2, _SC("ac")},
    {_SC("reduce"),array_reduce,2, _SC("ac")},
    {_SC("filter"),array_filter,2, _SC("ac")},
    {_SC("find"),array_find,2, _SC("a.")},
    {NULL,(PSFUNCTION)0,0,NULL}
};

//STRING DEFAULT DELEGATE//////////////////////////
static PSInteger string_slice(HPSCRIPTVM v)
{
    PSInteger sidx,eidx;
    PSObjectPtr o;
    if(PS_FAILED(get_slice_params(v,sidx,eidx,o)))return -1;
    PSInteger slen = _string(o)->_len;
    if(sidx < 0)sidx = slen + sidx;
    if(eidx < 0)eidx = slen + eidx;
    if(eidx < sidx) return ps_throwerror(v,_SC("wrong indexes"));
    if(eidx > slen || sidx < 0) return ps_throwerror(v, _SC("slice out of range"));
    v->Push(PSString::Create(_ss(v),&_stringval(o)[sidx],eidx-sidx));
    return 1;
}

static PSInteger string_find(HPSCRIPTVM v)
{
    PSInteger top,start_idx=0;
    const PSChar *str,*substr,*ret;
    if(((top=ps_gettop(v))>1) && PS_SUCCEEDED(ps_getstring(v,1,&str)) && PS_SUCCEEDED(ps_getstring(v,2,&substr))){
        if(top>2)ps_getinteger(v,3,&start_idx);
        if((ps_getsize(v,1)>start_idx) && (start_idx>=0)){
            ret=scstrstr(&str[start_idx],substr);
            if(ret){
                ps_pushinteger(v,(PSInteger)(ret-str));
                return 1;
            }
        }
        return 0;
    }
    return ps_throwerror(v,_SC("invalid param"));
}

#define STRING_TOFUNCZ(func) static PSInteger string_##func(HPSCRIPTVM v) \
{\
    PSInteger sidx,eidx; \
    PSObjectPtr str; \
    if(PS_FAILED(get_slice_params(v,sidx,eidx,str)))return -1; \
    PSInteger slen = _string(str)->_len; \
    if(sidx < 0)sidx = slen + sidx; \
    if(eidx < 0)eidx = slen + eidx; \
    if(eidx < sidx) return ps_throwerror(v,_SC("wrong indexes")); \
    if(eidx > slen || sidx < 0) return ps_throwerror(v,_SC("slice out of range")); \
    PSInteger len=_string(str)->_len; \
    const PSChar *sthis=_stringval(str); \
    PSChar *snew=(_ss(v)->GetScratchPad(ps_rsl(len))); \
    memcpy(snew,sthis,ps_rsl(len));\
    for(PSInteger i=sidx;i<eidx;i++) snew[i] = func(sthis[i]); \
    v->Push(PSString::Create(_ss(v),snew,len)); \
    return 1; \
}


STRING_TOFUNCZ(tolower)
STRING_TOFUNCZ(toupper)

const PSRegFunction PSSharedState::_string_default_delegate_funcz[]={
    {_SC("len"),default_delegate_len,1, _SC("s")},
    {_SC("tointeger"),default_delegate_tointeger,-1, _SC("sn")},
    {_SC("tofloat"),default_delegate_tofloat,1, _SC("s")},
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("slice"),string_slice,-1, _SC("s n  n")},
    {_SC("find"),string_find,-2, _SC("s s n")},
    {_SC("tolower"),string_tolower,-1, _SC("s n n")},
    {_SC("toupper"),string_toupper,-1, _SC("s n n")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {NULL,(PSFUNCTION)0,0,NULL}
};

//INTEGER DEFAULT DELEGATE//////////////////////////
const PSRegFunction PSSharedState::_number_default_delegate_funcz[]={
    {_SC("tointeger"),default_delegate_tointeger,1, _SC("n|b")},
    {_SC("tofloat"),default_delegate_tofloat,1, _SC("n|b")},
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("tochar"),number_delegate_tochar,1, _SC("n|b")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {NULL,(PSFUNCTION)0,0,NULL}
};

//CLOSURE DEFAULT DELEGATE//////////////////////////
static PSInteger closure_pcall(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_call(v,ps_gettop(v)-1,PSTrue,PSFalse))?1:PS_ERROR;
}

static PSInteger closure_call(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_call(v,ps_gettop(v)-1,PSTrue,PSTrue))?1:PS_ERROR;
}

static PSInteger _closure_acall(HPSCRIPTVM v,PSBool raiseerror)
{
    PSArray *aparams=_array(stack_get(v,2));
    PSInteger nparams=aparams->Size();
    v->Push(stack_get(v,1));
    for(PSInteger i=0;i<nparams;i++)v->Push(aparams->_values[i]);
    return PS_SUCCEEDED(ps_call(v,nparams,PSTrue,raiseerror))?1:PS_ERROR;
}

static PSInteger closure_acall(HPSCRIPTVM v)
{
    return _closure_acall(v,PSTrue);
}

static PSInteger closure_pacall(HPSCRIPTVM v)
{
    return _closure_acall(v,PSFalse);
}

static PSInteger closure_bindenv(HPSCRIPTVM v)
{
    if(PS_FAILED(ps_bindenv(v,1)))
        return PS_ERROR;
    return 1;
}

static PSInteger closure_getroot(HPSCRIPTVM v)
{
    if(PS_FAILED(ps_getclosureroot(v,-1)))
        return PS_ERROR;
    return 1;
}

static PSInteger closure_setroot(HPSCRIPTVM v)
{
    if(PS_FAILED(ps_setclosureroot(v,-2)))
        return PS_ERROR;
    return 1;
}

static PSInteger closure_getinfos(HPSCRIPTVM v) {
    PSObject o = stack_get(v,1);
    PSTable *res = PSTable::Create(_ss(v),4);
    if(type(o) == OT_CLOSURE) {
        PSFunctionProto *f = _closure(o)->_function;
        PSInteger nparams = f->_nparameters + (f->_varparams?1:0);
        PSObjectPtr params = PSArray::Create(_ss(v),nparams);
    PSObjectPtr defparams = PSArray::Create(_ss(v),f->_ndefaultparams);
        for(PSInteger n = 0; n<f->_nparameters; n++) {
            _array(params)->Set((PSInteger)n,f->_parameters[n]);
        }
    for(PSInteger j = 0; j<f->_ndefaultparams; j++) {
            _array(defparams)->Set((PSInteger)j,_closure(o)->_defaultparams[j]);
        }
        if(f->_varparams) {
            _array(params)->Set(nparams-1,PSString::Create(_ss(v),_SC("..."),-1));
        }
        res->NewSlot(PSString::Create(_ss(v),_SC("native"),-1),false);
        res->NewSlot(PSString::Create(_ss(v),_SC("name"),-1),f->_name);
        res->NewSlot(PSString::Create(_ss(v),_SC("src"),-1),f->_sourcename);
        res->NewSlot(PSString::Create(_ss(v),_SC("parameters"),-1),params);
        res->NewSlot(PSString::Create(_ss(v),_SC("varargs"),-1),f->_varparams);
    res->NewSlot(PSString::Create(_ss(v),_SC("defparams"),-1),defparams);
    }
    else { //OT_NATIVECLOSURE
        PSNativeClosure *nc = _nativeclosure(o);
        res->NewSlot(PSString::Create(_ss(v),_SC("native"),-1),true);
        res->NewSlot(PSString::Create(_ss(v),_SC("name"),-1),nc->_name);
        res->NewSlot(PSString::Create(_ss(v),_SC("paramscheck"),-1),nc->_nparamscheck);
        PSObjectPtr typecheck;
        if(nc->_typecheck.size() > 0) {
            typecheck =
                PSArray::Create(_ss(v), nc->_typecheck.size());
            for(PSUnsignedInteger n = 0; n<nc->_typecheck.size(); n++) {
                    _array(typecheck)->Set((PSInteger)n,nc->_typecheck[n]);
            }
        }
        res->NewSlot(PSString::Create(_ss(v),_SC("typecheck"),-1),typecheck);
    }
    v->Push(res);
    return 1;
}



const PSRegFunction PSSharedState::_closure_default_delegate_funcz[]={
    {_SC("call"),closure_call,-1, _SC("c")},
    {_SC("pcall"),closure_pcall,-1, _SC("c")},
    {_SC("acall"),closure_acall,2, _SC("ca")},
    {_SC("pacall"),closure_pacall,2, _SC("ca")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("bindenv"),closure_bindenv,2, _SC("c x|y|t")},
    {_SC("getinfos"),closure_getinfos,1, _SC("c")},
    {_SC("getroot"),closure_getroot,1, _SC("c")},
    {_SC("setroot"),closure_setroot,2, _SC("ct")},
    {NULL,(PSFUNCTION)0,0,NULL}
};

//GENERATOR DEFAULT DELEGATE
static PSInteger generator_getstatus(HPSCRIPTVM v)
{
    PSObject &o=stack_get(v,1);
    switch(_generator(o)->_state){
        case PSGenerator::eSuspended:v->Push(PSString::Create(_ss(v),_SC("suspended")));break;
        case PSGenerator::eRunning:v->Push(PSString::Create(_ss(v),_SC("running")));break;
        case PSGenerator::eDead:v->Push(PSString::Create(_ss(v),_SC("dead")));break;
    }
    return 1;
}

const PSRegFunction PSSharedState::_generator_default_delegate_funcz[]={
    {_SC("getstatus"),generator_getstatus,1, _SC("g")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {NULL,(PSFUNCTION)0,0,NULL}
};

//THREAD DEFAULT DELEGATE
static PSInteger thread_call(HPSCRIPTVM v)
{
    PSObjectPtr o = stack_get(v,1);
    if(type(o) == OT_THREAD) {
        PSInteger nparams = ps_gettop(v);
        _thread(o)->Push(_thread(o)->_roottable);
        for(PSInteger i = 2; i<(nparams+1); i++)
            ps_move(_thread(o),v,i);
        if(PS_SUCCEEDED(ps_call(_thread(o),nparams,PSTrue,PSTrue))) {
            ps_move(v,_thread(o),-1);
            ps_pop(_thread(o),1);
            return 1;
        }
        v->_lasterror = _thread(o)->_lasterror;
        return PS_ERROR;
    }
    return ps_throwerror(v,_SC("wrong parameter"));
}

static PSInteger thread_wakeup(HPSCRIPTVM v)
{
    PSObjectPtr o = stack_get(v,1);
    if(type(o) == OT_THREAD) {
        PSVM *thread = _thread(o);
        PSInteger state = ps_getvmstate(thread);
        if(state != PS_VMSTATE_SUSPENDED) {
            switch(state) {
                case PS_VMSTATE_IDLE:
                    return ps_throwerror(v,_SC("cannot wakeup a idle thread"));
                break;
                case PS_VMSTATE_RUNNING:
                    return ps_throwerror(v,_SC("cannot wakeup a running thread"));
                break;
            }
        }

        PSInteger wakeupret = ps_gettop(v)>1?PSTrue:PSFalse;
        if(wakeupret) {
            ps_move(thread,v,2);
        }
        if(PS_SUCCEEDED(ps_wakeupvm(thread,wakeupret,PSTrue,PSTrue,PSFalse))) {
            ps_move(v,thread,-1);
            ps_pop(thread,1); //pop retval
            if(ps_getvmstate(thread) == PS_VMSTATE_IDLE) {
                ps_settop(thread,1); //pop roottable
            }
            return 1;
        }
        ps_settop(thread,1);
        v->_lasterror = thread->_lasterror;
        return PS_ERROR;
    }
    return ps_throwerror(v,_SC("wrong parameter"));
}

static PSInteger thread_wakeupthrow(HPSCRIPTVM v)
{
    PSObjectPtr o = stack_get(v,1);
    if(type(o) == OT_THREAD) {
        PSVM *thread = _thread(o);
        PSInteger state = ps_getvmstate(thread);
        if(state != PS_VMSTATE_SUSPENDED) {
            switch(state) {
                case PS_VMSTATE_IDLE:
                    return ps_throwerror(v,_SC("cannot wakeup a idle thread"));
                break;
                case PS_VMSTATE_RUNNING:
                    return ps_throwerror(v,_SC("cannot wakeup a running thread"));
                break;
            }
        }

        ps_move(thread,v,2);
        ps_throwobject(thread);
        PSBool rethrow_error = PSTrue;
        if(ps_gettop(v) > 2) {
            ps_getbool(v,3,&rethrow_error);
        }
        if(PS_SUCCEEDED(ps_wakeupvm(thread,PSFalse,PSTrue,PSTrue,PSTrue))) {
            ps_move(v,thread,-1);
            ps_pop(thread,1); //pop retval
            if(ps_getvmstate(thread) == PS_VMSTATE_IDLE) {
                ps_settop(thread,1); //pop roottable
            }
            return 1;
        }
        ps_settop(thread,1);
        if(rethrow_error) {
            v->_lasterror = thread->_lasterror;
            return PS_ERROR;
        }
        return PS_OK;
    }
    return ps_throwerror(v,_SC("wrong parameter"));
}

static PSInteger thread_getstatus(HPSCRIPTVM v)
{
    PSObjectPtr &o = stack_get(v,1);
    switch(ps_getvmstate(_thread(o))) {
        case PS_VMSTATE_IDLE:
            ps_pushstring(v,_SC("idle"),-1);
        break;
        case PS_VMSTATE_RUNNING:
            ps_pushstring(v,_SC("running"),-1);
        break;
        case PS_VMSTATE_SUSPENDED:
            ps_pushstring(v,_SC("suspended"),-1);
        break;
        default:
            return ps_throwerror(v,_SC("internal VM error"));
    }
    return 1;
}

static PSInteger thread_getstackinfos(HPSCRIPTVM v)
{
    PSObjectPtr o = stack_get(v,1);
    if(type(o) == OT_THREAD) {
        PSVM *thread = _thread(o);
        PSInteger threadtop = ps_gettop(thread);
        PSInteger level;
        ps_getinteger(v,-1,&level);
        PSRESULT res = __getcallstackinfos(thread,level);
        if(PS_FAILED(res))
        {
            ps_settop(thread,threadtop);
            if(type(thread->_lasterror) == OT_STRING) {
                ps_throwerror(v,_stringval(thread->_lasterror));
            }
            else {
                ps_throwerror(v,_SC("unknown error"));
            }
        }
        if(res > 0) {
            //some result
            ps_move(v,thread,-1);
            ps_settop(thread,threadtop);
            return 1;
        }
        //no result
        ps_settop(thread,threadtop);
        return 0;

    }
    return ps_throwerror(v,_SC("wrong parameter"));
}

const PSRegFunction PSSharedState::_thread_default_delegate_funcz[] = {
    {_SC("call"), thread_call, -1, _SC("v")},
    {_SC("wakeup"), thread_wakeup, -1, _SC("v")},
    {_SC("wakeupthrow"), thread_wakeupthrow, -2, _SC("v.b")},
    {_SC("getstatus"), thread_getstatus, 1, _SC("v")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("getstackinfos"),thread_getstackinfos,2, _SC("vn")},
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {NULL,(PSFUNCTION)0,0,NULL}
};

static PSInteger class_getattributes(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_getattributes(v,-2))?1:PS_ERROR;
}

static PSInteger class_setattributes(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_setattributes(v,-3))?1:PS_ERROR;
}

static PSInteger class_instance(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_createinstance(v,-1))?1:PS_ERROR;
}

static PSInteger class_getbase(HPSCRIPTVM v)
{
    return PS_SUCCEEDED(ps_getbase(v,-1))?1:PS_ERROR;
}

static PSInteger class_newmember(HPSCRIPTVM v)
{
    PSInteger top = ps_gettop(v);
    PSBool bstatic = PSFalse;
    if(top == 5)
    {
        ps_tobool(v,-1,&bstatic);
        ps_pop(v,1);
    }

    if(top < 4) {
        ps_pushnull(v);
    }
    return PS_SUCCEEDED(ps_newmember(v,-4,bstatic))?1:PS_ERROR;
}

static PSInteger class_rawnewmember(HPSCRIPTVM v)
{
    PSInteger top = ps_gettop(v);
    PSBool bstatic = PSFalse;
    if(top == 5)
    {
        ps_tobool(v,-1,&bstatic);
        ps_pop(v,1);
    }

    if(top < 4) {
        ps_pushnull(v);
    }
    return PS_SUCCEEDED(ps_rawnewmember(v,-4,bstatic))?1:PS_ERROR;
}

const PSRegFunction PSSharedState::_class_default_delegate_funcz[] = {
    {_SC("getattributes"), class_getattributes, 2, _SC("y.")},
    {_SC("setattributes"), class_setattributes, 3, _SC("y..")},
    {_SC("rawget"),container_rawget,2, _SC("y")},
    {_SC("rawset"),container_rawset,3, _SC("y")},
    {_SC("rawin"),container_rawexists,2, _SC("y")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {_SC("instance"),class_instance,1, _SC("y")},
    {_SC("getbase"),class_getbase,1, _SC("y")},
    {_SC("newmember"),class_newmember,-3, _SC("y")},
    {_SC("rawnewmember"),class_rawnewmember,-3, _SC("y")},
    {NULL,(PSFUNCTION)0,0,NULL}
};


static PSInteger instance_getclass(HPSCRIPTVM v)
{
    if(PS_SUCCEEDED(ps_getclass(v,1)))
        return 1;
    return PS_ERROR;
}

const PSRegFunction PSSharedState::_instance_default_delegate_funcz[] = {
    {_SC("getclass"), instance_getclass, 1, _SC("x")},
    {_SC("rawget"),container_rawget,2, _SC("x")},
    {_SC("rawset"),container_rawset,3, _SC("x")},
    {_SC("rawin"),container_rawexists,2, _SC("x")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {NULL,(PSFUNCTION)0,0,NULL}
};

static PSInteger weakref_ref(HPSCRIPTVM v)
{
    if(PS_FAILED(ps_getweakrefval(v,1)))
        return PS_ERROR;
    return 1;
}

const PSRegFunction PSSharedState::_weakref_default_delegate_funcz[] = {
    {_SC("ref"),weakref_ref,1, _SC("r")},
    {_SC("weakref"),obj_delegate_weakref,1, NULL },
    {_SC("tostring"),default_delegate_tostring,1, _SC(".")},
    {NULL,(PSFUNCTION)0,0,NULL}
};
