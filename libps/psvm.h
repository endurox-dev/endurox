/*	see copyright notice in pscript.h */
#ifndef _PSVM_H_
#define _PSVM_H_

#include "psopcodes.h"
#include "psobject.h"
#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 15

#define PS_SUSPEND_FLAG -666
#define DONT_FALL_BACK 666
#define EXISTS_FALL_BACK -1
//base lib
void ps_base_register(HPSCRIPTVM v);

struct PSExceptionTrap{
	PSExceptionTrap() {}
	PSExceptionTrap(PSInteger ss, PSInteger stackbase,PSInstruction *ip, PSInteger ex_target){ _stacksize = ss; _stackbase = stackbase; _ip = ip; _extarget = ex_target;}
	PSExceptionTrap(const PSExceptionTrap &et) { (*this) = et;	}
	PSInteger _stackbase;
	PSInteger _stacksize;
	PSInstruction *_ip;
	PSInteger _extarget;
};

#define _INLINE 

#define STK(a) _stack._vals[_stackbase+(a)]
#define TARGET _stack._vals[_stackbase+arg0]

typedef psvector<PSExceptionTrap> ExceptionsTraps;

struct PSVM : public CHAINABLE_OBJ
{
	struct CallInfo{
		//CallInfo() { _generator = NULL;}
		PSInstruction *_ip;
		PSObjectPtr *_literals;
		PSObjectPtr _closure;
		PSGenerator *_generator;
		PSInt32 _etraps;
		PSInt32 _prevstkbase;
		PSInt32 _prevtop;
		PSInt32 _target;
		PSInt32 _ncalls;
		PSBool _root;
	};
	
typedef psvector<CallInfo> CallInfoVec;
public:
	void DebugHookProxy(PSInteger type, const PSChar * sourcename, PSInteger line, const PSChar * funcname);
	static void _DebugHookProxy(HPSCRIPTVM v, PSInteger type, const PSChar * sourcename, PSInteger line, const PSChar * funcname);
	enum ExecutionType { ET_CALL, ET_RESUME_GENERATOR, ET_RESUME_VM,ET_RESUME_THROW_VM };
	PSVM(PSSharedState *ss);
	~PSVM();
	bool Init(PSVM *friendvm, PSInteger stacksize);
	bool Execute(PSObjectPtr &func, PSInteger nargs, PSInteger stackbase, PSObjectPtr &outres, PSBool raiseerror, ExecutionType et = ET_CALL);
	//starts a native call return when the NATIVE closure returns
	bool CallNative(PSNativeClosure *nclosure, PSInteger nargs, PSInteger newbase, PSObjectPtr &retval,bool &suspend);
	//starts a PSCRIPT call in the same "Execution loop"
	bool StartCall(PSClosure *closure, PSInteger target, PSInteger nargs, PSInteger stackbase, bool tailcall);
	bool CreateClassInstance(PSClass *theclass, PSObjectPtr &inst, PSObjectPtr &constructor);
	//call a generic closure pure PSCRIPT or NATIVE
	bool Call(PSObjectPtr &closure, PSInteger nparams, PSInteger stackbase, PSObjectPtr &outres,PSBool raiseerror);
	PSRESULT Suspend();

	void CallDebugHook(PSInteger type,PSInteger forcedline=0);
	void CallErrorHandler(PSObjectPtr &e);
	bool Get(const PSObjectPtr &self, const PSObjectPtr &key, PSObjectPtr &dest, bool raw, PSInteger selfidx);
	PSInteger FallBackGet(const PSObjectPtr &self,const PSObjectPtr &key,PSObjectPtr &dest);
	bool InvokeDefaultDelegate(const PSObjectPtr &self,const PSObjectPtr &key,PSObjectPtr &dest);
	bool Set(const PSObjectPtr &self, const PSObjectPtr &key, const PSObjectPtr &val, PSInteger selfidx);
	PSInteger FallBackSet(const PSObjectPtr &self,const PSObjectPtr &key,const PSObjectPtr &val);
	bool NewSlot(const PSObjectPtr &self, const PSObjectPtr &key, const PSObjectPtr &val,bool bstatic);
	bool NewSlotA(const PSObjectPtr &self,const PSObjectPtr &key,const PSObjectPtr &val,const PSObjectPtr &attrs,bool bstatic,bool raw);
	bool DeleteSlot(const PSObjectPtr &self, const PSObjectPtr &key, PSObjectPtr &res);
	bool Clone(const PSObjectPtr &self, PSObjectPtr &target);
	bool ObjCmp(const PSObjectPtr &o1, const PSObjectPtr &o2,PSInteger &res);
	bool StringCat(const PSObjectPtr &str, const PSObjectPtr &obj, PSObjectPtr &dest);
	static bool IsEqual(const PSObjectPtr &o1,const PSObjectPtr &o2,bool &res);
	bool ToString(const PSObjectPtr &o,PSObjectPtr &res);
	PSString *PrintObjVal(const PSObjectPtr &o);

 
	void Raise_Error(const PSChar *s, ...);
	void Raise_Error(const PSObjectPtr &desc);
	void Raise_IdxError(const PSObjectPtr &o);
	void Raise_CompareError(const PSObject &o1, const PSObject &o2);
	void Raise_ParamTypeError(PSInteger nparam,PSInteger typemask,PSInteger type);

	void FindOuter(PSObjectPtr &target, PSObjectPtr *stackindex);
	void RelocateOuters();
	void CloseOuters(PSObjectPtr *stackindex);

	bool TypeOf(const PSObjectPtr &obj1, PSObjectPtr &dest);
	bool CallMetaMethod(PSObjectPtr &closure, PSMetaMethod mm, PSInteger nparams, PSObjectPtr &outres);
	bool ArithMetaMethod(PSInteger op, const PSObjectPtr &o1, const PSObjectPtr &o2, PSObjectPtr &dest);
	bool Return(PSInteger _arg0, PSInteger _arg1, PSObjectPtr &retval);
	//new stuff
	_INLINE bool ARITH_OP(PSUnsignedInteger op,PSObjectPtr &trg,const PSObjectPtr &o1,const PSObjectPtr &o2);
	_INLINE bool BW_OP(PSUnsignedInteger op,PSObjectPtr &trg,const PSObjectPtr &o1,const PSObjectPtr &o2);
	_INLINE bool NEG_OP(PSObjectPtr &trg,const PSObjectPtr &o1);
	_INLINE bool CMP_OP(CmpOP op, const PSObjectPtr &o1,const PSObjectPtr &o2,PSObjectPtr &res);
	bool CLOSURE_OP(PSObjectPtr &target, PSFunctionProto *func);
	bool CLASS_OP(PSObjectPtr &target,PSInteger base,PSInteger attrs);
	//return true if the loop is finished
	bool FOREACH_OP(PSObjectPtr &o1,PSObjectPtr &o2,PSObjectPtr &o3,PSObjectPtr &o4,PSInteger arg_2,int exitpos,int &jump);
	//_INLINE bool LOCAL_INC(PSInteger op,PSObjectPtr &target, PSObjectPtr &a, PSObjectPtr &incr);
	_INLINE bool PLOCAL_INC(PSInteger op,PSObjectPtr &target, PSObjectPtr &a, PSObjectPtr &incr);
	_INLINE bool DerefInc(PSInteger op,PSObjectPtr &target, PSObjectPtr &self, PSObjectPtr &key, PSObjectPtr &incr, bool postfix,PSInteger arg0);
#ifdef _DEBUG_DUMP
	void dumpstack(PSInteger stackbase=-1, bool dumpall = false);
#endif

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(PSCollectable **chain);
	PSObjectType GetType() {return OT_THREAD;}
#endif
	void Finalize();
	void GrowCallStack() {
		PSInteger newsize = _alloccallsstacksize*2;
		_callstackdata.resize(newsize);
		_callsstack = &_callstackdata[0];
		_alloccallsstacksize = newsize;
	}
	bool EnterFrame(PSInteger newbase, PSInteger newtop, bool tailcall);
	void LeaveFrame();
	void Release(){ ps_delete(this,PSVM); }
////////////////////////////////////////////////////////////////////////////
	//stack functions for the api
	void Remove(PSInteger n);

	static bool IsFalse(PSObjectPtr &o);
	
	void Pop();
	void Pop(PSInteger n);
	void Push(const PSObjectPtr &o);
	void PushNull();
	PSObjectPtr &Top();
	PSObjectPtr &PopGet();
	PSObjectPtr &GetUp(PSInteger n);
	PSObjectPtr &GetAt(PSInteger n);

	PSObjectPtrVec _stack;

	PSInteger _top;
	PSInteger _stackbase;
	PSOuter	*_openouters;
	PSObjectPtr _roottable;
	PSObjectPtr _lasterror;
	PSObjectPtr _errorhandler;

	bool _debughook;
	PSDEBUGHOOK _debughook_native;
	PSObjectPtr _debughook_closure;

	PSObjectPtr temp_reg;
	

	CallInfo* _callsstack;
	PSInteger _callsstacksize;
	PSInteger _alloccallsstacksize;
	psvector<CallInfo>  _callstackdata;

	ExceptionsTraps _etraps;
	CallInfo *ci;
	void *_foreignptr;
	//VMs sharing the same state
	PSSharedState *_sharedstate;
	PSInteger _nnativecalls;
	PSInteger _nmetamethodscall;
	//suspend infos
	PSBool _suspended;
	PSBool _suspended_root;
	PSInteger _suspended_target;
	PSInteger _suspended_traps;
};

struct AutoDec{
	AutoDec(PSInteger *n) { _n = n; }
	~AutoDec() { (*_n)--; }
	PSInteger *_n;
};

inline PSObjectPtr &stack_get(HPSCRIPTVM v,PSInteger idx){return ((idx>=0)?(v->GetAt(idx+v->_stackbase-1)):(v->GetUp(idx)));}

#define _ss(_vm_) (_vm_)->_sharedstate

#ifndef NO_GARBAGE_COLLECTOR
#define _opt_ss(_vm_) (_vm_)->_sharedstate
#else
#define _opt_ss(_vm_) NULL
#endif

#define PUSH_CALLINFO(v,nci){ \
	PSInteger css = v->_callsstacksize; \
	if(css == v->_alloccallsstacksize) { \
		v->GrowCallStack(); \
	} \
	v->ci = &v->_callsstack[css]; \
	*(v->ci) = nci; \
	v->_callsstacksize++; \
}

#define POP_CALLINFO(v){ \
	PSInteger css = --v->_callsstacksize; \
	v->ci->_closure.Null(); \
	v->ci = css?&v->_callsstack[css-1]:NULL;	\
}
#endif //_PSVM_H_
