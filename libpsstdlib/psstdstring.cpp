/* see copyright notice in pscript.h */
#include <pscript.h>
#include <psstdstring.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define MAX_FORMAT_LEN  20
#define MAX_WFORMAT_LEN 3
#define ADDITIONAL_FORMAT_SPACE (100*sizeof(PSChar))

static PSBool isfmtchr(PSChar ch)
{
    switch(ch) {
    case '-': case '+': case ' ': case '#': case '0': return PSTrue;
    }
    return PSFalse;
}

static PSInteger validate_format(HPSCRIPTVM v, PSChar *fmt, const PSChar *src, PSInteger n,PSInteger &width)
{
    PSChar *dummy;
    PSChar swidth[MAX_WFORMAT_LEN];
    PSInteger wc = 0;
    PSInteger start = n;
    fmt[0] = '%';
    while (isfmtchr(src[n])) n++;
    while (scisdigit(src[n])) {
        swidth[wc] = src[n];
        n++;
        wc++;
        if(wc>=MAX_WFORMAT_LEN)
            return ps_throwerror(v,_SC("width format too long"));
    }
    swidth[wc] = '\0';
    if(wc > 0) {
        width = scstrtol(swidth,&dummy,10);
    }
    else
        width = 0;
    if (src[n] == '.') {
        n++;

        wc = 0;
        while (scisdigit(src[n])) {
            swidth[wc] = src[n];
            n++;
            wc++;
            if(wc>=MAX_WFORMAT_LEN)
                return ps_throwerror(v,_SC("precision format too long"));
        }
        swidth[wc] = '\0';
        if(wc > 0) {
            width += scstrtol(swidth,&dummy,10);

        }
    }
    if (n-start > MAX_FORMAT_LEN )
        return ps_throwerror(v,_SC("format too long"));
    memcpy(&fmt[1],&src[start],((n-start)+1)*sizeof(PSChar));
    fmt[(n-start)+2] = '\0';
    return n;
}

PSRESULT psstd_format(HPSCRIPTVM v,PSInteger nformatstringidx,PSInteger *outlen,PSChar **output)
{
    const PSChar *format;
    PSChar *dest;
    PSChar fmt[MAX_FORMAT_LEN];
    ps_getstring(v,nformatstringidx,&format);
    PSInteger format_size = ps_getsize(v,nformatstringidx);
    PSInteger allocated = (format_size+2)*sizeof(PSChar);
    dest = ps_getscratchpad(v,allocated);
    PSInteger n = 0,i = 0, nparam = nformatstringidx+1, w = 0;
    //while(format[n] != '\0')
    while(n < format_size)
    {
        if(format[n] != '%') {
            assert(i < allocated);
            dest[i++] = format[n];
            n++;
        }
        else if(format[n+1] == '%') { //handles %%
                dest[i++] = '%';
                n += 2;
        }
        else {
            n++;
            if( nparam > ps_gettop(v) )
                return ps_throwerror(v,_SC("not enough paramters for the given format string"));
            n = validate_format(v,fmt,format,n,w);
            if(n < 0) return -1;
            PSInteger addlen = 0;
            PSInteger valtype = 0;
            const PSChar *ts = NULL;
            PSInteger ti = 0;
            PSFloat tf = 0;
            switch(format[n]) {
            case 's':
                if(PS_FAILED(ps_getstring(v,nparam,&ts)))
                    return ps_throwerror(v,_SC("string expected for the specified format"));
                addlen = (ps_getsize(v,nparam)*sizeof(PSChar))+((w+1)*sizeof(PSChar));
                valtype = 's';
                break;
            case 'i': case 'd': case 'o': case 'u':  case 'x':  case 'X':
#ifdef _PS64
                {
                size_t flen = scstrlen(fmt);
                PSInteger fpos = flen - 1;
                PSChar f = fmt[fpos];
                const PSChar *prec = (const PSChar *)_PRINT_INT_PREC;
                while(*prec != _SC('\0')) {
                    fmt[fpos++] = *prec++;
                }
                fmt[fpos++] = f;
                fmt[fpos++] = _SC('\0');
                }
#endif
            case 'c':
                if(PS_FAILED(ps_getinteger(v,nparam,&ti)))
                    return ps_throwerror(v,_SC("integer expected for the specified format"));
                addlen = (ADDITIONAL_FORMAT_SPACE)+((w+1)*sizeof(PSChar));
                valtype = 'i';
                break;
            case 'f': case 'g': case 'G': case 'e':  case 'E':
                if(PS_FAILED(ps_getfloat(v,nparam,&tf)))
                    return ps_throwerror(v,_SC("float expected for the specified format"));
                addlen = (ADDITIONAL_FORMAT_SPACE)+((w+1)*sizeof(PSChar));
                valtype = 'f';
                break;
            default:
                return ps_throwerror(v,_SC("invalid format"));
            }
            n++;
            allocated += addlen + sizeof(PSChar);
            dest = ps_getscratchpad(v,allocated);
            switch(valtype) {
            case 's': i += scsprintf(&dest[i],allocated,fmt,ts); break;
            case 'i': i += scsprintf(&dest[i],allocated,fmt,ti); break;
            case 'f': i += scsprintf(&dest[i],allocated,fmt,tf); break;
            };
            nparam ++;
        }
    }
    *outlen = i;
    dest[i] = '\0';
    *output = dest;
    return PS_OK;
}

static PSInteger _string_format(HPSCRIPTVM v)
{
    PSChar *dest = NULL;
    PSInteger length = 0;
    if(PS_FAILED(psstd_format(v,2,&length,&dest)))
        return -1;
    ps_pushstring(v,dest,length);
    return 1;
}

static void __strip_l(const PSChar *str,const PSChar **start)
{
    const PSChar *t = str;
    while(((*t) != '\0') && scisspace(*t)){ t++; }
    *start = t;
}

static void __strip_r(const PSChar *str,PSInteger len,const PSChar **end)
{
    if(len == 0) {
        *end = str;
        return;
    }
    const PSChar *t = &str[len-1];
    while(t >= str && scisspace(*t)) { t--; }
    *end = t + 1;
}

static PSInteger _string_strip(HPSCRIPTVM v)
{
    const PSChar *str,*start,*end;
    ps_getstring(v,2,&str);
    PSInteger len = ps_getsize(v,2);
    __strip_l(str,&start);
    __strip_r(str,len,&end);
    ps_pushstring(v,start,end - start);
    return 1;
}

static PSInteger _string_lstrip(HPSCRIPTVM v)
{
    const PSChar *str,*start;
    ps_getstring(v,2,&str);
    __strip_l(str,&start);
    ps_pushstring(v,start,-1);
    return 1;
}

static PSInteger _string_rstrip(HPSCRIPTVM v)
{
    const PSChar *str,*end;
    ps_getstring(v,2,&str);
    PSInteger len = ps_getsize(v,2);
    __strip_r(str,len,&end);
    ps_pushstring(v,str,end - str);
    return 1;
}

static PSInteger _string_split(HPSCRIPTVM v)
{
    const PSChar *str,*seps;
    PSChar *stemp;
    ps_getstring(v,2,&str);
    ps_getstring(v,3,&seps);
    PSInteger sepsize = ps_getsize(v,3);
    if(sepsize == 0) return ps_throwerror(v,_SC("empty separators string"));
    PSInteger memsize = (ps_getsize(v,2)+1)*sizeof(PSChar);
    stemp = ps_getscratchpad(v,memsize);
    memcpy(stemp,str,memsize);
    PSChar *start = stemp;
    PSChar *end = stemp;
    ps_newarray(v,0);
    while(*end != '\0')
    {
        PSChar cur = *end;
        for(PSInteger i = 0; i < sepsize; i++)
        {
            if(cur == seps[i])
            {
                *end = 0;
                ps_pushstring(v,start,-1);
                ps_arrayappend(v,-2);
                start = end + 1;
                break;
            }
        }
        end++;
    }
    if(end != start)
    {
        ps_pushstring(v,start,-1);
        ps_arrayappend(v,-2);
    }
    return 1;
}

static PSInteger _string_escape(HPSCRIPTVM v)
{
    const PSChar *str;
    PSChar *dest,*resstr;
    PSInteger size;
    ps_getstring(v,2,&str);
    size = ps_getsize(v,2);
    if(size == 0) {
        ps_push(v,2);
        return 1;
    }
#ifdef PSUNICODE
#if WCHAR_SIZE == 2
    const PSChar *escpat = _SC("\\x%04x");
    const PSInteger maxescsize = 6;
#else //WCHAR_SIZE == 4
    const PSChar *escpat = _SC("\\x%08x");
    const PSInteger maxescsize = 10;
#endif
#else
    const PSChar *escpat = _SC("\\x%02x");
    const PSInteger maxescsize = 4;
#endif
    PSInteger destcharsize = (size * maxescsize); //assumes every char could be escaped
    resstr = dest = (PSChar *)ps_getscratchpad(v,destcharsize * sizeof(PSChar));
    PSChar c;
    PSChar escch;
    PSInteger escaped = 0;
    for(int n = 0; n < size; n++){
        c = *str++;
        escch = 0;
        if(scisprint(c) || c == 0) {
            switch(c) {
            case '\a': escch = 'a'; break;
            case '\b': escch = 'b'; break;
            case '\t': escch = 't'; break;
            case '\n': escch = 'n'; break;
            case '\v': escch = 'v'; break;
            case '\f': escch = 'f'; break;
            case '\r': escch = 'r'; break;
            case '\\': escch = '\\'; break;
            case '\"': escch = '\"'; break;
            case '\'': escch = '\''; break;
            case 0: escch = '0'; break;
            }
            if(escch) {
                *dest++ = '\\';
                *dest++ = escch;
                escaped++;
            }
            else {
                *dest++ = c;
            }
        }
        else {

            dest += scsprintf(dest, destcharsize, escpat, c);
            escaped++;
        }
    }

    if(escaped) {
        ps_pushstring(v,resstr,dest - resstr);
    }
    else {
        ps_push(v,2); //nothing escaped
    }
    return 1;
}

static PSInteger _string_startswith(HPSCRIPTVM v)
{
    const PSChar *str,*cmp;
    ps_getstring(v,2,&str);
    ps_getstring(v,3,&cmp);
    PSInteger len = ps_getsize(v,2);
    PSInteger cmplen = ps_getsize(v,3);
    PSBool ret = PSFalse;
    if(cmplen <= len) {
        ret = memcmp(str,cmp,ps_rsl(cmplen)) == 0 ? PSTrue : PSFalse;
    }
    ps_pushbool(v,ret);
    return 1;
}

static PSInteger _string_endswith(HPSCRIPTVM v)
{
    const PSChar *str,*cmp;
    ps_getstring(v,2,&str);
    ps_getstring(v,3,&cmp);
    PSInteger len = ps_getsize(v,2);
    PSInteger cmplen = ps_getsize(v,3);
    PSBool ret = PSFalse;
    if(cmplen <= len) {
        ret = memcmp(&str[len - cmplen],cmp,ps_rsl(cmplen)) == 0 ? PSTrue : PSFalse;
    }
    ps_pushbool(v,ret);
    return 1;
}

#define SETUP_REX(v) \
    PSRex *self = NULL; \
    ps_getinstanceup(v,1,(PSUserPointer *)&self,0);

static PSInteger _rexobj_releasehook(PSUserPointer p, PSInteger PS_UNUSED_ARG(size))
{
    PSRex *self = ((PSRex *)p);
    psstd_rex_free(self);
    return 1;
}

static PSInteger _regexp_match(HPSCRIPTVM v)
{
    SETUP_REX(v);
    const PSChar *str;
    ps_getstring(v,2,&str);
    if(psstd_rex_match(self,str) == PSTrue)
    {
        ps_pushbool(v,PSTrue);
        return 1;
    }
    ps_pushbool(v,PSFalse);
    return 1;
}

static void _addrexmatch(HPSCRIPTVM v,const PSChar *str,const PSChar *begin,const PSChar *end)
{
    ps_newtable(v);
    ps_pushstring(v,_SC("begin"),-1);
    ps_pushinteger(v,begin - str);
    ps_rawset(v,-3);
    ps_pushstring(v,_SC("end"),-1);
    ps_pushinteger(v,end - str);
    ps_rawset(v,-3);
}

static PSInteger _regexp_search(HPSCRIPTVM v)
{
    SETUP_REX(v);
    const PSChar *str,*begin,*end;
    PSInteger start = 0;
    ps_getstring(v,2,&str);
    if(ps_gettop(v) > 2) ps_getinteger(v,3,&start);
    if(psstd_rex_search(self,str+start,&begin,&end) == PSTrue) {
        _addrexmatch(v,str,begin,end);
        return 1;
    }
    return 0;
}

static PSInteger _regexp_capture(HPSCRIPTVM v)
{
    SETUP_REX(v);
    const PSChar *str,*begin,*end;
    PSInteger start = 0;
    ps_getstring(v,2,&str);
    if(ps_gettop(v) > 2) ps_getinteger(v,3,&start);
    if(psstd_rex_search(self,str+start,&begin,&end) == PSTrue) {
        PSInteger n = psstd_rex_getsubexpcount(self);
        PSRexMatch match;
        ps_newarray(v,0);
        for(PSInteger i = 0;i < n; i++) {
            psstd_rex_getsubexp(self,i,&match);
            if(match.len > 0)
                _addrexmatch(v,str,match.begin,match.begin+match.len);
            else
                _addrexmatch(v,str,str,str); //empty match
            ps_arrayappend(v,-2);
        }
        return 1;
    }
    return 0;
}

static PSInteger _regexp_subexpcount(HPSCRIPTVM v)
{
    SETUP_REX(v);
    ps_pushinteger(v,psstd_rex_getsubexpcount(self));
    return 1;
}

static PSInteger _regexp_constructor(HPSCRIPTVM v)
{
    const PSChar *error,*pattern;
    ps_getstring(v,2,&pattern);
    PSRex *rex = psstd_rex_compile(pattern,&error);
    if(!rex) return ps_throwerror(v,error);
    ps_setinstanceup(v,1,rex);
    ps_setreleasehook(v,1,_rexobj_releasehook);
    return 0;
}

static PSInteger _regexp__typeof(HPSCRIPTVM v)
{
    ps_pushstring(v,_SC("regexp"),-1);
    return 1;
}

#define _DECL_REX_FUNC(name,nparams,pmask) {_SC(#name),_regexp_##name,nparams,pmask}
static const PSRegFunction rexobj_funcs[]={
    _DECL_REX_FUNC(constructor,2,_SC(".s")),
    _DECL_REX_FUNC(search,-2,_SC("xsn")),
    _DECL_REX_FUNC(match,2,_SC("xs")),
    _DECL_REX_FUNC(capture,-2,_SC("xsn")),
    _DECL_REX_FUNC(subexpcount,1,_SC("x")),
    _DECL_REX_FUNC(_typeof,1,_SC("x")),
    {NULL,(PSFUNCTION)0,0,NULL}
};
#undef _DECL_REX_FUNC

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_string_##name,nparams,pmask}
static const PSRegFunction stringlib_funcs[]={
    _DECL_FUNC(format,-2,_SC(".s")),
    _DECL_FUNC(strip,2,_SC(".s")),
    _DECL_FUNC(lstrip,2,_SC(".s")),
    _DECL_FUNC(rstrip,2,_SC(".s")),
    _DECL_FUNC(split,3,_SC(".ss")),
    _DECL_FUNC(escape,2,_SC(".s")),
    _DECL_FUNC(startswith,3,_SC(".ss")),
    _DECL_FUNC(endswith,3,_SC(".ss")),
    {NULL,(PSFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC


PSInteger psstd_register_stringlib(HPSCRIPTVM v)
{
    ps_pushstring(v,_SC("regexp"),-1);
    ps_newclass(v,PSFalse);
    PSInteger i = 0;
    while(rexobj_funcs[i].name != 0) {
        const PSRegFunction &f = rexobj_funcs[i];
        ps_pushstring(v,f.name,-1);
        ps_newclosure(v,f.f,0);
        ps_setparamscheck(v,f.nparamscheck,f.typemask);
        ps_setnativeclosurename(v,-1,f.name);
        ps_newslot(v,-3,PSFalse);
        i++;
    }
    ps_newslot(v,-3,PSFalse);

    i = 0;
    while(stringlib_funcs[i].name!=0)
    {
        ps_pushstring(v,stringlib_funcs[i].name,-1);
        ps_newclosure(v,stringlib_funcs[i].f,0);
        ps_setparamscheck(v,stringlib_funcs[i].nparamscheck,stringlib_funcs[i].typemask);
        ps_setnativeclosurename(v,-1,stringlib_funcs[i].name);
        ps_newslot(v,-3,PSFalse);
        i++;
    }
    return 1;
}
