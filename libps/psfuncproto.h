/*	see copyright notice in pscript.h */
#ifndef _PSFUNCTION_H_
#define _PSFUNCTION_H_

#include "psopcodes.h"

enum PSOuterType {
	otLOCAL = 0,
	otOUTER = 1
};

struct PSOuterVar
{
	
	PSOuterVar(){}
	PSOuterVar(const PSObjectPtr &name,const PSObjectPtr &src,PSOuterType t)
	{
		_name = name;
		_src=src;
		_type=t;
	}
	PSOuterVar(const PSOuterVar &ov)
	{
		_type=ov._type;
		_src=ov._src;
		_name=ov._name;
	}
	PSOuterType _type;
	PSObjectPtr _name;
	PSObjectPtr _src;
};

struct PSLocalVarInfo
{
	PSLocalVarInfo():_start_op(0),_end_op(0),_pos(0){}
	PSLocalVarInfo(const PSLocalVarInfo &lvi)
	{
		_name=lvi._name;
		_start_op=lvi._start_op;
		_end_op=lvi._end_op;
		_pos=lvi._pos;
	}
	PSObjectPtr _name;
	PSUnsignedInteger _start_op;
	PSUnsignedInteger _end_op;
	PSUnsignedInteger _pos;
};

struct PSLineInfo { PSInteger _line;PSInteger _op; };

typedef psvector<PSOuterVar> PSOuterVarVec;
typedef psvector<PSLocalVarInfo> PSLocalVarInfoVec;
typedef psvector<PSLineInfo> PSLineInfoVec;

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(PSFunctionProto) \
		+((ni-1)*sizeof(PSInstruction))+(nl*sizeof(PSObjectPtr)) \
		+(nparams*sizeof(PSObjectPtr))+(nfuncs*sizeof(PSObjectPtr)) \
		+(nouters*sizeof(PSOuterVar))+(nlineinf*sizeof(PSLineInfo)) \
		+(localinf*sizeof(PSLocalVarInfo))+(defparams*sizeof(PSInteger)))


struct PSFunctionProto : public CHAINABLE_OBJ
{
private:
	PSFunctionProto(PSSharedState *ss);
	~PSFunctionProto();
	
public:
	static PSFunctionProto *Create(PSSharedState *ss,PSInteger ninstructions,
		PSInteger nliterals,PSInteger nparameters,
		PSInteger nfunctions,PSInteger noutervalues,
		PSInteger nlineinfos,PSInteger nlocalvarinfos,PSInteger ndefaultparams)
	{
		PSFunctionProto *f;
		//I compact the whole class and members in a single memory allocation
		f = (PSFunctionProto *)ps_vm_malloc(_FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams));
		new (f) PSFunctionProto(ss);
		f->_ninstructions = ninstructions;
		f->_literals = (PSObjectPtr*)&f->_instructions[ninstructions];
		f->_nliterals = nliterals;
		f->_parameters = (PSObjectPtr*)&f->_literals[nliterals];
		f->_nparameters = nparameters;
		f->_functions = (PSObjectPtr*)&f->_parameters[nparameters];
		f->_nfunctions = nfunctions;
		f->_outervalues = (PSOuterVar*)&f->_functions[nfunctions];
		f->_noutervalues = noutervalues;
		f->_lineinfos = (PSLineInfo *)&f->_outervalues[noutervalues];
		f->_nlineinfos = nlineinfos;
		f->_localvarinfos = (PSLocalVarInfo *)&f->_lineinfos[nlineinfos];
		f->_nlocalvarinfos = nlocalvarinfos;
		f->_defaultparams = (PSInteger *)&f->_localvarinfos[nlocalvarinfos];
		f->_ndefaultparams = ndefaultparams;

		_CONSTRUCT_VECTOR(PSObjectPtr,f->_nliterals,f->_literals);
		_CONSTRUCT_VECTOR(PSObjectPtr,f->_nparameters,f->_parameters);
		_CONSTRUCT_VECTOR(PSObjectPtr,f->_nfunctions,f->_functions);
		_CONSTRUCT_VECTOR(PSOuterVar,f->_noutervalues,f->_outervalues);
		//_CONSTRUCT_VECTOR(PSLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
		_CONSTRUCT_VECTOR(PSLocalVarInfo,f->_nlocalvarinfos,f->_localvarinfos);
		return f;
	}
	void Release(){ 
		_DESTRUCT_VECTOR(PSObjectPtr,_nliterals,_literals);
		_DESTRUCT_VECTOR(PSObjectPtr,_nparameters,_parameters);
		_DESTRUCT_VECTOR(PSObjectPtr,_nfunctions,_functions);
		_DESTRUCT_VECTOR(PSOuterVar,_noutervalues,_outervalues);
		//_DESTRUCT_VECTOR(PSLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
		_DESTRUCT_VECTOR(PSLocalVarInfo,_nlocalvarinfos,_localvarinfos);
		PSInteger size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_nlocalvarinfos,_ndefaultparams);
		this->~PSFunctionProto();
		ps_vm_free(this,size);
	}
	
	const PSChar* GetLocal(PSVM *v,PSUnsignedInteger stackbase,PSUnsignedInteger nseq,PSUnsignedInteger nop);
	PSInteger GetLine(PSInstruction *curr);
	bool Save(PSVM *v,PSUserPointer up,PSWRITEFUNC write);
	static bool Load(PSVM *v,PSUserPointer up,PSREADFUNC read,PSObjectPtr &ret);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(PSCollectable **chain);
	void Finalize(){ _NULL_PSOBJECT_VECTOR(_literals,_nliterals); }
	PSObjectType GetType() {return OT_FUNCPROTO;}
#endif
	PSObjectPtr _sourcename;
	PSObjectPtr _name;
    PSInteger _stacksize;
	bool _bgenerator;
	PSInteger _varparams;

	PSInteger _nlocalvarinfos;
	PSLocalVarInfo *_localvarinfos;

	PSInteger _nlineinfos;
	PSLineInfo *_lineinfos;

	PSInteger _nliterals;
	PSObjectPtr *_literals;

	PSInteger _nparameters;
	PSObjectPtr *_parameters;
	
	PSInteger _nfunctions;
	PSObjectPtr *_functions;

	PSInteger _noutervalues;
	PSOuterVar *_outervalues;

	PSInteger _ndefaultparams;
	PSInteger *_defaultparams;
	
	PSInteger _ninstructions;
	PSInstruction _instructions[1];
};

#endif //_PSFUNCTION_H_
