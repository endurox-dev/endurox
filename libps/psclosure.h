/*  see copyright notice in pscript.h */
#ifndef _PSCLOSURE_H_
#define _PSCLOSURE_H_


#define _CALC_CLOSURE_SIZE(func) (sizeof(PSClosure) + (func->_noutervalues*sizeof(PSObjectPtr)) + (func->_ndefaultparams*sizeof(PSObjectPtr)))

struct PSFunctionProto;
struct PSClass;
struct PSClosure : public CHAINABLE_OBJ
{
private:
    PSClosure(PSSharedState *ss,PSFunctionProto *func){_function = func; __ObjAddRef(_function); _base = NULL; INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); _env = NULL; _root=NULL;}
public:
    static PSClosure *Create(PSSharedState *ss,PSFunctionProto *func,PSWeakRef *root){
        PSInteger size = _CALC_CLOSURE_SIZE(func);
        PSClosure *nc=(PSClosure*)PS_MALLOC(size);
        new (nc) PSClosure(ss,func);
        nc->_outervalues = (PSObjectPtr *)(nc + 1);
        nc->_defaultparams = &nc->_outervalues[func->_noutervalues];
        nc->_root = root;
         __ObjAddRef(nc->_root);
        _CONSTRUCT_VECTOR(PSObjectPtr,func->_noutervalues,nc->_outervalues);
        _CONSTRUCT_VECTOR(PSObjectPtr,func->_ndefaultparams,nc->_defaultparams);
        return nc;
    }
    void Release(){
        PSFunctionProto *f = _function;
        PSInteger size = _CALC_CLOSURE_SIZE(f);
        _DESTRUCT_VECTOR(PSObjectPtr,f->_noutervalues,_outervalues);
        _DESTRUCT_VECTOR(PSObjectPtr,f->_ndefaultparams,_defaultparams);
        __ObjRelease(_function);
        this->~PSClosure();
        ps_vm_free(this,size);
    }
    void SetRoot(PSWeakRef *r)
    {
        __ObjRelease(_root);
        _root = r;
        __ObjAddRef(_root);
    }
    PSClosure *Clone()
    {
        PSFunctionProto *f = _function;
        PSClosure * ret = PSClosure::Create(_opt_ss(this),f,_root);
        ret->_env = _env;
        if(ret->_env) __ObjAddRef(ret->_env);
        _COPY_VECTOR(ret->_outervalues,_outervalues,f->_noutervalues);
        _COPY_VECTOR(ret->_defaultparams,_defaultparams,f->_ndefaultparams);
        return ret;
    }
    ~PSClosure();

    bool Save(PSVM *v,PSUserPointer up,PSWRITEFUNC write);
    static bool Load(PSVM *v,PSUserPointer up,PSREADFUNC read,PSObjectPtr &ret);
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable **chain);
    void Finalize(){
        PSFunctionProto *f = _function;
        _NULL_PSOBJECT_VECTOR(_outervalues,f->_noutervalues);
        _NULL_PSOBJECT_VECTOR(_defaultparams,f->_ndefaultparams);
    }
    PSObjectType GetType() {return OT_CLOSURE;}
#endif
    PSWeakRef *_env;
    PSWeakRef *_root;
    PSClass *_base;
    PSFunctionProto *_function;
    PSObjectPtr *_outervalues;
    PSObjectPtr *_defaultparams;
};

//////////////////////////////////////////////
struct PSOuter : public CHAINABLE_OBJ
{

private:
    PSOuter(PSSharedState *ss, PSObjectPtr *outer){_valptr = outer; _next = NULL; INIT_CHAIN(); ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); }

public:
    static PSOuter *Create(PSSharedState *ss, PSObjectPtr *outer)
    {
        PSOuter *nc  = (PSOuter*)PS_MALLOC(sizeof(PSOuter));
        new (nc) PSOuter(ss, outer);
        return nc;
    }
    ~PSOuter() { REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this); }

    void Release()
    {
        this->~PSOuter();
        ps_vm_free(this,sizeof(PSOuter));
    }

#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable **chain);
    void Finalize() { _value.Null(); }
    PSObjectType GetType() {return OT_OUTER;}
#endif

    PSObjectPtr *_valptr;  /* pointer to value on stack, or _value below */
    PSInteger    _idx;     /* idx in stack array, for relocation */
    PSObjectPtr  _value;   /* value of outer after stack frame is closed */
    PSOuter     *_next;    /* pointer to next outer when frame is open   */
};

//////////////////////////////////////////////
struct PSGenerator : public CHAINABLE_OBJ
{
    enum PSGeneratorState{eRunning,eSuspended,eDead};
private:
    PSGenerator(PSSharedState *ss,PSClosure *closure){_closure=closure;_state=eRunning;_ci._generator=NULL;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
    static PSGenerator *Create(PSSharedState *ss,PSClosure *closure){
        PSGenerator *nc=(PSGenerator*)PS_MALLOC(sizeof(PSGenerator));
        new (nc) PSGenerator(ss,closure);
        return nc;
    }
    ~PSGenerator()
    {
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
    }
    void Kill(){
        _state=eDead;
        _stack.resize(0);
        _closure.Null();}
    void Release(){
        ps_delete(this,PSGenerator);
    }

    bool Yield(PSVM *v,PSInteger target);
    bool Resume(PSVM *v,PSObjectPtr &dest);
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable **chain);
    void Finalize(){_stack.resize(0);_closure.Null();}
    PSObjectType GetType() {return OT_GENERATOR;}
#endif
    PSObjectPtr _closure;
    PSObjectPtrVec _stack;
    PSVM::CallInfo _ci;
    ExceptionsTraps _etraps;
    PSGeneratorState _state;
};

#define _CALC_NATVIVECLOSURE_SIZE(noutervalues) (sizeof(PSNativeClosure) + (noutervalues*sizeof(PSObjectPtr)))

struct PSNativeClosure : public CHAINABLE_OBJ
{
private:
    PSNativeClosure(PSSharedState *ss,PSFUNCTION func){_function=func;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); _env = NULL;}
public:
    static PSNativeClosure *Create(PSSharedState *ss,PSFUNCTION func,PSInteger nouters)
    {
        PSInteger size = _CALC_NATVIVECLOSURE_SIZE(nouters);
        PSNativeClosure *nc=(PSNativeClosure*)PS_MALLOC(size);
        new (nc) PSNativeClosure(ss,func);
        nc->_outervalues = (PSObjectPtr *)(nc + 1);
        nc->_noutervalues = nouters;
        _CONSTRUCT_VECTOR(PSObjectPtr,nc->_noutervalues,nc->_outervalues);
        return nc;
    }
    PSNativeClosure *Clone()
    {
        PSNativeClosure * ret = PSNativeClosure::Create(_opt_ss(this),_function,_noutervalues);
        ret->_env = _env;
        if(ret->_env) __ObjAddRef(ret->_env);
        ret->_name = _name;
        _COPY_VECTOR(ret->_outervalues,_outervalues,_noutervalues);
        ret->_typecheck.copy(_typecheck);
        ret->_nparamscheck = _nparamscheck;
        return ret;
    }
    ~PSNativeClosure()
    {
        __ObjRelease(_env);
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
    }
    void Release(){
        PSInteger size = _CALC_NATVIVECLOSURE_SIZE(_noutervalues);
        _DESTRUCT_VECTOR(PSObjectPtr,_noutervalues,_outervalues);
        this->~PSNativeClosure();
        ps_free(this,size);
    }

#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable **chain);
    void Finalize() { _NULL_PSOBJECT_VECTOR(_outervalues,_noutervalues); }
    PSObjectType GetType() {return OT_NATIVECLOSURE;}
#endif
    PSInteger _nparamscheck;
    PSIntVec _typecheck;
    PSObjectPtr *_outervalues;
    PSUnsignedInteger _noutervalues;
    PSWeakRef *_env;
    PSFUNCTION _function;
    PSObjectPtr _name;
};



#endif //_PSCLOSURE_H_
