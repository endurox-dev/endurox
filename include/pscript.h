/*
Copyright (c) 2003-2016 Alberto Demichelis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef _PSCRIPT_H_
#define _PSCRIPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PSCRIPT_API
#define PSCRIPT_API extern
#endif

#if (defined(_WIN64) || defined(_LP64))
#ifndef _PS64
#define _PS64
#endif
#endif


#define PSTrue  (1)
#define PSFalse (0)

struct PSVM;
struct PSTable;
struct PSArray;
struct PSString;
struct PSClosure;
struct PSGenerator;
struct PSNativeClosure;
struct PSUserData;
struct PSFunctionProto;
struct PSRefCounted;
struct PSClass;
struct PSInstance;
struct PSDelegable;
struct PSOuter;

#ifdef _UNICODE
#define PSUNICODE
#endif

#include "psconfig.h"

#define PSCRIPT_VERSION    _SC("Pscript 3.1 stable")
#define PSCRIPT_COPYRIGHT  _SC("Copyright (C) 2003-2016 Alberto Demichelis\nPlatform Script for Enduro/X by Mavimax SIA")
#define PSCRIPT_AUTHOR     _SC("Alberto Demichelis; Mavimax SIA")
#define PSCRIPT_VERSION_NUMBER 310

#define PS_VMSTATE_IDLE         0
#define PS_VMSTATE_RUNNING      1
#define PS_VMSTATE_SUSPENDED    2

#define PSCRIPT_EOB 0
#define PS_BYTECODE_STREAM_TAG  0xFAFA

#define PSOBJECT_REF_COUNTED    0x08000000
#define PSOBJECT_NUMERIC        0x04000000
#define PSOBJECT_DELEGABLE      0x02000000
#define PSOBJECT_CANBEFALSE     0x01000000

#define PS_MATCHTYPEMASKSTRING (-99999)

#define _RT_MASK 0x00FFFFFF
#define _RAW_TYPE(type) (type&_RT_MASK)

#define _RT_NULL            0x00000001
#define _RT_INTEGER         0x00000002
#define _RT_FLOAT           0x00000004
#define _RT_BOOL            0x00000008
#define _RT_STRING          0x00000010
#define _RT_TABLE           0x00000020
#define _RT_ARRAY           0x00000040
#define _RT_USERDATA        0x00000080
#define _RT_CLOSURE         0x00000100
#define _RT_NATIVECLOSURE   0x00000200
#define _RT_GENERATOR       0x00000400
#define _RT_USERPOINTER     0x00000800
#define _RT_THREAD          0x00001000
#define _RT_FUNCPROTO       0x00002000
#define _RT_CLASS           0x00004000
#define _RT_INSTANCE        0x00008000
#define _RT_WEAKREF         0x00010000
#define _RT_OUTER           0x00020000

typedef enum tagPSObjectType{
    OT_NULL =           (_RT_NULL|PSOBJECT_CANBEFALSE),
    OT_INTEGER =        (_RT_INTEGER|PSOBJECT_NUMERIC|PSOBJECT_CANBEFALSE),
    OT_FLOAT =          (_RT_FLOAT|PSOBJECT_NUMERIC|PSOBJECT_CANBEFALSE),
    OT_BOOL =           (_RT_BOOL|PSOBJECT_CANBEFALSE),
    OT_STRING =         (_RT_STRING|PSOBJECT_REF_COUNTED),
    OT_TABLE =          (_RT_TABLE|PSOBJECT_REF_COUNTED|PSOBJECT_DELEGABLE),
    OT_ARRAY =          (_RT_ARRAY|PSOBJECT_REF_COUNTED),
    OT_USERDATA =       (_RT_USERDATA|PSOBJECT_REF_COUNTED|PSOBJECT_DELEGABLE),
    OT_CLOSURE =        (_RT_CLOSURE|PSOBJECT_REF_COUNTED),
    OT_NATIVECLOSURE =  (_RT_NATIVECLOSURE|PSOBJECT_REF_COUNTED),
    OT_GENERATOR =      (_RT_GENERATOR|PSOBJECT_REF_COUNTED),
    OT_USERPOINTER =    _RT_USERPOINTER,
    OT_THREAD =         (_RT_THREAD|PSOBJECT_REF_COUNTED) ,
    OT_FUNCPROTO =      (_RT_FUNCPROTO|PSOBJECT_REF_COUNTED), //internal usage only
    OT_CLASS =          (_RT_CLASS|PSOBJECT_REF_COUNTED),
    OT_INSTANCE =       (_RT_INSTANCE|PSOBJECT_REF_COUNTED|PSOBJECT_DELEGABLE),
    OT_WEAKREF =        (_RT_WEAKREF|PSOBJECT_REF_COUNTED),
    OT_OUTER =          (_RT_OUTER|PSOBJECT_REF_COUNTED) //internal usage only
}PSObjectType;

#define ISREFCOUNTED(t) (t&PSOBJECT_REF_COUNTED)


typedef union tagPSObjectValue
{
    struct PSTable *pTable;
    struct PSArray *pArray;
    struct PSClosure *pClosure;
    struct PSOuter *pOuter;
    struct PSGenerator *pGenerator;
    struct PSNativeClosure *pNativeClosure;
    struct PSString *pString;
    struct PSUserData *pUserData;
    PSInteger nInteger;
    PSFloat fFloat;
    PSUserPointer pUserPointer;
    struct PSFunctionProto *pFunctionProto;
    struct PSRefCounted *pRefCounted;
    struct PSDelegable *pDelegable;
    struct PSVM *pThread;
    struct PSClass *pClass;
    struct PSInstance *pInstance;
    struct PSWeakRef *pWeakRef;
    PSRawObjectVal raw;
}PSObjectValue;


typedef struct tagPSObject
{
    PSObjectType _type;
    PSObjectValue _unVal;
}PSObject;

typedef struct  tagPSMemberHandle{
    PSBool _static;
    PSInteger _index;
}PSMemberHandle;

typedef struct tagPSStackInfos{
    const PSChar* funcname;
    const PSChar* source;
    PSInteger line;
}PSStackInfos;

typedef struct PSVM* HPSCRIPTVM;
typedef PSObject HPSOBJECT;
typedef PSMemberHandle HPSMEMBERHANDLE;
typedef PSInteger (*PSFUNCTION)(HPSCRIPTVM);
typedef PSInteger (*PSRELEASEHOOK)(PSUserPointer,PSInteger size);
typedef void (*PSCOMPILERERROR)(HPSCRIPTVM,const PSChar * /*desc*/,const PSChar * /*source*/,PSInteger /*line*/,PSInteger /*column*/);
typedef void (*PSPRINTFUNCTION)(HPSCRIPTVM,const PSChar * ,...);
typedef void (*PSDEBUGHOOK)(HPSCRIPTVM /*v*/, PSInteger /*type*/, const PSChar * /*sourcename*/, PSInteger /*line*/, const PSChar * /*funcname*/);
typedef PSInteger (*PSWRITEFUNC)(PSUserPointer,PSUserPointer,PSInteger);
typedef PSInteger (*PSREADFUNC)(PSUserPointer,PSUserPointer,PSInteger);

typedef PSInteger (*PSLEXREADFUNC)(PSUserPointer);

typedef struct tagPSRegFunction{
    const PSChar *name;
    PSFUNCTION f;
    PSInteger nparamscheck;
    const PSChar *typemask;
}PSRegFunction;

typedef struct tagPSFunctionInfo {
    PSUserPointer funcid;
    const PSChar *name;
    const PSChar *source;
    PSInteger line;
}PSFunctionInfo;

/*vm*/
PSCRIPT_API HPSCRIPTVM ps_open(PSInteger initialstacksize);
PSCRIPT_API HPSCRIPTVM ps_newthread(HPSCRIPTVM friendvm, PSInteger initialstacksize);
PSCRIPT_API void ps_seterrorhandler(HPSCRIPTVM v);
PSCRIPT_API void ps_close(HPSCRIPTVM v);
PSCRIPT_API void ps_setforeignptr(HPSCRIPTVM v,PSUserPointer p);
PSCRIPT_API PSUserPointer ps_getforeignptr(HPSCRIPTVM v);
PSCRIPT_API void ps_setsharedforeignptr(HPSCRIPTVM v,PSUserPointer p);
PSCRIPT_API PSUserPointer ps_getsharedforeignptr(HPSCRIPTVM v);
PSCRIPT_API void ps_setvmreleasehook(HPSCRIPTVM v,PSRELEASEHOOK hook);
PSCRIPT_API PSRELEASEHOOK ps_getvmreleasehook(HPSCRIPTVM v);
PSCRIPT_API void ps_setsharedreleasehook(HPSCRIPTVM v,PSRELEASEHOOK hook);
PSCRIPT_API PSRELEASEHOOK ps_getsharedreleasehook(HPSCRIPTVM v);
PSCRIPT_API void ps_setprintfunc(HPSCRIPTVM v, PSPRINTFUNCTION printfunc,PSPRINTFUNCTION errfunc);
PSCRIPT_API PSPRINTFUNCTION ps_getprintfunc(HPSCRIPTVM v);
PSCRIPT_API PSPRINTFUNCTION ps_geterrorfunc(HPSCRIPTVM v);
PSCRIPT_API PSRESULT ps_suspendvm(HPSCRIPTVM v);
PSCRIPT_API PSRESULT ps_wakeupvm(HPSCRIPTVM v,PSBool resumedret,PSBool retval,PSBool raiseerror,PSBool throwerror);
PSCRIPT_API PSInteger ps_getvmstate(HPSCRIPTVM v);
PSCRIPT_API PSInteger ps_getversion();

/*compiler*/
PSCRIPT_API PSRESULT ps_compile(HPSCRIPTVM v,PSLEXREADFUNC read,PSUserPointer p,const PSChar *sourcename,PSBool raiseerror);
PSCRIPT_API PSRESULT ps_compilebuffer(HPSCRIPTVM v,const PSChar *s,PSInteger size,const PSChar *sourcename,PSBool raiseerror);
PSCRIPT_API void ps_enabledebuginfo(HPSCRIPTVM v, PSBool enable);
PSCRIPT_API void ps_notifyallexceptions(HPSCRIPTVM v, PSBool enable);
PSCRIPT_API void ps_setcompilererrorhandler(HPSCRIPTVM v,PSCOMPILERERROR f);

/*stack operations*/
PSCRIPT_API void ps_push(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API void ps_pop(HPSCRIPTVM v,PSInteger nelemstopop);
PSCRIPT_API void ps_poptop(HPSCRIPTVM v);
PSCRIPT_API void ps_remove(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSInteger ps_gettop(HPSCRIPTVM v);
PSCRIPT_API void ps_settop(HPSCRIPTVM v,PSInteger newtop);
PSCRIPT_API PSRESULT ps_reservestack(HPSCRIPTVM v,PSInteger nsize);
PSCRIPT_API PSInteger ps_cmp(HPSCRIPTVM v);
PSCRIPT_API void ps_move(HPSCRIPTVM dest,HPSCRIPTVM src,PSInteger idx);

/*object creation handling*/
PSCRIPT_API PSUserPointer ps_newuserdata(HPSCRIPTVM v,PSUnsignedInteger size);
PSCRIPT_API void ps_newtable(HPSCRIPTVM v);
PSCRIPT_API void ps_newtableex(HPSCRIPTVM v,PSInteger initialcapacity);
PSCRIPT_API void ps_newarray(HPSCRIPTVM v,PSInteger size);
PSCRIPT_API void ps_newclosure(HPSCRIPTVM v,PSFUNCTION func,PSUnsignedInteger nfreevars);
PSCRIPT_API PSRESULT ps_setparamscheck(HPSCRIPTVM v,PSInteger nparamscheck,const PSChar *typemask);
PSCRIPT_API PSRESULT ps_bindenv(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_setclosureroot(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_getclosureroot(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API void ps_pushstring(HPSCRIPTVM v,const PSChar *s,PSInteger len);
PSCRIPT_API void ps_pushfloat(HPSCRIPTVM v,PSFloat f);
PSCRIPT_API void ps_pushinteger(HPSCRIPTVM v,PSInteger n);
PSCRIPT_API void ps_pushbool(HPSCRIPTVM v,PSBool b);
PSCRIPT_API void ps_pushuserpointer(HPSCRIPTVM v,PSUserPointer p);
PSCRIPT_API void ps_pushnull(HPSCRIPTVM v);
PSCRIPT_API void ps_pushthread(HPSCRIPTVM v, HPSCRIPTVM thread);
PSCRIPT_API PSObjectType ps_gettype(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_typeof(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSInteger ps_getsize(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSHash ps_gethash(HPSCRIPTVM v, PSInteger idx);
PSCRIPT_API PSRESULT ps_getbase(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSBool ps_instanceof(HPSCRIPTVM v);
PSCRIPT_API PSRESULT ps_tostring(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API void ps_tobool(HPSCRIPTVM v, PSInteger idx, PSBool *b);
PSCRIPT_API PSRESULT ps_getstring(HPSCRIPTVM v,PSInteger idx,const PSChar **c);
PSCRIPT_API PSRESULT ps_getinteger(HPSCRIPTVM v,PSInteger idx,PSInteger *i);
PSCRIPT_API PSRESULT ps_getfloat(HPSCRIPTVM v,PSInteger idx,PSFloat *f);
PSCRIPT_API PSRESULT ps_getbool(HPSCRIPTVM v,PSInteger idx,PSBool *b);
PSCRIPT_API PSRESULT ps_getthread(HPSCRIPTVM v,PSInteger idx,HPSCRIPTVM *thread);
PSCRIPT_API PSRESULT ps_getuserpointer(HPSCRIPTVM v,PSInteger idx,PSUserPointer *p);
PSCRIPT_API PSRESULT ps_getuserdata(HPSCRIPTVM v,PSInteger idx,PSUserPointer *p,PSUserPointer *typetag);
PSCRIPT_API PSRESULT ps_settypetag(HPSCRIPTVM v,PSInteger idx,PSUserPointer typetag);
PSCRIPT_API PSRESULT ps_gettypetag(HPSCRIPTVM v,PSInteger idx,PSUserPointer *typetag);
PSCRIPT_API void ps_setreleasehook(HPSCRIPTVM v,PSInteger idx,PSRELEASEHOOK hook);
PSCRIPT_API PSRELEASEHOOK ps_getreleasehook(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSChar *ps_getscratchpad(HPSCRIPTVM v,PSInteger minsize);
PSCRIPT_API PSRESULT ps_getfunctioninfo(HPSCRIPTVM v,PSInteger level,PSFunctionInfo *fi);
PSCRIPT_API PSRESULT ps_getclosureinfo(HPSCRIPTVM v,PSInteger idx,PSUnsignedInteger *nparams,PSUnsignedInteger *nfreevars);
PSCRIPT_API PSRESULT ps_getclosurename(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_setnativeclosurename(HPSCRIPTVM v,PSInteger idx,const PSChar *name);
PSCRIPT_API PSRESULT ps_setinstanceup(HPSCRIPTVM v, PSInteger idx, PSUserPointer p);
PSCRIPT_API PSRESULT ps_getinstanceup(HPSCRIPTVM v, PSInteger idx, PSUserPointer *p,PSUserPointer typetag);
PSCRIPT_API PSRESULT ps_setclassudsize(HPSCRIPTVM v, PSInteger idx, PSInteger udsize);
PSCRIPT_API PSRESULT ps_newclass(HPSCRIPTVM v,PSBool hasbase);
PSCRIPT_API PSRESULT ps_createinstance(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_setattributes(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_getattributes(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_getclass(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API void ps_weakref(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_getdefaultdelegate(HPSCRIPTVM v,PSObjectType t);
PSCRIPT_API PSRESULT ps_getmemberhandle(HPSCRIPTVM v,PSInteger idx,HPSMEMBERHANDLE *handle);
PSCRIPT_API PSRESULT ps_getbyhandle(HPSCRIPTVM v,PSInteger idx,const HPSMEMBERHANDLE *handle);
PSCRIPT_API PSRESULT ps_setbyhandle(HPSCRIPTVM v,PSInteger idx,const HPSMEMBERHANDLE *handle);

/*object manipulation*/
PSCRIPT_API void ps_pushroottable(HPSCRIPTVM v);
PSCRIPT_API void ps_pushregistrytable(HPSCRIPTVM v);
PSCRIPT_API void ps_pushconsttable(HPSCRIPTVM v);
PSCRIPT_API PSRESULT ps_setroottable(HPSCRIPTVM v);
PSCRIPT_API PSRESULT ps_setconsttable(HPSCRIPTVM v);
PSCRIPT_API PSRESULT ps_newslot(HPSCRIPTVM v, PSInteger idx, PSBool bstatic);
PSCRIPT_API PSRESULT ps_deleteslot(HPSCRIPTVM v,PSInteger idx,PSBool pushval);
PSCRIPT_API PSRESULT ps_set(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_get(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_rawget(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_rawset(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_rawdeleteslot(HPSCRIPTVM v,PSInteger idx,PSBool pushval);
PSCRIPT_API PSRESULT ps_newmember(HPSCRIPTVM v,PSInteger idx,PSBool bstatic);
PSCRIPT_API PSRESULT ps_rawnewmember(HPSCRIPTVM v,PSInteger idx,PSBool bstatic);
PSCRIPT_API PSRESULT ps_arrayappend(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_arraypop(HPSCRIPTVM v,PSInteger idx,PSBool pushval);
PSCRIPT_API PSRESULT ps_arrayresize(HPSCRIPTVM v,PSInteger idx,PSInteger newsize);
PSCRIPT_API PSRESULT ps_arrayreverse(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_arrayremove(HPSCRIPTVM v,PSInteger idx,PSInteger itemidx);
PSCRIPT_API PSRESULT ps_arrayinsert(HPSCRIPTVM v,PSInteger idx,PSInteger destpos);
PSCRIPT_API PSRESULT ps_setdelegate(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_getdelegate(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_clone(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_setfreevariable(HPSCRIPTVM v,PSInteger idx,PSUnsignedInteger nval);
PSCRIPT_API PSRESULT ps_next(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_getweakrefval(HPSCRIPTVM v,PSInteger idx);
PSCRIPT_API PSRESULT ps_clear(HPSCRIPTVM v,PSInteger idx);

/*calls*/
PSCRIPT_API PSRESULT ps_call(HPSCRIPTVM v,PSInteger params,PSBool retval,PSBool raiseerror);
PSCRIPT_API PSRESULT ps_resume(HPSCRIPTVM v,PSBool retval,PSBool raiseerror);
PSCRIPT_API const PSChar *ps_getlocal(HPSCRIPTVM v,PSUnsignedInteger level,PSUnsignedInteger idx);
PSCRIPT_API PSRESULT ps_getcallee(HPSCRIPTVM v);
PSCRIPT_API const PSChar *ps_getfreevariable(HPSCRIPTVM v,PSInteger idx,PSUnsignedInteger nval);
PSCRIPT_API PSRESULT ps_throwerror(HPSCRIPTVM v,const PSChar *err);
PSCRIPT_API PSRESULT ps_throwobject(HPSCRIPTVM v);
PSCRIPT_API void ps_reseterror(HPSCRIPTVM v);
PSCRIPT_API void ps_getlasterror(HPSCRIPTVM v);

/*raw object handling*/
PSCRIPT_API PSRESULT ps_getstackobj(HPSCRIPTVM v,PSInteger idx,HPSOBJECT *po);
PSCRIPT_API void ps_pushobject(HPSCRIPTVM v,HPSOBJECT obj);
PSCRIPT_API void ps_addref(HPSCRIPTVM v,HPSOBJECT *po);
PSCRIPT_API PSBool ps_release(HPSCRIPTVM v,HPSOBJECT *po);
PSCRIPT_API PSUnsignedInteger ps_getrefcount(HPSCRIPTVM v,HPSOBJECT *po);
PSCRIPT_API void ps_resetobject(HPSOBJECT *po);
PSCRIPT_API const PSChar *ps_objtostring(const HPSOBJECT *o);
PSCRIPT_API PSBool ps_objtobool(const HPSOBJECT *o);
PSCRIPT_API PSInteger ps_objtointeger(const HPSOBJECT *o);
PSCRIPT_API PSFloat ps_objtofloat(const HPSOBJECT *o);
PSCRIPT_API PSUserPointer ps_objtouserpointer(const HPSOBJECT *o);
PSCRIPT_API PSRESULT ps_getobjtypetag(const HPSOBJECT *o,PSUserPointer * typetag);
PSCRIPT_API PSUnsignedInteger ps_getvmrefcount(HPSCRIPTVM v, const HPSOBJECT *po);


/*GC*/
PSCRIPT_API PSInteger ps_collectgarbage(HPSCRIPTVM v);
PSCRIPT_API PSRESULT ps_resurrectunreachable(HPSCRIPTVM v);

/*serialization*/
PSCRIPT_API PSRESULT ps_writeclosure(HPSCRIPTVM vm,PSWRITEFUNC writef,PSUserPointer up);
PSCRIPT_API PSRESULT ps_readclosure(HPSCRIPTVM vm,PSREADFUNC readf,PSUserPointer up);

/*mem allocation*/
PSCRIPT_API void *ps_malloc(PSUnsignedInteger size);
PSCRIPT_API void *ps_realloc(void* p,PSUnsignedInteger oldsize,PSUnsignedInteger newsize);
PSCRIPT_API void ps_free(void *p,PSUnsignedInteger size);

/*debug*/
PSCRIPT_API PSRESULT ps_stackinfos(HPSCRIPTVM v,PSInteger level,PSStackInfos *si);
PSCRIPT_API void ps_setdebughook(HPSCRIPTVM v);
PSCRIPT_API void ps_setnativedebughook(HPSCRIPTVM v,PSDEBUGHOOK hook);

/*UTILITY MACRO*/
#define ps_isnumeric(o) ((o)._type&PSOBJECT_NUMERIC)
#define ps_istable(o) ((o)._type==OT_TABLE)
#define ps_isarray(o) ((o)._type==OT_ARRAY)
#define ps_isfunction(o) ((o)._type==OT_FUNCPROTO)
#define ps_isclosure(o) ((o)._type==OT_CLOSURE)
#define ps_isgenerator(o) ((o)._type==OT_GENERATOR)
#define ps_isnativeclosure(o) ((o)._type==OT_NATIVECLOSURE)
#define ps_isstring(o) ((o)._type==OT_STRING)
#define ps_isinteger(o) ((o)._type==OT_INTEGER)
#define ps_isfloat(o) ((o)._type==OT_FLOAT)
#define ps_isuserpointer(o) ((o)._type==OT_USERPOINTER)
#define ps_isuserdata(o) ((o)._type==OT_USERDATA)
#define ps_isthread(o) ((o)._type==OT_THREAD)
#define ps_isnull(o) ((o)._type==OT_NULL)
#define ps_isclass(o) ((o)._type==OT_CLASS)
#define ps_isinstance(o) ((o)._type==OT_INSTANCE)
#define ps_isbool(o) ((o)._type==OT_BOOL)
#define ps_isweakref(o) ((o)._type==OT_WEAKREF)
#define ps_type(o) ((o)._type)

/* deprecated */
#define ps_createslot(v,n) ps_newslot(v,n,PSFalse)

#define PS_OK (0)
#define PS_ERROR (-1)

#define PS_FAILED(res) (res<0)
#define PS_SUCCEEDED(res) (res>=0)

#ifdef __GNUC__
# define PS_UNUSED_ARG(x) __attribute__((unused)) x
#else
# define PS_UNUSED_ARG(x) x
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_PSCRIPT_H_*/
