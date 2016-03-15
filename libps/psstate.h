/*	see copyright notice in pscript.h */
#ifndef _PSSTATE_H_
#define _PSSTATE_H_

#include "psutils.h"
#include "psobject.h"
struct PSString;
struct PSTable;
//max number of character for a printed number
#define NUMBER_MAX_CHAR 50

struct PSStringTable
{
	PSStringTable(PSSharedState*ss);
	~PSStringTable();
	PSString *Add(const PSChar *,PSInteger len);
	void Remove(PSString *);
private:
	void Resize(PSInteger size);
	void AllocNodes(PSInteger size);
	PSString **_strings;
	PSUnsignedInteger _numofslots;
	PSUnsignedInteger _slotused;
	PSSharedState *_sharedstate;
};

struct RefTable {
	struct RefNode {
		PSObjectPtr obj;
		PSUnsignedInteger refs;
		struct RefNode *next;
	};
	RefTable();
	~RefTable();
	void AddRef(PSObject &obj);
	PSBool Release(PSObject &obj);
	PSUnsignedInteger GetRefCount(PSObject &obj);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(PSCollectable **chain);
#endif
	void Finalize();
private:
	RefNode *Get(PSObject &obj,PSHash &mainpos,RefNode **prev,bool add);
	RefNode *Add(PSHash mainpos,PSObject &obj);
	void Resize(PSUnsignedInteger size);
	void AllocNodes(PSUnsignedInteger size);
	PSUnsignedInteger _numofslots;
	PSUnsignedInteger _slotused;
	RefNode *_nodes;
	RefNode *_freelist;
	RefNode **_buckets;
};

#define ADD_STRING(ss,str,len) ss->_stringtable->Add(str,len)
#define REMOVE_STRING(ss,bstr) ss->_stringtable->Remove(bstr)

struct PSObjectPtr;

struct PSSharedState
{
	PSSharedState();
	~PSSharedState();
	void Init();
public:
	PSChar* GetScratchPad(PSInteger size);
	PSInteger GetMetaMethodIdxByName(const PSObjectPtr &name);
#ifndef NO_GARBAGE_COLLECTOR
	PSInteger CollectGarbage(PSVM *vm);
	void RunMark(PSVM *vm,PSCollectable **tchain);
	PSInteger ResurrectUnreachable(PSVM *vm);
	static void MarkObject(PSObjectPtr &o,PSCollectable **chain);
#endif
	PSObjectPtrVec *_metamethods;
	PSObjectPtr _metamethodsmap;
	PSObjectPtrVec *_systemstrings;
	PSObjectPtrVec *_types;
	PSStringTable *_stringtable;
	RefTable _refs_table;
	PSObjectPtr _registry;
	PSObjectPtr _consts;
	PSObjectPtr _constructoridx;
#ifndef NO_GARBAGE_COLLECTOR
	PSCollectable *_gc_chain;
#endif
	PSObjectPtr _root_vm;
	PSObjectPtr _table_default_delegate;
	static PSRegFunction _table_default_delegate_funcz[];
	PSObjectPtr _array_default_delegate;
	static PSRegFunction _array_default_delegate_funcz[];
	PSObjectPtr _string_default_delegate;
	static PSRegFunction _string_default_delegate_funcz[];
	PSObjectPtr _number_default_delegate;
	static PSRegFunction _number_default_delegate_funcz[];
	PSObjectPtr _generator_default_delegate;
	static PSRegFunction _generator_default_delegate_funcz[];
	PSObjectPtr _closure_default_delegate;
	static PSRegFunction _closure_default_delegate_funcz[];
	PSObjectPtr _thread_default_delegate;
	static PSRegFunction _thread_default_delegate_funcz[];
	PSObjectPtr _class_default_delegate;
	static PSRegFunction _class_default_delegate_funcz[];
	PSObjectPtr _instance_default_delegate;
	static PSRegFunction _instance_default_delegate_funcz[];
	PSObjectPtr _weakref_default_delegate;
	static PSRegFunction _weakref_default_delegate_funcz[];
	
	PSCOMPILERERROR _compilererrorhandler;
	PSPRINTFUNCTION _printfunc;
	PSPRINTFUNCTION _errorfunc;
	bool _debuginfo;
	bool _notifyallexceptions;
private:
	PSChar *_scratchpad;
	PSInteger _scratchpadsize;
};

#define _sp(s) (_sharedstate->GetScratchPad(s))
#define _spval (_sharedstate->GetScratchPad(-1))

#define _table_ddel		_table(_sharedstate->_table_default_delegate) 
#define _array_ddel		_table(_sharedstate->_array_default_delegate) 
#define _string_ddel	_table(_sharedstate->_string_default_delegate) 
#define _number_ddel	_table(_sharedstate->_number_default_delegate) 
#define _generator_ddel	_table(_sharedstate->_generator_default_delegate) 
#define _closure_ddel	_table(_sharedstate->_closure_default_delegate) 
#define _thread_ddel	_table(_sharedstate->_thread_default_delegate) 
#define _class_ddel		_table(_sharedstate->_class_default_delegate) 
#define _instance_ddel	_table(_sharedstate->_instance_default_delegate) 
#define _weakref_ddel	_table(_sharedstate->_weakref_default_delegate) 

#ifdef PSUNICODE //rsl REAL STRING LEN
#define rsl(l) ((l)<<1)
#else
#define rsl(l) (l)
#endif

//extern PSObjectPtr _null_;

bool CompileTypemask(PSIntVec &res,const PSChar *typemask);


#endif //_PSSTATE_H_
