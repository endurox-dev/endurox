/*
    see copyright notice in pscript.h
*/
#include "pspcheader.h"
#include <ctype.h>
#include <stdlib.h>
#include "pstable.h"
#include "psstring.h"
#include "pscompiler.h"
#include "pslexer.h"

#define CUR_CHAR (_currdata)
#define RETURN_TOKEN(t) { _prevtoken = _curtoken; _curtoken = t; return t;}
#define IS_EOB() (CUR_CHAR <= PSCRIPT_EOB)
#define NEXT() {Next();_currentcolumn++;}
#define INIT_TEMP_STRING() { _longstr.resize(0);}
#define APPEND_CHAR(c) { _longstr.push_back(c);}
#define TERMINATE_BUFFER() {_longstr.push_back(_SC('\0'));}
#define ADD_KEYWORD(key,id) _keywords->NewSlot( PSString::Create(ss, _SC(#key)) ,PSInteger(id))

PSLexer::PSLexer(){}
PSLexer::~PSLexer()
{
    _keywords->Release();
}

void PSLexer::Init(PSSharedState *ss, PSLEXREADFUNC rg, PSUserPointer up,CompilerErrorFunc efunc,void *ed)
{
    _errfunc = efunc;
    _errtarget = ed;
    _sharedstate = ss;
    _keywords = PSTable::Create(ss, 37);
    ADD_KEYWORD(while, TK_WHILE);
    ADD_KEYWORD(do, TK_DO);
    ADD_KEYWORD(if, TK_IF);
    ADD_KEYWORD(else, TK_ELSE);
    ADD_KEYWORD(break, TK_BREAK);
    ADD_KEYWORD(continue, TK_CONTINUE);
    ADD_KEYWORD(return, TK_RETURN);
    ADD_KEYWORD(null, TK_NULL);
    ADD_KEYWORD(function, TK_FUNCTION);
    ADD_KEYWORD(local, TK_LOCAL);
    ADD_KEYWORD(for, TK_FOR);
    ADD_KEYWORD(foreach, TK_FOREACH);
    ADD_KEYWORD(in, TK_IN);
    ADD_KEYWORD(typeof, TK_TYPEOF);
    ADD_KEYWORD(base, TK_BASE);
    ADD_KEYWORD(delete, TK_DELETE);
    ADD_KEYWORD(try, TK_TRY);
    ADD_KEYWORD(catch, TK_CATCH);
    ADD_KEYWORD(throw, TK_THROW);
    ADD_KEYWORD(clone, TK_CLONE);
    ADD_KEYWORD(yield, TK_YIELD);
    ADD_KEYWORD(resume, TK_RESUME);
    ADD_KEYWORD(switch, TK_SWITCH);
    ADD_KEYWORD(case, TK_CASE);
    ADD_KEYWORD(default, TK_DEFAULT);
    ADD_KEYWORD(this, TK_THIS);
    ADD_KEYWORD(class,TK_CLASS);
    ADD_KEYWORD(extends,TK_EXTENDS);
    ADD_KEYWORD(constructor,TK_CONSTRUCTOR);
    ADD_KEYWORD(instanceof,TK_INSTANCEOF);
    ADD_KEYWORD(true,TK_TRUE);
    ADD_KEYWORD(false,TK_FALSE);
    ADD_KEYWORD(static,TK_STATIC);
    ADD_KEYWORD(enum,TK_ENUM);
    ADD_KEYWORD(const,TK_CONST);
    ADD_KEYWORD(__LINE__,TK___LINE__);
    ADD_KEYWORD(__FILE__,TK___FILE__);

    _readf = rg;
    _up = up;
    _lasttokenline = _currentline = 1;
    _currentcolumn = 0;
    _prevtoken = -1;
    _reached_eof = PSFalse;
    Next();
}

void PSLexer::Error(const PSChar *err)
{
    _errfunc(_errtarget,err);
}

void PSLexer::Next()
{
    PSInteger t = _readf(_up);
    if(t > MAX_CHAR) Error(_SC("Invalid character"));
    if(t != 0) {
        _currdata = (LexChar)t;
        return;
    }
    _currdata = PSCRIPT_EOB;
    _reached_eof = PSTrue;
}

const PSChar *PSLexer::Tok2Str(PSInteger tok)
{
    PSObjectPtr itr, key, val;
    PSInteger nitr;
    while((nitr = _keywords->Next(false,itr, key, val)) != -1) {
        itr = (PSInteger)nitr;
        if(((PSInteger)_integer(val)) == tok)
            return _stringval(key);
    }
    return NULL;
}

void PSLexer::LexBlockComment()
{
    bool done = false;
    while(!done) {
        switch(CUR_CHAR) {
            case _SC('*'): { NEXT(); if(CUR_CHAR == _SC('/')) { done = true; NEXT(); }}; continue;
            case _SC('\n'): _currentline++; NEXT(); continue;
            case PSCRIPT_EOB: Error(_SC("missing \"*/\" in comment"));
            default: NEXT();
        }
    }
}
void PSLexer::LexLineComment()
{
    do { NEXT(); } while (CUR_CHAR != _SC('\n') && (!IS_EOB()));
}

PSInteger PSLexer::Lex()
{
    _lasttokenline = _currentline;
    while(CUR_CHAR != PSCRIPT_EOB) {
        switch(CUR_CHAR){
        case _SC('\t'): case _SC('\r'): case _SC(' '): NEXT(); continue;
        case _SC('\n'):
            _currentline++;
            _prevtoken=_curtoken;
            _curtoken=_SC('\n');
            NEXT();
            _currentcolumn=1;
            continue;
        case _SC('#'): LexLineComment(); continue;
        case _SC('/'):
            NEXT();
            switch(CUR_CHAR){
            case _SC('*'):
                NEXT();
                LexBlockComment();
                continue;
            case _SC('/'):
                LexLineComment();
                continue;
            case _SC('='):
                NEXT();
                RETURN_TOKEN(TK_DIVEQ);
                continue;
            case _SC('>'):
                NEXT();
                RETURN_TOKEN(TK_ATTR_CLOSE);
                continue;
            default:
                RETURN_TOKEN('/');
            }
        case _SC('='):
            NEXT();
            if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('=') }
            else { NEXT(); RETURN_TOKEN(TK_EQ); }
        case _SC('<'):
            NEXT();
            switch(CUR_CHAR) {
            case _SC('='):
                NEXT();
                if(CUR_CHAR == _SC('>')) {
                    NEXT();
                    RETURN_TOKEN(TK_3WAYSCMP);
                }
                RETURN_TOKEN(TK_LE)
                break;
            case _SC('-'): NEXT(); RETURN_TOKEN(TK_NEWSLOT); break;
            case _SC('<'): NEXT(); RETURN_TOKEN(TK_SHIFTL); break;
            case _SC('/'): NEXT(); RETURN_TOKEN(TK_ATTR_OPEN); break;
            }
            RETURN_TOKEN('<');
        case _SC('>'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_GE);}
            else if(CUR_CHAR == _SC('>')){
                NEXT();
                if(CUR_CHAR == _SC('>')){
                    NEXT();
                    RETURN_TOKEN(TK_USHIFTR);
                }
                RETURN_TOKEN(TK_SHIFTR);
            }
            else { RETURN_TOKEN('>') }
        case _SC('!'):
            NEXT();
            if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('!')}
            else { NEXT(); RETURN_TOKEN(TK_NE); }
        case _SC('@'): {
            PSInteger stype;
            NEXT();
            if(CUR_CHAR != _SC('"')) {
                RETURN_TOKEN('@');
            }
            if((stype=ReadString('"',true))!=-1) {
                RETURN_TOKEN(stype);
            }
            Error(_SC("error parsing the string"));
                       }
        case _SC('"'):
        case _SC('\''): {
            PSInteger stype;
            if((stype=ReadString(CUR_CHAR,false))!=-1){
                RETURN_TOKEN(stype);
            }
            Error(_SC("error parsing the string"));
            }
        case _SC('{'): case _SC('}'): case _SC('('): case _SC(')'): case _SC('['): case _SC(']'):
        case _SC(';'): case _SC(','): case _SC('?'): case _SC('^'): case _SC('~'):
            {PSInteger ret = CUR_CHAR;
            NEXT(); RETURN_TOKEN(ret); }
        case _SC('.'):
            NEXT();
            if (CUR_CHAR != _SC('.')){ RETURN_TOKEN('.') }
            NEXT();
            if (CUR_CHAR != _SC('.')){ Error(_SC("invalid token '..'")); }
            NEXT();
            RETURN_TOKEN(TK_VARPARAMS);
        case _SC('&'):
            NEXT();
            if (CUR_CHAR != _SC('&')){ RETURN_TOKEN('&') }
            else { NEXT(); RETURN_TOKEN(TK_AND); }
        case _SC('|'):
            NEXT();
            if (CUR_CHAR != _SC('|')){ RETURN_TOKEN('|') }
            else { NEXT(); RETURN_TOKEN(TK_OR); }
        case _SC(':'):
            NEXT();
            if (CUR_CHAR != _SC(':')){ RETURN_TOKEN(':') }
            else { NEXT(); RETURN_TOKEN(TK_DOUBLE_COLON); }
        case _SC('*'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_MULEQ);}
            else RETURN_TOKEN('*');
        case _SC('%'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_MODEQ);}
            else RETURN_TOKEN('%');
        case _SC('-'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_MINUSEQ);}
            else if  (CUR_CHAR == _SC('-')){ NEXT(); RETURN_TOKEN(TK_MINUSMINUS);}
            else RETURN_TOKEN('-');
        case _SC('+'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_PLUSEQ);}
            else if (CUR_CHAR == _SC('+')){ NEXT(); RETURN_TOKEN(TK_PLUSPLUS);}
            else RETURN_TOKEN('+');
        case PSCRIPT_EOB:
            return 0;
        default:{
                if (scisdigit(CUR_CHAR)) {
                    PSInteger ret = ReadNumber();
                    RETURN_TOKEN(ret);
                }
                else if (scisalpha(CUR_CHAR) || CUR_CHAR == _SC('_')) {
                    PSInteger t = ReadID();
                    RETURN_TOKEN(t);
                }
                else {
                    PSInteger c = CUR_CHAR;
                    if (sciscntrl((int)c)) Error(_SC("unexpected character(control)"));
                    NEXT();
                    RETURN_TOKEN(c);
                }
                RETURN_TOKEN(0);
            }
        }
    }
    return 0;
}

PSInteger PSLexer::GetIDType(const PSChar *s,PSInteger len)
{
    PSObjectPtr t;
    if(_keywords->GetStr(s,len, t)) {
        return PSInteger(_integer(t));
    }
    return TK_IDENTIFIER;
}

#ifdef PSUNICODE
#if WCHAR_SIZE == 2
PSInteger PSLexer::AddUTF16(PSUnsignedInteger ch)
{
    if (ch >= 0x10000)
    {
        PSUnsignedInteger code = (ch - 0x10000);
        APPEND_CHAR((PSChar)(0xD800 | (code >> 10)));
        APPEND_CHAR((PSChar)(0xDC00 | (code & 0x3FF)));
        return 2;
    }
    else {
        APPEND_CHAR((PSChar)ch);
        return 1;
    }
}
#endif
#else
PSInteger PSLexer::AddUTF8(PSUnsignedInteger ch)
{
    if (ch < 0x80) {
        APPEND_CHAR((char)ch);
        return 1;
    }
    if (ch < 0x800) {
        APPEND_CHAR((PSChar)((ch >> 6) | 0xC0));
        APPEND_CHAR((PSChar)((ch & 0x3F) | 0x80));
        return 2;
    }
    if (ch < 0x10000) {
        APPEND_CHAR((PSChar)((ch >> 12) | 0xE0));
        APPEND_CHAR((PSChar)(((ch >> 6) & 0x3F) | 0x80));
        APPEND_CHAR((PSChar)((ch & 0x3F) | 0x80));
        return 3;
    }
    if (ch < 0x110000) {
        APPEND_CHAR((PSChar)((ch >> 18) | 0xF0));
        APPEND_CHAR((PSChar)(((ch >> 12) & 0x3F) | 0x80));
        APPEND_CHAR((PSChar)(((ch >> 6) & 0x3F) | 0x80));
        APPEND_CHAR((PSChar)((ch & 0x3F) | 0x80));
        return 4;
    }
    return 0;
}
#endif

PSInteger PSLexer::ProcessStringHexEscape(PSChar *dest, PSInteger maxdigits)
{
    NEXT();
    if (!isxdigit(CUR_CHAR)) Error(_SC("hexadecimal number expected"));
    PSInteger n = 0;
    while (isxdigit(CUR_CHAR) && n < maxdigits) {
        dest[n] = CUR_CHAR;
        n++;
        NEXT();
    }
    dest[n] = 0;
    return n;
}

PSInteger PSLexer::ReadString(PSInteger ndelim,bool verbatim)
{
    INIT_TEMP_STRING();
    NEXT();
    if(IS_EOB()) return -1;
    for(;;) {
        while(CUR_CHAR != ndelim) {
            PSInteger x = CUR_CHAR;
            switch (x) {
            case PSCRIPT_EOB:
                Error(_SC("unfinished string"));
                return -1;
            case _SC('\n'):
                if(!verbatim) Error(_SC("newline in a constant"));
                APPEND_CHAR(CUR_CHAR); NEXT();
                _currentline++;
                break;
            case _SC('\\'):
                if(verbatim) {
                    APPEND_CHAR('\\'); NEXT();
                }
                else {
                    NEXT();
                    switch(CUR_CHAR) {
                    case _SC('x'):  {
                        const PSInteger maxdigits = sizeof(PSChar) * 2;
                        PSChar temp[maxdigits + 1];
                        ProcessStringHexEscape(temp, maxdigits);
                        PSChar *stemp;
                        APPEND_CHAR((PSChar)scstrtoul(temp, &stemp, 16));
                    }
                    break;
                    case _SC('U'):
                    case _SC('u'):  {
                        const PSInteger maxdigits = x == 'u' ? 4 : 8;
                        PSChar temp[8 + 1];
                        ProcessStringHexEscape(temp, maxdigits);
                        PSChar *stemp;
#ifdef PSUNICODE
#if WCHAR_SIZE == 2
                        AddUTF16(scstrtoul(temp, &stemp, 16));
#else
                        ADD_CHAR((PSChar)scstrtoul(temp, &stemp, 16));
#endif
#else
                        AddUTF8(scstrtoul(temp, &stemp, 16));
#endif
                    }
                    break;
                    case _SC('t'): APPEND_CHAR(_SC('\t')); NEXT(); break;
                    case _SC('a'): APPEND_CHAR(_SC('\a')); NEXT(); break;
                    case _SC('b'): APPEND_CHAR(_SC('\b')); NEXT(); break;
                    case _SC('n'): APPEND_CHAR(_SC('\n')); NEXT(); break;
                    case _SC('r'): APPEND_CHAR(_SC('\r')); NEXT(); break;
                    case _SC('v'): APPEND_CHAR(_SC('\v')); NEXT(); break;
                    case _SC('f'): APPEND_CHAR(_SC('\f')); NEXT(); break;
                    case _SC('0'): APPEND_CHAR(_SC('\0')); NEXT(); break;
                    case _SC('\\'): APPEND_CHAR(_SC('\\')); NEXT(); break;
                    case _SC('"'): APPEND_CHAR(_SC('"')); NEXT(); break;
                    case _SC('\''): APPEND_CHAR(_SC('\'')); NEXT(); break;
                    default:
                        Error(_SC("unrecognised escaper char"));
                    break;
                    }
                }
                break;
            default:
                APPEND_CHAR(CUR_CHAR);
                NEXT();
            }
        }
        NEXT();
        if(verbatim && CUR_CHAR == '"') { //double quotation
            APPEND_CHAR(CUR_CHAR);
            NEXT();
        }
        else {
            break;
        }
    }
    TERMINATE_BUFFER();
    PSInteger len = _longstr.size()-1;
    if(ndelim == _SC('\'')) {
        if(len == 0) Error(_SC("empty constant"));
        if(len > 1) Error(_SC("constant too long"));
        _nvalue = _longstr[0];
        return TK_INTEGER;
    }
    _svalue = &_longstr[0];
    return TK_STRING_LITERAL;
}

void LexHexadecimal(const PSChar *s,PSUnsignedInteger *res)
{
    *res = 0;
    while(*s != 0)
    {
        if(scisdigit(*s)) *res = (*res)*16+((*s++)-'0');
        else if(scisxdigit(*s)) *res = (*res)*16+(toupper(*s++)-'A'+10);
        else { assert(0); }
    }
}

void LexInteger(const PSChar *s,PSUnsignedInteger *res)
{
    *res = 0;
    while(*s != 0)
    {
        *res = (*res)*10+((*s++)-'0');
    }
}

PSInteger scisodigit(PSInteger c) { return c >= _SC('0') && c <= _SC('7'); }

void LexOctal(const PSChar *s,PSUnsignedInteger *res)
{
    *res = 0;
    while(*s != 0)
    {
        if(scisodigit(*s)) *res = (*res)*8+((*s++)-'0');
        else { assert(0); }
    }
}

PSInteger isexponent(PSInteger c) { return c == 'e' || c=='E'; }


#define MAX_HEX_DIGITS (sizeof(PSInteger)*2)
PSInteger PSLexer::ReadNumber()
{
#define TINT 1
#define TFLOAT 2
#define THEX 3
#define TSCIENTIFIC 4
#define TOCTAL 5
    PSInteger type = TINT, firstchar = CUR_CHAR;
    PSChar *sTemp;
    INIT_TEMP_STRING();
    NEXT();
    if(firstchar == _SC('0') && (toupper(CUR_CHAR) == _SC('X') || scisodigit(CUR_CHAR)) ) {
        if(scisodigit(CUR_CHAR)) {
            type = TOCTAL;
            while(scisodigit(CUR_CHAR)) {
                APPEND_CHAR(CUR_CHAR);
                NEXT();
            }
            if(scisdigit(CUR_CHAR)) Error(_SC("invalid octal number"));
        }
        else {
            NEXT();
            type = THEX;
            while(isxdigit(CUR_CHAR)) {
                APPEND_CHAR(CUR_CHAR);
                NEXT();
            }
            if(_longstr.size() > MAX_HEX_DIGITS) Error(_SC("too many digits for an Hex number"));
        }
    }
    else {
        APPEND_CHAR((int)firstchar);
        while (CUR_CHAR == _SC('.') || scisdigit(CUR_CHAR) || isexponent(CUR_CHAR)) {
            if(CUR_CHAR == _SC('.') || isexponent(CUR_CHAR)) type = TFLOAT;
            if(isexponent(CUR_CHAR)) {
                if(type != TFLOAT) Error(_SC("invalid numeric format"));
                type = TSCIENTIFIC;
                APPEND_CHAR(CUR_CHAR);
                NEXT();
                if(CUR_CHAR == '+' || CUR_CHAR == '-'){
                    APPEND_CHAR(CUR_CHAR);
                    NEXT();
                }
                if(!scisdigit(CUR_CHAR)) Error(_SC("exponent expected"));
            }

            APPEND_CHAR(CUR_CHAR);
            NEXT();
        }
    }
    TERMINATE_BUFFER();
    switch(type) {
    case TSCIENTIFIC:
    case TFLOAT:
        _fvalue = (PSFloat)scstrtod(&_longstr[0],&sTemp);
        return TK_FLOAT;
    case TINT:
        LexInteger(&_longstr[0],(PSUnsignedInteger *)&_nvalue);
        return TK_INTEGER;
    case THEX:
        LexHexadecimal(&_longstr[0],(PSUnsignedInteger *)&_nvalue);
        return TK_INTEGER;
    case TOCTAL:
        LexOctal(&_longstr[0],(PSUnsignedInteger *)&_nvalue);
        return TK_INTEGER;
    }
    return 0;
}

PSInteger PSLexer::ReadID()
{
    PSInteger res;
    INIT_TEMP_STRING();
    do {
        APPEND_CHAR(CUR_CHAR);
        NEXT();
    } while(scisalnum(CUR_CHAR) || CUR_CHAR == _SC('_'));
    TERMINATE_BUFFER();
    res = GetIDType(&_longstr[0],_longstr.size() - 1);
    if(res == TK_IDENTIFIER || res == TK_CONSTRUCTOR) {
        _svalue = &_longstr[0];
    }
    return res;
}
