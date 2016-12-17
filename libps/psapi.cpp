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
#include "psuserdata.h"
#include "pscompiler.h"
#include "psfuncstate.h"
#include "psclass.h"

static bool ps_aux_gettypedarg(HPSCRIPTVM v,PSInteger idx,PSObjectType type,PSObjectPtr **o)
{
    *o = &stack_get(v,idx);
    if(type(**o) != type){
        PSObjectPtr oval = v->PrintObjVal(**o);
        v->Raise_Error(_SC("wrong argument type, expected '%s' got '%.50s'"),IdType2Name(type),_stringval(oval));
        return false;
    }
    return true;
}

#define _GETSAFE_OBJ(v,idx,type,o) { if(!ps_aux_gettypedarg(v,idx,type,&o)) return PS_ERROR; }

#define ps_aux_paramscheck(v,count) \
{ \
    if(ps_gettop(v) < count){ v->Raise_Error(_SC("not enough params in the stack")); return PS_ERROR; }\
}


PSInteger ps_aux_invalidtype(HPSCRIPTVM v,PSObjectType type)
{
    PSUnsignedInteger buf_size = 100 *sizeof(PSChar);
    scsprintf(_ss(v)->GetScratchPad(buf_size), buf_size, _SC("unexpected type %s"), IdType2Name(type));
    return ps_throwerror(v, _ss(v)->GetScratchPad(-1));
}

HPSCRIPTVM ps_open(PSInteger initialstacksize)
{
    PSSharedState *ss;
    PSVM *v;
    ps_new(ss, PSSharedState);
    ss->Init();
    v = (PSVM *)PS_MALLOC(sizeof(PSVM));
    new (v) PSVM(ss);
    ss->_root_vm = v;
    if(v->Init(NULL, initialstacksize)) {
        return v;
    } else {
        ps_delete(v, PSVM);
        return NULL;
    }
    return v;
}

HPSCRIPTVM ps_newthread(HPSCRIPTVM friendvm, PSInteger initialstacksize)
{
    PSSharedState *ss;
    PSVM *v;
    ss=_ss(friendvm);

    v= (PSVM *)PS_MALLOC(sizeof(PSVM));
    new (v) PSVM(ss);

    if(v->Init(friendvm, initialstacksize)) {
        friendvm->Push(v);
        return v;
    } else {
        ps_delete(v, PSVM);
        return NULL;
    }
}

PSInteger ps_getvmstate(HPSCRIPTVM v)
{
    if(v->_suspended)
        return PS_VMSTATE_SUSPENDED;
    else {
        if(v->_callsstacksize != 0) return PS_VMSTATE_RUNNING;
        else return PS_VMSTATE_IDLE;
    }
}

void ps_seterrorhandler(HPSCRIPTVM v)
{
    PSObject o = stack_get(v, -1);
    if(ps_isclosure(o) || ps_isnativeclosure(o) || ps_isnull(o)) {
        v->_errorhandler = o;
        v->Pop();
    }
}

void ps_setnativedebughook(HPSCRIPTVM v,PSDEBUGHOOK hook)
{
    v->_debughook_native = hook;
    v->_debughook_closure.Null();
    v->_debughook = hook?true:false;
}

void ps_setdebughook(HPSCRIPTVM v)
{
    PSObject o = stack_get(v,-1);
    if(ps_isclosure(o) || ps_isnativeclosure(o) || ps_isnull(o)) {
        v->_debughook_closure = o;
        v->_debughook_native = NULL;
        v->_debughook = !ps_isnull(o);
        v->Pop();
    }
}

void ps_close(HPSCRIPTVM v)
{
    PSSharedState *ss = _ss(v);
    _thread(ss->_root_vm)->Finalize();
    ps_delete(ss, PSSharedState);
}

PSInteger ps_getversion()
{
    return PSCRIPT_VERSION_NUMBER;
}

PSRESULT ps_compile(HPSCRIPTVM v,PSLEXREADFUNC read,PSUserPointer p,const PSChar *sourcename,PSBool raiseerror)
{
    PSObjectPtr o;
#ifndef NO_COMPILER
    if(Compile(v, read, p, sourcename, o, raiseerror?true:false, _ss(v)->_debuginfo)) {
        v->Push(PSClosure::Create(_ss(v), _funcproto(o), _table(v->_roottable)->GetWeakRef(OT_TABLE)));
        return PS_OK;
    }
    return PS_ERROR;
#else
    return ps_throwerror(v,_SC("this is a no compiler build"));
#endif
}

void ps_enabledebuginfo(HPSCRIPTVM v, PSBool enable)
{
    _ss(v)->_debuginfo = enable?true:false;
}

void ps_notifyallexceptions(HPSCRIPTVM v, PSBool enable)
{
    _ss(v)->_notifyallexceptions = enable?true:false;
}

void ps_addref(HPSCRIPTVM v,HPSOBJECT *po)
{
    if(!ISREFCOUNTED(type(*po))) return;
#ifdef NO_GARBAGE_COLLECTOR
    __AddRef(po->_type,po->_unVal);
#else
    _ss(v)->_refs_table.AddRef(*po);
#endif
}

PSUnsignedInteger ps_getrefcount(HPSCRIPTVM v,HPSOBJECT *po)
{
    if(!ISREFCOUNTED(type(*po))) return 0;
#ifdef NO_GARBAGE_COLLECTOR
   return po->_unVal.pRefCounted->_uiRef;
#else
   return _ss(v)->_refs_table.GetRefCount(*po);
#endif
}

PSBool ps_release(HPSCRIPTVM v,HPSOBJECT *po)
{
    if(!ISREFCOUNTED(type(*po))) return PSTrue;
#ifdef NO_GARBAGE_COLLECTOR
    bool ret = (po->_unVal.pRefCounted->_uiRef <= 1) ? PSTrue : PSFalse;
    __Release(po->_type,po->_unVal);
    return ret; //the ret val doesn't work(and cannot be fixed)
#else
    return _ss(v)->_refs_table.Release(*po);
#endif
}

PSUnsignedInteger ps_getvmrefcount(HPSCRIPTVM PS_UNUSED_ARG(v), const HPSOBJECT *po)
{
    if (!ISREFCOUNTED(type(*po))) return 0;
    return po->_unVal.pRefCounted->_uiRef;
}

const PSChar *ps_objtostring(const HPSOBJECT *o)
{
    if(ps_type(*o) == OT_STRING) {
        return _stringval(*o);
    }
    return NULL;
}

PSInteger ps_objtointeger(const HPSOBJECT *o)
{
    if(ps_isnumeric(*o)) {
        return tointeger(*o);
    }
    return 0;
}

PSFloat ps_objtofloat(const HPSOBJECT *o)
{
    if(ps_isnumeric(*o)) {
        return tofloat(*o);
    }
    return 0;
}

PSBool ps_objtobool(const HPSOBJECT *o)
{
    if(ps_isbool(*o)) {
        return _integer(*o);
    }
    return PSFalse;
}

PSUserPointer ps_objtouserpointer(const HPSOBJECT *o)
{
    if(ps_isuserpointer(*o)) {
        return _userpointer(*o);
    }
    return 0;
}

void ps_pushnull(HPSCRIPTVM v)
{
    v->PushNull();
}

void ps_pushstring(HPSCRIPTVM v,const PSChar *s,PSInteger len)
{
    if(s)
        v->Push(PSObjectPtr(PSString::Create(_ss(v), s, len)));
    else v->PushNull();
}

void ps_pushinteger(HPSCRIPTVM v,PSInteger n)
{
    v->Push(n);
}

void ps_pushbool(HPSCRIPTVM v,PSBool b)
{
    v->Push(b?true:false);
}

void ps_pushfloat(HPSCRIPTVM v,PSFloat n)
{
    v->Push(n);
}

void ps_pushuserpointer(HPSCRIPTVM v,PSUserPointer p)
{
    v->Push(p);
}

void ps_pushthread(HPSCRIPTVM v, HPSCRIPTVM thread)
{
    v->Push(thread);
}

PSUserPointer ps_newuserdata(HPSCRIPTVM v,PSUnsignedInteger size)
{
    PSUserData *ud = PSUserData::Create(_ss(v), size + PS_ALIGNMENT);
    v->Push(ud);
    return (PSUserPointer)ps_aligning(ud + 1);
}

void ps_newtable(HPSCRIPTVM v)
{
    v->Push(PSTable::Create(_ss(v), 0));
}

void ps_newtableex(HPSCRIPTVM v,PSInteger initialcapacity)
{
    v->Push(PSTable::Create(_ss(v), initialcapacity));
}

void ps_newarray(HPSCRIPTVM v,PSInteger size)
{
    v->Push(PSArray::Create(_ss(v), size));
}

PSRESULT ps_newclass(HPSCRIPTVM v,PSBool hasbase)
{
    PSClass *baseclass = NULL;
    if(hasbase) {
        PSObjectPtr &base = stack_get(v,-1);
        if(type(base) != OT_CLASS)
            return ps_throwerror(v,_SC("invalid base type"));
        baseclass = _class(base);
    }
    PSClass *newclass = PSClass::Create(_ss(v), baseclass);
    if(baseclass) v->Pop();
    v->Push(newclass);
    return PS_OK;
}

PSBool ps_instanceof(HPSCRIPTVM v)
{
    PSObjectPtr &inst = stack_get(v,-1);
    PSObjectPtr &cl = stack_get(v,-2);
    if(type(inst) != OT_INSTANCE || type(cl) != OT_CLASS)
        return ps_throwerror(v,_SC("invalid param type"));
    return _instance(inst)->InstanceOf(_class(cl))?PSTrue:PSFalse;
}

PSRESULT ps_arrayappend(HPSCRIPTVM v,PSInteger idx)
{
    ps_aux_paramscheck(v,2);
    PSObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    _array(*arr)->Append(v->GetUp(-1));
    v->Pop();
    return PS_OK;
}

PSRESULT ps_arraypop(HPSCRIPTVM v,PSInteger idx,PSBool pushval)
{
    ps_aux_paramscheck(v, 1);
    PSObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    if(_array(*arr)->Size() > 0) {
        if(pushval != 0){ v->Push(_array(*arr)->Top()); }
        _array(*arr)->Pop();
        return PS_OK;
    }
    return ps_throwerror(v, _SC("empty array"));
}

PSRESULT ps_arrayresize(HPSCRIPTVM v,PSInteger idx,PSInteger newsize)
{
    ps_aux_paramscheck(v,1);
    PSObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    if(newsize >= 0) {
        _array(*arr)->Resize(newsize);
        return PS_OK;
    }
    return ps_throwerror(v,_SC("negative size"));
}


PSRESULT ps_arrayreverse(HPSCRIPTVM v,PSInteger idx)
{
    ps_aux_paramscheck(v, 1);
    PSObjectPtr *o;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,o);
    PSArray *arr = _array(*o);
    if(arr->Size() > 0) {
        PSObjectPtr t;
        PSInteger size = arr->Size();
        PSInteger n = size >> 1; size -= 1;
        for(PSInteger i = 0; i < n; i++) {
            t = arr->_values[i];
            arr->_values[i] = arr->_values[size-i];
            arr->_values[size-i] = t;
        }
        return PS_OK;
    }
    return PS_OK;
}

PSRESULT ps_arrayremove(HPSCRIPTVM v,PSInteger idx,PSInteger itemidx)
{
    ps_aux_paramscheck(v, 1);
    PSObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    return _array(*arr)->Remove(itemidx) ? PS_OK : ps_throwerror(v,_SC("index out of range"));
}

PSRESULT ps_arrayinsert(HPSCRIPTVM v,PSInteger idx,PSInteger destpos)
{
    ps_aux_paramscheck(v, 1);
    PSObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    PSRESULT ret = _array(*arr)->Insert(destpos, v->GetUp(-1)) ? PS_OK : ps_throwerror(v,_SC("index out of range"));
    v->Pop();
    return ret;
}

void ps_newclosure(HPSCRIPTVM v,PSFUNCTION func,PSUnsignedInteger nfreevars)
{
    PSNativeClosure *nc = PSNativeClosure::Create(_ss(v), func,nfreevars);
    nc->_nparamscheck = 0;
    for(PSUnsignedInteger i = 0; i < nfreevars; i++) {
        nc->_outervalues[i] = v->Top();
        v->Pop();
    }
    v->Push(PSObjectPtr(nc));
}

PSRESULT ps_getclosureinfo(HPSCRIPTVM v,PSInteger idx,PSUnsignedInteger *nparams,PSUnsignedInteger *nfreevars)
{
    PSObject o = stack_get(v, idx);
    if(type(o) == OT_CLOSURE) {
        PSClosure *c = _closure(o);
        PSFunctionProto *proto = c->_function;
        *nparams = (PSUnsignedInteger)proto->_nparameters;
        *nfreevars = (PSUnsignedInteger)proto->_noutervalues;
        return PS_OK;
    }
    else if(type(o) == OT_NATIVECLOSURE)
    {
        PSNativeClosure *c = _nativeclosure(o);
        *nparams = (PSUnsignedInteger)c->_nparamscheck;
        *nfreevars = c->_noutervalues;
        return PS_OK;
    }
    return ps_throwerror(v,_SC("the object is not a closure"));
}

PSRESULT ps_setnativeclosurename(HPSCRIPTVM v,PSInteger idx,const PSChar *name)
{
    PSObject o = stack_get(v, idx);
    if(ps_isnativeclosure(o)) {
        PSNativeClosure *nc = _nativeclosure(o);
        nc->_name = PSString::Create(_ss(v),name);
        return PS_OK;
    }
    return ps_throwerror(v,_SC("the object is not a nativeclosure"));
}

PSRESULT ps_setparamscheck(HPSCRIPTVM v,PSInteger nparamscheck,const PSChar *typemask)
{
    PSObject o = stack_get(v, -1);
    if(!ps_isnativeclosure(o))
        return ps_throwerror(v, _SC("native closure expected"));
    PSNativeClosure *nc = _nativeclosure(o);
    nc->_nparamscheck = nparamscheck;
    if(typemask) {
        PSIntVec res;
        if(!CompileTypemask(res, typemask))
            return ps_throwerror(v, _SC("invalid typemask"));
        nc->_typecheck.copy(res);
    }
    else {
        nc->_typecheck.resize(0);
    }
    if(nparamscheck == PS_MATCHTYPEMASKSTRING) {
        nc->_nparamscheck = nc->_typecheck.size();
    }
    return PS_OK;
}

PSRESULT ps_bindenv(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &o = stack_get(v,idx);
    if(!ps_isnativeclosure(o) &&
        !ps_isclosure(o))
        return ps_throwerror(v,_SC("the target is not a closure"));
    PSObjectPtr &env = stack_get(v,-1);
    if(!ps_istable(env) &&
        !ps_isarray(env) &&
        !ps_isclass(env) &&
        !ps_isinstance(env))
        return ps_throwerror(v,_SC("invalid environment"));
    PSWeakRef *w = _refcounted(env)->GetWeakRef(type(env));
    PSObjectPtr ret;
    if(ps_isclosure(o)) {
        PSClosure *c = _closure(o)->Clone();
        __ObjRelease(c->_env);
        c->_env = w;
        __ObjAddRef(c->_env);
        if(_closure(o)->_base) {
            c->_base = _closure(o)->_base;
            __ObjAddRef(c->_base);
        }
        ret = c;
    }
    else { //then must be a native closure
        PSNativeClosure *c = _nativeclosure(o)->Clone();
        __ObjRelease(c->_env);
        c->_env = w;
        __ObjAddRef(c->_env);
        ret = c;
    }
    v->Pop();
    v->Push(ret);
    return PS_OK;
}

PSRESULT ps_getclosurename(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &o = stack_get(v,idx);
    if(!ps_isnativeclosure(o) &&
        !ps_isclosure(o))
        return ps_throwerror(v,_SC("the target is not a closure"));
    if(ps_isnativeclosure(o))
    {
        v->Push(_nativeclosure(o)->_name);
    }
    else { //closure
        v->Push(_closure(o)->_function->_name);
    }
    return PS_OK;
}

PSRESULT ps_setclosureroot(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &c = stack_get(v,idx);
    PSObject o = stack_get(v, -1);
    if(!ps_isclosure(c)) return ps_throwerror(v, _SC("closure expected"));
    if(ps_istable(o)) {
        _closure(c)->SetRoot(_table(o)->GetWeakRef(OT_TABLE));
        v->Pop();
        return PS_OK;
    }
    return ps_throwerror(v, _SC("ivalid type"));
}

PSRESULT ps_getclosureroot(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &c = stack_get(v,idx);
    if(!ps_isclosure(c)) return ps_throwerror(v, _SC("closure expected"));
    v->Push(_closure(c)->_root->_obj);
    return PS_OK;
}

PSRESULT ps_clear(HPSCRIPTVM v,PSInteger idx)
{
    PSObject &o=stack_get(v,idx);
    switch(type(o)) {
        case OT_TABLE: _table(o)->Clear();  break;
        case OT_ARRAY: _array(o)->Resize(0); break;
        default:
            return ps_throwerror(v, _SC("clear only works on table and array"));
        break;

    }
    return PS_OK;
}

void ps_pushroottable(HPSCRIPTVM v)
{
    v->Push(v->_roottable);
}

void ps_pushregistrytable(HPSCRIPTVM v)
{
    v->Push(_ss(v)->_registry);
}

void ps_pushconsttable(HPSCRIPTVM v)
{
    v->Push(_ss(v)->_consts);
}

PSRESULT ps_setroottable(HPSCRIPTVM v)
{
    PSObject o = stack_get(v, -1);
    if(ps_istable(o) || ps_isnull(o)) {
        v->_roottable = o;
        v->Pop();
        return PS_OK;
    }
    return ps_throwerror(v, _SC("ivalid type"));
}

PSRESULT ps_setconsttable(HPSCRIPTVM v)
{
    PSObject o = stack_get(v, -1);
    if(ps_istable(o)) {
        _ss(v)->_consts = o;
        v->Pop();
        return PS_OK;
    }
    return ps_throwerror(v, _SC("ivalid type, expected table"));
}

void ps_setforeignptr(HPSCRIPTVM v,PSUserPointer p)
{
    v->_foreignptr = p;
}

PSUserPointer ps_getforeignptr(HPSCRIPTVM v)
{
    return v->_foreignptr;
}

void ps_setsharedforeignptr(HPSCRIPTVM v,PSUserPointer p)
{
    _ss(v)->_foreignptr = p;
}

PSUserPointer ps_getsharedforeignptr(HPSCRIPTVM v)
{
    return _ss(v)->_foreignptr;
}

void ps_setvmreleasehook(HPSCRIPTVM v,PSRELEASEHOOK hook)
{
    v->_releasehook = hook;
}

PSRELEASEHOOK ps_getvmreleasehook(HPSCRIPTVM v)
{
    return v->_releasehook;
}

void ps_setsharedreleasehook(HPSCRIPTVM v,PSRELEASEHOOK hook)
{
    _ss(v)->_releasehook = hook;
}

PSRELEASEHOOK ps_getsharedreleasehook(HPSCRIPTVM v)
{
    return _ss(v)->_releasehook;
}

void ps_push(HPSCRIPTVM v,PSInteger idx)
{
    v->Push(stack_get(v, idx));
}

PSObjectType ps_gettype(HPSCRIPTVM v,PSInteger idx)
{
    return type(stack_get(v, idx));
}

PSRESULT ps_typeof(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &o = stack_get(v, idx);
    PSObjectPtr res;
    if(!v->TypeOf(o,res)) {
        return PS_ERROR;
    }
    v->Push(res);
    return PS_OK;
}

PSRESULT ps_tostring(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &o = stack_get(v, idx);
    PSObjectPtr res;
    if(!v->ToString(o,res)) {
        return PS_ERROR;
    }
    v->Push(res);
    return PS_OK;
}

void ps_tobool(HPSCRIPTVM v, PSInteger idx, PSBool *b)
{
    PSObjectPtr &o = stack_get(v, idx);
    *b = PSVM::IsFalse(o)?PSFalse:PSTrue;
}

PSRESULT ps_getinteger(HPSCRIPTVM v,PSInteger idx,PSInteger *i)
{
    PSObjectPtr &o = stack_get(v, idx);
    if(ps_isnumeric(o)) {
        *i = tointeger(o);
        return PS_OK;
    }
    return PS_ERROR;
}

PSRESULT ps_getfloat(HPSCRIPTVM v,PSInteger idx,PSFloat *f)
{
    PSObjectPtr &o = stack_get(v, idx);
    if(ps_isnumeric(o)) {
        *f = tofloat(o);
        return PS_OK;
    }
    return PS_ERROR;
}

PSRESULT ps_getbool(HPSCRIPTVM v,PSInteger idx,PSBool *b)
{
    PSObjectPtr &o = stack_get(v, idx);
    if(ps_isbool(o)) {
        *b = _integer(o);
        return PS_OK;
    }
    return PS_ERROR;
}

PSRESULT ps_getstring(HPSCRIPTVM v,PSInteger idx,const PSChar **c)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_STRING,o);
    *c = _stringval(*o);
    return PS_OK;
}

PSRESULT ps_getthread(HPSCRIPTVM v,PSInteger idx,HPSCRIPTVM *thread)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_THREAD,o);
    *thread = _thread(*o);
    return PS_OK;
}

PSRESULT ps_clone(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &o = stack_get(v,idx);
    v->PushNull();
    if(!v->Clone(o, stack_get(v, -1))){
        v->Pop();
        return PS_ERROR;
    }
    return PS_OK;
}

PSInteger ps_getsize(HPSCRIPTVM v, PSInteger idx)
{
    PSObjectPtr &o = stack_get(v, idx);
    PSObjectType type = type(o);
    switch(type) {
    case OT_STRING:     return _string(o)->_len;
    case OT_TABLE:      return _table(o)->CountUsed();
    case OT_ARRAY:      return _array(o)->Size();
    case OT_USERDATA:   return _userdata(o)->_size;
    case OT_INSTANCE:   return _instance(o)->_class->_udsize;
    case OT_CLASS:      return _class(o)->_udsize;
    default:
        return ps_aux_invalidtype(v, type);
    }
}

PSHash ps_gethash(HPSCRIPTVM v, PSInteger idx)
{
    PSObjectPtr &o = stack_get(v, idx);
    return HashObj(o);
}

PSRESULT ps_getuserdata(HPSCRIPTVM v,PSInteger idx,PSUserPointer *p,PSUserPointer *typetag)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_USERDATA,o);
    (*p) = _userdataval(*o);
    if(typetag) *typetag = _userdata(*o)->_typetag;
    return PS_OK;
}

PSRESULT ps_settypetag(HPSCRIPTVM v,PSInteger idx,PSUserPointer typetag)
{
    PSObjectPtr &o = stack_get(v,idx);
    switch(type(o)) {
        case OT_USERDATA:   _userdata(o)->_typetag = typetag;   break;
        case OT_CLASS:      _class(o)->_typetag = typetag;      break;
        default:            return ps_throwerror(v,_SC("invalid object type"));
    }
    return PS_OK;
}

PSRESULT ps_getobjtypetag(const HPSOBJECT *o,PSUserPointer * typetag)
{
  switch(type(*o)) {
    case OT_INSTANCE: *typetag = _instance(*o)->_class->_typetag; break;
    case OT_USERDATA: *typetag = _userdata(*o)->_typetag; break;
    case OT_CLASS:    *typetag = _class(*o)->_typetag; break;
    default: return PS_ERROR;
  }
  return PS_OK;
}

PSRESULT ps_gettypetag(HPSCRIPTVM v,PSInteger idx,PSUserPointer *typetag)
{
    PSObjectPtr &o = stack_get(v,idx);
    if(PS_FAILED(ps_getobjtypetag(&o,typetag)))
        return ps_throwerror(v,_SC("invalid object type"));
    return PS_OK;
}

PSRESULT ps_getuserpointer(HPSCRIPTVM v, PSInteger idx, PSUserPointer *p)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_USERPOINTER,o);
    (*p) = _userpointer(*o);
    return PS_OK;
}

PSRESULT ps_setinstanceup(HPSCRIPTVM v, PSInteger idx, PSUserPointer p)
{
    PSObjectPtr &o = stack_get(v,idx);
    if(type(o) != OT_INSTANCE) return ps_throwerror(v,_SC("the object is not a class instance"));
    _instance(o)->_userpointer = p;
    return PS_OK;
}

PSRESULT ps_setclassudsize(HPSCRIPTVM v, PSInteger idx, PSInteger udsize)
{
    PSObjectPtr &o = stack_get(v,idx);
    if(type(o) != OT_CLASS) return ps_throwerror(v,_SC("the object is not a class"));
    if(_class(o)->_locked) return ps_throwerror(v,_SC("the class is locked"));
    _class(o)->_udsize = udsize;
    return PS_OK;
}


PSRESULT ps_getinstanceup(HPSCRIPTVM v, PSInteger idx, PSUserPointer *p,PSUserPointer typetag)
{
    PSObjectPtr &o = stack_get(v,idx);
    if(type(o) != OT_INSTANCE) return ps_throwerror(v,_SC("the object is not a class instance"));
    (*p) = _instance(o)->_userpointer;
    if(typetag != 0) {
        PSClass *cl = _instance(o)->_class;
        do{
            if(cl->_typetag == typetag)
                return PS_OK;
            cl = cl->_base;
        }while(cl != NULL);
        return ps_throwerror(v,_SC("invalid type tag"));
    }
    return PS_OK;
}

PSInteger ps_gettop(HPSCRIPTVM v)
{
    return (v->_top) - v->_stackbase;
}

void ps_settop(HPSCRIPTVM v, PSInteger newtop)
{
    PSInteger top = ps_gettop(v);
    if(top > newtop)
        ps_pop(v, top - newtop);
    else
        while(top++ < newtop) ps_pushnull(v);
}

void ps_pop(HPSCRIPTVM v, PSInteger nelemstopop)
{
    assert(v->_top >= nelemstopop);
    v->Pop(nelemstopop);
}

void ps_poptop(HPSCRIPTVM v)
{
    assert(v->_top >= 1);
    v->Pop();
}


void ps_remove(HPSCRIPTVM v, PSInteger idx)
{
    v->Remove(idx);
}

PSInteger ps_cmp(HPSCRIPTVM v)
{
    PSInteger res;
    v->ObjCmp(stack_get(v, -1), stack_get(v, -2),res);
    return res;
}

PSRESULT ps_newslot(HPSCRIPTVM v, PSInteger idx, PSBool bstatic)
{
    ps_aux_paramscheck(v, 3);
    PSObjectPtr &self = stack_get(v, idx);
    if(type(self) == OT_TABLE || type(self) == OT_CLASS) {
        PSObjectPtr &key = v->GetUp(-2);
        if(type(key) == OT_NULL) return ps_throwerror(v, _SC("null is not a valid key"));
        v->NewSlot(self, key, v->GetUp(-1),bstatic?true:false);
        v->Pop(2);
    }
    return PS_OK;
}

PSRESULT ps_deleteslot(HPSCRIPTVM v,PSInteger idx,PSBool pushval)
{
    ps_aux_paramscheck(v, 2);
    PSObjectPtr *self;
    _GETSAFE_OBJ(v, idx, OT_TABLE,self);
    PSObjectPtr &key = v->GetUp(-1);
    if(type(key) == OT_NULL) return ps_throwerror(v, _SC("null is not a valid key"));
    PSObjectPtr res;
    if(!v->DeleteSlot(*self, key, res)){
        v->Pop();
        return PS_ERROR;
    }
    if(pushval) v->GetUp(-1) = res;
    else v->Pop();
    return PS_OK;
}

PSRESULT ps_set(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &self = stack_get(v, idx);
    if(v->Set(self, v->GetUp(-2), v->GetUp(-1),DONT_FALL_BACK)) {
        v->Pop(2);
        return PS_OK;
    }
    return PS_ERROR;
}

PSRESULT ps_rawset(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &self = stack_get(v, idx);
    PSObjectPtr &key = v->GetUp(-2);
    if(type(key) == OT_NULL) {
        v->Pop(2);
        return ps_throwerror(v, _SC("null key"));
    }
    switch(type(self)) {
    case OT_TABLE:
        _table(self)->NewSlot(key, v->GetUp(-1));
        v->Pop(2);
        return PS_OK;
    break;
    case OT_CLASS:
        _class(self)->NewSlot(_ss(v), key, v->GetUp(-1),false);
        v->Pop(2);
        return PS_OK;
    break;
    case OT_INSTANCE:
        if(_instance(self)->Set(key, v->GetUp(-1))) {
            v->Pop(2);
            return PS_OK;
        }
    break;
    case OT_ARRAY:
        if(v->Set(self, key, v->GetUp(-1),false)) {
            v->Pop(2);
            return PS_OK;
        }
    break;
    default:
        v->Pop(2);
        return ps_throwerror(v, _SC("rawset works only on array/table/class and instance"));
    }
    v->Raise_IdxError(v->GetUp(-2));return PS_ERROR;
}

PSRESULT ps_newmember(HPSCRIPTVM v,PSInteger idx,PSBool bstatic)
{
    PSObjectPtr &self = stack_get(v, idx);
    if(type(self) != OT_CLASS) return ps_throwerror(v, _SC("new member only works with classes"));
    PSObjectPtr &key = v->GetUp(-3);
    if(type(key) == OT_NULL) return ps_throwerror(v, _SC("null key"));
    if(!v->NewSlotA(self,key,v->GetUp(-2),v->GetUp(-1),bstatic?true:false,false))
        return PS_ERROR;
    return PS_OK;
}

PSRESULT ps_rawnewmember(HPSCRIPTVM v,PSInteger idx,PSBool bstatic)
{
    PSObjectPtr &self = stack_get(v, idx);
    if(type(self) != OT_CLASS) return ps_throwerror(v, _SC("new member only works with classes"));
    PSObjectPtr &key = v->GetUp(-3);
    if(type(key) == OT_NULL) return ps_throwerror(v, _SC("null key"));
    if(!v->NewSlotA(self,key,v->GetUp(-2),v->GetUp(-1),bstatic?true:false,true))
        return PS_ERROR;
    return PS_OK;
}

PSRESULT ps_setdelegate(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &self = stack_get(v, idx);
    PSObjectPtr &mt = v->GetUp(-1);
    PSObjectType type = type(self);
    switch(type) {
    case OT_TABLE:
        if(type(mt) == OT_TABLE) {
            if(!_table(self)->SetDelegate(_table(mt))) return ps_throwerror(v, _SC("delagate cycle")); v->Pop();}
        else if(type(mt)==OT_NULL) {
            _table(self)->SetDelegate(NULL); v->Pop(); }
        else return ps_aux_invalidtype(v,type);
        break;
    case OT_USERDATA:
        if(type(mt)==OT_TABLE) {
            _userdata(self)->SetDelegate(_table(mt)); v->Pop(); }
        else if(type(mt)==OT_NULL) {
            _userdata(self)->SetDelegate(NULL); v->Pop(); }
        else return ps_aux_invalidtype(v, type);
        break;
    default:
            return ps_aux_invalidtype(v, type);
        break;
    }
    return PS_OK;
}

PSRESULT ps_rawdeleteslot(HPSCRIPTVM v,PSInteger idx,PSBool pushval)
{
    ps_aux_paramscheck(v, 2);
    PSObjectPtr *self;
    _GETSAFE_OBJ(v, idx, OT_TABLE,self);
    PSObjectPtr &key = v->GetUp(-1);
    PSObjectPtr t;
    if(_table(*self)->Get(key,t)) {
        _table(*self)->Remove(key);
    }
    if(pushval != 0)
        v->GetUp(-1) = t;
    else
        v->Pop();
    return PS_OK;
}

PSRESULT ps_getdelegate(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &self=stack_get(v,idx);
    switch(type(self)){
    case OT_TABLE:
    case OT_USERDATA:
        if(!_delegable(self)->_delegate){
            v->PushNull();
            break;
        }
        v->Push(PSObjectPtr(_delegable(self)->_delegate));
        break;
    default: return ps_throwerror(v,_SC("wrong type")); break;
    }
    return PS_OK;

}

PSRESULT ps_get(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &self=stack_get(v,idx);
    PSObjectPtr &obj = v->GetUp(-1);
    if(v->Get(self,obj,obj,false,DONT_FALL_BACK))
        return PS_OK;
    v->Pop();
    return PS_ERROR;
}

PSRESULT ps_rawget(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &self=stack_get(v,idx);
    PSObjectPtr &obj = v->GetUp(-1);
    switch(type(self)) {
    case OT_TABLE:
        if(_table(self)->Get(obj,obj))
            return PS_OK;
        break;
    case OT_CLASS:
        if(_class(self)->Get(obj,obj))
            return PS_OK;
        break;
    case OT_INSTANCE:
        if(_instance(self)->Get(obj,obj))
            return PS_OK;
        break;
    case OT_ARRAY:{
        if(ps_isnumeric(obj)){
            if(_array(self)->Get(tointeger(obj),obj)) {
                return PS_OK;
            }
        }
        else {
            v->Pop();
            return ps_throwerror(v,_SC("invalid index type for an array"));
        }
                  }
        break;
    default:
        v->Pop();
        return ps_throwerror(v,_SC("rawget works only on array/table/instance and class"));
    }
    v->Pop();
    return ps_throwerror(v,_SC("the index doesn't exist"));
}

PSRESULT ps_getstackobj(HPSCRIPTVM v,PSInteger idx,HPSOBJECT *po)
{
    *po=stack_get(v,idx);
    return PS_OK;
}

const PSChar *ps_getlocal(HPSCRIPTVM v,PSUnsignedInteger level,PSUnsignedInteger idx)
{
    PSUnsignedInteger cstksize=v->_callsstacksize;
    PSUnsignedInteger lvl=(cstksize-level)-1;
    PSInteger stackbase=v->_stackbase;
    if(lvl<cstksize){
        for(PSUnsignedInteger i=0;i<level;i++){
            PSVM::CallInfo &ci=v->_callsstack[(cstksize-i)-1];
            stackbase-=ci._prevstkbase;
        }
        PSVM::CallInfo &ci=v->_callsstack[lvl];
        if(type(ci._closure)!=OT_CLOSURE)
            return NULL;
        PSClosure *c=_closure(ci._closure);
        PSFunctionProto *func=c->_function;
        if(func->_noutervalues > (PSInteger)idx) {
            v->Push(*_outer(c->_outervalues[idx])->_valptr);
            return _stringval(func->_outervalues[idx]._name);
        }
        idx -= func->_noutervalues;
        return func->GetLocal(v,stackbase,idx,(PSInteger)(ci._ip-func->_instructions)-1);
    }
    return NULL;
}

void ps_pushobject(HPSCRIPTVM v,HPSOBJECT obj)
{
    v->Push(PSObjectPtr(obj));
}

void ps_resetobject(HPSOBJECT *po)
{
    po->_unVal.pUserPointer=NULL;po->_type=OT_NULL;
}

PSRESULT ps_throwerror(HPSCRIPTVM v,const PSChar *err)
{
    v->_lasterror=PSString::Create(_ss(v),err);
    return PS_ERROR;
}

PSRESULT ps_throwobject(HPSCRIPTVM v)
{
    v->_lasterror = v->GetUp(-1);
    v->Pop();
    return PS_ERROR;
}


void ps_reseterror(HPSCRIPTVM v)
{
    v->_lasterror.Null();
}

void ps_getlasterror(HPSCRIPTVM v)
{
    v->Push(v->_lasterror);
}

PSRESULT ps_reservestack(HPSCRIPTVM v,PSInteger nsize)
{
    if (((PSUnsignedInteger)v->_top + nsize) > v->_stack.size()) {
        if(v->_nmetamethodscall) {
            return ps_throwerror(v,_SC("cannot resize stack while in  a metamethod"));
        }
        v->_stack.resize(v->_stack.size() + ((v->_top + nsize) - v->_stack.size()));
    }
    return PS_OK;
}

PSRESULT ps_resume(HPSCRIPTVM v,PSBool retval,PSBool raiseerror)
{
    if (type(v->GetUp(-1)) == OT_GENERATOR)
    {
        v->PushNull(); //retval
        if (!v->Execute(v->GetUp(-2), 0, v->_top, v->GetUp(-1), raiseerror, PSVM::ET_RESUME_GENERATOR))
        {v->Raise_Error(v->_lasterror); return PS_ERROR;}
        if(!retval)
            v->Pop();
        return PS_OK;
    }
    return ps_throwerror(v,_SC("only generators can be resumed"));
}

PSRESULT ps_call(HPSCRIPTVM v,PSInteger params,PSBool retval,PSBool raiseerror)
{
    PSObjectPtr res;
    if(v->Call(v->GetUp(-(params+1)),params,v->_top-params,res,raiseerror?true:false)){

        if(!v->_suspended) {
            v->Pop(params);//pop closure and args
        }
        if(retval){
            v->Push(res); return PS_OK;
        }
        return PS_OK;
    }
    else {
        v->Pop(params);
        return PS_ERROR;
    }
    if(!v->_suspended)
        v->Pop(params);
    return ps_throwerror(v,_SC("call failed"));
}

PSRESULT ps_suspendvm(HPSCRIPTVM v)
{
    return v->Suspend();
}

PSRESULT ps_wakeupvm(HPSCRIPTVM v,PSBool wakeupret,PSBool retval,PSBool raiseerror,PSBool throwerror)
{
    PSObjectPtr ret;
    if(!v->_suspended)
        return ps_throwerror(v,_SC("cannot resume a vm that is not running any code"));
    PSInteger target = v->_suspended_target;
    if(wakeupret) {
        if(target != -1) {
            v->GetAt(v->_stackbase+v->_suspended_target)=v->GetUp(-1); //retval
        }
        v->Pop();
    } else if(target != -1) { v->GetAt(v->_stackbase+v->_suspended_target).Null(); }
    PSObjectPtr dummy;
    if(!v->Execute(dummy,-1,-1,ret,raiseerror,throwerror?PSVM::ET_RESUME_THROW_VM : PSVM::ET_RESUME_VM)) {
        return PS_ERROR;
    }
    if(retval)
        v->Push(ret);
    return PS_OK;
}

void ps_setreleasehook(HPSCRIPTVM v,PSInteger idx,PSRELEASEHOOK hook)
{
    if(ps_gettop(v) >= 1){
        PSObjectPtr &ud=stack_get(v,idx);
        switch( type(ud) ) {
        case OT_USERDATA:   _userdata(ud)->_hook = hook;    break;
        case OT_INSTANCE:   _instance(ud)->_hook = hook;    break;
        case OT_CLASS:      _class(ud)->_hook = hook;       break;
        default: break; //shutup compiler
        }
    }
}

PSRELEASEHOOK ps_getreleasehook(HPSCRIPTVM v,PSInteger idx)
{
    if(ps_gettop(v) >= 1){
        PSObjectPtr &ud=stack_get(v,idx);
        switch( type(ud) ) {
        case OT_USERDATA:   return _userdata(ud)->_hook;    break;
        case OT_INSTANCE:   return _instance(ud)->_hook;    break;
        case OT_CLASS:      return _class(ud)->_hook;       break;
        default: break; //shutup compiler
        }
    }
    return NULL;
}

void ps_setcompilererrorhandler(HPSCRIPTVM v,PSCOMPILERERROR f)
{
    _ss(v)->_compilererrorhandler = f;
}

PSRESULT ps_writeclosure(HPSCRIPTVM v,PSWRITEFUNC w,PSUserPointer up)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, -1, OT_CLOSURE,o);
    unsigned short tag = PS_BYTECODE_STREAM_TAG;
    if(_closure(*o)->_function->_noutervalues)
        return ps_throwerror(v,_SC("a closure with free valiables bound it cannot be serialized"));
    if(w(up,&tag,2) != 2)
        return ps_throwerror(v,_SC("io error"));
    if(!_closure(*o)->Save(v,up,w))
        return PS_ERROR;
    return PS_OK;
}

PSRESULT ps_readclosure(HPSCRIPTVM v,PSREADFUNC r,PSUserPointer up)
{
    PSObjectPtr closure;

    unsigned short tag;
    if(r(up,&tag,2) != 2)
        return ps_throwerror(v,_SC("io error"));
    if(tag != PS_BYTECODE_STREAM_TAG)
        return ps_throwerror(v,_SC("invalid stream"));
    if(!PSClosure::Load(v,up,r,closure))
        return PS_ERROR;
    v->Push(closure);
    return PS_OK;
}

PSChar *ps_getscratchpad(HPSCRIPTVM v,PSInteger minsize)
{
    return _ss(v)->GetScratchPad(minsize);
}

PSRESULT ps_resurrectunreachable(HPSCRIPTVM v)
{
#ifndef NO_GARBAGE_COLLECTOR
    _ss(v)->ResurrectUnreachable(v);
    return PS_OK;
#else
    return ps_throwerror(v,_SC("ps_resurrectunreachable requires a garbage collector build"));
#endif
}

PSInteger ps_collectgarbage(HPSCRIPTVM v)
{
#ifndef NO_GARBAGE_COLLECTOR
    return _ss(v)->CollectGarbage(v);
#else
    return -1;
#endif
}

PSRESULT ps_getcallee(HPSCRIPTVM v)
{
    if(v->_callsstacksize > 1)
    {
        v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
        return PS_OK;
    }
    return ps_throwerror(v,_SC("no closure in the calls stack"));
}

const PSChar *ps_getfreevariable(HPSCRIPTVM v,PSInteger idx,PSUnsignedInteger nval)
{
    PSObjectPtr &self=stack_get(v,idx);
    const PSChar *name = NULL;
    switch(type(self))
    {
    case OT_CLOSURE:{
        PSClosure *clo = _closure(self);
        PSFunctionProto *fp = clo->_function;
        if(((PSUnsignedInteger)fp->_noutervalues) > nval) {
            v->Push(*(_outer(clo->_outervalues[nval])->_valptr));
            PSOuterVar &ov = fp->_outervalues[nval];
            name = _stringval(ov._name);
        }
                    }
        break;
    case OT_NATIVECLOSURE:{
        PSNativeClosure *clo = _nativeclosure(self);
        if(clo->_noutervalues > nval) {
            v->Push(clo->_outervalues[nval]);
            name = _SC("@NATIVE");
        }
                          }
        break;
    default: break; //shutup compiler
    }
    return name;
}

PSRESULT ps_setfreevariable(HPSCRIPTVM v,PSInteger idx,PSUnsignedInteger nval)
{
    PSObjectPtr &self=stack_get(v,idx);
    switch(type(self))
    {
    case OT_CLOSURE:{
        PSFunctionProto *fp = _closure(self)->_function;
        if(((PSUnsignedInteger)fp->_noutervalues) > nval){
            *(_outer(_closure(self)->_outervalues[nval])->_valptr) = stack_get(v,-1);
        }
        else return ps_throwerror(v,_SC("invalid free var index"));
                    }
        break;
    case OT_NATIVECLOSURE:
        if(_nativeclosure(self)->_noutervalues > nval){
            _nativeclosure(self)->_outervalues[nval] = stack_get(v,-1);
        }
        else return ps_throwerror(v,_SC("invalid free var index"));
        break;
    default:
        return ps_aux_invalidtype(v,type(self));
    }
    v->Pop();
    return PS_OK;
}

PSRESULT ps_setattributes(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_CLASS,o);
    PSObjectPtr &key = stack_get(v,-2);
    PSObjectPtr &val = stack_get(v,-1);
    PSObjectPtr attrs;
    if(type(key) == OT_NULL) {
        attrs = _class(*o)->_attributes;
        _class(*o)->_attributes = val;
        v->Pop(2);
        v->Push(attrs);
        return PS_OK;
    }else if(_class(*o)->GetAttributes(key,attrs)) {
        _class(*o)->SetAttributes(key,val);
        v->Pop(2);
        v->Push(attrs);
        return PS_OK;
    }
    return ps_throwerror(v,_SC("wrong index"));
}

PSRESULT ps_getattributes(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_CLASS,o);
    PSObjectPtr &key = stack_get(v,-1);
    PSObjectPtr attrs;
    if(type(key) == OT_NULL) {
        attrs = _class(*o)->_attributes;
        v->Pop();
        v->Push(attrs);
        return PS_OK;
    }
    else if(_class(*o)->GetAttributes(key,attrs)) {
        v->Pop();
        v->Push(attrs);
        return PS_OK;
    }
    return ps_throwerror(v,_SC("wrong index"));
}

PSRESULT ps_getmemberhandle(HPSCRIPTVM v,PSInteger idx,HPSMEMBERHANDLE *handle)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_CLASS,o);
    PSObjectPtr &key = stack_get(v,-1);
    PSTable *m = _class(*o)->_members;
    PSObjectPtr val;
    if(m->Get(key,val)) {
        handle->_static = _isfield(val) ? PSFalse : PSTrue;
        handle->_index = _member_idx(val);
        v->Pop();
        return PS_OK;
    }
    return ps_throwerror(v,_SC("wrong index"));
}

PSRESULT _getmemberbyhandle(HPSCRIPTVM v,PSObjectPtr &self,const HPSMEMBERHANDLE *handle,PSObjectPtr *&val)
{
    switch(type(self)) {
        case OT_INSTANCE: {
                PSInstance *i = _instance(self);
                if(handle->_static) {
                    PSClass *c = i->_class;
                    val = &c->_methods[handle->_index].val;
                }
                else {
                    val = &i->_values[handle->_index];

                }
            }
            break;
        case OT_CLASS: {
                PSClass *c = _class(self);
                if(handle->_static) {
                    val = &c->_methods[handle->_index].val;
                }
                else {
                    val = &c->_defaultvalues[handle->_index].val;
                }
            }
            break;
        default:
            return ps_throwerror(v,_SC("wrong type(expected class or instance)"));
    }
    return PS_OK;
}

PSRESULT ps_getbyhandle(HPSCRIPTVM v,PSInteger idx,const HPSMEMBERHANDLE *handle)
{
    PSObjectPtr &self = stack_get(v,idx);
    PSObjectPtr *val = NULL;
    if(PS_FAILED(_getmemberbyhandle(v,self,handle,val))) {
        return PS_ERROR;
    }
    v->Push(_realval(*val));
    return PS_OK;
}

PSRESULT ps_setbyhandle(HPSCRIPTVM v,PSInteger idx,const HPSMEMBERHANDLE *handle)
{
    PSObjectPtr &self = stack_get(v,idx);
    PSObjectPtr &newval = stack_get(v,-1);
    PSObjectPtr *val = NULL;
    if(PS_FAILED(_getmemberbyhandle(v,self,handle,val))) {
        return PS_ERROR;
    }
    *val = newval;
    v->Pop();
    return PS_OK;
}

PSRESULT ps_getbase(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_CLASS,o);
    if(_class(*o)->_base)
        v->Push(PSObjectPtr(_class(*o)->_base));
    else
        v->PushNull();
    return PS_OK;
}

PSRESULT ps_getclass(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_INSTANCE,o);
    v->Push(PSObjectPtr(_instance(*o)->_class));
    return PS_OK;
}

PSRESULT ps_createinstance(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_CLASS,o);
    v->Push(_class(*o)->CreateInstance());
    return PS_OK;
}

void ps_weakref(HPSCRIPTVM v,PSInteger idx)
{
    PSObject &o=stack_get(v,idx);
    if(ISREFCOUNTED(type(o))) {
        v->Push(_refcounted(o)->GetWeakRef(type(o)));
        return;
    }
    v->Push(o);
}

PSRESULT ps_getweakrefval(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr &o = stack_get(v,idx);
    if(type(o) != OT_WEAKREF) {
        return ps_throwerror(v,_SC("the object must be a weakref"));
    }
    v->Push(_weakref(o)->_obj);
    return PS_OK;
}

PSRESULT ps_getdefaultdelegate(HPSCRIPTVM v,PSObjectType t)
{
    PSSharedState *ss = _ss(v);
    switch(t) {
    case OT_TABLE: v->Push(ss->_table_default_delegate); break;
    case OT_ARRAY: v->Push(ss->_array_default_delegate); break;
    case OT_STRING: v->Push(ss->_string_default_delegate); break;
    case OT_INTEGER: case OT_FLOAT: v->Push(ss->_number_default_delegate); break;
    case OT_GENERATOR: v->Push(ss->_generator_default_delegate); break;
    case OT_CLOSURE: case OT_NATIVECLOSURE: v->Push(ss->_closure_default_delegate); break;
    case OT_THREAD: v->Push(ss->_thread_default_delegate); break;
    case OT_CLASS: v->Push(ss->_class_default_delegate); break;
    case OT_INSTANCE: v->Push(ss->_instance_default_delegate); break;
    case OT_WEAKREF: v->Push(ss->_weakref_default_delegate); break;
    default: return ps_throwerror(v,_SC("the type doesn't have a default delegate"));
    }
    return PS_OK;
}

PSRESULT ps_next(HPSCRIPTVM v,PSInteger idx)
{
    PSObjectPtr o=stack_get(v,idx),&refpos = stack_get(v,-1),realkey,val;
    if(type(o) == OT_GENERATOR) {
        return ps_throwerror(v,_SC("cannot iterate a generator"));
    }
    int faketojump;
    if(!v->FOREACH_OP(o,realkey,val,refpos,0,666,faketojump))
        return PS_ERROR;
    if(faketojump != 666) {
        v->Push(realkey);
        v->Push(val);
        return PS_OK;
    }
    return PS_ERROR;
}

struct BufState{
    const PSChar *buf;
    PSInteger ptr;
    PSInteger size;
};

PSInteger buf_lexfeed(PSUserPointer file)
{
    BufState *buf=(BufState*)file;
    if(buf->size<(buf->ptr+1))
        return 0;
    return buf->buf[buf->ptr++];
}

PSRESULT ps_compilebuffer(HPSCRIPTVM v,const PSChar *s,PSInteger size,const PSChar *sourcename,PSBool raiseerror) {
    BufState buf;
    buf.buf = s;
    buf.size = size;
    buf.ptr = 0;
    return ps_compile(v, buf_lexfeed, &buf, sourcename, raiseerror);
}

void ps_move(HPSCRIPTVM dest,HPSCRIPTVM src,PSInteger idx)
{
    dest->Push(stack_get(src,idx));
}

void ps_setprintfunc(HPSCRIPTVM v, PSPRINTFUNCTION printfunc,PSPRINTFUNCTION errfunc)
{
    _ss(v)->_printfunc = printfunc;
    _ss(v)->_errorfunc = errfunc;
}

PSPRINTFUNCTION ps_getprintfunc(HPSCRIPTVM v)
{
    return _ss(v)->_printfunc;
}

PSPRINTFUNCTION ps_geterrorfunc(HPSCRIPTVM v)
{
    return _ss(v)->_errorfunc;
}

void *ps_malloc(PSUnsignedInteger size)
{
    return PS_MALLOC(size);
}

void *ps_realloc(void* p,PSUnsignedInteger oldsize,PSUnsignedInteger newsize)
{
    return PS_REALLOC(p,oldsize,newsize);
}

void ps_free(void *p,PSUnsignedInteger size)
{
    PS_FREE(p,size);
}
