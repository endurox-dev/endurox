/*  see copyright notice in pscript.h */
#ifndef _PSSTD_STREAM_H_
#define _PSSTD_STREAM_H_

PSInteger _stream_readblob(HPSCRIPTVM v);
PSInteger _stream_readline(HPSCRIPTVM v);
PSInteger _stream_readn(HPSCRIPTVM v);
PSInteger _stream_writeblob(HPSCRIPTVM v);
PSInteger _stream_writen(HPSCRIPTVM v);
PSInteger _stream_seek(HPSCRIPTVM v);
PSInteger _stream_tell(HPSCRIPTVM v);
PSInteger _stream_len(HPSCRIPTVM v);
PSInteger _stream_eos(HPSCRIPTVM v);
PSInteger _stream_flush(HPSCRIPTVM v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_SC(#name),_stream_##name,nparams,typecheck}
PSRESULT declare_stream(HPSCRIPTVM v,const PSChar* name,PSUserPointer typetag,const PSChar* reg_name,const PSRegFunction *methods,const PSRegFunction *globals);
#endif /*_PSSTD_STREAM_H_*/
