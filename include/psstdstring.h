/*	see copyright notice in pscript.h */
#ifndef _PSSTD_STRING_H_
#define _PSSTD_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int PSRexBool;
typedef struct PSRex PSRex;

typedef struct {
	const PSChar *begin;
	PSInteger len;
} PSRexMatch;

PSCRIPT_API PSRex *psstd_rex_compile(const PSChar *pattern,const PSChar **error);
PSCRIPT_API void psstd_rex_free(PSRex *exp);
PSCRIPT_API PSBool psstd_rex_match(PSRex* exp,const PSChar* text);
PSCRIPT_API PSBool psstd_rex_search(PSRex* exp,const PSChar* text, const PSChar** out_begin, const PSChar** out_end);
PSCRIPT_API PSBool psstd_rex_searchrange(PSRex* exp,const PSChar* text_begin,const PSChar* text_end,const PSChar** out_begin, const PSChar** out_end);
PSCRIPT_API PSInteger psstd_rex_getsubexpcount(PSRex* exp);
PSCRIPT_API PSBool psstd_rex_getsubexp(PSRex* exp, PSInteger n, PSRexMatch *subexp);

PSCRIPT_API PSRESULT psstd_format(HPSCRIPTVM v,PSInteger nformatstringidx,PSInteger *outlen,PSChar **output);

PSCRIPT_API PSRESULT psstd_register_stringlib(HPSCRIPTVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_PSSTD_STRING_H_*/
