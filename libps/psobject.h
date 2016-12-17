/*  see copyright notice in pscript.h */
#ifndef _PSOBJECT_H_
#define _PSOBJECT_H_

#include "psutils.h"

#ifdef _PS64
#define UINT_MINUS_ONE (0xFFFFFFFFFFFFFFFF)
#else
#define UINT_MINUS_ONE (0xFFFFFFFF)
#endif

#define PS_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define PS_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define PS_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

struct PSSharedState;

enum PSMetaMethod{
    MT_ADD=0,
    MT_SUB=1,
    MT_MUL=2,
    MT_DIV=3,
    MT_UNM=4,
    MT_MODULO=5,
    MT_SET=6,
    MT_GET=7,
    MT_TYPEOF=8,
    MT_NEXTI=9,
    MT_CMP=10,
    MT_CALL=11,
    MT_CLONED=12,
    MT_NEWSLOT=13,
    MT_DELSLOT=14,
    MT_TOSTRING=15,
    MT_NEWMEMBER=16,
    MT_INHERITED=17,
    MT_LAST = 18
};

#define MM_ADD      _SC("_add")
#define MM_SUB      _SC("_sub")
#define MM_MUL      _SC("_mul")
#define MM_DIV      _SC("_div")
#define MM_UNM      _SC("_unm")
#define MM_MODULO   _SC("_modulo")
#define MM_SET      _SC("_set")
#define MM_GET      _SC("_get")
#define MM_TYPEOF   _SC("_typeof")
#define MM_NEXTI    _SC("_nexti")
#define MM_CMP      _SC("_cmp")
#define MM_CALL     _SC("_call")
#define MM_CLONED   _SC("_cloned")
#define MM_NEWSLOT  _SC("_newslot")
#define MM_DELSLOT  _SC("_delslot")
#define MM_TOSTRING _SC("_tostring")
#define MM_NEWMEMBER _SC("_newmember")
#define MM_INHERITED _SC("_inherited")


#define _CONSTRUCT_VECTOR(type,size,ptr) { \
    for(PSInteger n = 0; n < ((PSInteger)size); n++) { \
            new (&ptr[n]) type(); \
        } \
}

#define _DESTRUCT_VECTOR(type,size,ptr) { \
    for(PSInteger nl = 0; nl < ((PSInteger)size); nl++) { \
            ptr[nl].~type(); \
    } \
}

#define _COPY_VECTOR(dest,src,size) { \
    for(PSInteger _n_ = 0; _n_ < ((PSInteger)size); _n_++) { \
        dest[_n_] = src[_n_]; \
    } \
}

#define _NULL_PSOBJECT_VECTOR(vec,size) { \
    for(PSInteger _n_ = 0; _n_ < ((PSInteger)size); _n_++) { \
        vec[_n_].Null(); \
    } \
}

#define MINPOWER2 4

struct PSRefCounted
{
    PSUnsignedInteger _uiRef;
    struct PSWeakRef *_weakref;
    PSRefCounted() { _uiRef = 0; _weakref = NULL; }
    virtual ~PSRefCounted();
    PSWeakRef *GetWeakRef(PSObjectType type);
    virtual void Release()=0;

};

struct PSWeakRef : PSRefCounted
{
    void Release();
    PSObject _obj;
};

#define _realval(o) (type((o)) != OT_WEAKREF?(PSObject)o:_weakref(o)->_obj)

struct PSObjectPtr;

#define __AddRef(type,unval) if(ISREFCOUNTED(type)) \
        { \
            unval.pRefCounted->_uiRef++; \
        }

#define __Release(type,unval) if(ISREFCOUNTED(type) && ((--unval.pRefCounted->_uiRef)==0))  \
        {   \
            unval.pRefCounted->Release();   \
        }

#define __ObjRelease(obj) { \
    if((obj)) { \
        (obj)->_uiRef--; \
        if((obj)->_uiRef == 0) \
            (obj)->Release(); \
        (obj) = NULL;   \
    } \
}

#define __ObjAddRef(obj) { \
    (obj)->_uiRef++; \
}

#define type(obj) ((obj)._type)
#define is_delegable(t) (type(t)&PSOBJECT_DELEGABLE)
#define raw_type(obj) _RAW_TYPE((obj)._type)

#define _integer(obj) ((obj)._unVal.nInteger)
#define _float(obj) ((obj)._unVal.fFloat)
#define _string(obj) ((obj)._unVal.pString)
#define _table(obj) ((obj)._unVal.pTable)
#define _array(obj) ((obj)._unVal.pArray)
#define _closure(obj) ((obj)._unVal.pClosure)
#define _generator(obj) ((obj)._unVal.pGenerator)
#define _nativeclosure(obj) ((obj)._unVal.pNativeClosure)
#define _userdata(obj) ((obj)._unVal.pUserData)
#define _userpointer(obj) ((obj)._unVal.pUserPointer)
#define _thread(obj) ((obj)._unVal.pThread)
#define _funcproto(obj) ((obj)._unVal.pFunctionProto)
#define _class(obj) ((obj)._unVal.pClass)
#define _instance(obj) ((obj)._unVal.pInstance)
#define _delegable(obj) ((PSDelegable *)(obj)._unVal.pDelegable)
#define _weakref(obj) ((obj)._unVal.pWeakRef)
#define _outer(obj) ((obj)._unVal.pOuter)
#define _refcounted(obj) ((obj)._unVal.pRefCounted)
#define _rawval(obj) ((obj)._unVal.raw)

#define _stringval(obj) (obj)._unVal.pString->_val
#define _userdataval(obj) ((PSUserPointer)ps_aligning((obj)._unVal.pUserData + 1))

#define tofloat(num) ((type(num)==OT_INTEGER)?(PSFloat)_integer(num):_float(num))
#define tointeger(num) ((type(num)==OT_FLOAT)?(PSInteger)_float(num):_integer(num))
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#if defined(PSUSEDOUBLE) && !defined(_PS64) || !defined(PSUSEDOUBLE) && defined(_PS64)
#define PS_REFOBJECT_INIT() PS_OBJECT_RAWINIT()
#else
#define PS_REFOBJECT_INIT()
#endif

#define _REF_TYPE_DECL(type,_class,sym) \
    PSObjectPtr(_class * x) \
    { \
        PS_OBJECT_RAWINIT() \
        _type=type; \
        _unVal.sym = x; \
        assert(_unVal.pTable); \
        _unVal.pRefCounted->_uiRef++; \
    } \
    inline PSObjectPtr& operator=(_class *x) \
    {  \
        PSObjectType tOldType; \
        PSObjectValue unOldVal; \
        tOldType=_type; \
        unOldVal=_unVal; \
        _type = type; \
        PS_REFOBJECT_INIT() \
        _unVal.sym = x; \
        _unVal.pRefCounted->_uiRef++; \
        __Release(tOldType,unOldVal); \
        return *this; \
    }

#define _SCALAR_TYPE_DECL(type,_class,sym) \
    PSObjectPtr(_class x) \
    { \
        PS_OBJECT_RAWINIT() \
        _type=type; \
        _unVal.sym = x; \
    } \
    inline PSObjectPtr& operator=(_class x) \
    {  \
        __Release(_type,_unVal); \
        _type = type; \
        PS_OBJECT_RAWINIT() \
        _unVal.sym = x; \
        return *this; \
    }
struct PSObjectPtr : public PSObject
{
    PSObjectPtr()
    {
        PS_OBJECT_RAWINIT()
        _type=OT_NULL;
        _unVal.pUserPointer=NULL;
    }
    PSObjectPtr(const PSObjectPtr &o)
    {
        _type = o._type;
        _unVal = o._unVal;
        __AddRef(_type,_unVal);
    }
    PSObjectPtr(const PSObject &o)
    {
        _type = o._type;
        _unVal = o._unVal;
        __AddRef(_type,_unVal);
    }
    _REF_TYPE_DECL(OT_TABLE,PSTable,pTable)
    _REF_TYPE_DECL(OT_CLASS,PSClass,pClass)
    _REF_TYPE_DECL(OT_INSTANCE,PSInstance,pInstance)
    _REF_TYPE_DECL(OT_ARRAY,PSArray,pArray)
    _REF_TYPE_DECL(OT_CLOSURE,PSClosure,pClosure)
    _REF_TYPE_DECL(OT_NATIVECLOSURE,PSNativeClosure,pNativeClosure)
    _REF_TYPE_DECL(OT_OUTER,PSOuter,pOuter)
    _REF_TYPE_DECL(OT_GENERATOR,PSGenerator,pGenerator)
    _REF_TYPE_DECL(OT_STRING,PSString,pString)
    _REF_TYPE_DECL(OT_USERDATA,PSUserData,pUserData)
    _REF_TYPE_DECL(OT_WEAKREF,PSWeakRef,pWeakRef)
    _REF_TYPE_DECL(OT_THREAD,PSVM,pThread)
    _REF_TYPE_DECL(OT_FUNCPROTO,PSFunctionProto,pFunctionProto)

    _SCALAR_TYPE_DECL(OT_INTEGER,PSInteger,nInteger)
    _SCALAR_TYPE_DECL(OT_FLOAT,PSFloat,fFloat)
    _SCALAR_TYPE_DECL(OT_USERPOINTER,PSUserPointer,pUserPointer)

    PSObjectPtr(bool bBool)
    {
        PS_OBJECT_RAWINIT()
        _type = OT_BOOL;
        _unVal.nInteger = bBool?1:0;
    }
    inline PSObjectPtr& operator=(bool b)
    {
        __Release(_type,_unVal);
        PS_OBJECT_RAWINIT()
        _type = OT_BOOL;
        _unVal.nInteger = b?1:0;
        return *this;
    }

    ~PSObjectPtr()
    {
        __Release(_type,_unVal);
    }

    inline PSObjectPtr& operator=(const PSObjectPtr& obj)
    {
        PSObjectType tOldType;
        PSObjectValue unOldVal;
        tOldType=_type;
        unOldVal=_unVal;
        _unVal = obj._unVal;
        _type = obj._type;
        __AddRef(_type,_unVal);
        __Release(tOldType,unOldVal);
        return *this;
    }
    inline PSObjectPtr& operator=(const PSObject& obj)
    {
        PSObjectType tOldType;
        PSObjectValue unOldVal;
        tOldType=_type;
        unOldVal=_unVal;
        _unVal = obj._unVal;
        _type = obj._type;
        __AddRef(_type,_unVal);
        __Release(tOldType,unOldVal);
        return *this;
    }
    inline void Null()
    {
        PSObjectType tOldType = _type;
        PSObjectValue unOldVal = _unVal;
        _type = OT_NULL;
        _unVal.raw = (PSRawObjectVal)NULL;
        __Release(tOldType ,unOldVal);
    }
    private:
        PSObjectPtr(const PSChar *){} //safety
};


inline void _Swap(PSObject &a,PSObject &b)
{
    PSObjectType tOldType = a._type;
    PSObjectValue unOldVal = a._unVal;
    a._type = b._type;
    a._unVal = b._unVal;
    b._type = tOldType;
    b._unVal = unOldVal;
}

/////////////////////////////////////////////////////////////////////////////////////
#ifndef NO_GARBAGE_COLLECTOR
#define MARK_FLAG 0x80000000
struct PSCollectable : public PSRefCounted {
    PSCollectable *_next;
    PSCollectable *_prev;
    PSSharedState *_sharedstate;
    virtual PSObjectType GetType()=0;
    virtual void Release()=0;
    virtual void Mark(PSCollectable **chain)=0;
    void UnMark();
    virtual void Finalize()=0;
    static void AddToChain(PSCollectable **chain,PSCollectable *c);
    static void RemoveFromChain(PSCollectable **chain,PSCollectable *c);
};


#define ADD_TO_CHAIN(chain,obj) AddToChain(chain,obj)
#define REMOVE_FROM_CHAIN(chain,obj) {if(!(_uiRef&MARK_FLAG))RemoveFromChain(chain,obj);}
#define CHAINABLE_OBJ PSCollectable
#define INIT_CHAIN() {_next=NULL;_prev=NULL;_sharedstate=ss;}
#else

#define ADD_TO_CHAIN(chain,obj) ((void)0)
#define REMOVE_FROM_CHAIN(chain,obj) ((void)0)
#define CHAINABLE_OBJ PSRefCounted
#define INIT_CHAIN() ((void)0)
#endif

struct PSDelegable : public CHAINABLE_OBJ {
    bool SetDelegate(PSTable *m);
    virtual bool GetMetaMethod(PSVM *v,PSMetaMethod mm,PSObjectPtr &res);
    PSTable *_delegate;
};

PSUnsignedInteger TranslateIndex(const PSObjectPtr &idx);
typedef psvector<PSObjectPtr> PSObjectPtrVec;
typedef psvector<PSInteger> PSIntVec;
const PSChar *GetTypeName(const PSObjectPtr &obj1);
const PSChar *IdType2Name(PSObjectType type);



#endif //_PSOBJECT_H_
