/* see copyright notice in pscript.h */
#include <pscript.h>
#include <psstdaux.h>
#include <assert.h>

void psstd_printcallstack(HPSCRIPTVM v)
{
    PSPRINTFUNCTION pf = ps_geterrorfunc(v);
    if(pf) {
        PSStackInfos si;
        PSInteger i;
        PSFloat f;
        const PSChar *s;
        PSInteger level=1; //1 is to skip this function that is level 0
        const PSChar *name=0;
        PSInteger seq=0;
        pf(v,_SC("\nCALLSTACK\n"));
        while(PS_SUCCEEDED(ps_stackinfos(v,level,&si)))
        {
            const PSChar *fn=_SC("unknown");
            const PSChar *src=_SC("unknown");
            if(si.funcname)fn=si.funcname;
            if(si.source)src=si.source;
            pf(v,_SC("*FUNCTION [%s()] %s line [%d]\n"),fn,src,si.line);
            level++;
        }
        level=0;
        pf(v,_SC("\nLOCALS\n"));

        for(level=0;level<10;level++){
            seq=0;
            while((name = ps_getlocal(v,level,seq)))
            {
                seq++;
                switch(ps_gettype(v,-1))
                {
                case OT_NULL:
                    pf(v,_SC("[%s] NULL\n"),name);
                    break;
                case OT_INTEGER:
                    ps_getinteger(v,-1,&i);
                    pf(v,_SC("[%s] %d\n"),name,i);
                    break;
                case OT_FLOAT:
                    ps_getfloat(v,-1,&f);
                    pf(v,_SC("[%s] %.14g\n"),name,f);
                    break;
                case OT_USERPOINTER:
                    pf(v,_SC("[%s] USERPOINTER\n"),name);
                    break;
                case OT_STRING:
                    ps_getstring(v,-1,&s);
                    pf(v,_SC("[%s] \"%s\"\n"),name,s);
                    break;
                case OT_TABLE:
                    pf(v,_SC("[%s] TABLE\n"),name);
                    break;
                case OT_ARRAY:
                    pf(v,_SC("[%s] ARRAY\n"),name);
                    break;
                case OT_CLOSURE:
                    pf(v,_SC("[%s] CLOSURE\n"),name);
                    break;
                case OT_NATIVECLOSURE:
                    pf(v,_SC("[%s] NATIVECLOSURE\n"),name);
                    break;
                case OT_GENERATOR:
                    pf(v,_SC("[%s] GENERATOR\n"),name);
                    break;
                case OT_USERDATA:
                    pf(v,_SC("[%s] USERDATA\n"),name);
                    break;
                case OT_THREAD:
                    pf(v,_SC("[%s] THREAD\n"),name);
                    break;
                case OT_CLASS:
                    pf(v,_SC("[%s] CLASS\n"),name);
                    break;
                case OT_INSTANCE:
                    pf(v,_SC("[%s] INSTANCE\n"),name);
                    break;
                case OT_WEAKREF:
                    pf(v,_SC("[%s] WEAKREF\n"),name);
                    break;
                case OT_BOOL:{
                    PSBool bval;
                    ps_getbool(v,-1,&bval);
                    pf(v,_SC("[%s] %s\n"),name,bval == PSTrue ? _SC("true"):_SC("false"));
                             }
                    break;
                default: assert(0); break;
                }
                ps_pop(v,1);
            }
        }
    }
}

static PSInteger _psstd_aux_printerror(HPSCRIPTVM v)
{
    PSPRINTFUNCTION pf = ps_geterrorfunc(v);
    if(pf) {
        const PSChar *sErr = 0;
        if(ps_gettop(v)>=1) {
            if(PS_SUCCEEDED(ps_getstring(v,2,&sErr)))   {
                pf(v,_SC("\nAN ERROR HAS OCCURED [%s]\n"),sErr);
            }
            else{
                pf(v,_SC("\nAN ERROR HAS OCCURED [unknown]\n"));
            }
            psstd_printcallstack(v);
        }
    }
    return 0;
}

void _psstd_compiler_error(HPSCRIPTVM v,const PSChar *sErr,const PSChar *sSource,PSInteger line,PSInteger column)
{
    PSPRINTFUNCTION pf = ps_geterrorfunc(v);
    if(pf) {
        pf(v,_SC("%s line = (%d) column = (%d) : error %s\n"),sSource,line,column,sErr);
    }
}

void psstd_seterrorhandlers(HPSCRIPTVM v)
{
    ps_setcompilererrorhandler(v,_psstd_compiler_error);
    ps_newclosure(v,_psstd_aux_printerror,0);
    ps_seterrorhandler(v);
}
