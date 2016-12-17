/*  see copyright notice in pscript.h */
#ifndef _PSSTD_AUXLIB_H_
#define _PSSTD_AUXLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

PSCRIPT_API void psstd_seterrorhandlers(HPSCRIPTVM v);
PSCRIPT_API void psstd_printcallstack(HPSCRIPTVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* _PSSTD_AUXLIB_H_ */
