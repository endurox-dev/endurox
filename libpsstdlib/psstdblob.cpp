/* see copyright notice in pscript.h */
#include <new>
#include <pscript.h>
#include <psstdio.h>
#include <string.h>
#include <psstdblob.h>
#include "psstdstream.h"
#include "psstdblobimpl.h"

#define PSSTD_BLOB_TYPE_TAG (PSSTD_STREAM_TYPE_TAG | 0x00000002)

//Blob


#define SETUP_BLOB(v) \
    PSBlob *self = NULL; \
    { if(PS_FAILED(ps_getinstanceup(v,1,(PSUserPointer*)&self,(PSUserPointer)PSSTD_BLOB_TYPE_TAG))) \
        return ps_throwerror(v,_SC("invalid type tag"));  } \
    if(!self || !self->IsValid())  \
        return ps_throwerror(v,_SC("the blob is invalid"));


static PSInteger _blob_resize(HPSCRIPTVM v)
{
    SETUP_BLOB(v);
    PSInteger size;
    ps_getinteger(v,2,&size);
    if(!self->Resize(size))
        return ps_throwerror(v,_SC("resize failed"));
    return 0;
}

static void __swap_dword(unsigned int *n)
{
    *n=(unsigned int)(((*n&0xFF000000)>>24)  |
            ((*n&0x00FF0000)>>8)  |
            ((*n&0x0000FF00)<<8)  |
            ((*n&0x000000FF)<<24));
}

static void __swap_word(unsigned short *n)
{
    *n=(unsigned short)((*n>>8)&0x00FF)| ((*n<<8)&0xFF00);
}

static PSInteger _blob_swap4(HPSCRIPTVM v)
{
    SETUP_BLOB(v);
    PSInteger num=(self->Len()-(self->Len()%4))>>2;
    unsigned int *t=(unsigned int *)self->GetBuf();
    for(PSInteger i = 0; i < num; i++) {
        __swap_dword(&t[i]);
    }
    return 0;
}

static PSInteger _blob_swap2(HPSCRIPTVM v)
{
    SETUP_BLOB(v);
    PSInteger num=(self->Len()-(self->Len()%2))>>1;
    unsigned short *t = (unsigned short *)self->GetBuf();
    for(PSInteger i = 0; i < num; i++) {
        __swap_word(&t[i]);
    }
    return 0;
}

static PSInteger _blob__set(HPSCRIPTVM v)
{
    SETUP_BLOB(v);
    PSInteger idx,val;
    ps_getinteger(v,2,&idx);
    ps_getinteger(v,3,&val);
    if(idx < 0 || idx >= self->Len())
        return ps_throwerror(v,_SC("index out of range"));
    ((unsigned char *)self->GetBuf())[idx] = (unsigned char) val;
    ps_push(v,3);
    return 1;
}

static PSInteger _blob__get(HPSCRIPTVM v)
{
    SETUP_BLOB(v);
    PSInteger idx;
    ps_getinteger(v,2,&idx);
    if(idx < 0 || idx >= self->Len())
        return ps_throwerror(v,_SC("index out of range"));
    ps_pushinteger(v,((unsigned char *)self->GetBuf())[idx]);
    return 1;
}

static PSInteger _blob__nexti(HPSCRIPTVM v)
{
    SETUP_BLOB(v);
    if(ps_gettype(v,2) == OT_NULL) {
        ps_pushinteger(v, 0);
        return 1;
    }
    PSInteger idx;
    if(PS_SUCCEEDED(ps_getinteger(v, 2, &idx))) {
        if(idx+1 < self->Len()) {
            ps_pushinteger(v, idx+1);
            return 1;
        }
        ps_pushnull(v);
        return 1;
    }
    return ps_throwerror(v,_SC("internal error (_nexti) wrong argument type"));
}

static PSInteger _blob__typeof(HPSCRIPTVM v)
{
    ps_pushstring(v,_SC("blob"),-1);
    return 1;
}

static PSInteger _blob_releasehook(PSUserPointer p, PSInteger PS_UNUSED_ARG(size))
{
    PSBlob *self = (PSBlob*)p;
    self->~PSBlob();
    ps_free(self,sizeof(PSBlob));
    return 1;
}

static PSInteger _blob_constructor(HPSCRIPTVM v)
{
    PSInteger nparam = ps_gettop(v);
    PSInteger size = 0;
    if(nparam == 2) {
        ps_getinteger(v, 2, &size);
    }
    if(size < 0) return ps_throwerror(v, _SC("cannot create blob with negative size"));
    //PSBlob *b = new PSBlob(size);

    PSBlob *b = new (ps_malloc(sizeof(PSBlob)))PSBlob(size);
    if(PS_FAILED(ps_setinstanceup(v,1,b))) {
        b->~PSBlob();
        ps_free(b,sizeof(PSBlob));
        return ps_throwerror(v, _SC("cannot create blob"));
    }
    ps_setreleasehook(v,1,_blob_releasehook);
    return 0;
}

static PSInteger _blob__cloned(HPSCRIPTVM v)
{
    PSBlob *other = NULL;
    {
        if(PS_FAILED(ps_getinstanceup(v,2,(PSUserPointer*)&other,(PSUserPointer)PSSTD_BLOB_TYPE_TAG)))
            return PS_ERROR;
    }
    //PSBlob *thisone = new PSBlob(other->Len());
    PSBlob *thisone = new (ps_malloc(sizeof(PSBlob)))PSBlob(other->Len());
    memcpy(thisone->GetBuf(),other->GetBuf(),thisone->Len());
    if(PS_FAILED(ps_setinstanceup(v,1,thisone))) {
        thisone->~PSBlob();
        ps_free(thisone,sizeof(PSBlob));
        return ps_throwerror(v, _SC("cannot clone blob"));
    }
    ps_setreleasehook(v,1,_blob_releasehook);
    return 0;
}

#define _DECL_BLOB_FUNC(name,nparams,typecheck) {_SC(#name),_blob_##name,nparams,typecheck}
static const PSRegFunction _blob_methods[] = {
    _DECL_BLOB_FUNC(constructor,-1,_SC("xn")),
    _DECL_BLOB_FUNC(resize,2,_SC("xn")),
    _DECL_BLOB_FUNC(swap2,1,_SC("x")),
    _DECL_BLOB_FUNC(swap4,1,_SC("x")),
    _DECL_BLOB_FUNC(_set,3,_SC("xnn")),
    _DECL_BLOB_FUNC(_get,2,_SC("xn")),
    _DECL_BLOB_FUNC(_typeof,1,_SC("x")),
    _DECL_BLOB_FUNC(_nexti,2,_SC("x")),
    _DECL_BLOB_FUNC(_cloned,2,_SC("xx")),
    {NULL,(PSFUNCTION)0,0,NULL}
};



//GLOBAL FUNCTIONS

static PSInteger _g_blob_casti2f(HPSCRIPTVM v)
{
    PSInteger i;
    ps_getinteger(v,2,&i);
    ps_pushfloat(v,*((const PSFloat *)&i));
    return 1;
}

static PSInteger _g_blob_castf2i(HPSCRIPTVM v)
{
    PSFloat f;
    ps_getfloat(v,2,&f);
    ps_pushinteger(v,*((const PSInteger *)&f));
    return 1;
}

static PSInteger _g_blob_swap2(HPSCRIPTVM v)
{
    PSInteger i;
    ps_getinteger(v,2,&i);
    short s=(short)i;
    ps_pushinteger(v,(s<<8)|((s>>8)&0x00FF));
    return 1;
}

static PSInteger _g_blob_swap4(HPSCRIPTVM v)
{
    PSInteger i;
    ps_getinteger(v,2,&i);
    unsigned int t4 = (unsigned int)i;
    __swap_dword(&t4);
    ps_pushinteger(v,(PSInteger)t4);
    return 1;
}

static PSInteger _g_blob_swapfloat(HPSCRIPTVM v)
{
    PSFloat f;
    ps_getfloat(v,2,&f);
    __swap_dword((unsigned int *)&f);
    ps_pushfloat(v,f);
    return 1;
}

#define _DECL_GLOBALBLOB_FUNC(name,nparams,typecheck) {_SC(#name),_g_blob_##name,nparams,typecheck}
static const PSRegFunction bloblib_funcs[]={
    _DECL_GLOBALBLOB_FUNC(casti2f,2,_SC(".n")),
    _DECL_GLOBALBLOB_FUNC(castf2i,2,_SC(".n")),
    _DECL_GLOBALBLOB_FUNC(swap2,2,_SC(".n")),
    _DECL_GLOBALBLOB_FUNC(swap4,2,_SC(".n")),
    _DECL_GLOBALBLOB_FUNC(swapfloat,2,_SC(".n")),
    {NULL,(PSFUNCTION)0,0,NULL}
};

PSRESULT psstd_getblob(HPSCRIPTVM v,PSInteger idx,PSUserPointer *ptr)
{
    PSBlob *blob;
    if(PS_FAILED(ps_getinstanceup(v,idx,(PSUserPointer *)&blob,(PSUserPointer)PSSTD_BLOB_TYPE_TAG)))
        return -1;
    *ptr = blob->GetBuf();
    return PS_OK;
}

PSInteger psstd_getblobsize(HPSCRIPTVM v,PSInteger idx)
{
    PSBlob *blob;
    if(PS_FAILED(ps_getinstanceup(v,idx,(PSUserPointer *)&blob,(PSUserPointer)PSSTD_BLOB_TYPE_TAG)))
        return -1;
    return blob->Len();
}

PSUserPointer psstd_createblob(HPSCRIPTVM v, PSInteger size)
{
    PSInteger top = ps_gettop(v);
    ps_pushregistrytable(v);
    ps_pushstring(v,_SC("std_blob"),-1);
    if(PS_SUCCEEDED(ps_get(v,-2))) {
        ps_remove(v,-2); //removes the registry
        ps_push(v,1); // push the this
        ps_pushinteger(v,size); //size
        PSBlob *blob = NULL;
        if(PS_SUCCEEDED(ps_call(v,2,PSTrue,PSFalse))
            && PS_SUCCEEDED(ps_getinstanceup(v,-1,(PSUserPointer *)&blob,(PSUserPointer)PSSTD_BLOB_TYPE_TAG))) {
            ps_remove(v,-2);
            return blob->GetBuf();
        }
    }
    ps_settop(v,top);
    return NULL;
}

PSRESULT psstd_register_bloblib(HPSCRIPTVM v)
{
    return declare_stream(v,_SC("blob"),(PSUserPointer)PSSTD_BLOB_TYPE_TAG,_SC("std_blob"),_blob_methods,bloblib_funcs);
}

