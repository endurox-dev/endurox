/*	see copyright notice in pscript.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif
#include <pscript.h>
#include <psstdblob.h>
#include <psstdsystem.h>
#include <psstdio.h>
#include <psstdmath.h>	
#include <psstdstring.h>
#include <psstdaux.h>

#ifdef PSUNICODE
#define scfprintf fwprintf
#define scfopen	_wfopen
#define scvprintf vfwprintf
#else
#define scfprintf fprintf
#define scfopen	fopen
#define scvprintf vfprintf
#endif


void PrintVersionInfos();

#if defined(_MSC_VER) && defined(_DEBUG)
int MemAllocHook( int allocType, void *userData, size_t size, int blockType, 
   long requestNumber, const unsigned char *filename, int lineNumber)
{
	//if(requestNumber==769)_asm int 3;
	return 1;
}
#endif


PSInteger quit(HPSCRIPTVM v)
{
	int *done;
	ps_getuserpointer(v,-1,(PSUserPointer*)&done);
	*done=1;
	return 0;
}

void printfunc(HPSCRIPTVM v,const PSChar *s,...)
{
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
}

void errorfunc(HPSCRIPTVM v,const PSChar *s,...)
{
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

void PrintVersionInfos()
{
	scfprintf(stdout,_SC("%s %s (%d bits)\n"),PSCRIPT_VERSION,PSCRIPT_COPYRIGHT,((int)(sizeof(PSInteger)*8)));
	scfprintf(stdout,_SC("Enduro/X Platform Script"));
}

void PrintUsage()
{
	scfprintf(stderr,_SC("usage: ps <options> <scriptpath [args]>.\n")
		_SC("Available options are:\n")
		_SC("   -c              compiles the file to bytecode(default output 'out.cnut')\n")
		_SC("   -o              specifies output file for the -c option\n")
		_SC("   -c              compiles only\n")
		_SC("   -d              generates debug infos\n")
		_SC("   -v              displays version infos\n")
		_SC("   -h              prints help\n"));
}

#define _INTERACTIVE 0
#define _DONE 2
#define _ERROR 3
//<<FIXME>> this func is a mess
int getargs(HPSCRIPTVM v,int argc, char* argv[],PSInteger *retval)
{
	int i;
	int compiles_only = 0;
	static PSChar temp[500];
	const PSChar *ret=NULL;
	char * output = NULL;
	int lineinfo=0;
	*retval = 0;
	if(argc>1)
	{
		int arg=1,exitloop=0;
		
		while(arg < argc && !exitloop)
		{

			if(argv[arg][0]=='-')
			{
				switch(argv[arg][1])
				{
				case 'd': //DEBUG(debug infos)
					ps_enabledebuginfo(v,1);
					break;
				case 'c':
					compiles_only = 1;
					break;
				case 'o':
					if(arg < argc) {
						arg++;
						output = argv[arg];
					}
					break;
				case 'v':
					PrintVersionInfos();
					return _DONE;
				
				case 'h':
					PrintVersionInfos();
					PrintUsage();
					return _DONE;
				default:
					PrintVersionInfos();
					scprintf(_SC("unknown prameter '-%c'\n"),argv[arg][1]);
					PrintUsage();
					*retval = -1;
					return _ERROR;
				}
			}else break;
			arg++;
		}

		// src file
		
		if(arg<argc) {
			const PSChar *filename=NULL;
#ifdef PSUNICODE
			mbstowcs(temp,argv[arg],strlen(argv[arg]));
			filename=temp;
#else
			filename=argv[arg];
#endif

			arg++;
			
			//ps_pushstring(v,_SC("ARGS"),-1);
			//ps_newarray(v,0);
			
			//ps_createslot(v,-3);
			//ps_pop(v,1);
			if(compiles_only) {
				if(PS_SUCCEEDED(psstd_loadfile(v,filename,PSTrue))){
					const PSChar *outfile = _SC("out.cnut");
					if(output) {
#ifdef PSUNICODE
						int len = (int)(strlen(output)+1);
						mbstowcs(ps_getscratchpad(v,len*sizeof(PSChar)),output,len);
						outfile = ps_getscratchpad(v,-1);
#else
						outfile = output;
#endif
					}
					if(PS_SUCCEEDED(psstd_writeclosuretofile(v,outfile)))
						return _DONE;
				}
			}
			else {
				//if(PS_SUCCEEDED(psstd_dofile(v,filename,PSFalse,PSTrue))) {
					//return _DONE;
				//}
				if(PS_SUCCEEDED(psstd_loadfile(v,filename,PSTrue))) {
					int callargs = 1;
					ps_pushroottable(v);
					for(i=arg;i<argc;i++)
					{
						const PSChar *a;
#ifdef PSUNICODE
						int alen=(int)strlen(argv[i]);
						a=ps_getscratchpad(v,(int)(alen*sizeof(PSChar)));
						mbstowcs(ps_getscratchpad(v,-1),argv[i],alen);
						ps_getscratchpad(v,-1)[alen] = _SC('\0');
#else
						a=argv[i];
#endif
						ps_pushstring(v,a,-1);
						callargs++;
						//ps_arrayappend(v,-2);
					}
					if(PS_SUCCEEDED(ps_call(v,callargs,PSTrue,PSTrue))) {
						PSObjectType type = ps_gettype(v,-1);
						if(type == OT_INTEGER) {
							*retval = type;
							ps_getinteger(v,-1,retval);
						}
						return _DONE;
					}
					else{
						return _ERROR;
					}
					
				}
			}
			//if this point is reached an error occured
			{
				const PSChar *err;
				ps_getlasterror(v);
				if(PS_SUCCEEDED(ps_getstring(v,-1,&err))) {
					scprintf(_SC("Error [%s]\n"),err);
					*retval = -2;
					return _ERROR;
				}
			}
			
		}
	}

	return _INTERACTIVE;
}

void Interactive(HPSCRIPTVM v)
{
	
#define MAXINPUT 1024
	PSChar buffer[MAXINPUT];
	PSInteger blocks =0;
	PSInteger string=0;
	PSInteger retval=0;
	PSInteger done=0;
	PrintVersionInfos();
		
	ps_pushroottable(v);
	ps_pushstring(v,_SC("quit"),-1);
	ps_pushuserpointer(v,&done);
	ps_newclosure(v,quit,1);
	ps_setparamscheck(v,1,NULL);
	ps_newslot(v,-3,PSFalse);
	ps_pop(v,1);

    while (!done) 
	{
		PSInteger i = 0;
		scprintf(_SC("\nps>"));
		for(;;) {
			int c;
			if(done)return;
			c = getchar();
			if (c == _SC('\n')) {
				if (i>0 && buffer[i-1] == _SC('\\'))
				{
					buffer[i-1] = _SC('\n');
				}
				else if(blocks==0)break;
				buffer[i++] = _SC('\n');
			}
			else if (c==_SC('}')) {blocks--; buffer[i++] = (PSChar)c;}
			else if(c==_SC('{') && !string){
					blocks++;
					buffer[i++] = (PSChar)c;
			}
			else if(c==_SC('"') || c==_SC('\'')){
					string=!string;
					buffer[i++] = (PSChar)c;
			}
			else if (i >= MAXINPUT-1) {
				scfprintf(stderr, _SC("ps : input line too long\n"));
				break;
			}
			else{
				buffer[i++] = (PSChar)c;
			}
		}
		buffer[i] = _SC('\0');
		
		if(buffer[0]==_SC('=')){
			scsprintf(ps_getscratchpad(v,MAXINPUT),_SC("return (%s)"),&buffer[1]);
			memcpy(buffer,ps_getscratchpad(v,-1),(scstrlen(ps_getscratchpad(v,-1))+1)*sizeof(PSChar));
			retval=1;
		}
		i=scstrlen(buffer);
		if(i>0){
			PSInteger oldtop=ps_gettop(v);
			if(PS_SUCCEEDED(ps_compilebuffer(v,buffer,i,_SC("interactive console"),PSTrue))){
				ps_pushroottable(v);
				if(PS_SUCCEEDED(ps_call(v,1,retval,PSTrue)) &&	retval){
					scprintf(_SC("\n"));
					ps_pushroottable(v);
					ps_pushstring(v,_SC("print"),-1);
					ps_get(v,-2);
					ps_pushroottable(v);
					ps_push(v,-4);
					ps_call(v,2,PSFalse,PSTrue);
					retval=0;
					scprintf(_SC("\n"));
				}
			}
			
			ps_settop(v,oldtop);
		}
	}
}

int main(int argc, char* argv[])
{
	HPSCRIPTVM v;
	PSInteger retval = 0;
	const PSChar *filename=NULL;
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetAllocHook(MemAllocHook);
#endif
	
	v=ps_open(1024);
	ps_setprintfunc(v,printfunc,errorfunc);

	ps_pushroottable(v);

	psstd_register_bloblib(v);
	psstd_register_iolib(v);
	psstd_register_systemlib(v);
	psstd_register_mathlib(v);
	psstd_register_stringlib(v);

	//aux library
	//sets error handlers
	psstd_seterrorhandlers(v);

	//gets arguments
	switch(getargs(v,argc,argv,&retval))
	{
	case _INTERACTIVE:
		Interactive(v);
		break;
	case _DONE:
	case _ERROR:
	default: 
		break;
	}

	ps_close(v);
	
#if defined(_MSC_VER) && defined(_DEBUG)
	_getch();
	_CrtMemDumpAllObjectsSince( NULL );
#endif
	return retval;
}

