/*	see copyright notice in pscript.h */
#ifndef _PSFUNCSTATE_H_
#define _PSFUNCSTATE_H_
///////////////////////////////////
#include "psutils.h"

struct PSFuncState
{
	PSFuncState(PSSharedState *ss,PSFuncState *parent,CompilerErrorFunc efunc,void *ed);
	~PSFuncState();
#ifdef _DEBUG_DUMP
	void Dump(PSFunctionProto *func);
#endif
	void Error(const PSChar *err);
	PSFuncState *PushChildState(PSSharedState *ss);
	void PopChildState();
	void AddInstruction(PSOpcode _op,PSInteger arg0=0,PSInteger arg1=0,PSInteger arg2=0,PSInteger arg3=0){PSInstruction i(_op,arg0,arg1,arg2,arg3);AddInstruction(i);}
	void AddInstruction(PSInstruction &i);
	void SetIntructionParams(PSInteger pos,PSInteger arg0,PSInteger arg1,PSInteger arg2=0,PSInteger arg3=0);
	void SetIntructionParam(PSInteger pos,PSInteger arg,PSInteger val);
	PSInstruction &GetInstruction(PSInteger pos){return _instructions[pos];}
	void PopInstructions(PSInteger size){for(PSInteger i=0;i<size;i++)_instructions.pop_back();}
	void SetStackSize(PSInteger n);
	PSInteger CountOuters(PSInteger stacksize);
	void SnoozeOpt(){_optimization=false;}
	void AddDefaultParam(PSInteger trg) { _defaultparams.push_back(trg); }
	PSInteger GetDefaultParamCount() { return _defaultparams.size(); }
	PSInteger GetCurrentPos(){return _instructions.size()-1;}
	PSInteger GetNumericConstant(const PSInteger cons);
	PSInteger GetNumericConstant(const PSFloat cons);
	PSInteger PushLocalVariable(const PSObject &name);
	void AddParameter(const PSObject &name);
	//void AddOuterValue(const PSObject &name);
	PSInteger GetLocalVariable(const PSObject &name);
	void MarkLocalAsOuter(PSInteger pos);
	PSInteger GetOuterVariable(const PSObject &name);
	PSInteger GenerateCode();
	PSInteger GetStackSize();
	PSInteger CalcStackFrameSize();
	void AddLineInfos(PSInteger line,bool lineop,bool force=false);
	PSFunctionProto *BuildProto();
	PSInteger AllocStackPos();
	PSInteger PushTarget(PSInteger n=-1);
	PSInteger PopTarget();
	PSInteger TopTarget();
	PSInteger GetUpTarget(PSInteger n);
	void DiscardTarget();
	bool IsLocal(PSUnsignedInteger stkpos);
	PSObject CreateString(const PSChar *s,PSInteger len = -1);
	PSObject CreateTable();
	bool IsConstant(const PSObject &name,PSObject &e);
	PSInteger _returnexp;
	PSLocalVarInfoVec _vlocals;
	PSIntVec _targetstack;
	PSInteger _stacksize;
	bool _varparams;
	bool _bgenerator;
	PSIntVec _unresolvedbreaks;
	PSIntVec _unresolvedcontinues;
	PSObjectPtrVec _functions;
	PSObjectPtrVec _parameters;
	PSOuterVarVec _outervalues;
	PSInstructionVec _instructions;
	PSLocalVarInfoVec _localvarinfos;
	PSObjectPtr _literals;
	PSObjectPtr _strings;
	PSObjectPtr _name;
	PSObjectPtr _sourcename;
	PSInteger _nliterals;
	PSLineInfoVec _lineinfos;
	PSFuncState *_parent;
	PSIntVec _scope_blocks;
	PSIntVec _breaktargets;
	PSIntVec _continuetargets;
	PSIntVec _defaultparams;
	PSInteger _lastline;
	PSInteger _traps; //contains number of nested exception traps
	PSInteger _outers;
	bool _optimization;
	PSSharedState *_sharedstate;
	psvector<PSFuncState*> _childstates;
	PSInteger GetConstant(const PSObject &cons);
private:
	CompilerErrorFunc _errfunc;
	void *_errtarget;
	PSSharedState *_ss;
};


#endif //_PSFUNCSTATE_H_

