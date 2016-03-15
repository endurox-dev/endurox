/*
	see copyright notice in pscript.h
*/
#include "pspcheader.h"
#include <stdarg.h>
#include "psvm.h"
#include "psfuncproto.h"
#include "psclosure.h"
#include "psstring.h"

PSRESULT ps_getfunctioninfo(HPSCRIPTVM v,PSInteger level,PSFunctionInfo *fi)
{
	PSInteger cssize = v->_callsstacksize;
	if (cssize > level) {
		PSVM::CallInfo &ci = v->_callsstack[cssize-level-1];
		if(ps_isclosure(ci._closure)) {
			PSClosure *c = _closure(ci._closure);
			PSFunctionProto *proto = c->_function;
			fi->funcid = proto;
			fi->name = type(proto->_name) == OT_STRING?_stringval(proto->_name):_SC("unknown");
			fi->source = type(proto->_name) == OT_STRING?_stringval(proto->_sourcename):_SC("unknown");
			return PS_OK;
		}
	}
	return ps_throwerror(v,_SC("the object is not a closure"));
}

PSRESULT ps_stackinfos(HPSCRIPTVM v, PSInteger level, PSStackInfos *si)
{
	PSInteger cssize = v->_callsstacksize;
	if (cssize > level) {
		memset(si, 0, sizeof(PSStackInfos));
		PSVM::CallInfo &ci = v->_callsstack[cssize-level-1];
		switch (type(ci._closure)) {
		case OT_CLOSURE:{
			PSFunctionProto *func = _closure(ci._closure)->_function;
			if (type(func->_name) == OT_STRING)
				si->funcname = _stringval(func->_name);
			if (type(func->_sourcename) == OT_STRING)
				si->source = _stringval(func->_sourcename);
			si->line = func->GetLine(ci._ip);
						}
			break;
		case OT_NATIVECLOSURE:
			si->source = _SC("NATIVE");
			si->funcname = _SC("unknown");
			if(type(_nativeclosure(ci._closure)->_name) == OT_STRING)
				si->funcname = _stringval(_nativeclosure(ci._closure)->_name);
			si->line = -1;
			break;
		default: break; //shutup compiler
		}
		return PS_OK;
	}
	return PS_ERROR;
}

void PSVM::Raise_Error(const PSChar *s, ...)
{
	va_list vl;
	va_start(vl, s);
	PSInteger buffersize = (PSInteger)scstrlen(s)+(NUMBER_MAX_CHAR*2);
	scvsprintf(_sp(rsl(buffersize)),buffersize, s, vl);
	va_end(vl);
	_lasterror = PSString::Create(_ss(this),_spval,-1);
}

void PSVM::Raise_Error(const PSObjectPtr &desc)
{
	_lasterror = desc;
}

PSString *PSVM::PrintObjVal(const PSObjectPtr &o)
{
	switch(type(o)) {
	case OT_STRING: return _string(o);
	case OT_INTEGER:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)), _PRINT_INT_FMT, _integer(o));
		return PSString::Create(_ss(this), _spval);
		break;
	case OT_FLOAT:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+1)), _SC("%.14g"), _float(o));
		return PSString::Create(_ss(this), _spval);
		break;
	default:
		return PSString::Create(_ss(this), GetTypeName(o));
	}
}

void PSVM::Raise_IdxError(const PSObjectPtr &o)
{
	PSObjectPtr oval = PrintObjVal(o);
	Raise_Error(_SC("the index '%.50s' does not exist"), _stringval(oval));
}

void PSVM::Raise_CompareError(const PSObject &o1, const PSObject &o2)
{
	PSObjectPtr oval1 = PrintObjVal(o1), oval2 = PrintObjVal(o2);
	Raise_Error(_SC("comparison between '%.50s' and '%.50s'"), _stringval(oval1), _stringval(oval2));
}


void PSVM::Raise_ParamTypeError(PSInteger nparam,PSInteger typemask,PSInteger type)
{
	PSObjectPtr exptypes = PSString::Create(_ss(this), _SC(""), -1);
	PSInteger found = 0;	
	for(PSInteger i=0; i<16; i++)
	{
		PSInteger mask = 0x00000001 << i;
		if(typemask & (mask)) {
			if(found>0) StringCat(exptypes,PSString::Create(_ss(this), _SC("|"), -1), exptypes);
			found ++;
			StringCat(exptypes,PSString::Create(_ss(this), IdType2Name((PSObjectType)mask), -1), exptypes);
		}
	}
	Raise_Error(_SC("parameter %d has an invalid type '%s' ; expected: '%s'"), nparam, IdType2Name((PSObjectType)type), _stringval(exptypes));
}
