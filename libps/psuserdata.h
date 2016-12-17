/*  see copyright notice in pscript.h */
#ifndef _PSUSERDATA_H_
#define _PSUSERDATA_H_

struct PSUserData : PSDelegable
{
    PSUserData(PSSharedState *ss){ _delegate = 0; _hook = NULL; INIT_CHAIN(); ADD_TO_CHAIN(&_ss(this)->_gc_chain, this); }
    ~PSUserData()
    {
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
        SetDelegate(NULL);
    }
    static PSUserData* Create(PSSharedState *ss, PSInteger size)
    {
        PSUserData* ud = (PSUserData*)PS_MALLOC(ps_aligning(sizeof(PSUserData))+size);
        new (ud) PSUserData(ss);
        ud->_size = size;
        ud->_typetag = 0;
        return ud;
    }
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable **chain);
    void Finalize(){SetDelegate(NULL);}
    PSObjectType GetType(){ return OT_USERDATA;}
#endif
    void Release() {
        if (_hook) _hook((PSUserPointer)ps_aligning(this + 1),_size);
        PSInteger tsize = _size;
        this->~PSUserData();
        PS_FREE(this, ps_aligning(sizeof(PSUserData)) + tsize);
    }


    PSInteger _size;
    PSRELEASEHOOK _hook;
    PSUserPointer _typetag;
    //PSChar _val[1];
};

#endif //_PSUSERDATA_H_
