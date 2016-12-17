/*  see copyright notice in pscript.h */
#ifndef _PSCLASS_H_
#define _PSCLASS_H_

struct PSInstance;

struct PSClassMember {
    PSObjectPtr val;
    PSObjectPtr attrs;
    void Null() {
        val.Null();
        attrs.Null();
    }
};

typedef psvector<PSClassMember> PSClassMemberVec;

#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000

#define _ismethod(o) (_integer(o)&MEMBER_TYPE_METHOD)
#define _isfield(o) (_integer(o)&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((PSInteger)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((PSInteger)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (_integer(o)&0xFF000000)
#define _member_idx(o) (_integer(o)&0x00FFFFFF)

struct PSClass : public CHAINABLE_OBJ
{
    PSClass(PSSharedState *ss,PSClass *base);
public:
    static PSClass* Create(PSSharedState *ss,PSClass *base) {
        PSClass *newclass = (PSClass *)PS_MALLOC(sizeof(PSClass));
        new (newclass) PSClass(ss, base);
        return newclass;
    }
    ~PSClass();
    bool NewSlot(PSSharedState *ss, const PSObjectPtr &key,const PSObjectPtr &val,bool bstatic);
    bool Get(const PSObjectPtr &key,PSObjectPtr &val) {
        if(_members->Get(key,val)) {
            if(_isfield(val)) {
                PSObjectPtr &o = _defaultvalues[_member_idx(val)].val;
                val = _realval(o);
            }
            else {
                val = _methods[_member_idx(val)].val;
            }
            return true;
        }
        return false;
    }
    bool GetConstructor(PSObjectPtr &ctor)
    {
        if(_constructoridx != -1) {
            ctor = _methods[_constructoridx].val;
            return true;
        }
        return false;
    }
    bool SetAttributes(const PSObjectPtr &key,const PSObjectPtr &val);
    bool GetAttributes(const PSObjectPtr &key,PSObjectPtr &outval);
    void Lock() { _locked = true; if(_base) _base->Lock(); }
    void Release() {
        if (_hook) { _hook(_typetag,0);}
        ps_delete(this, PSClass);
    }
    void Finalize();
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable ** );
    PSObjectType GetType() {return OT_CLASS;}
#endif
    PSInteger Next(const PSObjectPtr &refpos, PSObjectPtr &outkey, PSObjectPtr &outval);
    PSInstance *CreateInstance();
    PSTable *_members;
    PSClass *_base;
    PSClassMemberVec _defaultvalues;
    PSClassMemberVec _methods;
    PSObjectPtr _metamethods[MT_LAST];
    PSObjectPtr _attributes;
    PSUserPointer _typetag;
    PSRELEASEHOOK _hook;
    bool _locked;
    PSInteger _constructoridx;
    PSInteger _udsize;
};

#define calcinstancesize(_theclass_) \
    (_theclass_->_udsize + ps_aligning(sizeof(PSInstance) +  (sizeof(PSObjectPtr)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0))))

struct PSInstance : public PSDelegable
{
    void Init(PSSharedState *ss);
    PSInstance(PSSharedState *ss, PSClass *c, PSInteger memsize);
    PSInstance(PSSharedState *ss, PSInstance *c, PSInteger memsize);
public:
    static PSInstance* Create(PSSharedState *ss,PSClass *theclass) {

        PSInteger size = calcinstancesize(theclass);
        PSInstance *newinst = (PSInstance *)PS_MALLOC(size);
        new (newinst) PSInstance(ss, theclass,size);
        if(theclass->_udsize) {
            newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
        }
        return newinst;
    }
    PSInstance *Clone(PSSharedState *ss)
    {
        PSInteger size = calcinstancesize(_class);
        PSInstance *newinst = (PSInstance *)PS_MALLOC(size);
        new (newinst) PSInstance(ss, this,size);
        if(_class->_udsize) {
            newinst->_userpointer = ((unsigned char *)newinst) + (size - _class->_udsize);
        }
        return newinst;
    }
    ~PSInstance();
    bool Get(const PSObjectPtr &key,PSObjectPtr &val)  {
        if(_class->_members->Get(key,val)) {
            if(_isfield(val)) {
                PSObjectPtr &o = _values[_member_idx(val)];
                val = _realval(o);
            }
            else {
                val = _class->_methods[_member_idx(val)].val;
            }
            return true;
        }
        return false;
    }
    bool Set(const PSObjectPtr &key,const PSObjectPtr &val) {
        PSObjectPtr idx;
        if(_class->_members->Get(key,idx) && _isfield(idx)) {
            _values[_member_idx(idx)] = val;
            return true;
        }
        return false;
    }
    void Release() {
        _uiRef++;
        if (_hook) { _hook(_userpointer,0);}
        _uiRef--;
        if(_uiRef > 0) return;
        PSInteger size = _memsize;
        this->~PSInstance();
        PS_FREE(this, size);
    }
    void Finalize();
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable ** );
    PSObjectType GetType() {return OT_INSTANCE;}
#endif
    bool InstanceOf(PSClass *trg);
    bool GetMetaMethod(PSVM *v,PSMetaMethod mm,PSObjectPtr &res);

    PSClass *_class;
    PSUserPointer _userpointer;
    PSRELEASEHOOK _hook;
    PSInteger _memsize;
    PSObjectPtr _values[1];
};

#endif //_PSCLASS_H_
