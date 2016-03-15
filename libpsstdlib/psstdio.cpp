/* see copyright notice in pscript.h */
#include <new>
#include <stdio.h>
#include <pscript.h>
#include <psstdio.h>
#include "psstdstream.h"

#define PSSTD_FILE_TYPE_TAG (PSSTD_STREAM_TYPE_TAG | 0x00000001)
//basic API
PSFILE psstd_fopen(const PSChar *filename ,const PSChar *mode)
{
#ifndef PSUNICODE
	return (PSFILE)fopen(filename,mode);
#else
	return (PSFILE)_wfopen(filename,mode);
#endif
}

PSInteger psstd_fread(void* buffer, PSInteger size, PSInteger count, PSFILE file)
{
	return (PSInteger)fread(buffer,size,count,(FILE *)file);
}

PSInteger psstd_fwrite(const PSUserPointer buffer, PSInteger size, PSInteger count, PSFILE file)
{
	return (PSInteger)fwrite(buffer,size,count,(FILE *)file);
}

PSInteger psstd_fseek(PSFILE file, PSInteger offset, PSInteger origin)
{
	PSInteger realorigin;
	switch(origin) {
		case PS_SEEK_CUR: realorigin = SEEK_CUR; break;
		case PS_SEEK_END: realorigin = SEEK_END; break;
		case PS_SEEK_SET: realorigin = SEEK_SET; break;
		default: return -1; //failed
	}
	return fseek((FILE *)file,(long)offset,(int)realorigin);
}

PSInteger psstd_ftell(PSFILE file)
{
	return ftell((FILE *)file);
}

PSInteger psstd_fflush(PSFILE file)
{
	return fflush((FILE *)file);
}

PSInteger psstd_fclose(PSFILE file)
{
	return fclose((FILE *)file);
}

PSInteger psstd_feof(PSFILE file)
{
	return feof((FILE *)file);
}

//File
struct PSFile : public PSStream {
	PSFile() { _handle = NULL; _owns = false;}
	PSFile(PSFILE file, bool owns) { _handle = file; _owns = owns;}
	virtual ~PSFile() { Close(); }
	bool Open(const PSChar *filename ,const PSChar *mode) {
		Close();
		if( (_handle = psstd_fopen(filename,mode)) ) {
			_owns = true;
			return true;
		}
		return false;
	}
	void Close() {
		if(_handle && _owns) { 
			psstd_fclose(_handle);
			_handle = NULL;
			_owns = false;
		}
	}
	PSInteger Read(void *buffer,PSInteger size) {
		return psstd_fread(buffer,1,size,_handle);
	}
	PSInteger Write(void *buffer,PSInteger size) {
		return psstd_fwrite(buffer,1,size,_handle);
	}
	PSInteger Flush() {
		return psstd_fflush(_handle);
	}
	PSInteger Tell() {
		return psstd_ftell(_handle);
	}
	PSInteger Len() {
		PSInteger prevpos=Tell();
		Seek(0,PS_SEEK_END);
		PSInteger size=Tell();
		Seek(prevpos,PS_SEEK_SET);
		return size;
	}
	PSInteger Seek(PSInteger offset, PSInteger origin)	{
		return psstd_fseek(_handle,offset,origin);
	}
	bool IsValid() { return _handle?true:false; }
	bool EOS() { return Tell()==Len()?true:false;}
	PSFILE GetHandle() {return _handle;}
private:
	PSFILE _handle;
	bool _owns;
};

static PSInteger _file__typeof(HPSCRIPTVM v)
{
	ps_pushstring(v,_SC("file"),-1);
	return 1;
}

static PSInteger _file_releasehook(PSUserPointer p, PSInteger size)
{
	PSFile *self = (PSFile*)p;
	self->~PSFile();
	ps_free(self,sizeof(PSFile));
	return 1;
}

static PSInteger _file_constructor(HPSCRIPTVM v)
{
	const PSChar *filename,*mode;
	bool owns = true;
	PSFile *f;
	PSFILE newf;
	if(ps_gettype(v,2) == OT_STRING && ps_gettype(v,3) == OT_STRING) {
		ps_getstring(v, 2, &filename);
		ps_getstring(v, 3, &mode);
		newf = psstd_fopen(filename, mode);
		if(!newf) return ps_throwerror(v, _SC("cannot open file"));
	} else if(ps_gettype(v,2) == OT_USERPOINTER) {
		owns = !(ps_gettype(v,3) == OT_NULL);
		ps_getuserpointer(v,2,&newf);
	} else {
		return ps_throwerror(v,_SC("wrong parameter"));
	}
	
	f = new (ps_malloc(sizeof(PSFile)))PSFile(newf,owns);
	if(PS_FAILED(ps_setinstanceup(v,1,f))) {
		f->~PSFile();
		ps_free(f,sizeof(PSFile));
		return ps_throwerror(v, _SC("cannot create blob with negative size"));
	}
	ps_setreleasehook(v,1,_file_releasehook);
	return 0;
}

static PSInteger _file_close(HPSCRIPTVM v)
{
	PSFile *self = NULL;
	if(PS_SUCCEEDED(ps_getinstanceup(v,1,(PSUserPointer*)&self,(PSUserPointer)PSSTD_FILE_TYPE_TAG))
		&& self != NULL)
	{
		self->Close();
	}
	return 0;
}

//bindings
#define _DECL_FILE_FUNC(name,nparams,typecheck) {_SC(#name),_file_##name,nparams,typecheck}
static PSRegFunction _file_methods[] = {
	_DECL_FILE_FUNC(constructor,3,_SC("x")),
	_DECL_FILE_FUNC(_typeof,1,_SC("x")),
	_DECL_FILE_FUNC(close,1,_SC("x")),
	{0,0,0,0},
};



PSRESULT psstd_createfile(HPSCRIPTVM v, PSFILE file,PSBool own)
{
	PSInteger top = ps_gettop(v);
	ps_pushregistrytable(v);
	ps_pushstring(v,_SC("std_file"),-1);
	if(PS_SUCCEEDED(ps_get(v,-2))) {
		ps_remove(v,-2); //removes the registry
		ps_pushroottable(v); // push the this
		ps_pushuserpointer(v,file); //file
		if(own){
			ps_pushinteger(v,1); //true
		}
		else{
			ps_pushnull(v); //false
		}
		if(PS_SUCCEEDED( ps_call(v,3,PSTrue,PSFalse) )) {
			ps_remove(v,-2);
			return PS_OK;
		}
	}
	ps_settop(v,top);
	return PS_OK;
}

PSRESULT psstd_getfile(HPSCRIPTVM v, PSInteger idx, PSFILE *file)
{
	PSFile *fileobj = NULL;
	if(PS_SUCCEEDED(ps_getinstanceup(v,idx,(PSUserPointer*)&fileobj,(PSUserPointer)PSSTD_FILE_TYPE_TAG))) {
		*file = fileobj->GetHandle();
		return PS_OK;
	}
	return ps_throwerror(v,_SC("not a file"));
}



static PSInteger _io_file_lexfeed_PLAIN(PSUserPointer file)
{
	PSInteger ret;
	char c;
	if( ( ret=psstd_fread(&c,sizeof(c),1,(FILE *)file )>0) )
		return c;
	return 0;
}

#ifdef PSUNICODE
static PSInteger _io_file_lexfeed_UTF8(PSUserPointer file)
{
#define READ() \
	if(psstd_fread(&inchar,sizeof(inchar),1,(FILE *)file) != 1) \
		return 0;

	static const PSInteger utf8_lengths[16] =
	{
		1,1,1,1,1,1,1,1,        /* 0000 to 0111 : 1 byte (plain ASCII) */
		0,0,0,0,                /* 1000 to 1011 : not valid */
		2,2,                    /* 1100, 1101 : 2 bytes */
		3,                      /* 1110 : 3 bytes */
		4                       /* 1111 :4 bytes */
	};
	static unsigned char byte_masks[5] = {0,0,0x1f,0x0f,0x07};
	unsigned char inchar;
	PSInteger c = 0;
	READ();
	c = inchar;
	//
	if(c >= 0x80) {
		PSInteger tmp;
		PSInteger codelen = utf8_lengths[c>>4];
		if(codelen == 0) 
			return 0;
			//"invalid UTF-8 stream";
		tmp = c&byte_masks[codelen];
		for(PSInteger n = 0; n < codelen-1; n++) {
			tmp<<=6;
			READ();
			tmp |= inchar & 0x3F;
		}
		c = tmp;
	}
	return c;
}
#endif

static PSInteger _io_file_lexfeed_UCS2_LE(PSUserPointer file)
{
	PSInteger ret;
	wchar_t c;
	if( ( ret=psstd_fread(&c,sizeof(c),1,(FILE *)file )>0) )
		return (PSChar)c;
	return 0;
}

static PSInteger _io_file_lexfeed_UCS2_BE(PSUserPointer file)
{
	PSInteger ret;
	unsigned short c;
	if( ( ret=psstd_fread(&c,sizeof(c),1,(FILE *)file )>0) ) {
		c = ((c>>8)&0x00FF)| ((c<<8)&0xFF00);
		return (PSChar)c;
	}
	return 0;
}

PSInteger file_read(PSUserPointer file,PSUserPointer buf,PSInteger size)
{
	PSInteger ret;
	if( ( ret = psstd_fread(buf,1,size,(PSFILE)file ))!=0 )return ret;
	return -1;
}

PSInteger file_write(PSUserPointer file,PSUserPointer p,PSInteger size)
{
	return psstd_fwrite(p,1,size,(PSFILE)file);
}

PSRESULT psstd_loadfile(HPSCRIPTVM v,const PSChar *filename,PSBool printerror)
{
	PSFILE file = psstd_fopen(filename,_SC("rb"));
	PSInteger ret;
	unsigned short us;
	unsigned char uc;
	PSLEXREADFUNC func = _io_file_lexfeed_PLAIN;
	if(file){
		ret = psstd_fread(&us,1,2,file);
		if(ret != 2) {
			//probably an empty file
			us = 0;
		}
		if(us == PS_BYTECODE_STREAM_TAG) { //BYTECODE
			psstd_fseek(file,0,PS_SEEK_SET);
			if(PS_SUCCEEDED(ps_readclosure(v,file_read,file))) {
				psstd_fclose(file);
				return PS_OK;
			}
		}
		else { //SCRIPT
			switch(us)
			{
				//gotta swap the next 2 lines on BIG endian machines
				case 0xFFFE: func = _io_file_lexfeed_UCS2_BE; break;//UTF-16 little endian;
				case 0xFEFF: func = _io_file_lexfeed_UCS2_LE; break;//UTF-16 big endian;
				case 0xBBEF: 
					if(psstd_fread(&uc,1,sizeof(uc),file) == 0) { 
						psstd_fclose(file); 
						return ps_throwerror(v,_SC("io error")); 
					}
					if(uc != 0xBF) { 
						psstd_fclose(file); 
						return ps_throwerror(v,_SC("Unrecognozed ecoding")); 
					}
#ifdef PSUNICODE
					func = _io_file_lexfeed_UTF8;
#else
					func = _io_file_lexfeed_PLAIN;
#endif
					break;//UTF-8 ;
				default: psstd_fseek(file,0,PS_SEEK_SET); break; // ascii
			}

			if(PS_SUCCEEDED(ps_compile(v,func,file,filename,printerror))){
				psstd_fclose(file);
				return PS_OK;
			}
		}
		psstd_fclose(file);
		return PS_ERROR;
	}
	return ps_throwerror(v,_SC("cannot open the file"));
}

PSRESULT psstd_dofile(HPSCRIPTVM v,const PSChar *filename,PSBool retval,PSBool printerror)
{
	if(PS_SUCCEEDED(psstd_loadfile(v,filename,printerror))) {
		ps_push(v,-2);
		if(PS_SUCCEEDED(ps_call(v,1,retval,PSTrue))) {
			ps_remove(v,retval?-2:-1); //removes the closure
			return 1;
		}
		ps_pop(v,1); //removes the closure
	}
	return PS_ERROR;
}

PSRESULT psstd_writeclosuretofile(HPSCRIPTVM v,const PSChar *filename)
{
	PSFILE file = psstd_fopen(filename,_SC("wb+"));
	if(!file) return ps_throwerror(v,_SC("cannot open the file"));
	if(PS_SUCCEEDED(ps_writeclosure(v,file_write,file))) {
		psstd_fclose(file);
		return PS_OK;
	}
	psstd_fclose(file);
	return PS_ERROR; //forward the error
}

PSInteger _g_io_loadfile(HPSCRIPTVM v)
{
	const PSChar *filename;
	PSBool printerror = PSFalse;
	ps_getstring(v,2,&filename);
	if(ps_gettop(v) >= 3) {
		ps_getbool(v,3,&printerror);
	}
	if(PS_SUCCEEDED(psstd_loadfile(v,filename,printerror)))
		return 1;
	return PS_ERROR; //propagates the error
}

PSInteger _g_io_writeclosuretofile(HPSCRIPTVM v)
{
	const PSChar *filename;
	ps_getstring(v,2,&filename);
	if(PS_SUCCEEDED(psstd_writeclosuretofile(v,filename)))
		return 1;
	return PS_ERROR; //propagates the error
}

PSInteger _g_io_dofile(HPSCRIPTVM v)
{
	const PSChar *filename;
	PSBool printerror = PSFalse;
	ps_getstring(v,2,&filename);
	if(ps_gettop(v) >= 3) {
		ps_getbool(v,3,&printerror);
	}
	ps_push(v,1); //repush the this
	if(PS_SUCCEEDED(psstd_dofile(v,filename,PSTrue,printerror)))
		return 1;
	return PS_ERROR; //propagates the error
}

#define _DECL_GLOBALIO_FUNC(name,nparams,typecheck) {_SC(#name),_g_io_##name,nparams,typecheck}
static PSRegFunction iolib_funcs[]={
	_DECL_GLOBALIO_FUNC(loadfile,-2,_SC(".sb")),
	_DECL_GLOBALIO_FUNC(dofile,-2,_SC(".sb")),
	_DECL_GLOBALIO_FUNC(writeclosuretofile,3,_SC(".sc")),
	{0,0}
};

PSRESULT psstd_register_iolib(HPSCRIPTVM v)
{
	PSInteger top = ps_gettop(v);
	//create delegate
	declare_stream(v,_SC("file"),(PSUserPointer)PSSTD_FILE_TYPE_TAG,_SC("std_file"),_file_methods,iolib_funcs);
	ps_pushstring(v,_SC("stdout"),-1);
	psstd_createfile(v,stdout,PSFalse);
	ps_newslot(v,-3,PSFalse);
	ps_pushstring(v,_SC("stdin"),-1);
	psstd_createfile(v,stdin,PSFalse);
	ps_newslot(v,-3,PSFalse);
	ps_pushstring(v,_SC("stderr"),-1);
	psstd_createfile(v,stderr,PSFalse);
	ps_newslot(v,-3,PSFalse);
	ps_settop(v,top);
	return PS_OK;
}
