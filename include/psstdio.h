/*  see copyright notice in pscript.h */
#ifndef _PSSTDIO_H_
#define _PSSTDIO_H_

#ifdef __cplusplus

#define PSSTD_STREAM_TYPE_TAG 0x80000000

struct PSStream {
    virtual PSInteger Read(void *buffer, PSInteger size) = 0;
    virtual PSInteger Write(void *buffer, PSInteger size) = 0;
    virtual PSInteger Flush() = 0;
    virtual PSInteger Tell() = 0;
    virtual PSInteger Len() = 0;
    virtual PSInteger Seek(PSInteger offset, PSInteger origin) = 0;
    virtual bool IsValid() = 0;
    virtual bool EOS() = 0;
};

extern "C" {
#endif

#define PS_SEEK_CUR 0
#define PS_SEEK_END 1
#define PS_SEEK_SET 2

typedef void* PSFILE;

PSCRIPT_API PSFILE psstd_fopen(const PSChar *,const PSChar *);
PSCRIPT_API PSInteger psstd_fread(PSUserPointer, PSInteger, PSInteger, PSFILE);
PSCRIPT_API PSInteger psstd_fwrite(const PSUserPointer, PSInteger, PSInteger, PSFILE);
PSCRIPT_API PSInteger psstd_fseek(PSFILE , PSInteger , PSInteger);
PSCRIPT_API PSInteger psstd_ftell(PSFILE);
PSCRIPT_API PSInteger psstd_fflush(PSFILE);
PSCRIPT_API PSInteger psstd_fclose(PSFILE);
PSCRIPT_API PSInteger psstd_feof(PSFILE);

PSCRIPT_API PSRESULT psstd_createfile(HPSCRIPTVM v, PSFILE file,PSBool own);
PSCRIPT_API PSRESULT psstd_getfile(HPSCRIPTVM v, PSInteger idx, PSFILE *file);

//compiler helpers
PSCRIPT_API PSRESULT psstd_loadfile(HPSCRIPTVM v,const PSChar *filename,PSBool printerror);
PSCRIPT_API PSRESULT psstd_dofile(HPSCRIPTVM v,const PSChar *filename,PSBool retval,PSBool printerror);
PSCRIPT_API PSRESULT psstd_writeclosuretofile(HPSCRIPTVM v,const PSChar *filename);

PSCRIPT_API PSRESULT psstd_register_iolib(HPSCRIPTVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_PSSTDIO_H_*/

