/*
    see copyright notice in pscript.h
*/
#include "pspcheader.h"
#ifndef NO_COMPILER
#include <stdarg.h>
#include <setjmp.h>
#include "psopcodes.h"
#include "psstring.h"
#include "psfuncproto.h"
#include "pscompiler.h"
#include "psfuncstate.h"
#include "pslexer.h"
#include "psvm.h"
#include "pstable.h"

#define EXPR   1
#define OBJECT 2
#define BASE   3
#define LOCAL  4
#define OUTER  5

struct PSExpState {
  PSInteger  etype;       /* expr. type; one of EXPR, OBJECT, BASE, OUTER or LOCAL */
  PSInteger  epos;        /* expr. location on stack; -1 for OBJECT and BASE */
  bool       donot_get;   /* signal not to deref the next value */
};

#define MAX_COMPILER_ERROR_LEN 256

struct PSScope {
    PSInteger outers;
    PSInteger stacksize;
};

#define BEGIN_SCOPE() PSScope __oldscope__ = _scope; \
                     _scope.outers = _fs->_outers; \
                     _scope.stacksize = _fs->GetStackSize();

#define RESOLVE_OUTERS() if(_fs->GetStackSize() != _scope.stacksize) { \
                            if(_fs->CountOuters(_scope.stacksize)) { \
                                _fs->AddInstruction(_OP_CLOSE,0,_scope.stacksize); \
                            } \
                        }

#define END_SCOPE_NO_CLOSE() {  if(_fs->GetStackSize() != _scope.stacksize) { \
                            _fs->SetStackSize(_scope.stacksize); \
                        } \
                        _scope = __oldscope__; \
                    }

#define END_SCOPE() {   PSInteger oldouters = _fs->_outers;\
                        if(_fs->GetStackSize() != _scope.stacksize) { \
                            _fs->SetStackSize(_scope.stacksize); \
                            if(oldouters != _fs->_outers) { \
                                _fs->AddInstruction(_OP_CLOSE,0,_scope.stacksize); \
                            } \
                        } \
                        _scope = __oldscope__; \
                    }

#define BEGIN_BREAKBLE_BLOCK()  PSInteger __nbreaks__=_fs->_unresolvedbreaks.size(); \
                            PSInteger __ncontinues__=_fs->_unresolvedcontinues.size(); \
                            _fs->_breaktargets.push_back(0);_fs->_continuetargets.push_back(0);

#define END_BREAKBLE_BLOCK(continue_target) {__nbreaks__=_fs->_unresolvedbreaks.size()-__nbreaks__; \
                    __ncontinues__=_fs->_unresolvedcontinues.size()-__ncontinues__; \
                    if(__ncontinues__>0)ResolveContinues(_fs,__ncontinues__,continue_target); \
                    if(__nbreaks__>0)ResolveBreaks(_fs,__nbreaks__); \
                    _fs->_breaktargets.pop_back();_fs->_continuetargets.pop_back();}

class PSCompiler
{
public:
    PSCompiler(PSVM *v, PSLEXREADFUNC rg, PSUserPointer up, const PSChar* sourcename, bool raiseerror, bool lineinfo)
    {
        _vm=v;
        _lex.Init(_ss(v), rg, up,ThrowError,this);
        _sourcename = PSString::Create(_ss(v), sourcename);
        _lineinfo = lineinfo;_raiseerror = raiseerror;
        _scope.outers = 0;
        _scope.stacksize = 0;
        _compilererror[0] = _SC('\0');
    }
    static void ThrowError(void *ud, const PSChar *s) {
        PSCompiler *c = (PSCompiler *)ud;
        c->Error(s);
    }
    void Error(const PSChar *s, ...)
    {
        va_list vl;
        va_start(vl, s);
        scvsprintf(_compilererror, MAX_COMPILER_ERROR_LEN, s, vl);
        va_end(vl);
        longjmp(_errorjmp,1);
    }
    void Lex(){ _token = _lex.Lex();}
    PSObject Expect(PSInteger tok)
    {

        if(_token != tok) {
            if(_token == TK_CONSTRUCTOR && tok == TK_IDENTIFIER) {
                //do nothing
            }
            else {
                const PSChar *etypename;
                if(tok > 255) {
                    switch(tok)
                    {
                    case TK_IDENTIFIER:
                        etypename = _SC("IDENTIFIER");
                        break;
                    case TK_STRING_LITERAL:
                        etypename = _SC("STRING_LITERAL");
                        break;
                    case TK_INTEGER:
                        etypename = _SC("INTEGER");
                        break;
                    case TK_FLOAT:
                        etypename = _SC("FLOAT");
                        break;
                    default:
                        etypename = _lex.Tok2Str(tok);
                    }
                    Error(_SC("expected '%s'"), etypename);
                }
                Error(_SC("expected '%c'"), tok);
            }
        }
        PSObjectPtr ret;
        switch(tok)
        {
        case TK_IDENTIFIER:
            ret = _fs->CreateString(_lex._svalue);
            break;
        case TK_STRING_LITERAL:
            ret = _fs->CreateString(_lex._svalue,_lex._longstr.size()-1);
            break;
        case TK_INTEGER:
            ret = PSObjectPtr(_lex._nvalue);
            break;
        case TK_FLOAT:
            ret = PSObjectPtr(_lex._fvalue);
            break;
        }
        Lex();
        return ret;
    }
    bool IsEndOfStatement() { return ((_lex._prevtoken == _SC('\n')) || (_token == PSCRIPT_EOB) || (_token == _SC('}')) || (_token == _SC(';'))); }
    void OptionalSemicolon()
    {
        if(_token == _SC(';')) { Lex(); return; }
        if(!IsEndOfStatement()) {
            Error(_SC("end of statement expected (; or lf)"));
        }
    }
    void MoveIfCurrentTargetIsLocal() {
        PSInteger trg = _fs->TopTarget();
        if(_fs->IsLocal(trg)) {
            trg = _fs->PopTarget(); //pops the target and moves it
            _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), trg);
        }
    }
    bool Compile(PSObjectPtr &o)
    {
        _debugline = 1;
        _debugop = 0;

        PSFuncState funcstate(_ss(_vm), NULL,ThrowError,this);
        funcstate._name = PSString::Create(_ss(_vm), _SC("main"));
        _fs = &funcstate;
        _fs->AddParameter(_fs->CreateString(_SC("this")));
        _fs->AddParameter(_fs->CreateString(_SC("vargv")));
        _fs->_varparams = true;
        _fs->_sourcename = _sourcename;
        PSInteger stacksize = _fs->GetStackSize();
        if(setjmp(_errorjmp) == 0) {
            Lex();
            while(_token > 0){
                Statement();
                if(_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
            }
            _fs->SetStackSize(stacksize);
            _fs->AddLineInfos(_lex._currentline, _lineinfo, true);
            _fs->AddInstruction(_OP_RETURN, 0xFF);
            _fs->SetStackSize(0);
            o =_fs->BuildProto();
#ifdef _DEBUG_DUMP
            _fs->Dump(_funcproto(o));
#endif
        }
        else {
            if(_raiseerror && _ss(_vm)->_compilererrorhandler) {
                _ss(_vm)->_compilererrorhandler(_vm, _compilererror, type(_sourcename) == OT_STRING?_stringval(_sourcename):_SC("unknown"),
                    _lex._currentline, _lex._currentcolumn);
            }
            _vm->_lasterror = PSString::Create(_ss(_vm), _compilererror, -1);
            return false;
        }
        return true;
    }
    void Statements()
    {
        while(_token != _SC('}') && _token != TK_DEFAULT && _token != TK_CASE) {
            Statement();
            if(_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
        }
    }
    void Statement(bool closeframe = true)
    {
        _fs->AddLineInfos(_lex._currentline, _lineinfo);
        switch(_token){
        case _SC(';'):  Lex();                  break;
        case TK_IF:     IfStatement();          break;
        case TK_WHILE:      WhileStatement();       break;
        case TK_DO:     DoWhileStatement();     break;
        case TK_FOR:        ForStatement();         break;
        case TK_FOREACH:    ForEachStatement();     break;
        case TK_SWITCH: SwitchStatement();      break;
        case TK_LOCAL:      LocalDeclStatement();   break;
        case TK_RETURN:
        case TK_YIELD: {
            PSOpcode op;
            if(_token == TK_RETURN) {
                op = _OP_RETURN;
            }
            else {
                op = _OP_YIELD;
                _fs->_bgenerator = true;
            }
            Lex();
            if(!IsEndOfStatement()) {
                PSInteger retexp = _fs->GetCurrentPos()+1;
                CommaExpr();
                if(op == _OP_RETURN && _fs->_traps > 0)
                    _fs->AddInstruction(_OP_POPTRAP, _fs->_traps, 0);
                _fs->_returnexp = retexp;
                _fs->AddInstruction(op, 1, _fs->PopTarget(),_fs->GetStackSize());
            }
            else{
                if(op == _OP_RETURN && _fs->_traps > 0)
                    _fs->AddInstruction(_OP_POPTRAP, _fs->_traps ,0);
                _fs->_returnexp = -1;
                _fs->AddInstruction(op, 0xFF,0,_fs->GetStackSize());
            }
            break;}
        case TK_BREAK:
            if(_fs->_breaktargets.size() <= 0)Error(_SC("'break' has to be in a loop block"));
            if(_fs->_breaktargets.top() > 0){
                _fs->AddInstruction(_OP_POPTRAP, _fs->_breaktargets.top(), 0);
            }
            RESOLVE_OUTERS();
            _fs->AddInstruction(_OP_JMP, 0, -1234);
            _fs->_unresolvedbreaks.push_back(_fs->GetCurrentPos());
            Lex();
            break;
        case TK_CONTINUE:
            if(_fs->_continuetargets.size() <= 0)Error(_SC("'continue' has to be in a loop block"));
            if(_fs->_continuetargets.top() > 0) {
                _fs->AddInstruction(_OP_POPTRAP, _fs->_continuetargets.top(), 0);
            }
            RESOLVE_OUTERS();
            _fs->AddInstruction(_OP_JMP, 0, -1234);
            _fs->_unresolvedcontinues.push_back(_fs->GetCurrentPos());
            Lex();
            break;
        case TK_FUNCTION:
            FunctionStatement();
            break;
        case TK_CLASS:
            ClassStatement();
            break;
        case TK_ENUM:
            EnumStatement();
            break;
        case _SC('{'):{
                BEGIN_SCOPE();
                Lex();
                Statements();
                Expect(_SC('}'));
                if(closeframe) {
                    END_SCOPE();
                }
                else {
                    END_SCOPE_NO_CLOSE();
                }
            }
            break;
        case TK_TRY:
            TryCatchStatement();
            break;
        case TK_THROW:
            Lex();
            CommaExpr();
            _fs->AddInstruction(_OP_THROW, _fs->PopTarget());
            break;
        case TK_CONST:
            {
            Lex();
            PSObject id = Expect(TK_IDENTIFIER);
            Expect('=');
            PSObject val = ExpectScalar();
            OptionalSemicolon();
            PSTable *enums = _table(_ss(_vm)->_consts);
            PSObjectPtr strongid = id;
            enums->NewSlot(strongid,PSObjectPtr(val));
            strongid.Null();
            }
            break;
        default:
            CommaExpr();
            _fs->DiscardTarget();
            //_fs->PopTarget();
            break;
        }
        _fs->SnoozeOpt();
    }
    void EmitDerefOp(PSOpcode op)
    {
        PSInteger val = _fs->PopTarget();
        PSInteger key = _fs->PopTarget();
        PSInteger src = _fs->PopTarget();
        _fs->AddInstruction(op,_fs->PushTarget(),src,key,val);
    }
    void Emit2ArgsOP(PSOpcode op, PSInteger p3 = 0)
    {
        PSInteger p2 = _fs->PopTarget(); //src in OP_GET
        PSInteger p1 = _fs->PopTarget(); //key in OP_GET
        _fs->AddInstruction(op,_fs->PushTarget(), p1, p2, p3);
    }
    void EmitCompoundArith(PSInteger tok, PSInteger etype, PSInteger pos)
    {
        /* Generate code depending on the expression type */
        switch(etype) {
        case LOCAL:{
            PSInteger p2 = _fs->PopTarget(); //src in OP_GET
            PSInteger p1 = _fs->PopTarget(); //key in OP_GET
            _fs->PushTarget(p1);
            //EmitCompArithLocal(tok, p1, p1, p2);
            _fs->AddInstruction(ChooseArithOpByToken(tok),p1, p2, p1, 0);
            _fs->SnoozeOpt();
                   }
            break;
        case OBJECT:
        case BASE:
            {
                PSInteger val = _fs->PopTarget();
                PSInteger key = _fs->PopTarget();
                PSInteger src = _fs->PopTarget();
                /* _OP_COMPARITH mixes dest obj and source val in the arg1 */
                _fs->AddInstruction(_OP_COMPARITH, _fs->PushTarget(), (src<<16)|val, key, ChooseCompArithCharByToken(tok));
            }
            break;
        case OUTER:
            {
                PSInteger val = _fs->TopTarget();
                PSInteger tmp = _fs->PushTarget();
                _fs->AddInstruction(_OP_GETOUTER,   tmp, pos);
                _fs->AddInstruction(ChooseArithOpByToken(tok), tmp, val, tmp, 0);
                _fs->PopTarget();
                _fs->PopTarget();
                _fs->AddInstruction(_OP_SETOUTER, _fs->PushTarget(), pos, tmp);
            }
            break;
        }
    }
    void CommaExpr()
    {
        for(Expression();_token == ',';_fs->PopTarget(), Lex(), CommaExpr());
    }
    void Expression()
    {
         PSExpState es = _es;
        _es.etype     = EXPR;
        _es.epos      = -1;
        _es.donot_get = false;
        LogicalOrExp();
        switch(_token)  {
        case _SC('='):
        case TK_NEWSLOT:
        case TK_MINUSEQ:
        case TK_PLUSEQ:
        case TK_MULEQ:
        case TK_DIVEQ:
        case TK_MODEQ:{
            PSInteger op = _token;
            PSInteger ds = _es.etype;
            PSInteger pos = _es.epos;
            if(ds == EXPR) Error(_SC("can't assign expression"));
            else if(ds == BASE) Error(_SC("'base' cannot be modified"));
            Lex(); Expression();

            switch(op){
            case TK_NEWSLOT:
                if(ds == OBJECT || ds == BASE)
                    EmitDerefOp(_OP_NEWSLOT);
                else //if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
                    Error(_SC("can't 'create' a local slot"));
                break;
            case _SC('='): //ASSIGN
                switch(ds) {
                case LOCAL:
                    {
                        PSInteger src = _fs->PopTarget();
                        PSInteger dst = _fs->TopTarget();
                        _fs->AddInstruction(_OP_MOVE, dst, src);
                    }
                    break;
                case OBJECT:
                case BASE:
                    EmitDerefOp(_OP_SET);
                    break;
                case OUTER:
                    {
                        PSInteger src = _fs->PopTarget();
                        PSInteger dst = _fs->PushTarget();
                        _fs->AddInstruction(_OP_SETOUTER, dst, pos, src);
                    }
                }
                break;
            case TK_MINUSEQ:
            case TK_PLUSEQ:
            case TK_MULEQ:
            case TK_DIVEQ:
            case TK_MODEQ:
                EmitCompoundArith(op, ds, pos);
                break;
            }
            }
            break;
        case _SC('?'): {
            Lex();
            _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
            PSInteger jzpos = _fs->GetCurrentPos();
            PSInteger trg = _fs->PushTarget();
            Expression();
            PSInteger first_exp = _fs->PopTarget();
            if(trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
            PSInteger endfirstexp = _fs->GetCurrentPos();
            _fs->AddInstruction(_OP_JMP, 0, 0);
            Expect(_SC(':'));
            PSInteger jmppos = _fs->GetCurrentPos();
            Expression();
            PSInteger second_exp = _fs->PopTarget();
            if(trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
            _fs->SetIntructionParam(jmppos, 1, _fs->GetCurrentPos() - jmppos);
            _fs->SetIntructionParam(jzpos, 1, endfirstexp - jzpos + 1);
            _fs->SnoozeOpt();
            }
            break;
        }
        _es = es;
    }
    template<typename T> void INVOKE_EXP(T f)
    {
        PSExpState es = _es;
        _es.etype     = EXPR;
        _es.epos      = -1;
        _es.donot_get = false;
        (this->*f)();
        _es = es;
    }
    template<typename T> void BIN_EXP(PSOpcode op, T f,PSInteger op3 = 0)
    {
        Lex();
        INVOKE_EXP(f);
        PSInteger op1 = _fs->PopTarget();PSInteger op2 = _fs->PopTarget();
        _fs->AddInstruction(op, _fs->PushTarget(), op1, op2, op3);
    }
    void LogicalOrExp()
    {
        LogicalAndExp();
        for(;;) if(_token == TK_OR) {
            PSInteger first_exp = _fs->PopTarget();
            PSInteger trg = _fs->PushTarget();
            _fs->AddInstruction(_OP_OR, trg, 0, first_exp, 0);
            PSInteger jpos = _fs->GetCurrentPos();
            if(trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
            Lex(); INVOKE_EXP(&PSCompiler::LogicalOrExp);
            _fs->SnoozeOpt();
            PSInteger second_exp = _fs->PopTarget();
            if(trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
            _fs->SnoozeOpt();
            _fs->SetIntructionParam(jpos, 1, (_fs->GetCurrentPos() - jpos));
            break;
        }else return;
    }
    void LogicalAndExp()
    {
        BitwiseOrExp();
        for(;;) switch(_token) {
        case TK_AND: {
            PSInteger first_exp = _fs->PopTarget();
            PSInteger trg = _fs->PushTarget();
            _fs->AddInstruction(_OP_AND, trg, 0, first_exp, 0);
            PSInteger jpos = _fs->GetCurrentPos();
            if(trg != first_exp) _fs->AddInstruction(_OP_MOVE, trg, first_exp);
            Lex(); INVOKE_EXP(&PSCompiler::LogicalAndExp);
            _fs->SnoozeOpt();
            PSInteger second_exp = _fs->PopTarget();
            if(trg != second_exp) _fs->AddInstruction(_OP_MOVE, trg, second_exp);
            _fs->SnoozeOpt();
            _fs->SetIntructionParam(jpos, 1, (_fs->GetCurrentPos() - jpos));
            break;
            }

        default:
            return;
        }
    }
    void BitwiseOrExp()
    {
        BitwiseXorExp();
        for(;;) if(_token == _SC('|'))
        {BIN_EXP(_OP_BITW, &PSCompiler::BitwiseXorExp,BW_OR);
        }else return;
    }
    void BitwiseXorExp()
    {
        BitwiseAndExp();
        for(;;) if(_token == _SC('^'))
        {BIN_EXP(_OP_BITW, &PSCompiler::BitwiseAndExp,BW_XOR);
        }else return;
    }
    void BitwiseAndExp()
    {
        EqExp();
        for(;;) if(_token == _SC('&'))
        {BIN_EXP(_OP_BITW, &PSCompiler::EqExp,BW_AND);
        }else return;
    }
    void EqExp()
    {
        CompExp();
        for(;;) switch(_token) {
        case TK_EQ: BIN_EXP(_OP_EQ, &PSCompiler::CompExp); break;
        case TK_NE: BIN_EXP(_OP_NE, &PSCompiler::CompExp); break;
        case TK_3WAYSCMP: BIN_EXP(_OP_CMP, &PSCompiler::CompExp,CMP_3W); break;
        default: return;
        }
    }
    void CompExp()
    {
        ShiftExp();
        for(;;) switch(_token) {
        case _SC('>'): BIN_EXP(_OP_CMP, &PSCompiler::ShiftExp,CMP_G); break;
        case _SC('<'): BIN_EXP(_OP_CMP, &PSCompiler::ShiftExp,CMP_L); break;
        case TK_GE: BIN_EXP(_OP_CMP, &PSCompiler::ShiftExp,CMP_GE); break;
        case TK_LE: BIN_EXP(_OP_CMP, &PSCompiler::ShiftExp,CMP_LE); break;
        case TK_IN: BIN_EXP(_OP_EXISTS, &PSCompiler::ShiftExp); break;
        case TK_INSTANCEOF: BIN_EXP(_OP_INSTANCEOF, &PSCompiler::ShiftExp); break;
        default: return;
        }
    }
    void ShiftExp()
    {
        PlusExp();
        for(;;) switch(_token) {
        case TK_USHIFTR: BIN_EXP(_OP_BITW, &PSCompiler::PlusExp,BW_USHIFTR); break;
        case TK_SHIFTL: BIN_EXP(_OP_BITW, &PSCompiler::PlusExp,BW_SHIFTL); break;
        case TK_SHIFTR: BIN_EXP(_OP_BITW, &PSCompiler::PlusExp,BW_SHIFTR); break;
        default: return;
        }
    }
    PSOpcode ChooseArithOpByToken(PSInteger tok)
    {
        switch(tok) {
            case TK_PLUSEQ: case '+': return _OP_ADD;
            case TK_MINUSEQ: case '-': return _OP_SUB;
            case TK_MULEQ: case '*': return _OP_MUL;
            case TK_DIVEQ: case '/': return _OP_DIV;
            case TK_MODEQ: case '%': return _OP_MOD;
            default: assert(0);
        }
        return _OP_ADD;
    }
    PSInteger ChooseCompArithCharByToken(PSInteger tok)
    {
        PSInteger oper;
        switch(tok){
        case TK_MINUSEQ: oper = '-'; break;
        case TK_PLUSEQ: oper = '+'; break;
        case TK_MULEQ: oper = '*'; break;
        case TK_DIVEQ: oper = '/'; break;
        case TK_MODEQ: oper = '%'; break;
        default: oper = 0; //shut up compiler
            assert(0); break;
        };
        return oper;
    }
    void PlusExp()
    {
        MultExp();
        for(;;) switch(_token) {
        case _SC('+'): case _SC('-'):
            BIN_EXP(ChooseArithOpByToken(_token), &PSCompiler::MultExp); break;
        default: return;
        }
    }

    void MultExp()
    {
        PrefixedExpr();
        for(;;) switch(_token) {
        case _SC('*'): case _SC('/'): case _SC('%'):
            BIN_EXP(ChooseArithOpByToken(_token), &PSCompiler::PrefixedExpr); break;
        default: return;
        }
    }
    //if 'pos' != -1 the previous variable is a local variable
    void PrefixedExpr()
    {
        PSInteger pos = Factor();
        for(;;) {
            switch(_token) {
            case _SC('.'):
                pos = -1;
                Lex();

                _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(Expect(TK_IDENTIFIER)));
                if(_es.etype==BASE) {
                    Emit2ArgsOP(_OP_GET);
                    pos = _fs->TopTarget();
                    _es.etype = EXPR;
                    _es.epos   = pos;
                }
                else {
                    if(NeedGet()) {
                        Emit2ArgsOP(_OP_GET);
                    }
                    _es.etype = OBJECT;
                }
                break;
            case _SC('['):
                if(_lex._prevtoken == _SC('\n')) Error(_SC("cannot brake deref/or comma needed after [exp]=exp slot declaration"));
                Lex(); Expression(); Expect(_SC(']'));
                pos = -1;
                if(_es.etype==BASE) {
                    Emit2ArgsOP(_OP_GET);
                    pos = _fs->TopTarget();
                    _es.etype = EXPR;
                    _es.epos   = pos;
                }
                else {
                    if(NeedGet()) {
                        Emit2ArgsOP(_OP_GET);
                    }
                    _es.etype = OBJECT;
                }
                break;
            case TK_MINUSMINUS:
            case TK_PLUSPLUS:
                {
                    if(IsEndOfStatement()) return;
                    PSInteger diff = (_token==TK_MINUSMINUS) ? -1 : 1;
                    Lex();
                    switch(_es.etype)
                    {
                        case EXPR: Error(_SC("can't '++' or '--' an expression")); break;
                        case OBJECT:
                        case BASE:
                            if(_es.donot_get == true)  { Error(_SC("can't '++' or '--' an expression")); break; } //mmh dor this make sense?
                            Emit2ArgsOP(_OP_PINC, diff);
                            break;
                        case LOCAL: {
                            PSInteger src = _fs->PopTarget();
                            _fs->AddInstruction(_OP_PINCL, _fs->PushTarget(), src, 0, diff);
                                    }
                            break;
                        case OUTER: {
                            PSInteger tmp1 = _fs->PushTarget();
                            PSInteger tmp2 = _fs->PushTarget();
                            _fs->AddInstruction(_OP_GETOUTER, tmp2, _es.epos);
                            _fs->AddInstruction(_OP_PINCL,    tmp1, tmp2, 0, diff);
                            _fs->AddInstruction(_OP_SETOUTER, tmp2, _es.epos, tmp2);
                            _fs->PopTarget();
                        }
                    }
                }
                return;
                break;
            case _SC('('):
                switch(_es.etype) {
                    case OBJECT: {
                        PSInteger key     = _fs->PopTarget();  /* location of the key */
                        PSInteger table   = _fs->PopTarget();  /* location of the object */
                        PSInteger closure = _fs->PushTarget(); /* location for the closure */
                        PSInteger ttarget = _fs->PushTarget(); /* location for 'this' pointer */
                        _fs->AddInstruction(_OP_PREPCALL, closure, key, table, ttarget);
                        }
                        break;
                    case BASE:
                        //Emit2ArgsOP(_OP_GET);
                        _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
                        break;
                    case OUTER:
                        _fs->AddInstruction(_OP_GETOUTER, _fs->PushTarget(), _es.epos);
                        _fs->AddInstruction(_OP_MOVE,     _fs->PushTarget(), 0);
                        break;
                    default:
                        _fs->AddInstruction(_OP_MOVE, _fs->PushTarget(), 0);
                }
                _es.etype = EXPR;
                Lex();
                FunctionCallArgs();
                break;
            default: return;
            }
        }
    }
    PSInteger Factor()
    {
        //_es.etype = EXPR;
        switch(_token)
        {
        case TK_STRING_LITERAL:
            _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(_fs->CreateString(_lex._svalue,_lex._longstr.size()-1)));
            Lex();
            break;
        case TK_BASE:
            Lex();
            _fs->AddInstruction(_OP_GETBASE, _fs->PushTarget());
            _es.etype  = BASE;
            _es.epos   = _fs->TopTarget();
            return (_es.epos);
            break;
        case TK_IDENTIFIER:
        case TK_CONSTRUCTOR:
        case TK_THIS:{
                PSObject id;
                PSObject constant;

                switch(_token) {
                    case TK_IDENTIFIER:  id = _fs->CreateString(_lex._svalue);       break;
                    case TK_THIS:        id = _fs->CreateString(_SC("this"),4);        break;
                    case TK_CONSTRUCTOR: id = _fs->CreateString(_SC("constructor"),11); break;
                }

                PSInteger pos = -1;
                Lex();
                if((pos = _fs->GetLocalVariable(id)) != -1) {
                    /* Handle a local variable (includes 'this') */
                    _fs->PushTarget(pos);
                    _es.etype  = LOCAL;
                    _es.epos   = pos;
                }

                else if((pos = _fs->GetOuterVariable(id)) != -1) {
                    /* Handle a free var */
                    if(NeedGet()) {
                        _es.epos  = _fs->PushTarget();
                        _fs->AddInstruction(_OP_GETOUTER, _es.epos, pos);
                        /* _es.etype = EXPR; already default value */
                    }
                    else {
                        _es.etype = OUTER;
                        _es.epos  = pos;
                    }
                }

                else if(_fs->IsConstant(id, constant)) {
                    /* Handle named constant */
                    PSObjectPtr constval;
                    PSObject    constid;
                    if(type(constant) == OT_TABLE) {
                        Expect('.');
                        constid = Expect(TK_IDENTIFIER);
                        if(!_table(constant)->Get(constid, constval)) {
                            constval.Null();
                            Error(_SC("invalid constant [%s.%s]"), _stringval(id), _stringval(constid));
                        }
                    }
                    else {
                        constval = constant;
                    }
                    _es.epos = _fs->PushTarget();

                    /* generate direct or literal function depending on size */
                    PSObjectType ctype = type(constval);
                    switch(ctype) {
                        case OT_INTEGER: EmitLoadConstInt(_integer(constval),_es.epos); break;
                        case OT_FLOAT: EmitLoadConstFloat(_float(constval),_es.epos); break;
                        case OT_BOOL: _fs->AddInstruction(_OP_LOADBOOL, _es.epos, _integer(constval)); break;
                        default: _fs->AddInstruction(_OP_LOAD,_es.epos,_fs->GetConstant(constval)); break;
                    }
                    _es.etype = EXPR;
                }
                else {
                    /* Handle a non-local variable, aka a field. Push the 'this' pointer on
                    * the virtual stack (always found in offset 0, so no instruction needs to
                    * be generated), and push the key next. Generate an _OP_LOAD instruction
                    * for the latter. If we are not using the variable as a dref expr, generate
                    * the _OP_GET instruction.
                    */
                    _fs->PushTarget(0);
                    _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
                    if(NeedGet()) {
                        Emit2ArgsOP(_OP_GET);
                    }
                    _es.etype = OBJECT;
                }
                return _es.epos;
            }
            break;
        case TK_DOUBLE_COLON:  // "::"
            _fs->AddInstruction(_OP_LOADROOT, _fs->PushTarget());
            _es.etype = OBJECT;
            _token = _SC('.'); /* hack: drop into PrefixExpr, case '.'*/
            _es.epos = -1;
            return _es.epos;
            break;
        case TK_NULL:
            _fs->AddInstruction(_OP_LOADNULLS, _fs->PushTarget(),1);
            Lex();
            break;
        case TK_INTEGER: EmitLoadConstInt(_lex._nvalue,-1); Lex();  break;
        case TK_FLOAT: EmitLoadConstFloat(_lex._fvalue,-1); Lex(); break;
        case TK_TRUE: case TK_FALSE:
            _fs->AddInstruction(_OP_LOADBOOL, _fs->PushTarget(),_token == TK_TRUE?1:0);
            Lex();
            break;
        case _SC('['): {
                _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(),0,0,NOT_ARRAY);
                PSInteger apos = _fs->GetCurrentPos(),key = 0;
                Lex();
                while(_token != _SC(']')) {
                    Expression();
                    if(_token == _SC(',')) Lex();
                    PSInteger val = _fs->PopTarget();
                    PSInteger array = _fs->TopTarget();
                    _fs->AddInstruction(_OP_APPENDARRAY, array, val, AAT_STACK);
                    key++;
                }
                _fs->SetIntructionParam(apos, 1, key);
                Lex();
            }
            break;
        case _SC('{'):
            _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(),0,NOT_TABLE);
            Lex();ParseTableOrClass(_SC(','),_SC('}'));
            break;
        case TK_FUNCTION: FunctionExp(_token);break;
        case _SC('@'): FunctionExp(_token,true);break;
        case TK_CLASS: Lex(); ClassExp();break;
        case _SC('-'):
            Lex();
            switch(_token) {
            case TK_INTEGER: EmitLoadConstInt(-_lex._nvalue,-1); Lex(); break;
            case TK_FLOAT: EmitLoadConstFloat(-_lex._fvalue,-1); Lex(); break;
            default: UnaryOP(_OP_NEG);
            }
            break;
        case _SC('!'): Lex(); UnaryOP(_OP_NOT); break;
        case _SC('~'):
            Lex();
            if(_token == TK_INTEGER)  { EmitLoadConstInt(~_lex._nvalue,-1); Lex(); break; }
            UnaryOP(_OP_BWNOT);
            break;
        case TK_TYPEOF : Lex() ;UnaryOP(_OP_TYPEOF); break;
        case TK_RESUME : Lex(); UnaryOP(_OP_RESUME); break;
        case TK_CLONE : Lex(); UnaryOP(_OP_CLONE); break;
        case TK_MINUSMINUS :
        case TK_PLUSPLUS :PrefixIncDec(_token); break;
        case TK_DELETE : DeleteExpr(); break;
        case _SC('('): Lex(); CommaExpr(); Expect(_SC(')'));
            break;
        case TK___LINE__: EmitLoadConstInt(_lex._currentline,-1); Lex(); break;
        case TK___FILE__: _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(_sourcename)); Lex(); break;
        default: Error(_SC("expression expected"));
        }
        _es.etype = EXPR;
        return -1;
    }
    void EmitLoadConstInt(PSInteger value,PSInteger target)
    {
        if(target < 0) {
            target = _fs->PushTarget();
        }
        if(value <= INT_MAX && value > INT_MIN) { //does it fit in 32 bits?
            _fs->AddInstruction(_OP_LOADINT, target,value);
        }
        else {
            _fs->AddInstruction(_OP_LOAD, target, _fs->GetNumericConstant(value));
        }
    }
    void EmitLoadConstFloat(PSFloat value,PSInteger target)
    {
        if(target < 0) {
            target = _fs->PushTarget();
        }
        if(sizeof(PSFloat) == sizeof(PSInt32)) {
            _fs->AddInstruction(_OP_LOADFLOAT, target,*((PSInt32 *)&value));
        }
        else {
            _fs->AddInstruction(_OP_LOAD, target, _fs->GetNumericConstant(value));
        }
    }
    void UnaryOP(PSOpcode op)
    {
        PrefixedExpr();
        PSInteger src = _fs->PopTarget();
        _fs->AddInstruction(op, _fs->PushTarget(), src);
    }
    bool NeedGet()
    {
        switch(_token) {
        case _SC('='): case _SC('('): case TK_NEWSLOT: case TK_MODEQ: case TK_MULEQ:
        case TK_DIVEQ: case TK_MINUSEQ: case TK_PLUSEQ:
            return false;
        case TK_PLUSPLUS: case TK_MINUSMINUS:
            if (!IsEndOfStatement()) {
                return false;
            }
        break;
        }
        return (!_es.donot_get || ( _es.donot_get && (_token == _SC('.') || _token == _SC('['))));
    }
    void FunctionCallArgs()
    {
        PSInteger nargs = 1;//this
         while(_token != _SC(')')) {
             Expression();
             MoveIfCurrentTargetIsLocal();
             nargs++;
             if(_token == _SC(',')){
                 Lex();
                 if(_token == ')') Error(_SC("expression expected, found ')'"));
             }
         }
         Lex();
         for(PSInteger i = 0; i < (nargs - 1); i++) _fs->PopTarget();
         PSInteger stackbase = _fs->PopTarget();
         PSInteger closure = _fs->PopTarget();
         _fs->AddInstruction(_OP_CALL, _fs->PushTarget(), closure, stackbase, nargs);
    }
    void ParseTableOrClass(PSInteger separator,PSInteger terminator)
    {
        PSInteger tpos = _fs->GetCurrentPos(),nkeys = 0;
        while(_token != terminator) {
            bool hasattrs = false;
            bool isstatic = false;
            //check if is an attribute
            if(separator == ';') {
                if(_token == TK_ATTR_OPEN) {
                    _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(),0,NOT_TABLE); Lex();
                    ParseTableOrClass(',',TK_ATTR_CLOSE);
                    hasattrs = true;
                }
                if(_token == TK_STATIC) {
                    isstatic = true;
                    Lex();
                }
            }
            switch(_token) {
            case TK_FUNCTION:
            case TK_CONSTRUCTOR:{
                PSInteger tk = _token;
                Lex();
                PSObject id = tk == TK_FUNCTION ? Expect(TK_IDENTIFIER) : _fs->CreateString(_SC("constructor"));
                Expect(_SC('('));
                _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
                CreateFunction(id);
                _fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, 0);
                                }
                                break;
            case _SC('['):
                Lex(); CommaExpr(); Expect(_SC(']'));
                Expect(_SC('=')); Expression();
                break;
            case TK_STRING_LITERAL: //JSON
                if(separator == ',') { //only works for tables
                    _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(Expect(TK_STRING_LITERAL)));
                    Expect(_SC(':')); Expression();
                    break;
                }
            default :
                _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(Expect(TK_IDENTIFIER)));
                Expect(_SC('=')); Expression();
            }
            if(_token == separator) Lex();//optional comma/semicolon
            nkeys++;
            PSInteger val = _fs->PopTarget();
            PSInteger key = _fs->PopTarget();
            PSInteger attrs = hasattrs ? _fs->PopTarget():-1;
            ((void)attrs);
            assert((hasattrs && (attrs == key-1)) || !hasattrs);
            unsigned char flags = (hasattrs?NEW_SLOT_ATTRIBUTES_FLAG:0)|(isstatic?NEW_SLOT_STATIC_FLAG:0);
            PSInteger table = _fs->TopTarget(); //<<BECAUSE OF THIS NO COMMON EMIT FUNC IS POSSIBLE
            if(separator == _SC(',')) { //hack recognizes a table from the separator
                _fs->AddInstruction(_OP_NEWSLOT, 0xFF, table, key, val);
            }
            else {
                _fs->AddInstruction(_OP_NEWSLOTA, flags, table, key, val); //this for classes only as it invokes _newmember
            }
        }
        if(separator == _SC(',')) //hack recognizes a table from the separator
            _fs->SetIntructionParam(tpos, 1, nkeys);
        Lex();
    }
    void LocalDeclStatement()
    {
        PSObject varname;
        Lex();
        if( _token == TK_FUNCTION) {
            Lex();
            varname = Expect(TK_IDENTIFIER);
            Expect(_SC('('));
            CreateFunction(varname,false);
            _fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, 0);
            _fs->PopTarget();
            _fs->PushLocalVariable(varname);
            return;
        }

        do {
            varname = Expect(TK_IDENTIFIER);
            if(_token == _SC('=')) {
                Lex(); Expression();
                PSInteger src = _fs->PopTarget();
                PSInteger dest = _fs->PushTarget();
                if(dest != src) _fs->AddInstruction(_OP_MOVE, dest, src);
            }
            else{
                _fs->AddInstruction(_OP_LOADNULLS, _fs->PushTarget(),1);
            }
            _fs->PopTarget();
            _fs->PushLocalVariable(varname);
            if(_token == _SC(',')) Lex(); else break;
        } while(1);
    }
    void IfBlock()
    {
        if (_token == _SC('{'))
        {
            BEGIN_SCOPE();
            Lex();
            Statements();
            Expect(_SC('}'));
            if (true) {
                END_SCOPE();
            }
            else {
                END_SCOPE_NO_CLOSE();
            }
        }
        else {
            //BEGIN_SCOPE();
            Statement();
            if (_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
            //END_SCOPE();
        }
    }
    void IfStatement()
    {
        PSInteger jmppos;
        bool haselse = false;
        Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
        _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
        PSInteger jnepos = _fs->GetCurrentPos();



        IfBlock();
        //
        /*static int n = 0;
        if (_token != _SC('}') && _token != TK_ELSE) {
            printf("IF %d-----------------------!!!!!!!!!\n", n);
            if (n == 5)
            {
                printf("asd");
            }
            n++;
            //OptionalSemicolon();
        }*/


        PSInteger endifblock = _fs->GetCurrentPos();
        if(_token == TK_ELSE){
            haselse = true;
            //BEGIN_SCOPE();
            _fs->AddInstruction(_OP_JMP);
            jmppos = _fs->GetCurrentPos();
            Lex();
            //Statement(); if(_lex._prevtoken != _SC('}')) OptionalSemicolon();
            IfBlock();
            //END_SCOPE();
            _fs->SetIntructionParam(jmppos, 1, _fs->GetCurrentPos() - jmppos);
        }
        _fs->SetIntructionParam(jnepos, 1, endifblock - jnepos + (haselse?1:0));
    }
    void WhileStatement()
    {
        PSInteger jzpos, jmppos;
        jmppos = _fs->GetCurrentPos();
        Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));

        BEGIN_BREAKBLE_BLOCK();
        _fs->AddInstruction(_OP_JZ, _fs->PopTarget());
        jzpos = _fs->GetCurrentPos();
        BEGIN_SCOPE();

        Statement();

        END_SCOPE();
        _fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1);
        _fs->SetIntructionParam(jzpos, 1, _fs->GetCurrentPos() - jzpos);

        END_BREAKBLE_BLOCK(jmppos);
    }
    void DoWhileStatement()
    {
        Lex();
        PSInteger jmptrg = _fs->GetCurrentPos();
        BEGIN_BREAKBLE_BLOCK()
        BEGIN_SCOPE();
        Statement();
        END_SCOPE();
        Expect(TK_WHILE);
        PSInteger continuetrg = _fs->GetCurrentPos();
        Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
        _fs->AddInstruction(_OP_JZ, _fs->PopTarget(), 1);
        _fs->AddInstruction(_OP_JMP, 0, jmptrg - _fs->GetCurrentPos() - 1);
        END_BREAKBLE_BLOCK(continuetrg);
    }
    void ForStatement()
    {
        Lex();
        BEGIN_SCOPE();
        Expect(_SC('('));
        if(_token == TK_LOCAL) LocalDeclStatement();
        else if(_token != _SC(';')){
            CommaExpr();
            _fs->PopTarget();
        }
        Expect(_SC(';'));
        _fs->SnoozeOpt();
        PSInteger jmppos = _fs->GetCurrentPos();
        PSInteger jzpos = -1;
        if(_token != _SC(';')) { CommaExpr(); _fs->AddInstruction(_OP_JZ, _fs->PopTarget()); jzpos = _fs->GetCurrentPos(); }
        Expect(_SC(';'));
        _fs->SnoozeOpt();
        PSInteger expstart = _fs->GetCurrentPos() + 1;
        if(_token != _SC(')')) {
            CommaExpr();
            _fs->PopTarget();
        }
        Expect(_SC(')'));
        _fs->SnoozeOpt();
        PSInteger expend = _fs->GetCurrentPos();
        PSInteger expsize = (expend - expstart) + 1;
        PSInstructionVec exp;
        if(expsize > 0) {
            for(PSInteger i = 0; i < expsize; i++)
                exp.push_back(_fs->GetInstruction(expstart + i));
            _fs->PopInstructions(expsize);
        }
        BEGIN_BREAKBLE_BLOCK()
        Statement();
        PSInteger continuetrg = _fs->GetCurrentPos();
        if(expsize > 0) {
            for(PSInteger i = 0; i < expsize; i++)
                _fs->AddInstruction(exp[i]);
        }
        _fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1, 0);
        if(jzpos>  0) _fs->SetIntructionParam(jzpos, 1, _fs->GetCurrentPos() - jzpos);
        END_SCOPE();

        END_BREAKBLE_BLOCK(continuetrg);
    }
    void ForEachStatement()
    {
        PSObject idxname, valname;
        Lex(); Expect(_SC('(')); valname = Expect(TK_IDENTIFIER);
        if(_token == _SC(',')) {
            idxname = valname;
            Lex(); valname = Expect(TK_IDENTIFIER);
        }
        else{
            idxname = _fs->CreateString(_SC("@INDEX@"));
        }
        Expect(TK_IN);

        //save the stack size
        BEGIN_SCOPE();
        //put the table in the stack(evaluate the table expression)
        Expression(); Expect(_SC(')'));
        PSInteger container = _fs->TopTarget();
        //push the index local var
        PSInteger indexpos = _fs->PushLocalVariable(idxname);
        _fs->AddInstruction(_OP_LOADNULLS, indexpos,1);
        //push the value local var
        PSInteger valuepos = _fs->PushLocalVariable(valname);
        _fs->AddInstruction(_OP_LOADNULLS, valuepos,1);
        //push reference index
        PSInteger itrpos = _fs->PushLocalVariable(_fs->CreateString(_SC("@ITERATOR@"))); //use invalid id to make it inaccessible
        _fs->AddInstruction(_OP_LOADNULLS, itrpos,1);
        PSInteger jmppos = _fs->GetCurrentPos();
        _fs->AddInstruction(_OP_FOREACH, container, 0, indexpos);
        PSInteger foreachpos = _fs->GetCurrentPos();
        _fs->AddInstruction(_OP_POSTFOREACH, container, 0, indexpos);
        //generate the statement code
        BEGIN_BREAKBLE_BLOCK()
        Statement();
        _fs->AddInstruction(_OP_JMP, 0, jmppos - _fs->GetCurrentPos() - 1);
        _fs->SetIntructionParam(foreachpos, 1, _fs->GetCurrentPos() - foreachpos);
        _fs->SetIntructionParam(foreachpos + 1, 1, _fs->GetCurrentPos() - foreachpos);
        END_BREAKBLE_BLOCK(foreachpos - 1);
        //restore the local variable stack(remove index,val and ref idx)
        _fs->PopTarget();
        END_SCOPE();
    }
    void SwitchStatement()
    {
        Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
        Expect(_SC('{'));
        PSInteger expr = _fs->TopTarget();
        bool bfirst = true;
        PSInteger tonextcondjmp = -1;
        PSInteger skipcondjmp = -1;
        PSInteger __nbreaks__ = _fs->_unresolvedbreaks.size();
        _fs->_breaktargets.push_back(0);
        while(_token == TK_CASE) {
            if(!bfirst) {
                _fs->AddInstruction(_OP_JMP, 0, 0);
                skipcondjmp = _fs->GetCurrentPos();
                _fs->SetIntructionParam(tonextcondjmp, 1, _fs->GetCurrentPos() - tonextcondjmp);
            }
            //condition
            Lex(); Expression(); Expect(_SC(':'));
            PSInteger trg = _fs->PopTarget();
            PSInteger eqtarget = trg;
            bool local = _fs->IsLocal(trg);
            if(local) {
                eqtarget = _fs->PushTarget(); //we need to allocate a extra reg
            }
            _fs->AddInstruction(_OP_EQ, eqtarget, trg, expr);
            _fs->AddInstruction(_OP_JZ, eqtarget, 0);
            if(local) {
                _fs->PopTarget();
            }

            //end condition
            if(skipcondjmp != -1) {
                _fs->SetIntructionParam(skipcondjmp, 1, (_fs->GetCurrentPos() - skipcondjmp));
            }
            tonextcondjmp = _fs->GetCurrentPos();
            BEGIN_SCOPE();
            Statements();
            END_SCOPE();
            bfirst = false;
        }
        if(tonextcondjmp != -1)
            _fs->SetIntructionParam(tonextcondjmp, 1, _fs->GetCurrentPos() - tonextcondjmp);
        if(_token == TK_DEFAULT) {
            Lex(); Expect(_SC(':'));
            BEGIN_SCOPE();
            Statements();
            END_SCOPE();
        }
        Expect(_SC('}'));
        _fs->PopTarget();
        __nbreaks__ = _fs->_unresolvedbreaks.size() - __nbreaks__;
        if(__nbreaks__ > 0)ResolveBreaks(_fs, __nbreaks__);
        _fs->_breaktargets.pop_back();
    }
    void FunctionStatement()
    {
        PSObject id;
        Lex(); id = Expect(TK_IDENTIFIER);
        _fs->PushTarget(0);
        _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
        if(_token == TK_DOUBLE_COLON) Emit2ArgsOP(_OP_GET);

        while(_token == TK_DOUBLE_COLON) {
            Lex();
            id = Expect(TK_IDENTIFIER);
            _fs->AddInstruction(_OP_LOAD, _fs->PushTarget(), _fs->GetConstant(id));
            if(_token == TK_DOUBLE_COLON) Emit2ArgsOP(_OP_GET);
        }
        Expect(_SC('('));
        CreateFunction(id);
        _fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, 0);
        EmitDerefOp(_OP_NEWSLOT);
        _fs->PopTarget();
    }
    void ClassStatement()
    {
        PSExpState es;
        Lex();
        es = _es;
        _es.donot_get = true;
        PrefixedExpr();
        if(_es.etype == EXPR) {
            Error(_SC("invalid class name"));
        }
        else if(_es.etype == OBJECT || _es.etype == BASE) {
            ClassExp();
            EmitDerefOp(_OP_NEWSLOT);
            _fs->PopTarget();
        }
        else {
            Error(_SC("cannot create a class in a local with the syntax(class <local>)"));
        }
        _es = es;
    }
    PSObject ExpectScalar()
    {
        PSObject val;
        val._type = OT_NULL; val._unVal.nInteger = 0; //shut up GCC 4.x
        switch(_token) {
            case TK_INTEGER:
                val._type = OT_INTEGER;
                val._unVal.nInteger = _lex._nvalue;
                break;
            case TK_FLOAT:
                val._type = OT_FLOAT;
                val._unVal.fFloat = _lex._fvalue;
                break;
            case TK_STRING_LITERAL:
                val = _fs->CreateString(_lex._svalue,_lex._longstr.size()-1);
                break;
            case TK_TRUE:
            case TK_FALSE:
                val._type = OT_BOOL;
                val._unVal.nInteger = _token == TK_TRUE ? 1 : 0;
                break;
            case '-':
                Lex();
                switch(_token)
                {
                case TK_INTEGER:
                    val._type = OT_INTEGER;
                    val._unVal.nInteger = -_lex._nvalue;
                break;
                case TK_FLOAT:
                    val._type = OT_FLOAT;
                    val._unVal.fFloat = -_lex._fvalue;
                break;
                default:
                    Error(_SC("scalar expected : integer,float"));
                }
                break;
            default:
                Error(_SC("scalar expected : integer,float or string"));
        }
        Lex();
        return val;
    }
    void EnumStatement()
    {
        Lex();
        PSObject id = Expect(TK_IDENTIFIER);
        Expect(_SC('{'));

        PSObject table = _fs->CreateTable();
        PSInteger nval = 0;
        while(_token != _SC('}')) {
            PSObject key = Expect(TK_IDENTIFIER);
            PSObject val;
            if(_token == _SC('=')) {
                Lex();
                val = ExpectScalar();
            }
            else {
                val._type = OT_INTEGER;
                val._unVal.nInteger = nval++;
            }
            _table(table)->NewSlot(PSObjectPtr(key),PSObjectPtr(val));
            if(_token == ',') Lex();
        }
        PSTable *enums = _table(_ss(_vm)->_consts);
        PSObjectPtr strongid = id;
        enums->NewSlot(PSObjectPtr(strongid),PSObjectPtr(table));
        strongid.Null();
        Lex();
    }
    void TryCatchStatement()
    {
        PSObject exid;
        Lex();
        _fs->AddInstruction(_OP_PUSHTRAP,0,0);
        _fs->_traps++;
        if(_fs->_breaktargets.size()) _fs->_breaktargets.top()++;
        if(_fs->_continuetargets.size()) _fs->_continuetargets.top()++;
        PSInteger trappos = _fs->GetCurrentPos();
        {
            BEGIN_SCOPE();
            Statement();
            END_SCOPE();
        }
        _fs->_traps--;
        _fs->AddInstruction(_OP_POPTRAP, 1, 0);
        if(_fs->_breaktargets.size()) _fs->_breaktargets.top()--;
        if(_fs->_continuetargets.size()) _fs->_continuetargets.top()--;
        _fs->AddInstruction(_OP_JMP, 0, 0);
        PSInteger jmppos = _fs->GetCurrentPos();
        _fs->SetIntructionParam(trappos, 1, (_fs->GetCurrentPos() - trappos));
        Expect(TK_CATCH); Expect(_SC('(')); exid = Expect(TK_IDENTIFIER); Expect(_SC(')'));
        {
            BEGIN_SCOPE();
            PSInteger ex_target = _fs->PushLocalVariable(exid);
            _fs->SetIntructionParam(trappos, 0, ex_target);
            Statement();
            _fs->SetIntructionParams(jmppos, 0, (_fs->GetCurrentPos() - jmppos), 0);
            END_SCOPE();
        }
    }
    void FunctionExp(PSInteger ftype,bool lambda = false)
    {
        Lex(); Expect(_SC('('));
        PSObjectPtr dummy;
        CreateFunction(dummy,lambda);
        _fs->AddInstruction(_OP_CLOSURE, _fs->PushTarget(), _fs->_functions.size() - 1, ftype == TK_FUNCTION?0:1);
    }
    void ClassExp()
    {
        PSInteger base = -1;
        PSInteger attrs = -1;
        if(_token == TK_EXTENDS) {
            Lex(); Expression();
            base = _fs->TopTarget();
        }
        if(_token == TK_ATTR_OPEN) {
            Lex();
            _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(),0,NOT_TABLE);
            ParseTableOrClass(_SC(','),TK_ATTR_CLOSE);
            attrs = _fs->TopTarget();
        }
        Expect(_SC('{'));
        if(attrs != -1) _fs->PopTarget();
        if(base != -1) _fs->PopTarget();
        _fs->AddInstruction(_OP_NEWOBJ, _fs->PushTarget(), base, attrs,NOT_CLASS);
        ParseTableOrClass(_SC(';'),_SC('}'));
    }
    void DeleteExpr()
    {
        PSExpState es;
        Lex();
        es = _es;
        _es.donot_get = true;
        PrefixedExpr();
        if(_es.etype==EXPR) Error(_SC("can't delete an expression"));
        if(_es.etype==OBJECT || _es.etype==BASE) {
            Emit2ArgsOP(_OP_DELETE);
        }
        else {
            Error(_SC("cannot delete an (outer) local"));
        }
        _es = es;
    }
    void PrefixIncDec(PSInteger token)
    {
        PSExpState  es;
        PSInteger diff = (token==TK_MINUSMINUS) ? -1 : 1;
        Lex();
        es = _es;
        _es.donot_get = true;
        PrefixedExpr();
        if(_es.etype==EXPR) {
            Error(_SC("can't '++' or '--' an expression"));
        }
        else if(_es.etype==OBJECT || _es.etype==BASE) {
            Emit2ArgsOP(_OP_INC, diff);
        }
        else if(_es.etype==LOCAL) {
            PSInteger src = _fs->TopTarget();
            _fs->AddInstruction(_OP_INCL, src, src, 0, diff);

        }
        else if(_es.etype==OUTER) {
            PSInteger tmp = _fs->PushTarget();
            _fs->AddInstruction(_OP_GETOUTER, tmp, _es.epos);
            _fs->AddInstruction(_OP_INCL,     tmp, tmp, 0, diff);
            _fs->AddInstruction(_OP_SETOUTER, tmp, _es.epos, tmp);
        }
        _es = es;
    }
    void CreateFunction(PSObject &name,bool lambda = false)
    {
        PSFuncState *funcstate = _fs->PushChildState(_ss(_vm));
        funcstate->_name = name;
        PSObject paramname;
        funcstate->AddParameter(_fs->CreateString(_SC("this")));
        funcstate->_sourcename = _sourcename;
        PSInteger defparams = 0;
        while(_token!=_SC(')')) {
            if(_token == TK_VARPARAMS) {
                if(defparams > 0) Error(_SC("function with default parameters cannot have variable number of parameters"));
                funcstate->AddParameter(_fs->CreateString(_SC("vargv")));
                funcstate->_varparams = true;
                Lex();
                if(_token != _SC(')')) Error(_SC("expected ')'"));
                break;
            }
            else {
                paramname = Expect(TK_IDENTIFIER);
                funcstate->AddParameter(paramname);
                if(_token == _SC('=')) {
                    Lex();
                    Expression();
                    funcstate->AddDefaultParam(_fs->TopTarget());
                    defparams++;
                }
                else {
                    if(defparams > 0) Error(_SC("expected '='"));
                }
                if(_token == _SC(',')) Lex();
                else if(_token != _SC(')')) Error(_SC("expected ')' or ','"));
            }
        }
        Expect(_SC(')'));
        for(PSInteger n = 0; n < defparams; n++) {
            _fs->PopTarget();
        }

        PSFuncState *currchunk = _fs;
        _fs = funcstate;
        if(lambda) {
            Expression();
            _fs->AddInstruction(_OP_RETURN, 1, _fs->PopTarget());}
        else {
            Statement(false);
        }
        funcstate->AddLineInfos(_lex._prevtoken == _SC('\n')?_lex._lasttokenline:_lex._currentline, _lineinfo, true);
        funcstate->AddInstruction(_OP_RETURN, -1);
        funcstate->SetStackSize(0);

        PSFunctionProto *func = funcstate->BuildProto();
#ifdef _DEBUG_DUMP
        funcstate->Dump(func);
#endif
        _fs = currchunk;
        _fs->_functions.push_back(func);
        _fs->PopChildState();
    }
    void ResolveBreaks(PSFuncState *funcstate, PSInteger ntoresolve)
    {
        while(ntoresolve > 0) {
            PSInteger pos = funcstate->_unresolvedbreaks.back();
            funcstate->_unresolvedbreaks.pop_back();
            //set the jmp instruction
            funcstate->SetIntructionParams(pos, 0, funcstate->GetCurrentPos() - pos, 0);
            ntoresolve--;
        }
    }
    void ResolveContinues(PSFuncState *funcstate, PSInteger ntoresolve, PSInteger targetpos)
    {
        while(ntoresolve > 0) {
            PSInteger pos = funcstate->_unresolvedcontinues.back();
            funcstate->_unresolvedcontinues.pop_back();
            //set the jmp instruction
            funcstate->SetIntructionParams(pos, 0, targetpos - pos, 0);
            ntoresolve--;
        }
    }
private:
    PSInteger _token;
    PSFuncState *_fs;
    PSObjectPtr _sourcename;
    PSLexer _lex;
    bool _lineinfo;
    bool _raiseerror;
    PSInteger _debugline;
    PSInteger _debugop;
    PSExpState   _es;
    PSScope _scope;
    PSChar _compilererror[MAX_COMPILER_ERROR_LEN];
    jmp_buf _errorjmp;
    PSVM *_vm;
};

bool Compile(PSVM *vm,PSLEXREADFUNC rg, PSUserPointer up, const PSChar *sourcename, PSObjectPtr &out, bool raiseerror, bool lineinfo)
{
    PSCompiler p(vm, rg, up, sourcename, raiseerror, lineinfo);
    return p.Compile(out);
}

#endif
