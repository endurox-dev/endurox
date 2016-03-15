/*	see copyright notice in pscript.h */
#ifndef _PSSTDBLOB_H_
#define _PSSTDBLOB_H_

#ifdef __cplusplus
extern "C" {
#endif

PSCRIPT_API PSUserPointer psstd_createblob(HPSCRIPTVM v, PSInteger size);
PSCRIPT_API PSRESULT psstd_getblob(HPSCRIPTVM v,PSInteger idx,PSUserPointer *ptr);
PSCRIPT_API PSInteger psstd_getblobsize(HPSCRIPTVM v,PSInteger idx);

PSCRIPT_API PSRESULT psstd_register_bloblib(HPSCRIPTVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_PSSTDBLOB_H_*/

