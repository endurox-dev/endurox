/* see copyright notice in pscript.h */
#include <pscript.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <psstdsystem.h>

#ifdef PSUNICODE
#include <wchar.h>
#define scgetenv _wgetenv
#define scsystem _wsystem
#define scasctime _wasctime
#define scremove _wremove
#define screname _wrename
#else
#define scgetenv getenv
#define scsystem system
#define scasctime asctime
#define scremove remove
#define screname rename
#endif

static PSInteger _system_getenv(HPSCRIPTVM v)
{
    const PSChar *s;
    if(PS_SUCCEEDED(ps_getstring(v,2,&s))){
        ps_pushstring(v,scgetenv(s),-1);
        return 1;
    }
    return 0;
}


static PSInteger _system_system(HPSCRIPTVM v)
{
    const PSChar *s;
    if(PS_SUCCEEDED(ps_getstring(v,2,&s))){
        ps_pushinteger(v,scsystem(s));
        return 1;
    }
    return ps_throwerror(v,_SC("wrong param"));
}


static PSInteger _system_clock(HPSCRIPTVM v)
{
    ps_pushfloat(v,((PSFloat)clock())/(PSFloat)CLOCKS_PER_SEC);
    return 1;
}

static PSInteger _system_time(HPSCRIPTVM v)
{
    PSInteger t = (PSInteger)time(NULL);
    ps_pushinteger(v,t);
    return 1;
}

static PSInteger _system_remove(HPSCRIPTVM v)
{
    const PSChar *s;
    ps_getstring(v,2,&s);
    if(scremove(s)==-1)
        return ps_throwerror(v,_SC("remove() failed"));
    return 0;
}

static PSInteger _system_rename(HPSCRIPTVM v)
{
    const PSChar *oldn,*newn;
    ps_getstring(v,2,&oldn);
    ps_getstring(v,3,&newn);
    if(screname(oldn,newn)==-1)
        return ps_throwerror(v,_SC("rename() failed"));
    return 0;
}

static void _set_integer_slot(HPSCRIPTVM v,const PSChar *name,PSInteger val)
{
    ps_pushstring(v,name,-1);
    ps_pushinteger(v,val);
    ps_rawset(v,-3);
}

static PSInteger _system_date(HPSCRIPTVM v)
{
    time_t t;
    PSInteger it;
    PSInteger format = 'l';
    if(ps_gettop(v) > 1) {
        ps_getinteger(v,2,&it);
        t = it;
        if(ps_gettop(v) > 2) {
            ps_getinteger(v,3,(PSInteger*)&format);
        }
    }
    else {
        time(&t);
    }
    tm *date;
    if(format == 'u')
        date = gmtime(&t);
    else
        date = localtime(&t);
    if(!date)
        return ps_throwerror(v,_SC("crt api failure"));
    ps_newtable(v);
    _set_integer_slot(v, _SC("sec"), date->tm_sec);
    _set_integer_slot(v, _SC("min"), date->tm_min);
    _set_integer_slot(v, _SC("hour"), date->tm_hour);
    _set_integer_slot(v, _SC("day"), date->tm_mday);
    _set_integer_slot(v, _SC("month"), date->tm_mon);
    _set_integer_slot(v, _SC("year"), date->tm_year+1900);
    _set_integer_slot(v, _SC("wday"), date->tm_wday);
    _set_integer_slot(v, _SC("yday"), date->tm_yday);
    return 1;
}



#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
static const PSRegFunction systemlib_funcs[]={
    _DECL_FUNC(getenv,2,_SC(".s")),
    _DECL_FUNC(system,2,_SC(".s")),
    _DECL_FUNC(clock,0,NULL),
    _DECL_FUNC(time,1,NULL),
    _DECL_FUNC(date,-1,_SC(".nn")),
    _DECL_FUNC(remove,2,_SC(".s")),
    _DECL_FUNC(rename,3,_SC(".ss")),
    {NULL,(PSFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC

PSInteger psstd_register_systemlib(HPSCRIPTVM v)
{
    PSInteger i=0;
    while(systemlib_funcs[i].name!=0)
    {
        ps_pushstring(v,systemlib_funcs[i].name,-1);
        ps_newclosure(v,systemlib_funcs[i].f,0);
        ps_setparamscheck(v,systemlib_funcs[i].nparamscheck,systemlib_funcs[i].typemask);
        ps_setnativeclosurename(v,-1,systemlib_funcs[i].name);
        ps_newslot(v,-3,PSFalse);
        i++;
    }
    return 1;
}
