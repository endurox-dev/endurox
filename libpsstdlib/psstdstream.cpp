/* see copyright notice in pscript.h */
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pscript.h>
#include <psstdio.h>
#include <psstdblob.h>
#include "psstdstream.h"
#include "psstdblobimpl.h"

#define SETUP_STREAM(v) \
    PSStream *self = NULL; \
    if(PS_FAILED(ps_getinstanceup(v,1,(PSUserPointer*)&self,(PSUserPointer)PSSTD_STREAM_TYPE_TAG))) \
        return ps_throwerror(v,_SC("invalid type tag")); \
    if(!self || !self->IsValid())  \
        return ps_throwerror(v,_SC("the stream is invalid"));

PSInteger _stream_readblob(HPSCRIPTVM v)
{
    SETUP_STREAM(v);
    PSUserPointer data,blobp;
    PSInteger size,res;
    ps_getinteger(v,2,&size);
    if(size > self->Len()) {
        size = self->Len();
    }
    data = ps_getscratchpad(v,size);
    res = self->Read(data,size);
    if(res <= 0)
        return ps_throwerror(v,_SC("no data left to read"));
    blobp = psstd_createblob(v,res);
    memcpy(blobp,data,res);
    return 1;
}

#define SAFE_READN(ptr,len) { \
    if(self->Read(ptr,len) != len) return ps_throwerror(v,_SC("io error")); \
    }
PSInteger _stream_readn(HPSCRIPTVM v)
{
    SETUP_STREAM(v);
    PSInteger format;
    ps_getinteger(v, 2, &format);
    switch(format) {
    case 'l': {
        PSInteger i;
        SAFE_READN(&i, sizeof(i));
        ps_pushinteger(v, i);
              }
        break;
    case 'i': {
        PSInt32 i;
        SAFE_READN(&i, sizeof(i));
        ps_pushinteger(v, i);
              }
        break;
    case 's': {
        short s;
        SAFE_READN(&s, sizeof(short));
        ps_pushinteger(v, s);
              }
        break;
    case 'w': {
        unsigned short w;
        SAFE_READN(&w, sizeof(unsigned short));
        ps_pushinteger(v, w);
              }
        break;
    case 'c': {
        char c;
        SAFE_READN(&c, sizeof(char));
        ps_pushinteger(v, c);
              }
        break;
    case 'b': {
        unsigned char c;
        SAFE_READN(&c, sizeof(unsigned char));
        ps_pushinteger(v, c);
              }
        break;
    case 'f': {
        float f;
        SAFE_READN(&f, sizeof(float));
        ps_pushfloat(v, f);
              }
        break;
    case 'd': {
        double d;
        SAFE_READN(&d, sizeof(double));
        ps_pushfloat(v, (PSFloat)d);
              }
        break;
    default:
        return ps_throwerror(v, _SC("invalid format"));
    }
    return 1;
}

PSInteger _stream_writeblob(HPSCRIPTVM v)
{
    PSUserPointer data;
    PSInteger size;
    SETUP_STREAM(v);
    if(PS_FAILED(psstd_getblob(v,2,&data)))
        return ps_throwerror(v,_SC("invalid parameter"));
    size = psstd_getblobsize(v,2);
    if(self->Write(data,size) != size)
        return ps_throwerror(v,_SC("io error"));
    ps_pushinteger(v,size);
    return 1;
}

PSInteger _stream_writen(HPSCRIPTVM v)
{
    SETUP_STREAM(v);
    PSInteger format, ti;
    PSFloat tf;
    ps_getinteger(v, 3, &format);
    switch(format) {
    case 'l': {
        PSInteger i;
        ps_getinteger(v, 2, &ti);
        i = ti;
        self->Write(&i, sizeof(PSInteger));
              }
        break;
    case 'i': {
        PSInt32 i;
        ps_getinteger(v, 2, &ti);
        i = (PSInt32)ti;
        self->Write(&i, sizeof(PSInt32));
              }
        break;
    case 's': {
        short s;
        ps_getinteger(v, 2, &ti);
        s = (short)ti;
        self->Write(&s, sizeof(short));
              }
        break;
    case 'w': {
        unsigned short w;
        ps_getinteger(v, 2, &ti);
        w = (unsigned short)ti;
        self->Write(&w, sizeof(unsigned short));
              }
        break;
    case 'c': {
        char c;
        ps_getinteger(v, 2, &ti);
        c = (char)ti;
        self->Write(&c, sizeof(char));
                  }
        break;
    case 'b': {
        unsigned char b;
        ps_getinteger(v, 2, &ti);
        b = (unsigned char)ti;
        self->Write(&b, sizeof(unsigned char));
              }
        break;
    case 'f': {
        float f;
        ps_getfloat(v, 2, &tf);
        f = (float)tf;
        self->Write(&f, sizeof(float));
              }
        break;
    case 'd': {
        double d;
        ps_getfloat(v, 2, &tf);
        d = tf;
        self->Write(&d, sizeof(double));
              }
        break;
    default:
        return ps_throwerror(v, _SC("invalid format"));
    }
    return 0;
}


//Write string, mvitolin
PSInteger _stream_writes(HPSCRIPTVM v)
{
       PSInteger size;
       SETUP_STREAM(v);
        const PSChar *s;
        
        if(PS_SUCCEEDED(ps_getstring(v,2,&s))){

            size = strlen(s);
            if(self->Write((void *)s,strlen(s)) != size)
                    return ps_throwerror(v,_SC("io error"));        
            return 1;
        }
       ps_pushinteger(v,size);
       return 1;
}



PSInteger _stream_seek(HPSCRIPTVM v)
{
    SETUP_STREAM(v);
    PSInteger offset, origin = PS_SEEK_SET;
    ps_getinteger(v, 2, &offset);
    if(ps_gettop(v) > 2) {
        PSInteger t;
        ps_getinteger(v, 3, &t);
        switch(t) {
            case 'b': origin = PS_SEEK_SET; break;
            case 'c': origin = PS_SEEK_CUR; break;
            case 'e': origin = PS_SEEK_END; break;
            default: return ps_throwerror(v,_SC("invalid origin"));
        }
    }
    ps_pushinteger(v, self->Seek(offset, origin));
    return 1;
}

PSInteger _stream_tell(HPSCRIPTVM v)
{
    SETUP_STREAM(v);
    ps_pushinteger(v, self->Tell());
    return 1;
}

PSInteger _stream_len(HPSCRIPTVM v)
{
    SETUP_STREAM(v);
    ps_pushinteger(v, self->Len());
    return 1;
}

PSInteger _stream_flush(HPSCRIPTVM v)
{
    SETUP_STREAM(v);
    if(!self->Flush())
        ps_pushinteger(v, 1);
    else
        ps_pushnull(v);
    return 1;
}

PSInteger _stream_eos(HPSCRIPTVM v)
{
    SETUP_STREAM(v);
    if(self->EOS())
        ps_pushinteger(v, 1);
    else
        ps_pushnull(v);
    return 1;
}

 PSInteger _stream__cloned(HPSCRIPTVM v)
 {
     return ps_throwerror(v,_SC("this object cannot be cloned"));
 }

static const PSRegFunction _stream_methods[] = {
    _DECL_STREAM_FUNC(readblob,2,_SC("xn")),
    _DECL_STREAM_FUNC(readn,2,_SC("xn")),
    _DECL_STREAM_FUNC(writeblob,-2,_SC("xx")),
    _DECL_STREAM_FUNC(writen,3,_SC("xnn")),
    _DECL_STREAM_FUNC(writes,2,_SC("xs")),
    _DECL_STREAM_FUNC(seek,-2,_SC("xnn")),
    _DECL_STREAM_FUNC(tell,1,_SC("x")),
    _DECL_STREAM_FUNC(len,1,_SC("x")),
    _DECL_STREAM_FUNC(eos,1,_SC("x")),
    _DECL_STREAM_FUNC(flush,1,_SC("x")),
    _DECL_STREAM_FUNC(_cloned,0,NULL),
    {NULL,(PSFUNCTION)0,0,NULL}
};

void init_streamclass(HPSCRIPTVM v)
{
    ps_pushregistrytable(v);
    ps_pushstring(v,_SC("std_stream"),-1);
    if(PS_FAILED(ps_get(v,-2))) {
        ps_pushstring(v,_SC("std_stream"),-1);
        ps_newclass(v,PSFalse);
        ps_settypetag(v,-1,(PSUserPointer)PSSTD_STREAM_TYPE_TAG);
        PSInteger i = 0;
        while(_stream_methods[i].name != 0) {
            const PSRegFunction &f = _stream_methods[i];
            ps_pushstring(v,f.name,-1);
            ps_newclosure(v,f.f,0);
            ps_setparamscheck(v,f.nparamscheck,f.typemask);
            ps_newslot(v,-3,PSFalse);
            i++;
        }
        ps_newslot(v,-3,PSFalse);
        ps_pushroottable(v);
        ps_pushstring(v,_SC("stream"),-1);
        ps_pushstring(v,_SC("std_stream"),-1);
        ps_get(v,-4);
        ps_newslot(v,-3,PSFalse);
        ps_pop(v,1);
    }
    else {
        ps_pop(v,1); //result
    }
    ps_pop(v,1);
}

PSRESULT declare_stream(HPSCRIPTVM v,const PSChar* name,PSUserPointer typetag,const PSChar* reg_name,const PSRegFunction *methods,const PSRegFunction *globals)
{
    if(ps_gettype(v,-1) != OT_TABLE)
        return ps_throwerror(v,_SC("table expected"));
    PSInteger top = ps_gettop(v);
    //create delegate
    init_streamclass(v);
    ps_pushregistrytable(v);
    ps_pushstring(v,reg_name,-1);
    ps_pushstring(v,_SC("std_stream"),-1);
    if(PS_SUCCEEDED(ps_get(v,-3))) {
        ps_newclass(v,PSTrue);
        ps_settypetag(v,-1,typetag);
        PSInteger i = 0;
        while(methods[i].name != 0) {
            const PSRegFunction &f = methods[i];
            ps_pushstring(v,f.name,-1);
            ps_newclosure(v,f.f,0);
            ps_setparamscheck(v,f.nparamscheck,f.typemask);
            ps_setnativeclosurename(v,-1,f.name);
            ps_newslot(v,-3,PSFalse);
            i++;
        }
        ps_newslot(v,-3,PSFalse);
        ps_pop(v,1);

        i = 0;
        while(globals[i].name!=0)
        {
            const PSRegFunction &f = globals[i];
            ps_pushstring(v,f.name,-1);
            ps_newclosure(v,f.f,0);
            ps_setparamscheck(v,f.nparamscheck,f.typemask);
            ps_setnativeclosurename(v,-1,f.name);
            ps_newslot(v,-3,PSFalse);
            i++;
        }
        //register the class in the target table
        ps_pushstring(v,name,-1);
        ps_pushregistrytable(v);
        ps_pushstring(v,reg_name,-1);
        ps_get(v,-2);
        ps_remove(v,-2);
        ps_newslot(v,-3,PSFalse);

        ps_settop(v,top);
        return PS_OK;
    }
    ps_settop(v,top);
    return PS_ERROR;
}
