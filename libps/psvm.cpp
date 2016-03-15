/*
	see copyright notice in pscript.h
*/
#include "pspcheader.h"
#include <math.h>
#include <stdlib.h>
#include "psopcodes.h"
#include "psvm.h"
#include "psfuncproto.h"
#include "psclosure.h"
#include "psstring.h"
#include "pstable.h"
#include "psuserdata.h"
#include "psarray.h"
#include "psclass.h"

#define TOP() (_stack._vals[_top-1])

bool PSVM::BW_OP(PSUnsignedInteger op,PSObjectPtr &trg,const PSObjectPtr &o1,const PSObjectPtr &o2)
{
	PSInteger res;
	if((type(o1)|type(o2)) == OT_INTEGER)
	{
		PSInteger i1 = _integer(o1), i2 = _integer(o2);
		switch(op) {
			case BW_AND:	res = i1 & i2; break;
			case BW_OR:		res = i1 | i2; break;
			case BW_XOR:	res = i1 ^ i2; break;
			case BW_SHIFTL:	res = i1 << i2; break;
			case BW_SHIFTR:	res = i1 >> i2; break;
			case BW_USHIFTR:res = (PSInteger)(*((PSUnsignedInteger*)&i1) >> i2); break;
			default: { Raise_Error(_SC("internal vm error bitwise op failed")); return false; }
		}
	} 
	else { Raise_Error(_SC("bitwise op between '%s' and '%s'"),GetTypeName(o1),GetTypeName(o2)); return false;}
	trg = res;
	return true;
}

#define _ARITH_(op,trg,o1,o2) \
{ \
	PSInteger tmask = type(o1)|type(o2); \
	switch(tmask) { \
		case OT_INTEGER: trg = _integer(o1) op _integer(o2);break; \
		case (OT_FLOAT|OT_INTEGER): \
		case (OT_FLOAT): trg = tofloat(o1) op tofloat(o2); break;\
		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
	} \
}

#define _ARITH_NOZERO(op,trg,o1,o2,err) \
{ \
	PSInteger tmask = type(o1)|type(o2); \
	switch(tmask) { \
		case OT_INTEGER: { PSInteger i2 = _integer(o2); if(i2 == 0) { Raise_Error(err); PS_THROW(); } trg = _integer(o1) op i2; } break;\
		case (OT_FLOAT|OT_INTEGER): \
		case (OT_FLOAT): trg = tofloat(o1) op tofloat(o2); break;\
		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
	} \
}

bool PSVM::ARITH_OP(PSUnsignedInteger op,PSObjectPtr &trg,const PSObjectPtr &o1,const PSObjectPtr &o2)
{
	PSInteger tmask = type(o1)|type(o2);
	switch(tmask) {
		case OT_INTEGER:{
			PSInteger res, i1 = _integer(o1), i2 = _integer(o2);
			switch(op) {
			case '+': res = i1 + i2; break;
			case '-': res = i1 - i2; break;
			case '/': if(i2 == 0) { Raise_Error(_SC("division by zero")); return false; }
					res = i1 / i2; 
					break;
			case '*': res = i1 * i2; break;
			case '%': if(i2 == 0) { Raise_Error(_SC("modulo by zero")); return false; }
					res = i1 % i2; 
					break;
			default: res = 0xDEADBEEF;
			}
			trg = res; }
			break;
		case (OT_FLOAT|OT_INTEGER):
		case (OT_FLOAT):{
			PSFloat res, f1 = tofloat(o1), f2 = tofloat(o2);
			switch(op) {
			case '+': res = f1 + f2; break;
			case '-': res = f1 - f2; break;
			case '/': res = f1 / f2; break;
			case '*': res = f1 * f2; break;
			case '%': res = PSFloat(fmod((double)f1,(double)f2)); break;
			default: res = 0x0f;
			}
			trg = res; }
			break;
		default:
			if(op == '+' &&	(tmask & _RT_STRING)){
				if(!StringCat(o1, o2, trg)) return false;
			}
			else if(!ArithMetaMethod(op,o1,o2,trg)) { 
				return false; 
			}
	}
	return true;
}

PSVM::PSVM(PSSharedState *ss)
{
	_sharedstate=ss;
	_suspended = PSFalse;
	_suspended_target = -1;
	_suspended_root = PSFalse;
	_suspended_traps = -1;
	_foreignptr = NULL;
	_nnativecalls = 0;
	_nmetamethodscall = 0;
	_lasterror.Null();
	_errorhandler.Null();
	_debughook = false;
	_debughook_native = NULL;
	_debughook_closure.Null();
	_openouters = NULL;
	ci = NULL;
	INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);
}

void PSVM::Finalize()
{
	if(_openouters) CloseOuters(&_stack._vals[0]);
	_roottable.Null();
	_lasterror.Null();
	_errorhandler.Null();
	_debughook = false;
	_debughook_native = NULL;
	_debughook_closure.Null();
	temp_reg.Null();
	_callstackdata.resize(0);
	PSInteger size=_stack.size();
	for(PSInteger i=0;i<size;i++)
		_stack[i].Null();
}

PSVM::~PSVM()
{
	Finalize();
	REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

bool PSVM::ArithMetaMethod(PSInteger op,const PSObjectPtr &o1,const PSObjectPtr &o2,PSObjectPtr &dest)
{
	PSMetaMethod mm;
	switch(op){
		case _SC('+'): mm=MT_ADD; break;
		case _SC('-'): mm=MT_SUB; break;
		case _SC('/'): mm=MT_DIV; break;
		case _SC('*'): mm=MT_MUL; break;
		case _SC('%'): mm=MT_MODULO; break;
		default: mm = MT_ADD; assert(0); break; //shutup compiler
	}
	if(is_delegable(o1) && _delegable(o1)->_delegate) {
		
		PSObjectPtr closure;
		if(_delegable(o1)->GetMetaMethod(this, mm, closure)) {
			Push(o1);Push(o2);
			return CallMetaMethod(closure,mm,2,dest);
		}
	}
	Raise_Error(_SC("arith op %c on between '%s' and '%s'"),op,GetTypeName(o1),GetTypeName(o2)); 
	return false;
}

bool PSVM::NEG_OP(PSObjectPtr &trg,const PSObjectPtr &o)
{
	
	switch(type(o)) {
	case OT_INTEGER:
		trg = -_integer(o);
		return true;
	case OT_FLOAT:
		trg = -_float(o);
		return true;
	case OT_TABLE:
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o)->_delegate) {
			PSObjectPtr closure;
			if(_delegable(o)->GetMetaMethod(this, MT_UNM, closure)) {
				Push(o);
				if(!CallMetaMethod(closure, MT_UNM, 1, temp_reg)) return false;
				_Swap(trg,temp_reg);
				return true;

			}
		}
	default:break; //shutup compiler
	}
	Raise_Error(_SC("attempt to negate a %s"), GetTypeName(o));
	return false;
}

#define _RET_SUCCEED(exp) { result = (exp); return true; } 
bool PSVM::ObjCmp(const PSObjectPtr &o1,const PSObjectPtr &o2,PSInteger &result)
{
	PSObjectType t1 = type(o1), t2 = type(o2);
	if(t1 == t2) {
		if(_rawval(o1) == _rawval(o2))_RET_SUCCEED(0);
		PSObjectPtr res;
		switch(t1){
		case OT_STRING:
			_RET_SUCCEED(scstrcmp(_stringval(o1),_stringval(o2)));
		case OT_INTEGER:
			_RET_SUCCEED((_integer(o1)<_integer(o2))?-1:1);
		case OT_FLOAT:
			_RET_SUCCEED((_float(o1)<_float(o2))?-1:1);
		case OT_TABLE:
		case OT_USERDATA:
		case OT_INSTANCE:
			if(_delegable(o1)->_delegate) {
				PSObjectPtr closure;
				if(_delegable(o1)->GetMetaMethod(this, MT_CMP, closure)) {
					Push(o1);Push(o2);
					if(CallMetaMethod(closure,MT_CMP,2,res)) {
						if(type(res) != OT_INTEGER) {
							Raise_Error(_SC("_cmp must return an integer"));
							return false;
						}
						_RET_SUCCEED(_integer(res))
					}
					return false;
				}
			}
			//continues through (no break needed)
		default: 
			_RET_SUCCEED( _userpointer(o1) < _userpointer(o2)?-1:1 );
		}
		assert(0);
		//if(type(res)!=OT_INTEGER) { Raise_CompareError(o1,o2); return false; }
		//	_RET_SUCCEED(_integer(res));
		
	}
	else{
		if(ps_isnumeric(o1) && ps_isnumeric(o2)){
			if((t1==OT_INTEGER) && (t2==OT_FLOAT)) { 
				if( _integer(o1)==_float(o2) ) { _RET_SUCCEED(0); }
				else if( _integer(o1)<_float(o2) ) { _RET_SUCCEED(-1); }
				_RET_SUCCEED(1);
			}
			else{
				if( _float(o1)==_integer(o2) ) { _RET_SUCCEED(0); }
				else if( _float(o1)<_integer(o2) ) { _RET_SUCCEED(-1); }
				_RET_SUCCEED(1);
			}
		}
		else if(t1==OT_NULL) {_RET_SUCCEED(-1);}
		else if(t2==OT_NULL) {_RET_SUCCEED(1);}
		else { Raise_CompareError(o1,o2); return false; }
		
	}
	assert(0);
	_RET_SUCCEED(0); //cannot happen
}

bool PSVM::CMP_OP(CmpOP op, const PSObjectPtr &o1,const PSObjectPtr &o2,PSObjectPtr &res)
{
	PSInteger r;
	if(ObjCmp(o1,o2,r)) {
		switch(op) {
			case CMP_G: res = (r > 0); return true;
			case CMP_GE: res = (r >= 0); return true;
			case CMP_L: res = (r < 0); return true;
			case CMP_LE: res = (r <= 0); return true;
			case CMP_3W: res = r; return true;
		}
		assert(0);
	}
	return false;
}

bool PSVM::ToString(const PSObjectPtr &o,PSObjectPtr &res)
{
	switch(type(o)) {
	case OT_STRING:
		res = o;
		return true;
	case OT_FLOAT:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)),_SC("%g"),_float(o));
		break;
	case OT_INTEGER:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)),_PRINT_INT_FMT,_integer(o));
		break;
	case OT_BOOL:
		scsprintf(_sp(rsl(6)),_integer(o)?_SC("true"):_SC("false"));
		break;
	case OT_TABLE:
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o)->_delegate) {
			PSObjectPtr closure;
			if(_delegable(o)->GetMetaMethod(this, MT_TOSTRING, closure)) {
				Push(o);
				if(CallMetaMethod(closure,MT_TOSTRING,1,res)) {;
					if(type(res) == OT_STRING)
						return true;
				} 
				else {
					return false;
				}
			}
		}
	default:
		scsprintf(_sp(rsl(sizeof(void*)+20)),_SC("(%s : 0x%p)"),GetTypeName(o),(void*)_rawval(o));
	}
	res = PSString::Create(_ss(this),_spval);
	return true;
}


bool PSVM::StringCat(const PSObjectPtr &str,const PSObjectPtr &obj,PSObjectPtr &dest)
{
	PSObjectPtr a, b;
	if(!ToString(str, a)) return false;
	if(!ToString(obj, b)) return false;
	PSInteger l = _string(a)->_len , ol = _string(b)->_len;
	PSChar *s = _sp(rsl(l + ol + 1));
	memcpy(s, _stringval(a), rsl(l)); 
	memcpy(s + l, _stringval(b), rsl(ol));
	dest = PSString::Create(_ss(this), _spval, l + ol);
	return true;
}

bool PSVM::TypeOf(const PSObjectPtr &obj1,PSObjectPtr &dest)
{
	if(is_delegable(obj1) && _delegable(obj1)->_delegate) {
		PSObjectPtr closure;
		if(_delegable(obj1)->GetMetaMethod(this, MT_TYPEOF, closure)) {
			Push(obj1);
			return CallMetaMethod(closure,MT_TYPEOF,1,dest);
		}
	}
	dest = PSString::Create(_ss(this),GetTypeName(obj1));
	return true;
}

bool PSVM::Init(PSVM *friendvm, PSInteger stacksize)
{
	_stack.resize(stacksize);
	_alloccallsstacksize = 4;
	_callstackdata.resize(_alloccallsstacksize);
	_callsstacksize = 0;
	_callsstack = &_callstackdata[0];
	_stackbase = 0;
	_top = 0;
	if(!friendvm) {
		_roottable = PSTable::Create(_ss(this), 0);
		ps_base_register(this);
	}
	else {
		_roottable = friendvm->_roottable;
		_errorhandler = friendvm->_errorhandler;
		_debughook = friendvm->_debughook;
		_debughook_native = friendvm->_debughook_native;
		_debughook_closure = friendvm->_debughook_closure;
	}
	return true;
}


bool PSVM::StartCall(PSClosure *closure,PSInteger target,PSInteger args,PSInteger stackbase,bool tailcall)
{
	PSFunctionProto *func = closure->_function;

	PSInteger paramssize = func->_nparameters;
	const PSInteger newtop = stackbase + func->_stacksize;
	PSInteger nargs = args;
	if(func->_varparams)
	{
		paramssize--;
		if (nargs < paramssize) {
			Raise_Error(_SC("wrong number of parameters"));
			return false;
		}

		//dumpstack(stackbase);
		PSInteger nvargs = nargs - paramssize;
		PSArray *arr = PSArray::Create(_ss(this),nvargs);
		PSInteger pbase = stackbase+paramssize;
		for(PSInteger n = 0; n < nvargs; n++) {
			arr->_values[n] = _stack._vals[pbase];
			_stack._vals[pbase].Null();
			pbase++;

		}
		_stack._vals[stackbase+paramssize] = arr;
		//dumpstack(stackbase);
	}
	else if (paramssize != nargs) {
		PSInteger ndef = func->_ndefaultparams;
		PSInteger diff;
		if(ndef && nargs < paramssize && (diff = paramssize - nargs) <= ndef) {
			for(PSInteger n = ndef - diff; n < ndef; n++) {
				_stack._vals[stackbase + (nargs++)] = closure->_defaultparams[n];
			}
		}
		else {
			Raise_Error(_SC("wrong number of parameters"));
			return false;
		}
	}

	if(closure->_env) {
		_stack._vals[stackbase] = closure->_env->_obj;
	}

	if(!EnterFrame(stackbase, newtop, tailcall)) return false;

	ci->_closure  = closure;
	ci->_literals = func->_literals;
	ci->_ip       = func->_instructions;
	ci->_target   = (PSInt32)target;

	if (_debughook) {
		CallDebugHook(_SC('c'));
	}

	if (closure->_function->_bgenerator) {
		PSFunctionProto *f = closure->_function;
		PSGenerator *gen = PSGenerator::Create(_ss(this), closure);
		if(!gen->Yield(this,f->_stacksize))
			return false;
		PSObjectPtr temp;
		Return(1, target, temp);
		STK(target) = gen;
	}


	return true;
}

bool PSVM::Return(PSInteger _arg0, PSInteger _arg1, PSObjectPtr &retval)
{
	PSBool    _isroot      = ci->_root;
	PSInteger callerbase   = _stackbase - ci->_prevstkbase;

	if (_debughook) {
		for(PSInteger i=0; i<ci->_ncalls; i++) {
			CallDebugHook(_SC('r'));
		}
	}

	PSObjectPtr *dest;
	if (_isroot) {
		dest = &(retval);
	} else if (ci->_target == -1) {
		dest = NULL;
	} else {
		dest = &_stack._vals[callerbase + ci->_target];
	}
	if (dest) {
		if(_arg0 != 0xFF) {
			*dest = _stack._vals[_stackbase+_arg1];
		}
		else {
			dest->Null();
		}
		//*dest = (_arg0 != 0xFF) ? _stack._vals[_stackbase+_arg1] : _null_;
	}
	LeaveFrame();
	return _isroot ? true : false;
}

#define _RET_ON_FAIL(exp) { if(!exp) return false; }

bool PSVM::PLOCAL_INC(PSInteger op,PSObjectPtr &target, PSObjectPtr &a, PSObjectPtr &incr)
{
 	PSObjectPtr trg;
	_RET_ON_FAIL(ARITH_OP( op , trg, a, incr));
	target = a;
	a = trg;
	return true;
}

bool PSVM::DerefInc(PSInteger op,PSObjectPtr &target, PSObjectPtr &self, PSObjectPtr &key, PSObjectPtr &incr, bool postfix,PSInteger selfidx)
{
	PSObjectPtr tmp, tself = self, tkey = key;
	if (!Get(tself, tkey, tmp, false, selfidx)) { return false; }
	_RET_ON_FAIL(ARITH_OP( op , target, tmp, incr))
	if (!Set(tself, tkey, target,selfidx)) { return false; }
	if (postfix) target = tmp;
	return true;
}

#define arg0 (_i_._arg0)
#define sarg0 ((PSInteger)*((signed char *)&_i_._arg0))
#define arg1 (_i_._arg1)
#define sarg1 (*((PSInt32 *)&_i_._arg1))
#define arg2 (_i_._arg2)
#define arg3 (_i_._arg3)
#define sarg3 ((PSInteger)*((signed char *)&_i_._arg3))

PSRESULT PSVM::Suspend()
{
	if (_suspended)
		return ps_throwerror(this, _SC("cannot suspend an already suspended vm"));
	if (_nnativecalls!=2)
		return ps_throwerror(this, _SC("cannot suspend through native calls/metamethods"));
	return PS_SUSPEND_FLAG;
}


#define _FINISH(howmuchtojump) {jump = howmuchtojump; return true; }
bool PSVM::FOREACH_OP(PSObjectPtr &o1,PSObjectPtr &o2,PSObjectPtr 
&o3,PSObjectPtr &o4,PSInteger arg_2,int exitpos,int &jump)
{
	PSInteger nrefidx;
	switch(type(o1)) {
	case OT_TABLE:
		if((nrefidx = _table(o1)->Next(false,o4, o2, o3)) == -1) _FINISH(exitpos);
		o4 = (PSInteger)nrefidx; _FINISH(1);
	case OT_ARRAY:
		if((nrefidx = _array(o1)->Next(o4, o2, o3)) == -1) _FINISH(exitpos);
		o4 = (PSInteger) nrefidx; _FINISH(1);
	case OT_STRING:
		if((nrefidx = _string(o1)->Next(o4, o2, o3)) == -1)_FINISH(exitpos);
		o4 = (PSInteger)nrefidx; _FINISH(1);
	case OT_CLASS:
		if((nrefidx = _class(o1)->Next(o4, o2, o3)) == -1)_FINISH(exitpos);
		o4 = (PSInteger)nrefidx; _FINISH(1);
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o1)->_delegate) {
			PSObjectPtr itr;
			PSObjectPtr closure;
			if(_delegable(o1)->GetMetaMethod(this, MT_NEXTI, closure)) {
				Push(o1);
				Push(o4);
				if(CallMetaMethod(closure, MT_NEXTI, 2, itr)) {
					o4 = o2 = itr;
					if(type(itr) == OT_NULL) _FINISH(exitpos);
					if(!Get(o1, itr, o3, false, DONT_FALL_BACK)) {
						Raise_Error(_SC("_nexti returned an invalid idx")); // cloud be changed
						return false;
					}
					_FINISH(1);
				}
				else {
					return false;
				}
			}
			Raise_Error(_SC("_nexti failed"));
			return false;
		}
		break;
	case OT_GENERATOR:
		if(_generator(o1)->_state == PSGenerator::eDead) _FINISH(exitpos);
		if(_generator(o1)->_state == PSGenerator::eSuspended) {
			PSInteger idx = 0;
			if(type(o4) == OT_INTEGER) {
				idx = _integer(o4) + 1;
			}
			o2 = idx;
			o4 = idx;
			_generator(o1)->Resume(this, o3);
			_FINISH(0);
		}
	default: 
		Raise_Error(_SC("cannot iterate %s"), GetTypeName(o1));
	}
	return false; //cannot be hit(just to avoid warnings)
}

#define COND_LITERAL (arg3!=0?ci->_literals[arg1]:STK(arg1))

#define PS_THROW() { goto exception_trap; }

#define _GUARD(exp) { if(!exp) { PS_THROW();} }

bool PSVM::CLOSURE_OP(PSObjectPtr &target, PSFunctionProto *func)
{
	PSInteger nouters;
	PSClosure *closure = PSClosure::Create(_ss(this), func);
	if((nouters = func->_noutervalues)) {
		for(PSInteger i = 0; i<nouters; i++) {
			PSOuterVar &v = func->_outervalues[i];
			switch(v._type){
			case otLOCAL:
				FindOuter(closure->_outervalues[i], &STK(_integer(v._src)));
				break;
			case otOUTER:
				closure->_outervalues[i] = _closure(ci->_closure)->_outervalues[_integer(v._src)];
				break;
			}
		}
	}
	PSInteger ndefparams;
	if((ndefparams = func->_ndefaultparams)) {
		for(PSInteger i = 0; i < ndefparams; i++) {
			PSInteger spos = func->_defaultparams[i];
			closure->_defaultparams[i] = _stack._vals[_stackbase + spos];
		}
	}
	target = closure;
	return true;

}


bool PSVM::CLASS_OP(PSObjectPtr &target,PSInteger baseclass,PSInteger attributes)
{
	PSClass *base = NULL;
	PSObjectPtr attrs;
	if(baseclass != -1) {
		if(type(_stack._vals[_stackbase+baseclass]) != OT_CLASS) { Raise_Error(_SC("trying to inherit from a %s"),GetTypeName(_stack._vals[_stackbase+baseclass])); return false; }
		base = _class(_stack._vals[_stackbase + baseclass]);
	}
	if(attributes != MAX_FUNC_STACKSIZE) {
		attrs = _stack._vals[_stackbase+attributes];
	}
	target = PSClass::Create(_ss(this),base);
	if(type(_class(target)->_metamethods[MT_INHERITED]) != OT_NULL) {
		int nparams = 2;
		PSObjectPtr ret;
		Push(target); Push(attrs);
		if(!Call(_class(target)->_metamethods[MT_INHERITED],nparams,_top - nparams, ret, false)) {
			Pop(nparams);
			return false;
		}
		Pop(nparams);
	}
	_class(target)->_attributes = attrs;
	return true;
}

bool PSVM::IsEqual(const PSObjectPtr &o1,const PSObjectPtr &o2,bool &res)
{
	if(type(o1) == type(o2)) {
		res = (_rawval(o1) == _rawval(o2));
	}
	else {
		if(ps_isnumeric(o1) && ps_isnumeric(o2)) {
			res = (tofloat(o1) == tofloat(o2));
		}
		else {
			res = false;
		}
	}
	return true;
}

bool PSVM::IsFalse(PSObjectPtr &o)
{
	if(((type(o) & PSOBJECT_CANBEFALSE) 
		&& ( ((type(o) == OT_FLOAT) && (_float(o) == PSFloat(0.0))) ))
#if !defined(PSUSEDOUBLE) || (defined(PSUSEDOUBLE) && defined(_PS64))
		|| (_integer(o) == 0) )  //OT_NULL|OT_INTEGER|OT_BOOL
#else
		|| (((type(o) != OT_FLOAT) && (_integer(o) == 0))) )  //OT_NULL|OT_INTEGER|OT_BOOL
#endif
	{
		return true;
	}
	return false;
}

bool PSVM::Execute(PSObjectPtr &closure, PSInteger nargs, PSInteger stackbase,PSObjectPtr &outres, PSBool raiseerror,ExecutionType et)
{
	if ((_nnativecalls + 1) > MAX_NATIVE_CALLS) { Raise_Error(_SC("Native stack overflow")); return false; }
	_nnativecalls++;
	AutoDec ad(&_nnativecalls);
	PSInteger traps = 0;
	CallInfo *prevci = ci;
		
	switch(et) {
		case ET_CALL: {
			temp_reg = closure;
			if(!StartCall(_closure(temp_reg), _top - nargs, nargs, stackbase, false)) { 
				//call the handler if there are no calls in the stack, if not relies on the previous node
				if(ci == NULL) CallErrorHandler(_lasterror);
				return false;
			}
			if(ci == prevci) {
				outres = STK(_top-nargs);
				return true;
			}
			ci->_root = PSTrue;
					  }
			break;
		case ET_RESUME_GENERATOR: _generator(closure)->Resume(this, outres); ci->_root = PSTrue; traps += ci->_etraps; break;
		case ET_RESUME_VM:
		case ET_RESUME_THROW_VM:
			traps = _suspended_traps;
			ci->_root = _suspended_root;
			_suspended = PSFalse;
			if(et  == ET_RESUME_THROW_VM) { PS_THROW(); }
			break;
	}
	
exception_restore:
	//
	{
		for(;;)
		{
			const PSInstruction &_i_ = *ci->_ip++;
			//dumpstack(_stackbase);
			//scprintf("\n[%d] %s %d %d %d %d\n",ci->_ip-ci->_iv->_vals,g_InstrDesc[_i_.op].name,arg0,arg1,arg2,arg3);
			switch(_i_.op)
			{
			case _OP_LINE: if (_debughook) CallDebugHook(_SC('l'),arg1); continue;
			case _OP_LOAD: TARGET = ci->_literals[arg1]; continue;
			case _OP_LOADINT: 
#ifndef _PS64
				TARGET = (PSInteger)arg1; continue;
#else
				TARGET = (PSInteger)((PSUnsignedInteger32)arg1); continue;
#endif
			case _OP_LOADFLOAT: TARGET = *((PSFloat *)&arg1); continue;
			case _OP_DLOAD: TARGET = ci->_literals[arg1]; STK(arg2) = ci->_literals[arg3];continue;
			case _OP_TAILCALL:{
				PSObjectPtr &t = STK(arg1);
				if (type(t) == OT_CLOSURE 
					&& (!_closure(t)->_function->_bgenerator)){
					PSObjectPtr clo = t;
					if(_openouters) CloseOuters(&(_stack._vals[_stackbase]));
					for (PSInteger i = 0; i < arg3; i++) STK(i) = STK(arg2 + i);
					_GUARD(StartCall(_closure(clo), ci->_target, arg3, _stackbase, true));
					continue;
				}
							  }
			case _OP_CALL: {
					PSObjectPtr clo = STK(arg1);
					switch (type(clo)) {
					case OT_CLOSURE:
						_GUARD(StartCall(_closure(clo), sarg0, arg3, _stackbase+arg2, false));
						continue;
					case OT_NATIVECLOSURE: {
						bool suspend;
						_GUARD(CallNative(_nativeclosure(clo), arg3, _stackbase+arg2, clo,suspend));
						if(suspend){
							_suspended = PSTrue;
							_suspended_target = sarg0;
							_suspended_root = ci->_root;
							_suspended_traps = traps;
							outres = clo;
							return true;
						}
						if(sarg0 != -1) {
							STK(arg0) = clo;
						}
										   }
						continue;
					case OT_CLASS:{
						PSObjectPtr inst;
						_GUARD(CreateClassInstance(_class(clo),inst,clo));
						if(sarg0 != -1) {
							STK(arg0) = inst;
						}
						PSInteger stkbase;
						switch(type(clo)) {
							case OT_CLOSURE:
								stkbase = _stackbase+arg2;
								_stack._vals[stkbase] = inst;
								_GUARD(StartCall(_closure(clo), -1, arg3, stkbase, false));
								break;
							case OT_NATIVECLOSURE:
								bool suspend;
								stkbase = _stackbase+arg2;
								_stack._vals[stkbase] = inst;
								_GUARD(CallNative(_nativeclosure(clo), arg3, stkbase, clo,suspend));
								break;
							default: break; //shutup GCC 4.x
						}
						}
						break;
					case OT_TABLE:
					case OT_USERDATA:
					case OT_INSTANCE:{
						PSObjectPtr closure;
						if(_delegable(clo)->_delegate && _delegable(clo)->GetMetaMethod(this,MT_CALL,closure)) {
							Push(clo);
							for (PSInteger i = 0; i < arg3; i++) Push(STK(arg2 + i));
							if(!CallMetaMethod(closure, MT_CALL, arg3+1, clo)) PS_THROW();
							if(sarg0 != -1) {
								STK(arg0) = clo;
							}
							break;
						}
									 
						//Raise_Error(_SC("attempt to call '%s'"), GetTypeName(clo));
						//PS_THROW();
					  }
					default:
						Raise_Error(_SC("attempt to call '%s'"), GetTypeName(clo));
						PS_THROW();
					}
				}
				  continue;
			case _OP_PREPCALL:
			case _OP_PREPCALLK:	{
					PSObjectPtr &key = _i_.op == _OP_PREPCALLK?(ci->_literals)[arg1]:STK(arg1);
					PSObjectPtr &o = STK(arg2);
					if (!Get(o, key, temp_reg,false,arg2)) {
						PS_THROW();
					}
					STK(arg3) = o;
					_Swap(TARGET,temp_reg);//TARGET = temp_reg;
				}
				continue;
			case _OP_GETK:
				if (!Get(STK(arg2), ci->_literals[arg1], temp_reg, false,arg2)) { PS_THROW();}
				_Swap(TARGET,temp_reg);//TARGET = temp_reg;
				continue;
			case _OP_MOVE: TARGET = STK(arg1); continue;
			case _OP_NEWSLOT:
				_GUARD(NewSlot(STK(arg1), STK(arg2), STK(arg3),false));
				if(arg0 != 0xFF) TARGET = STK(arg3);
				continue;
			case _OP_DELETE: _GUARD(DeleteSlot(STK(arg1), STK(arg2), TARGET)); continue;
			case _OP_SET:
				if (!Set(STK(arg1), STK(arg2), STK(arg3),arg1)) { PS_THROW(); }
				if (arg0 != 0xFF) TARGET = STK(arg3);
				continue;
			case _OP_GET:
				if (!Get(STK(arg1), STK(arg2), temp_reg, false,arg1)) { PS_THROW(); }
				_Swap(TARGET,temp_reg);//TARGET = temp_reg;
				continue;
			case _OP_EQ:{
				bool res;
				if(!IsEqual(STK(arg2),COND_LITERAL,res)) { PS_THROW(); }
				TARGET = res?true:false;
				}continue;
			case _OP_NE:{ 
				bool res;
				if(!IsEqual(STK(arg2),COND_LITERAL,res)) { PS_THROW(); }
				TARGET = (!res)?true:false;
				} continue;
			case _OP_ADD: _ARITH_(+,TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_SUB: _ARITH_(-,TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_MUL: _ARITH_(*,TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_DIV: _ARITH_NOZERO(/,TARGET,STK(arg2),STK(arg1),_SC("division by zero")); continue;
			case _OP_MOD: ARITH_OP('%',TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_BITW:	_GUARD(BW_OP( arg3,TARGET,STK(arg2),STK(arg1))); continue;
			case _OP_RETURN:
				if((ci)->_generator) {
					(ci)->_generator->Kill();
				}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps==0);
					//outres = temp_reg;
					_Swap(outres,temp_reg);
					return true;
				}
				continue;
			case _OP_LOADNULLS:{ for(PSInt32 n=0; n < arg1; n++) STK(arg0+n).Null(); }continue;
			case _OP_LOADROOT:	TARGET = _roottable; continue;
			case _OP_LOADBOOL: TARGET = arg1?true:false; continue;
			case _OP_DMOVE: STK(arg0) = STK(arg1); STK(arg2) = STK(arg3); continue;
			case _OP_JMP: ci->_ip += (sarg1); continue;
			//case _OP_JNZ: if(!IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
			case _OP_JCMP: 
				_GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg0),temp_reg));
				if(IsFalse(temp_reg)) ci->_ip+=(sarg1);
				continue;
			case _OP_JZ: if(IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
			case _OP_GETOUTER: {
				PSClosure *cur_cls = _closure(ci->_closure);
				PSOuter *otr = _outer(cur_cls->_outervalues[arg1]);
				TARGET = *(otr->_valptr);
				}
			continue;
			case _OP_SETOUTER: {
				PSClosure *cur_cls = _closure(ci->_closure);
				PSOuter   *otr = _outer(cur_cls->_outervalues[arg1]);
				*(otr->_valptr) = STK(arg2);
				if(arg0 != 0xFF) {
					TARGET = STK(arg2);
				}
				}
			continue;
			case _OP_NEWOBJ: 
				switch(arg3) {
					case NOT_TABLE: TARGET = PSTable::Create(_ss(this), arg1); continue;
					case NOT_ARRAY: TARGET = PSArray::Create(_ss(this), 0); _array(TARGET)->Reserve(arg1); continue;
					case NOT_CLASS: _GUARD(CLASS_OP(TARGET,arg1,arg2)); continue;
					default: assert(0); continue;
				}
			case _OP_APPENDARRAY: 
				{
					PSObject val;
					val._unVal.raw = 0;
				switch(arg2) {
				case AAT_STACK:
					val = STK(arg1); break;
				case AAT_LITERAL:
					val = ci->_literals[arg1]; break;
				case AAT_INT:
					val._type = OT_INTEGER;
#ifndef _PS64
					val._unVal.nInteger = (PSInteger)arg1; 
#else
					val._unVal.nInteger = (PSInteger)((PSUnsignedInteger32)arg1);
#endif
					break;
				case AAT_FLOAT:
					val._type = OT_FLOAT;
					val._unVal.fFloat = *((PSFloat *)&arg1);
					break;
				case AAT_BOOL:
					val._type = OT_BOOL;
					val._unVal.nInteger = arg1;
					break;
				default: assert(0); break;

				}
				_array(STK(arg0))->Append(val);	continue;
				}
			case _OP_COMPARITH: {
				PSInteger selfidx = (((PSUnsignedInteger)arg1&0xFFFF0000)>>16);
				_GUARD(DerefInc(arg3, TARGET, STK(selfidx), STK(arg2), STK(arg1&0x0000FFFF), false, selfidx)); 
								}
				continue;
			case _OP_INC: {PSObjectPtr o(sarg3); _GUARD(DerefInc('+',TARGET, STK(arg1), STK(arg2), o, false, arg1));} continue;
			case _OP_INCL: {
				PSObjectPtr &a = STK(arg1);
				if(type(a) == OT_INTEGER) {
					a._unVal.nInteger = _integer(a) + sarg3;
				}
				else {
					PSObjectPtr o(sarg3); //_GUARD(LOCAL_INC('+',TARGET, STK(arg1), o));
					_ARITH_(+,a,a,o);
				}
						   } continue;
			case _OP_PINC: {PSObjectPtr o(sarg3); _GUARD(DerefInc('+',TARGET, STK(arg1), STK(arg2), o, true, arg1));} continue;
			case _OP_PINCL:	{
				PSObjectPtr &a = STK(arg1);
				if(type(a) == OT_INTEGER) {
					TARGET = a;
					a._unVal.nInteger = _integer(a) + sarg3;
				}
				else {
					PSObjectPtr o(sarg3); _GUARD(PLOCAL_INC('+',TARGET, STK(arg1), o));
				}
				
						} continue;
			case _OP_CMP:	_GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg1),TARGET))	continue;
			case _OP_EXISTS: TARGET = Get(STK(arg1), STK(arg2), temp_reg, true,DONT_FALL_BACK)?true:false;continue;
			case _OP_INSTANCEOF: 
				if(type(STK(arg1)) != OT_CLASS)
				{Raise_Error(_SC("cannot apply instanceof between a %s and a %s"),GetTypeName(STK(arg1)),GetTypeName(STK(arg2))); PS_THROW();}
				TARGET = (type(STK(arg2)) == OT_INSTANCE) ? (_instance(STK(arg2))->InstanceOf(_class(STK(arg1)))?true:false) : false;
				continue;
			case _OP_AND: 
				if(IsFalse(STK(arg2))) {
					TARGET = STK(arg2);
					ci->_ip += (sarg1);
				}
				continue;
			case _OP_OR:
				if(!IsFalse(STK(arg2))) {
					TARGET = STK(arg2);
					ci->_ip += (sarg1);
				}
				continue;
			case _OP_NEG: _GUARD(NEG_OP(TARGET,STK(arg1))); continue;
			case _OP_NOT: TARGET = IsFalse(STK(arg1)); continue;
			case _OP_BWNOT:
				if(type(STK(arg1)) == OT_INTEGER) {
					PSInteger t = _integer(STK(arg1));
					TARGET = PSInteger(~t);
					continue;
				}
				Raise_Error(_SC("attempt to perform a bitwise op on a %s"), GetTypeName(STK(arg1)));
				PS_THROW();
			case _OP_CLOSURE: {
				PSClosure *c = ci->_closure._unVal.pClosure;
				PSFunctionProto *fp = c->_function;
				if(!CLOSURE_OP(TARGET,fp->_functions[arg1]._unVal.pFunctionProto)) { PS_THROW(); }
				continue;
			}
			case _OP_YIELD:{
				if(ci->_generator) {
					if(sarg1 != MAX_FUNC_STACKSIZE) temp_reg = STK(arg1);
					_GUARD(ci->_generator->Yield(this,arg2));
					traps -= ci->_etraps;
					if(sarg1 != MAX_FUNC_STACKSIZE) _Swap(STK(arg1),temp_reg);//STK(arg1) = temp_reg;
				}
				else { Raise_Error(_SC("trying to yield a '%s',only genenerator can be yielded"), GetTypeName(ci->_generator)); PS_THROW();}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps == 0);
					outres = temp_reg;
					return true;
				}
					
				}
				continue;
			case _OP_RESUME:
				if(type(STK(arg1)) != OT_GENERATOR){ Raise_Error(_SC("trying to resume a '%s',only genenerator can be resumed"), GetTypeName(STK(arg1))); PS_THROW();}
				_GUARD(_generator(STK(arg1))->Resume(this, TARGET));
				traps += ci->_etraps;
                continue;
			case _OP_FOREACH:{ int tojump;
				_GUARD(FOREACH_OP(STK(arg0),STK(arg2),STK(arg2+1),STK(arg2+2),arg2,sarg1,tojump));
				ci->_ip += tojump; }
				continue;
			case _OP_POSTFOREACH:
				assert(type(STK(arg0)) == OT_GENERATOR);
				if(_generator(STK(arg0))->_state == PSGenerator::eDead) 
					ci->_ip += (sarg1 - 1);
				continue;
			case _OP_CLONE: _GUARD(Clone(STK(arg1), TARGET)); continue;
			case _OP_TYPEOF: _GUARD(TypeOf(STK(arg1), TARGET)) continue;
			case _OP_PUSHTRAP:{
				PSInstruction *_iv = _closure(ci->_closure)->_function->_instructions;
				_etraps.push_back(PSExceptionTrap(_top,_stackbase, &_iv[(ci->_ip-_iv)+arg1], arg0)); traps++;
				ci->_etraps++;
							  }
				continue;
			case _OP_POPTRAP: {
				for(PSInteger i = 0; i < arg0; i++) {
					_etraps.pop_back(); traps--;
					ci->_etraps--;
				}
							  }
				continue;
			case _OP_THROW:	Raise_Error(TARGET); PS_THROW(); continue;
			case _OP_NEWSLOTA:
				_GUARD(NewSlotA(STK(arg1),STK(arg2),STK(arg3),(arg0&NEW_SLOT_ATTRIBUTES_FLAG) ? STK(arg2-1) : PSObjectPtr(),(arg0&NEW_SLOT_STATIC_FLAG)?true:false,false));
				continue;
			case _OP_GETBASE:{
				PSClosure *clo = _closure(ci->_closure);
				if(clo->_base) {
					TARGET = clo->_base;
				}
				else {
					TARGET.Null();
				}
				continue;
			}
			case _OP_CLOSE:
				if(_openouters) CloseOuters(&(STK(arg1)));
				continue;
			}
			
		}
	}
exception_trap:
	{
		PSObjectPtr currerror = _lasterror;
//		dumpstack(_stackbase);
//		PSInteger n = 0;
		PSInteger last_top = _top;
		
		if(_ss(this)->_notifyallexceptions || (!traps && raiseerror)) CallErrorHandler(currerror);

		while( ci ) {
			if(ci->_etraps > 0) {
				PSExceptionTrap &et = _etraps.top();
				ci->_ip = et._ip;
				_top = et._stacksize;
				_stackbase = et._stackbase;
				_stack._vals[_stackbase + et._extarget] = currerror;
				_etraps.pop_back(); traps--; ci->_etraps--;
				while(last_top >= _top) _stack._vals[last_top--].Null();
				goto exception_restore;
			}
			else if (_debughook) { 
					//notify debugger of a "return"
					//even if it really an exception unwinding the stack
					for(PSInteger i = 0; i < ci->_ncalls; i++) {
						CallDebugHook(_SC('r'));
					}
			}
			if(ci->_generator) ci->_generator->Kill();
			bool mustbreak = ci && ci->_root;
			LeaveFrame();
			if(mustbreak) break;
		}
						
		_lasterror = currerror;
		return false;
	}
	assert(0);
}

bool PSVM::CreateClassInstance(PSClass *theclass, PSObjectPtr &inst, PSObjectPtr &constructor)
{
	inst = theclass->CreateInstance();
	if(!theclass->GetConstructor(constructor)) {
		constructor.Null();
	}
	return true;
}

void PSVM::CallErrorHandler(PSObjectPtr &error)
{
	if(type(_errorhandler) != OT_NULL) {
		PSObjectPtr out;
		Push(_roottable); Push(error);
		Call(_errorhandler, 2, _top-2, out,PSFalse);
		Pop(2);
	}
}


void PSVM::CallDebugHook(PSInteger type,PSInteger forcedline)
{
	_debughook = false;
	PSFunctionProto *func=_closure(ci->_closure)->_function;
	if(_debughook_native) {
		const PSChar *src = type(func->_sourcename) == OT_STRING?_stringval(func->_sourcename):NULL;
		const PSChar *fname = type(func->_name) == OT_STRING?_stringval(func->_name):NULL;
		PSInteger line = forcedline?forcedline:func->GetLine(ci->_ip);
		_debughook_native(this,type,src,line,fname);
	}
	else {
		PSObjectPtr temp_reg;
		PSInteger nparams=5;
		Push(_roottable); Push(type); Push(func->_sourcename); Push(forcedline?forcedline:func->GetLine(ci->_ip)); Push(func->_name);
		Call(_debughook_closure,nparams,_top-nparams,temp_reg,PSFalse);
		Pop(nparams);
	}
	_debughook = true;
}

bool PSVM::CallNative(PSNativeClosure *nclosure, PSInteger nargs, PSInteger newbase, PSObjectPtr &retval, bool &suspend)
{
	PSInteger nparamscheck = nclosure->_nparamscheck;
	PSInteger newtop = newbase + nargs + nclosure->_noutervalues;
	
	if (_nnativecalls + 1 > MAX_NATIVE_CALLS) {
		Raise_Error(_SC("Native stack overflow"));
		return false;
	}

	if(nparamscheck && (((nparamscheck > 0) && (nparamscheck != nargs)) ||
		((nparamscheck < 0) && (nargs < (-nparamscheck)))))
	{
		Raise_Error(_SC("wrong number of parameters"));
		return false;
	}

	PSInteger tcs;
	PSIntVec &tc = nclosure->_typecheck;
	if((tcs = tc.size())) {
		for(PSInteger i = 0; i < nargs && i < tcs; i++) {
			if((tc._vals[i] != -1) && !(type(_stack._vals[newbase+i]) & tc._vals[i])) {
				Raise_ParamTypeError(i,tc._vals[i],type(_stack._vals[newbase+i]));
				return false;
			}
		}
	}

	if(!EnterFrame(newbase, newtop, false)) return false;
	ci->_closure  = nclosure;

	PSInteger outers = nclosure->_noutervalues;
	for (PSInteger i = 0; i < outers; i++) {
		_stack._vals[newbase+nargs+i] = nclosure->_outervalues[i];
	}
	if(nclosure->_env) {
		_stack._vals[newbase] = nclosure->_env->_obj;
	}

	_nnativecalls++;
	PSInteger ret = (nclosure->_function)(this);
	_nnativecalls--;

	suspend = false;
	if (ret == PS_SUSPEND_FLAG) {
		suspend = true;
	}
	else if (ret < 0) {
		LeaveFrame();
		Raise_Error(_lasterror);
		return false;
	}
	if(ret) {
		retval = _stack._vals[_top-1];
	}
	else {
		retval.Null();
	}
	//retval = ret ? _stack._vals[_top-1] : _null_;
	LeaveFrame();
	return true;
}

#define FALLBACK_OK			0
#define FALLBACK_NO_MATCH	1
#define FALLBACK_ERROR		2

bool PSVM::Get(const PSObjectPtr &self,const PSObjectPtr &key,PSObjectPtr &dest,bool raw, PSInteger selfidx)
{
	switch(type(self)){
	case OT_TABLE:
		if(_table(self)->Get(key,dest))return true;
		break;
	case OT_ARRAY:
		if(ps_isnumeric(key)) { if(_array(self)->Get(tointeger(key),dest)) { return true; } if(selfidx != EXISTS_FALL_BACK) Raise_IdxError(key); return false; }
		break;
	case OT_INSTANCE:
		if(_instance(self)->Get(key,dest)) return true;
		break;
	case OT_CLASS: 
		if(_class(self)->Get(key,dest)) return true;
		break;
	case OT_STRING:
		if(ps_isnumeric(key)){
			PSInteger n = tointeger(key);
			if(abs((int)n) < _string(self)->_len) {
				if(n < 0) n = _string(self)->_len - n;
				dest = PSInteger(_stringval(self)[n]);
				return true;
			}
			if(selfidx != EXISTS_FALL_BACK) Raise_IdxError(key);
			return false;
		}
		break;
	default:break; //shut up compiler
	}
	if(!raw) {
		switch(FallBackGet(self,key,dest)) {
			case FALLBACK_OK: return true; //okie
			case FALLBACK_NO_MATCH: break; //keep falling back
			case FALLBACK_ERROR: return false; // the metamethod failed
		}
		if(InvokeDefaultDelegate(self,key,dest)) {
			return true;
		}
	}
//#ifdef ROOT_FALLBACK
	if(selfidx == 0) {
		if(_table(_roottable)->Get(key,dest)) return true;
	}
//#endif
	if(selfidx != EXISTS_FALL_BACK) Raise_IdxError(key);
	return false;
}

bool PSVM::InvokeDefaultDelegate(const PSObjectPtr &self,const PSObjectPtr &key,PSObjectPtr &dest)
{
	PSTable *ddel = NULL;
	switch(type(self)) {
		case OT_CLASS: ddel = _class_ddel; break;
		case OT_TABLE: ddel = _table_ddel; break;
		case OT_ARRAY: ddel = _array_ddel; break;
		case OT_STRING: ddel = _string_ddel; break;
		case OT_INSTANCE: ddel = _instance_ddel; break;
		case OT_INTEGER:case OT_FLOAT:case OT_BOOL: ddel = _number_ddel; break;
		case OT_GENERATOR: ddel = _generator_ddel; break;
		case OT_CLOSURE: case OT_NATIVECLOSURE:	ddel = _closure_ddel; break;
		case OT_THREAD: ddel = _thread_ddel; break;
		case OT_WEAKREF: ddel = _weakref_ddel; break;
		default: return false;
	}
	return  ddel->Get(key,dest);
}


PSInteger PSVM::FallBackGet(const PSObjectPtr &self,const PSObjectPtr &key,PSObjectPtr &dest)
{
	switch(type(self)){
	case OT_TABLE:
	case OT_USERDATA:
        //delegation
		if(_delegable(self)->_delegate) {
			if(Get(PSObjectPtr(_delegable(self)->_delegate),key,dest,false,DONT_FALL_BACK)) return FALLBACK_OK;	
		}
		else {
			return FALLBACK_NO_MATCH;
		}
		//go through
	case OT_INSTANCE: {
		PSObjectPtr closure;
		if(_delegable(self)->GetMetaMethod(this, MT_GET, closure)) {
			Push(self);Push(key);
			_nmetamethodscall++;
			AutoDec ad(&_nmetamethodscall);
			if(Call(closure, 2, _top - 2, dest, PSFalse)) {
				Pop(2);
				return FALLBACK_OK;
			}
			else {
				Pop(2);
				if(type(_lasterror) != OT_NULL) { //NULL means "clean failure" (not found)
					return FALLBACK_ERROR;
				}
			}
		}
					  }
		break;
	default: break;//shutup GCC 4.x
	}
	// no metamethod or no fallback type
	return FALLBACK_NO_MATCH;
}

bool PSVM::Set(const PSObjectPtr &self,const PSObjectPtr &key,const PSObjectPtr &val,PSInteger selfidx)
{
	switch(type(self)){
	case OT_TABLE:
		if(_table(self)->Set(key,val)) return true;
		break;
	case OT_INSTANCE:
		if(_instance(self)->Set(key,val)) return true;
		break;
	case OT_ARRAY:
		if(!ps_isnumeric(key)) { Raise_Error(_SC("indexing %s with %s"),GetTypeName(self),GetTypeName(key)); return false; }
		if(!_array(self)->Set(tointeger(key),val)) {
			Raise_IdxError(key);
			return false;
		}
		return true;
	default:
		Raise_Error(_SC("trying to set '%s'"),GetTypeName(self));
		return false;
	}

	switch(FallBackSet(self,key,val)) {
		case FALLBACK_OK: return true; //okie
		case FALLBACK_NO_MATCH: break; //keep falling back
		case FALLBACK_ERROR: return false; // the metamethod failed
	}
	if(selfidx == 0) {
		if(_table(_roottable)->Set(key,val))
			return true;
	}
	Raise_IdxError(key);
	return false;
}

PSInteger PSVM::FallBackSet(const PSObjectPtr &self,const PSObjectPtr &key,const PSObjectPtr &val)
{
	switch(type(self)) {
	case OT_TABLE:
		if(_table(self)->_delegate) {
			if(Set(_table(self)->_delegate,key,val,DONT_FALL_BACK))	return FALLBACK_OK;
		}
		//keps on going
	case OT_INSTANCE:
	case OT_USERDATA:{
		PSObjectPtr closure;
		PSObjectPtr t;
		if(_delegable(self)->GetMetaMethod(this, MT_SET, closure)) {
			Push(self);Push(key);Push(val);
			_nmetamethodscall++;
			AutoDec ad(&_nmetamethodscall);
			if(Call(closure, 3, _top - 3, t, PSFalse)) {
				Pop(3);
				return FALLBACK_OK;
			}
			else {
				if(type(_lasterror) != OT_NULL) { //NULL means "clean failure" (not found)
					//error
					Pop(3);
					return FALLBACK_ERROR;
				}
			}
		}
					 }
		break;
		default: break;//shutup GCC 4.x
	}
	// no metamethod or no fallback type
	return FALLBACK_NO_MATCH;
}

bool PSVM::Clone(const PSObjectPtr &self,PSObjectPtr &target)
{
	PSObjectPtr temp_reg;
	PSObjectPtr newobj;
	switch(type(self)){
	case OT_TABLE:
		newobj = _table(self)->Clone();
		goto cloned_mt;
	case OT_INSTANCE: {
		newobj = _instance(self)->Clone(_ss(this));
cloned_mt:
		PSObjectPtr closure;
		if(_delegable(newobj)->_delegate && _delegable(newobj)->GetMetaMethod(this,MT_CLONED,closure)) {
			Push(newobj);
			Push(self);
			if(!CallMetaMethod(closure,MT_CLONED,2,temp_reg))
				return false;
		}
		}
		target = newobj;
		return true;
	case OT_ARRAY: 
		target = _array(self)->Clone();
		return true;
	default: 
		Raise_Error(_SC("cloning a %s"), GetTypeName(self));
		return false;
	}
}

bool PSVM::NewSlotA(const PSObjectPtr &self,const PSObjectPtr &key,const PSObjectPtr &val,const PSObjectPtr &attrs,bool bstatic,bool raw)
{
	if(type(self) != OT_CLASS) {
		Raise_Error(_SC("object must be a class"));
		return false;
	}
	PSClass *c = _class(self);
	if(!raw) {
		PSObjectPtr &mm = c->_metamethods[MT_NEWMEMBER];
		if(type(mm) != OT_NULL ) {
			Push(self); Push(key); Push(val);
			Push(attrs);
			Push(bstatic);
			return CallMetaMethod(mm,MT_NEWMEMBER,5,temp_reg);
		}
	}
	if(!NewSlot(self, key, val,bstatic))
		return false;
	if(type(attrs) != OT_NULL) {
		c->SetAttributes(key,attrs);
	}
	return true;
}

bool PSVM::NewSlot(const PSObjectPtr &self,const PSObjectPtr &key,const PSObjectPtr &val,bool bstatic)
{
	if(type(key) == OT_NULL) { Raise_Error(_SC("null cannot be used as index")); return false; }
	switch(type(self)) {
	case OT_TABLE: {
		bool rawcall = true;
		if(_table(self)->_delegate) {
			PSObjectPtr res;
			if(!_table(self)->Get(key,res)) {
				PSObjectPtr closure;
				if(_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this,MT_NEWSLOT,closure)) {
					Push(self);Push(key);Push(val);
					if(!CallMetaMethod(closure,MT_NEWSLOT,3,res)) {
						return false;
					}
					rawcall = false;
				}
				else {
					rawcall = true;
				}
			}
		}
		if(rawcall) _table(self)->NewSlot(key,val); //cannot fail
		
		break;}
	case OT_INSTANCE: {
		PSObjectPtr res;
		PSObjectPtr closure;
		if(_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this,MT_NEWSLOT,closure)) {
			Push(self);Push(key);Push(val);
			if(!CallMetaMethod(closure,MT_NEWSLOT,3,res)) {
				return false;
			}
			break;
		}
		Raise_Error(_SC("class instances do not support the new slot operator"));
		return false;
		break;}
	case OT_CLASS: 
		if(!_class(self)->NewSlot(_ss(this),key,val,bstatic)) {
			if(_class(self)->_locked) {
				Raise_Error(_SC("trying to modify a class that has already been instantiated"));
				return false;
			}
			else {
				PSObjectPtr oval = PrintObjVal(key);
				Raise_Error(_SC("the property '%s' already exists"),_stringval(oval));
				return false;
			}
		}
		break;
	default:
		Raise_Error(_SC("indexing %s with %s"),GetTypeName(self),GetTypeName(key));
		return false;
		break;
	}
	return true;
}



bool PSVM::DeleteSlot(const PSObjectPtr &self,const PSObjectPtr &key,PSObjectPtr &res)
{
	switch(type(self)) {
	case OT_TABLE:
	case OT_INSTANCE:
	case OT_USERDATA: {
		PSObjectPtr t;
		//bool handled = false;
		PSObjectPtr closure;
		if(_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this,MT_DELSLOT,closure)) {
			Push(self);Push(key);
			return CallMetaMethod(closure,MT_DELSLOT,2,res);
		}
		else {
			if(type(self) == OT_TABLE) {
				if(_table(self)->Get(key,t)) {
					_table(self)->Remove(key);
				}
				else {
					Raise_IdxError((PSObject &)key);
					return false;
				}
			}
			else {
				Raise_Error(_SC("cannot delete a slot from %s"),GetTypeName(self));
				return false;
			}
		}
		res = t;
				}
		break;
	default:
		Raise_Error(_SC("attempt to delete a slot from a %s"),GetTypeName(self));
		return false;
	}
	return true;
}

bool PSVM::Call(PSObjectPtr &closure,PSInteger nparams,PSInteger stackbase,PSObjectPtr &outres,PSBool raiseerror)
{
#ifdef _DEBUG
PSInteger prevstackbase = _stackbase;
#endif
	switch(type(closure)) {
	case OT_CLOSURE:
		return Execute(closure, nparams, stackbase, outres, raiseerror);
		break;
	case OT_NATIVECLOSURE:{
		bool suspend;
		return CallNative(_nativeclosure(closure), nparams, stackbase, outres,suspend);
		
						  }
		break;
	case OT_CLASS: {
		PSObjectPtr constr;
		PSObjectPtr temp;
		CreateClassInstance(_class(closure),outres,constr);
		if(type(constr) != OT_NULL) {
			_stack[stackbase] = outres;
			return Call(constr,nparams,stackbase,temp,raiseerror);
		}
		return true;
				   }
		break;
	default:
		return false;
	}
#ifdef _DEBUG
	if(!_suspended) {
		assert(_stackbase == prevstackbase);
	}
#endif
	return true;
}

bool PSVM::CallMetaMethod(PSObjectPtr &closure,PSMetaMethod mm,PSInteger nparams,PSObjectPtr &outres)
{
	//PSObjectPtr closure;
	
	_nmetamethodscall++;
	if(Call(closure, nparams, _top - nparams, outres, PSFalse)) {
		_nmetamethodscall--;
		Pop(nparams);
		return true;
	}
	_nmetamethodscall--;
	//}
	Pop(nparams);
	return false;
}

void PSVM::FindOuter(PSObjectPtr &target, PSObjectPtr *stackindex)
{
	PSOuter **pp = &_openouters;
	PSOuter *p;
	PSOuter *otr;

	while ((p = *pp) != NULL && p->_valptr >= stackindex) {
		if (p->_valptr == stackindex) {
			target = PSObjectPtr(p);
			return;
		}
		pp = &p->_next;
	}
	otr = PSOuter::Create(_ss(this), stackindex);
	otr->_next = *pp;
	otr->_idx  = (stackindex - _stack._vals);
	__ObjAddRef(otr);
	*pp = otr;
	target = PSObjectPtr(otr);
}

bool PSVM::EnterFrame(PSInteger newbase, PSInteger newtop, bool tailcall)
{
	if( !tailcall ) {
		if( _callsstacksize == _alloccallsstacksize ) {
			GrowCallStack();
		}
		ci = &_callsstack[_callsstacksize++];
		ci->_prevstkbase = (PSInt32)(newbase - _stackbase);
		ci->_prevtop = (PSInt32)(_top - _stackbase);
		ci->_etraps = 0;
		ci->_ncalls = 1;
		ci->_generator = NULL;
		ci->_root = PSFalse;
	}
	else {
		ci->_ncalls++;
	}

	_stackbase = newbase;
	_top = newtop;
	if(newtop + MIN_STACK_OVERHEAD > (PSInteger)_stack.size()) {
		if(_nmetamethodscall) {
			Raise_Error(_SC("stack overflow, cannot resize stack while in  a metamethod"));
			return false;
		}
		_stack.resize(_stack.size() + (MIN_STACK_OVERHEAD << 2));
		RelocateOuters();
	}
	return true;
}

void PSVM::LeaveFrame() {
	PSInteger last_top = _top;
	PSInteger last_stackbase = _stackbase;
	PSInteger css = --_callsstacksize;

	/* First clean out the call stack frame */
	ci->_closure.Null();
	_stackbase -= ci->_prevstkbase;
	_top = _stackbase + ci->_prevtop;
	ci = (css) ? &_callsstack[css-1] : NULL;

	if(_openouters) CloseOuters(&(_stack._vals[last_stackbase]));
	while (last_top >= _top) {
		_stack._vals[last_top--].Null();
	}
}

void PSVM::RelocateOuters()
{
	PSOuter *p = _openouters;
	while (p) {
		p->_valptr = _stack._vals + p->_idx;
		p = p->_next;
	}
}

void PSVM::CloseOuters(PSObjectPtr *stackindex) {
  PSOuter *p;
  while ((p = _openouters) != NULL && p->_valptr >= stackindex) {
    p->_value = *(p->_valptr);
    p->_valptr = &p->_value;
    _openouters = p->_next;
  	__ObjRelease(p);
  }
}

void PSVM::Remove(PSInteger n) {
	n = (n >= 0)?n + _stackbase - 1:_top + n;
	for(PSInteger i = n; i < _top; i++){
		_stack[i] = _stack[i+1];
	}
	_stack[_top].Null();
	_top--;
}

void PSVM::Pop() {
	_stack[--_top].Null();
}

void PSVM::Pop(PSInteger n) {
	for(PSInteger i = 0; i < n; i++){
		_stack[--_top].Null();
	}
}

void PSVM::PushNull() { _stack[_top++].Null(); }
void PSVM::Push(const PSObjectPtr &o) { _stack[_top++] = o; }
PSObjectPtr &PSVM::Top() { return _stack[_top-1]; }
PSObjectPtr &PSVM::PopGet() { return _stack[--_top]; }
PSObjectPtr &PSVM::GetUp(PSInteger n) { return _stack[_top+n]; }
PSObjectPtr &PSVM::GetAt(PSInteger n) { return _stack[n]; }

#ifdef _DEBUG_DUMP
void PSVM::dumpstack(PSInteger stackbase,bool dumpall)
{
	PSInteger size=dumpall?_stack.size():_top;
	PSInteger n=0;
	scprintf(_SC("\n>>>>stack dump<<<<\n"));
	CallInfo &ci=_callsstack[_callsstacksize-1];
	scprintf(_SC("IP: %p\n"),ci._ip);
	scprintf(_SC("prev stack base: %d\n"),ci._prevstkbase);
	scprintf(_SC("prev top: %d\n"),ci._prevtop);
	for(PSInteger i=0;i<size;i++){
		PSObjectPtr &obj=_stack[i];	
		if(stackbase==i)scprintf(_SC(">"));else scprintf(_SC(" "));
		scprintf(_SC("[%d]:"),n);
		switch(type(obj)){
		case OT_FLOAT:			scprintf(_SC("FLOAT %.3f"),_float(obj));break;
		case OT_INTEGER:		scprintf(_SC("INTEGER %d"),_integer(obj));break;
		case OT_BOOL:			scprintf(_SC("BOOL %s"),_integer(obj)?"true":"false");break;
		case OT_STRING:			scprintf(_SC("STRING %s"),_stringval(obj));break;
		case OT_NULL:			scprintf(_SC("NULL"));	break;
		case OT_TABLE:			scprintf(_SC("TABLE %p[%p]"),_table(obj),_table(obj)->_delegate);break;
		case OT_ARRAY:			scprintf(_SC("ARRAY %p"),_array(obj));break;
		case OT_CLOSURE:		scprintf(_SC("CLOSURE [%p]"),_closure(obj));break;
		case OT_NATIVECLOSURE:	scprintf(_SC("NATIVECLOSURE"));break;
		case OT_USERDATA:		scprintf(_SC("USERDATA %p[%p]"),_userdataval(obj),_userdata(obj)->_delegate);break;
		case OT_GENERATOR:		scprintf(_SC("GENERATOR %p"),_generator(obj));break;
		case OT_THREAD:			scprintf(_SC("THREAD [%p]"),_thread(obj));break;
		case OT_USERPOINTER:	scprintf(_SC("USERPOINTER %p"),_userpointer(obj));break;
		case OT_CLASS:			scprintf(_SC("CLASS %p"),_class(obj));break;
		case OT_INSTANCE:		scprintf(_SC("INSTANCE %p"),_instance(obj));break;
		case OT_WEAKREF:		scprintf(_SC("WEAKERF %p"),_weakref(obj));break;
		default:
			assert(0);
			break;
		};
		scprintf(_SC("\n"));
		++n;
	}
}



#endif
