/*
    see copyright notice in pscript.h
*/
#include "pspcheader.h"
#include "psvm.h"
#include "pstable.h"
#include "psclass.h"
#include "psfuncproto.h"
#include "psclosure.h"



PSClass::PSClass(PSSharedState *ss,PSClass *base)
{
    _base = base;
    _typetag = 0;
    _hook = NULL;
    _udsize = 0;
    _locked = false;
    _constructoridx = -1;
    if(_base) {
        _constructoridx = _base->_constructoridx;
        _udsize = _base->_udsize;
        _defaultvalues.copy(base->_defaultvalues);
        _methods.copy(base->_methods);
        _COPY_VECTOR(_metamethods,base->_metamethods,MT_LAST);
        __ObjAddRef(_base);
    }
    _members = base?base->_members->Clone() : PSTable::Create(ss,0);
    __ObjAddRef(_members);

    INIT_CHAIN();
    ADD_TO_CHAIN(&_sharedstate->_gc_chain, this);
}

void PSClass::Finalize() {
    _attributes.Null();
    _NULL_PSOBJECT_VECTOR(_defaultvalues,_defaultvalues.size());
    _methods.resize(0);
    _NULL_PSOBJECT_VECTOR(_metamethods,MT_LAST);
    __ObjRelease(_members);
    if(_base) {
        __ObjRelease(_base);
    }
}

PSClass::~PSClass()
{
    REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
    Finalize();
}

bool PSClass::NewSlot(PSSharedState *ss,const PSObjectPtr &key,const PSObjectPtr &val,bool bstatic)
{
    PSObjectPtr temp;
    bool belongs_to_static_table = type(val) == OT_CLOSURE || type(val) == OT_NATIVECLOSURE || bstatic;
    if(_locked && !belongs_to_static_table)
        return false; //the class already has an instance so cannot be modified
    if(_members->Get(key,temp) && _isfield(temp)) //overrides the default value
    {
        _defaultvalues[_member_idx(temp)].val = val;
        return true;
    }
    if(belongs_to_static_table) {
        PSInteger mmidx;
        if((type(val) == OT_CLOSURE || type(val) == OT_NATIVECLOSURE) &&
            (mmidx = ss->GetMetaMethodIdxByName(key)) != -1) {
            _metamethods[mmidx] = val;
        }
        else {
            PSObjectPtr theval = val;
            if(_base && type(val) == OT_CLOSURE) {
                theval = _closure(val)->Clone();
                _closure(theval)->_base = _base;
                __ObjAddRef(_base); //ref for the closure
            }
            if(type(temp) == OT_NULL) {
                bool isconstructor;
                PSVM::IsEqual(ss->_constructoridx, key, isconstructor);
                if(isconstructor) {
                    _constructoridx = (PSInteger)_methods.size();
                }
                PSClassMember m;
                m.val = theval;
                _members->NewSlot(key,PSObjectPtr(_make_method_idx(_methods.size())));
                _methods.push_back(m);
            }
            else {
                _methods[_member_idx(temp)].val = theval;
            }
        }
        return true;
    }
    PSClassMember m;
    m.val = val;
    _members->NewSlot(key,PSObjectPtr(_make_field_idx(_defaultvalues.size())));
    _defaultvalues.push_back(m);
    return true;
}

PSInstance *PSClass::CreateInstance()
{
    if(!_locked) Lock();
    return PSInstance::Create(_opt_ss(this),this);
}

PSInteger PSClass::Next(const PSObjectPtr &refpos, PSObjectPtr &outkey, PSObjectPtr &outval)
{
    PSObjectPtr oval;
    PSInteger idx = _members->Next(false,refpos,outkey,oval);
    if(idx != -1) {
        if(_ismethod(oval)) {
            outval = _methods[_member_idx(oval)].val;
        }
        else {
            PSObjectPtr &o = _defaultvalues[_member_idx(oval)].val;
            outval = _realval(o);
        }
    }
    return idx;
}

bool PSClass::SetAttributes(const PSObjectPtr &key,const PSObjectPtr &val)
{
    PSObjectPtr idx;
    if(_members->Get(key,idx)) {
        if(_isfield(idx))
            _defaultvalues[_member_idx(idx)].attrs = val;
        else
            _methods[_member_idx(idx)].attrs = val;
        return true;
    }
    return false;
}

bool PSClass::GetAttributes(const PSObjectPtr &key,PSObjectPtr &outval)
{
    PSObjectPtr idx;
    if(_members->Get(key,idx)) {
        outval = (_isfield(idx)?_defaultvalues[_member_idx(idx)].attrs:_methods[_member_idx(idx)].attrs);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////
void PSInstance::Init(PSSharedState *ss)
{
    _userpointer = NULL;
    _hook = NULL;
    __ObjAddRef(_class);
    _delegate = _class->_members;
    INIT_CHAIN();
    ADD_TO_CHAIN(&_sharedstate->_gc_chain, this);
}

PSInstance::PSInstance(PSSharedState *ss, PSClass *c, PSInteger memsize)
{
    _memsize = memsize;
    _class = c;
    PSUnsignedInteger nvalues = _class->_defaultvalues.size();
    for(PSUnsignedInteger n = 0; n < nvalues; n++) {
        new (&_values[n]) PSObjectPtr(_class->_defaultvalues[n].val);
    }
    Init(ss);
}

PSInstance::PSInstance(PSSharedState *ss, PSInstance *i, PSInteger memsize)
{
    _memsize = memsize;
    _class = i->_class;
    PSUnsignedInteger nvalues = _class->_defaultvalues.size();
    for(PSUnsignedInteger n = 0; n < nvalues; n++) {
        new (&_values[n]) PSObjectPtr(i->_values[n]);
    }
    Init(ss);
}

void PSInstance::Finalize()
{
    PSUnsignedInteger nvalues = _class->_defaultvalues.size();
    __ObjRelease(_class);
    _NULL_PSOBJECT_VECTOR(_values,nvalues);
}

PSInstance::~PSInstance()
{
    REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
    if(_class){ Finalize(); } //if _class is null it was already finalized by the GC
}

bool PSInstance::GetMetaMethod(PSVM PS_UNUSED_ARG(*v),PSMetaMethod mm,PSObjectPtr &res)
{
    if(type(_class->_metamethods[mm]) != OT_NULL) {
        res = _class->_metamethods[mm];
        return true;
    }
    return false;
}

bool PSInstance::InstanceOf(PSClass *trg)
{
    PSClass *parent = _class;
    while(parent != NULL) {
        if(parent == trg)
            return true;
        parent = parent->_base;
    }
    return false;
}
