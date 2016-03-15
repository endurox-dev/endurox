/*	see copyright notice in pscript.h */
#ifndef _PSLEXER_H_
#define _PSLEXER_H_

#ifdef PSUNICODE
typedef PSChar LexChar;
#else
typedef	unsigned char LexChar;
#endif

struct PSLexer
{
	PSLexer();
	~PSLexer();
	void Init(PSSharedState *ss,PSLEXREADFUNC rg,PSUserPointer up,CompilerErrorFunc efunc,void *ed);
	void Error(const PSChar *err);
	PSInteger Lex();
	const PSChar *Tok2Str(PSInteger tok);
private:
	PSInteger GetIDType(PSChar *s);
	PSInteger ReadString(PSInteger ndelim,bool verbatim);
	PSInteger ReadNumber();
	void LexBlockComment();
	void LexLineComment();
	PSInteger ReadID();
	void Next();
	PSInteger _curtoken;
	PSTable *_keywords;
	PSBool _reached_eof;
public:
	PSInteger _prevtoken;
	PSInteger _currentline;
	PSInteger _lasttokenline;
	PSInteger _currentcolumn;
	const PSChar *_svalue;
	PSInteger _nvalue;
	PSFloat _fvalue;
	PSLEXREADFUNC _readf;
	PSUserPointer _up;
	LexChar _currdata;
	PSSharedState *_sharedstate;
	psvector<PSChar> _longstr;
	CompilerErrorFunc _errfunc;
	void *_errtarget;
};

#endif
