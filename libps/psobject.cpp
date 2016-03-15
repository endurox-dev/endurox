/*
	see copyright notice in pscript.h
*/
#include "pspcheader.h"
#include "psvm.h"
#include "psstring.h"
#include "psarray.h"
#include "pstable.h"
#include "psuserdata.h"
#include "psfuncproto.h"
#include "psclass.h"
#include "psclosure.h"


const PSChar *IdType2Name(PSObjectType type)
{
	switch(_RAW_TYPE(type))
	{
	case _RT_NULL:return _SC("null");
	case _RT_INTEGER:return _SC("integer");
	case _RT_FLOAT:return _SC("float");
	case _RT_BOOL:return _SC("bool");
	case _RT_STRING:return _SC("string");
	case _RT_TABLE:return _SC("table");
	case _RT_ARRAY:return _SC("array");
	case _RT_GENERATOR:return _SC("generator");
	case _RT_CLOSURE:
	case _RT_NATIVECLOSURE:
		return _SC("function");
	case _RT_USERDATA:
	case _RT_USERPOINTER:
		return _SC("userdata");
	case _RT_THREAD: return _SC("thread");
	case _RT_FUNCPROTO: return _SC("function");
	case _RT_CLASS: return _SC("class");
	case _RT_INSTANCE: return _SC("instance");
	case _RT_WEAKREF: return _SC("weakref");
	case _RT_OUTER: return _SC("outer");
	default:
		return NULL;
	}
}

const PSChar *GetTypeName(const PSObjectPtr &obj1)
{
	return IdType2Name(type(obj1));	
}

PSString *PSString::Create(PSSharedState *ss,const PSChar *s,PSInteger len)
{
	PSString *str=ADD_STRING(ss,s,len);
	return str;
}

void PSString::Release()
{
	REMOVE_STRING(_sharedstate,this);
}

PSInteger PSString::Next(const PSObjectPtr &refpos, PSObjectPtr &outkey, PSObjectPtr &outval)
{
	PSInteger idx = (PSInteger)TranslateIndex(refpos);
	while(idx < _len){
		outkey = (PSInteger)idx;
		outval = (PSInteger)((PSUnsignedInteger)_val[idx]);
		//return idx for the next iteration
		return ++idx;
	}
	//nothing to iterate anymore
	return -1;
}

PSUnsignedInteger TranslateIndex(const PSObjectPtr &idx)
{
	switch(type(idx)){
		case OT_NULL:
			return 0;
		case OT_INTEGER:
			return (PSUnsignedInteger)_integer(idx);
		default: assert(0); break;
	}
	return 0;
}

PSWeakRef *PSRefCounted::GetWeakRef(PSObjectType type)
{
	if(!_weakref) {
		ps_new(_weakref,PSWeakRef);
		_weakref->_obj._type = type;
		_weakref->_obj._unVal.pRefCounted = this;
	}
	return _weakref;
}

PSRefCounted::~PSRefCounted()
{
	if(_weakref) {
		_weakref->_obj._type = OT_NULL;
		_weakref->_obj._unVal.pRefCounted = NULL;
	}
}

void PSWeakRef::Release() { 
	if(ISREFCOUNTED(_obj._type)) { 
		_obj._unVal.pRefCounted->_weakref = NULL;
	} 
	ps_delete(this,PSWeakRef);
}

bool PSDelegable::GetMetaMethod(PSVM *v,PSMetaMethod mm,PSObjectPtr &res) {
	if(_delegate) {
		return _delegate->Get((*_ss(v)->_metamethods)[mm],res);
	}
	return false;
}

bool PSDelegable::SetDelegate(PSTable *mt)
{
	PSTable *temp = mt;
	if(temp == this) return false;
	while (temp) {
		if (temp->_delegate == this) return false; //cycle detected
		temp = temp->_delegate;
	}
	if (mt)	__ObjAddRef(mt);
	__ObjRelease(_delegate);
	_delegate = mt;
	return true;
}

bool PSGenerator::Yield(PSVM *v,PSInteger target)
{
	if(_state==eSuspended) { v->Raise_Error(_SC("internal vm error, yielding dead generator"));  return false;}
	if(_state==eDead) { v->Raise_Error(_SC("internal vm error, yielding a dead generator")); return false; }
	PSInteger size = v->_top-v->_stackbase;
	
	_stack.resize(size);
	PSObject _this = v->_stack[v->_stackbase];
	_stack._vals[0] = ISREFCOUNTED(type(_this)) ? PSObjectPtr(_refcounted(_this)->GetWeakRef(type(_this))) : _this;
	for(PSInteger n =1; n<target; n++) {
		_stack._vals[n] = v->_stack[v->_stackbase+n];
	}
	for(PSInteger j =0; j < size; j++)
	{
		v->_stack[v->_stackbase+j].Null();
	}

	_ci = *v->ci;
	_ci._generator=NULL;
	for(PSInteger i=0;i<_ci._etraps;i++) {
		_etraps.push_back(v->_etraps.top());
		v->_etraps.pop_back();
	}
	_state=eSuspended;
	return true;
}

bool PSGenerator::Resume(PSVM *v,PSObjectPtr &dest)
{
	if(_state==eDead){ v->Raise_Error(_SC("resuming dead generator")); return false; }
	if(_state==eRunning){ v->Raise_Error(_SC("resuming active generator")); return false; }
	PSInteger size = _stack.size();
	PSInteger target = &dest - &(v->_stack._vals[v->_stackbase]);
	assert(target>=0 && target<=255);
	if(!v->EnterFrame(v->_top, v->_top + size, false)) 
		return false;
	v->ci->_generator   = this;
	v->ci->_target      = (PSInt32)target;
	v->ci->_closure     = _ci._closure;
	v->ci->_ip          = _ci._ip;
	v->ci->_literals    = _ci._literals;
	v->ci->_ncalls      = _ci._ncalls;
	v->ci->_etraps      = _ci._etraps;
	v->ci->_root        = _ci._root;


	for(PSInteger i=0;i<_ci._etraps;i++) {
		v->_etraps.push_back(_etraps.top());
		_etraps.pop_back();
	}
	PSObject _this = _stack._vals[0];
	v->_stack[v->_stackbase] = type(_this) == OT_WEAKREF ? _weakref(_this)->_obj : _this;

	for(PSInteger n = 1; n<size; n++) {
		v->_stack[v->_stackbase+n] = _stack._vals[n];
		_stack._vals[n].Null();
	}

	_state=eRunning;
	if (v->_debughook)
		v->CallDebugHook(_SC('c'));

	return true;
}

void PSArray::Extend(const PSArray *a){
	PSInteger xlen;
	if((xlen=a->Size()))
		for(PSInteger i=0;i<xlen;i++)
			Append(a->_values[i]);
}

const PSChar* PSFunctionProto::GetLocal(PSVM *vm,PSUnsignedInteger stackbase,PSUnsignedInteger nseq,PSUnsignedInteger nop)
{
	PSUnsignedInteger nvars=_nlocalvarinfos;
	const PSChar *res=NULL; 
	if(nvars>=nseq){
 		for(PSUnsignedInteger i=0;i<nvars;i++){
			if(_localvarinfos[i]._start_op<=nop && _localvarinfos[i]._end_op>=nop)
			{
				if(nseq==0){
					vm->Push(vm->_stack[stackbase+_localvarinfos[i]._pos]);
					res=_stringval(_localvarinfos[i]._name);
					break;
				}
				nseq--;
			}
		}
	}
	return res;
}


PSInteger PSFunctionProto::GetLine(PSInstruction *curr)
{
	PSInteger op = (PSInteger)(curr-_instructions);
	PSInteger line=_lineinfos[0]._line;
	PSInteger low = 0;
	PSInteger high = _nlineinfos - 1;
	PSInteger mid = 0;
	while(low <= high)
	{
		mid = low + ((high - low) >> 1);
		PSInteger curop = _lineinfos[mid]._op;
		if(curop > op)
		{
			high = mid - 1;
		}
		else if(curop < op) {
			if(mid < (_nlineinfos - 1) 
				&& _lineinfos[mid + 1]._op >= op) {
				break;
			}
			low = mid + 1;
		}
		else { //equal
			break;
		}
	}

	line = _lineinfos[mid]._line;
	return line;
}

PSClosure::~PSClosure()
{
	__ObjRelease(_env);
	__ObjRelease(_base);
	REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

#define _CHECK_IO(exp)  { if(!exp)return false; }
bool SafeWrite(HPSCRIPTVM v,PSWRITEFUNC write,PSUserPointer up,PSUserPointer dest,PSInteger size)
{
	if(write(up,dest,size) != size) {
		v->Raise_Error(_SC("io error (write function failure)"));
		return false;
	}
	return true;
}

bool SafeRead(HPSCRIPTVM v,PSWRITEFUNC read,PSUserPointer up,PSUserPointer dest,PSInteger size)
{
	if(size && read(up,dest,size) != size) {
		v->Raise_Error(_SC("io error, read function failure, the origin stream could be corrupted/trucated"));
		return false;
	}
	return true;
}

bool WriteTag(HPSCRIPTVM v,PSWRITEFUNC write,PSUserPointer up,PSUnsignedInteger32 tag)
{
	return SafeWrite(v,write,up,&tag,sizeof(tag));
}

bool CheckTag(HPSCRIPTVM v,PSWRITEFUNC read,PSUserPointer up,PSUnsignedInteger32 tag)
{
	PSUnsignedInteger32 t;
	_CHECK_IO(SafeRead(v,read,up,&t,sizeof(t)));
	if(t != tag){
		v->Raise_Error(_SC("invalid or corrupted closure stream"));
		return false;
	}
	return true;
}

bool WriteObject(HPSCRIPTVM v,PSUserPointer up,PSWRITEFUNC write,PSObjectPtr &o)
{
	PSUnsignedInteger32 _type = (PSUnsignedInteger32)type(o);
	_CHECK_IO(SafeWrite(v,write,up,&_type,sizeof(_type)));
	switch(type(o)){
	case OT_STRING:
		_CHECK_IO(SafeWrite(v,write,up,&_string(o)->_len,sizeof(PSInteger)));
		_CHECK_IO(SafeWrite(v,write,up,_stringval(o),rsl(_string(o)->_len)));
		break;
	case OT_BOOL:
	case OT_INTEGER:
		_CHECK_IO(SafeWrite(v,write,up,&_integer(o),sizeof(PSInteger)));break;
	case OT_FLOAT:
		_CHECK_IO(SafeWrite(v,write,up,&_float(o),sizeof(PSFloat)));break;
	case OT_NULL:
		break;
	default:
		v->Raise_Error(_SC("cannot serialize a %s"),GetTypeName(o));
		return false;
	}
	return true;
}

bool ReadObject(HPSCRIPTVM v,PSUserPointer up,PSREADFUNC read,PSObjectPtr &o)
{
	PSUnsignedInteger32 _type;
	_CHECK_IO(SafeRead(v,read,up,&_type,sizeof(_type)));
	PSObjectType t = (PSObjectType)_type;
	switch(t){
	case OT_STRING:{
		PSInteger len;
		_CHECK_IO(SafeRead(v,read,up,&len,sizeof(PSInteger)));
		_CHECK_IO(SafeRead(v,read,up,_ss(v)->GetScratchPad(rsl(len)),rsl(len)));
		o=PSString::Create(_ss(v),_ss(v)->GetScratchPad(-1),len);
				   }
		break;
	case OT_INTEGER:{
		PSInteger i;
		_CHECK_IO(SafeRead(v,read,up,&i,sizeof(PSInteger))); o = i; break;
					}
	case OT_BOOL:{
		PSInteger i;
		_CHECK_IO(SafeRead(v,read,up,&i,sizeof(PSInteger))); o._type = OT_BOOL; o._unVal.nInteger = i; break;
					}
	case OT_FLOAT:{
		PSFloat f;
		_CHECK_IO(SafeRead(v,read,up,&f,sizeof(PSFloat))); o = f; break;
				  }
	case OT_NULL:
		o.Null();
		break;
	default:
		v->Raise_Error(_SC("cannot serialize a %s"),IdType2Name(t));
		return false;
	}
	return true;
}

bool PSClosure::Save(PSVM *v,PSUserPointer up,PSWRITEFUNC write)
{
	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_HEAD));
	_CHECK_IO(WriteTag(v,write,up,sizeof(PSChar)));
	_CHECK_IO(WriteTag(v,write,up,sizeof(PSInteger)));
	_CHECK_IO(WriteTag(v,write,up,sizeof(PSFloat)));
	_CHECK_IO(_function->Save(v,up,write));
	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_TAIL));
	return true;
}

bool PSClosure::Load(PSVM *v,PSUserPointer up,PSREADFUNC read,PSObjectPtr &ret)
{
	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_HEAD));
	_CHECK_IO(CheckTag(v,read,up,sizeof(PSChar)));
	_CHECK_IO(CheckTag(v,read,up,sizeof(PSInteger)));
	_CHECK_IO(CheckTag(v,read,up,sizeof(PSFloat)));
	PSObjectPtr func;
	_CHECK_IO(PSFunctionProto::Load(v,up,read,func));
	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_TAIL));
	ret = PSClosure::Create(_ss(v),_funcproto(func));
	return true;
}

PSFunctionProto::PSFunctionProto(PSSharedState *ss)
{
	_stacksize=0;
	_bgenerator=false;
	INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);
}

PSFunctionProto::~PSFunctionProto()
{
	REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

bool PSFunctionProto::Save(PSVM *v,PSUserPointer up,PSWRITEFUNC write)
{
	PSInteger i,nliterals = _nliterals,nparameters = _nparameters;
	PSInteger noutervalues = _noutervalues,nlocalvarinfos = _nlocalvarinfos;
	PSInteger nlineinfos=_nlineinfos,ninstructions = _ninstructions,nfunctions=_nfunctions;
	PSInteger ndefaultparams = _ndefaultparams;
	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(WriteObject(v,up,write,_sourcename));
	_CHECK_IO(WriteObject(v,up,write,_name));
	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,&nliterals,sizeof(nliterals)));
	_CHECK_IO(SafeWrite(v,write,up,&nparameters,sizeof(nparameters)));
	_CHECK_IO(SafeWrite(v,write,up,&noutervalues,sizeof(noutervalues)));
	_CHECK_IO(SafeWrite(v,write,up,&nlocalvarinfos,sizeof(nlocalvarinfos)));
	_CHECK_IO(SafeWrite(v,write,up,&nlineinfos,sizeof(nlineinfos)));
	_CHECK_IO(SafeWrite(v,write,up,&ndefaultparams,sizeof(ndefaultparams)));
	_CHECK_IO(SafeWrite(v,write,up,&ninstructions,sizeof(ninstructions)));
	_CHECK_IO(SafeWrite(v,write,up,&nfunctions,sizeof(nfunctions)));
	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	for(i=0;i<nliterals;i++){
		_CHECK_IO(WriteObject(v,up,write,_literals[i]));
	}

	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	for(i=0;i<nparameters;i++){
		_CHECK_IO(WriteObject(v,up,write,_parameters[i]));
	}

	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	for(i=0;i<noutervalues;i++){
		_CHECK_IO(SafeWrite(v,write,up,&_outervalues[i]._type,sizeof(PSUnsignedInteger)));
		_CHECK_IO(WriteObject(v,up,write,_outervalues[i]._src));
		_CHECK_IO(WriteObject(v,up,write,_outervalues[i]._name));
	}

	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	for(i=0;i<nlocalvarinfos;i++){
		PSLocalVarInfo &lvi=_localvarinfos[i];
		_CHECK_IO(WriteObject(v,up,write,lvi._name));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._pos,sizeof(PSUnsignedInteger)));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._start_op,sizeof(PSUnsignedInteger)));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._end_op,sizeof(PSUnsignedInteger)));
	}

	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_lineinfos,sizeof(PSLineInfo)*nlineinfos));

	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_defaultparams,sizeof(PSInteger)*ndefaultparams));

	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_instructions,sizeof(PSInstruction)*ninstructions));

	_CHECK_IO(WriteTag(v,write,up,PS_CLOSURESTREAM_PART));
	for(i=0;i<nfunctions;i++){
		_CHECK_IO(_funcproto(_functions[i])->Save(v,up,write));
	}
	_CHECK_IO(SafeWrite(v,write,up,&_stacksize,sizeof(_stacksize)));
	_CHECK_IO(SafeWrite(v,write,up,&_bgenerator,sizeof(_bgenerator)));
	_CHECK_IO(SafeWrite(v,write,up,&_varparams,sizeof(_varparams)));
	return true;
}

bool PSFunctionProto::Load(PSVM *v,PSUserPointer up,PSREADFUNC read,PSObjectPtr &ret)
{
	PSInteger i, nliterals,nparameters;
	PSInteger noutervalues ,nlocalvarinfos ;
	PSInteger nlineinfos,ninstructions ,nfunctions,ndefaultparams ;
	PSObjectPtr sourcename, name;
	PSObjectPtr o;
	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(ReadObject(v, up, read, sourcename));
	_CHECK_IO(ReadObject(v, up, read, name));
	
	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, &nliterals, sizeof(nliterals)));
	_CHECK_IO(SafeRead(v,read,up, &nparameters, sizeof(nparameters)));
	_CHECK_IO(SafeRead(v,read,up, &noutervalues, sizeof(noutervalues)));
	_CHECK_IO(SafeRead(v,read,up, &nlocalvarinfos, sizeof(nlocalvarinfos)));
	_CHECK_IO(SafeRead(v,read,up, &nlineinfos, sizeof(nlineinfos)));
	_CHECK_IO(SafeRead(v,read,up, &ndefaultparams, sizeof(ndefaultparams)));
	_CHECK_IO(SafeRead(v,read,up, &ninstructions, sizeof(ninstructions)));
	_CHECK_IO(SafeRead(v,read,up, &nfunctions, sizeof(nfunctions)));
	

	PSFunctionProto *f = PSFunctionProto::Create(_opt_ss(v),ninstructions,nliterals,nparameters,
			nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams);
	PSObjectPtr proto = f; //gets a ref in case of failure
	f->_sourcename = sourcename;
	f->_name = name;

	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));

	for(i = 0;i < nliterals; i++){
		_CHECK_IO(ReadObject(v, up, read, o));
		f->_literals[i] = o;
	}
	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));

	for(i = 0; i < nparameters; i++){
		_CHECK_IO(ReadObject(v, up, read, o));
		f->_parameters[i] = o;
	}
	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));

	for(i = 0; i < noutervalues; i++){
		PSUnsignedInteger type;
		PSObjectPtr name;
		_CHECK_IO(SafeRead(v,read,up, &type, sizeof(PSUnsignedInteger)));
		_CHECK_IO(ReadObject(v, up, read, o));
		_CHECK_IO(ReadObject(v, up, read, name));
		f->_outervalues[i] = PSOuterVar(name,o, (PSOuterType)type);
	}
	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));

	for(i = 0; i < nlocalvarinfos; i++){
		PSLocalVarInfo lvi;
		_CHECK_IO(ReadObject(v, up, read, lvi._name));
		_CHECK_IO(SafeRead(v,read,up, &lvi._pos, sizeof(PSUnsignedInteger)));
		_CHECK_IO(SafeRead(v,read,up, &lvi._start_op, sizeof(PSUnsignedInteger)));
		_CHECK_IO(SafeRead(v,read,up, &lvi._end_op, sizeof(PSUnsignedInteger)));
		f->_localvarinfos[i] = lvi;
	}
	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_lineinfos, sizeof(PSLineInfo)*nlineinfos));

	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_defaultparams, sizeof(PSInteger)*ndefaultparams));

	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_instructions, sizeof(PSInstruction)*ninstructions));

	_CHECK_IO(CheckTag(v,read,up,PS_CLOSURESTREAM_PART));
	for(i = 0; i < nfunctions; i++){
		_CHECK_IO(_funcproto(o)->Load(v, up, read, o));
		f->_functions[i] = o;
	}
	_CHECK_IO(SafeRead(v,read,up, &f->_stacksize, sizeof(f->_stacksize)));
	_CHECK_IO(SafeRead(v,read,up, &f->_bgenerator, sizeof(f->_bgenerator)));
	_CHECK_IO(SafeRead(v,read,up, &f->_varparams, sizeof(f->_varparams)));
	
	ret = f;
	return true;
}

#ifndef NO_GARBAGE_COLLECTOR

#define START_MARK() 	if(!(_uiRef&MARK_FLAG)){ \
		_uiRef|=MARK_FLAG;

#define END_MARK() RemoveFromChain(&_sharedstate->_gc_chain, this); \
		AddToChain(chain, this); }

void PSVM::Mark(PSCollectable **chain)
{
	START_MARK()
		PSSharedState::MarkObject(_lasterror,chain);
		PSSharedState::MarkObject(_errorhandler,chain);
		PSSharedState::MarkObject(_debughook_closure,chain);
		PSSharedState::MarkObject(_roottable, chain);
		PSSharedState::MarkObject(temp_reg, chain);
		for(PSUnsignedInteger i = 0; i < _stack.size(); i++) PSSharedState::MarkObject(_stack[i], chain);
		for(PSInteger k = 0; k < _callsstacksize; k++) PSSharedState::MarkObject(_callsstack[k]._closure, chain);
	END_MARK()
}

void PSArray::Mark(PSCollectable **chain)
{
	START_MARK()
		PSInteger len = _values.size();
		for(PSInteger i = 0;i < len; i++) PSSharedState::MarkObject(_values[i], chain);
	END_MARK()
}
void PSTable::Mark(PSCollectable **chain)
{
	START_MARK()
		if(_delegate) _delegate->Mark(chain);
		PSInteger len = _numofnodes;
		for(PSInteger i = 0; i < len; i++){
			PSSharedState::MarkObject(_nodes[i].key, chain);
			PSSharedState::MarkObject(_nodes[i].val, chain);
		}
	END_MARK()
}

void PSClass::Mark(PSCollectable **chain)
{
	START_MARK()
		_members->Mark(chain);
		if(_base) _base->Mark(chain);
		PSSharedState::MarkObject(_attributes, chain);
		for(PSUnsignedInteger i =0; i< _defaultvalues.size(); i++) {
			PSSharedState::MarkObject(_defaultvalues[i].val, chain);
			PSSharedState::MarkObject(_defaultvalues[i].attrs, chain);
		}
		for(PSUnsignedInteger j =0; j< _methods.size(); j++) {
			PSSharedState::MarkObject(_methods[j].val, chain);
			PSSharedState::MarkObject(_methods[j].attrs, chain);
		}
		for(PSUnsignedInteger k =0; k< MT_LAST; k++) {
			PSSharedState::MarkObject(_metamethods[k], chain);
		}
	END_MARK()
}

void PSInstance::Mark(PSCollectable **chain)
{
	START_MARK()
		_class->Mark(chain);
		PSUnsignedInteger nvalues = _class->_defaultvalues.size();
		for(PSUnsignedInteger i =0; i< nvalues; i++) {
			PSSharedState::MarkObject(_values[i], chain);
		}
	END_MARK()
}

void PSGenerator::Mark(PSCollectable **chain)
{
	START_MARK()
		for(PSUnsignedInteger i = 0; i < _stack.size(); i++) PSSharedState::MarkObject(_stack[i], chain);
		PSSharedState::MarkObject(_closure, chain);
	END_MARK()
}

void PSFunctionProto::Mark(PSCollectable **chain)
{
	START_MARK()
		for(PSInteger i = 0; i < _nliterals; i++) PSSharedState::MarkObject(_literals[i], chain);
		for(PSInteger k = 0; k < _nfunctions; k++) PSSharedState::MarkObject(_functions[k], chain);
	END_MARK()
}

void PSClosure::Mark(PSCollectable **chain)
{
	START_MARK()
		if(_base) _base->Mark(chain);
		PSFunctionProto *fp = _function;
		fp->Mark(chain);
		for(PSInteger i = 0; i < fp->_noutervalues; i++) PSSharedState::MarkObject(_outervalues[i], chain);
		for(PSInteger k = 0; k < fp->_ndefaultparams; k++) PSSharedState::MarkObject(_defaultparams[k], chain);
	END_MARK()
}

void PSNativeClosure::Mark(PSCollectable **chain)
{
	START_MARK()
		for(PSUnsignedInteger i = 0; i < _noutervalues; i++) PSSharedState::MarkObject(_outervalues[i], chain);
	END_MARK()
}

void PSOuter::Mark(PSCollectable **chain)
{
	START_MARK()
    /* If the valptr points to a closed value, that value is alive */
    if(_valptr == &_value) {
      PSSharedState::MarkObject(_value, chain);
    }
	END_MARK()
}

void PSUserData::Mark(PSCollectable **chain){
	START_MARK()
		if(_delegate) _delegate->Mark(chain);
	END_MARK()
}

void PSCollectable::UnMark() { _uiRef&=~MARK_FLAG; }

#endif

