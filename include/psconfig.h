
#ifdef _PS64

#ifdef _MSC_VER
typedef __int64 PSInteger;
typedef unsigned __int64 PSUnsignedInteger;
typedef unsigned __int64 PSHash; /*should be the same size of a pointer*/
#else
typedef long long PSInteger;
typedef unsigned long long PSUnsignedInteger;
typedef unsigned long long PSHash; /*should be the same size of a pointer*/
#endif
typedef int PSInt32;
typedef unsigned int PSUnsignedInteger32;
#else
typedef int PSInteger;
typedef int PSInt32; /*must be 32 bits(also on 64bits processors)*/
typedef unsigned int PSUnsignedInteger32; /*must be 32 bits(also on 64bits processors)*/
typedef unsigned int PSUnsignedInteger;
typedef unsigned int PSHash; /*should be the same size of a pointer*/
#endif


#ifdef PSUSEDOUBLE
typedef double PSFloat;
#else
typedef float PSFloat;
#endif

#if defined(PSUSEDOUBLE) && !defined(_PS64) || !defined(PSUSEDOUBLE) && defined(_PS64)
#ifdef _MSC_VER
typedef __int64 PSRawObjectVal; //must be 64bits
#else
typedef long long PSRawObjectVal; //must be 64bits
#endif
#define PS_OBJECT_RAWINIT() { _unVal.raw = 0; }
#else
typedef PSUnsignedInteger PSRawObjectVal; //is 32 bits on 32 bits builds and 64 bits otherwise
#define PS_OBJECT_RAWINIT()
#endif

#ifndef PS_ALIGNMENT // PS_ALIGNMENT shall be less than or equal to PS_MALLOC alignments, and its value shall be power of 2.
#if defined(PSUSEDOUBLE) || defined(_PS64)
#define PS_ALIGNMENT 8
#else
#define PS_ALIGNMENT 4
#endif
#endif

typedef void* PSUserPointer;
typedef PSUnsignedInteger PSBool;
typedef PSInteger PSRESULT;

#ifdef PSUNICODE
#include <wchar.h>
#include <wctype.h>


typedef wchar_t PSChar;


#define scstrcmp    wcscmp
#ifdef _WIN32
#define scsprintf   _snwprintf
#else
#define scsprintf   swprintf
#endif
#define scstrlen    wcslen
#define scstrtod    wcstod
#ifdef _PS64
#define scstrtol    wcstoll
#else
#define scstrtol    wcstol
#endif
#define scstrtoul   wcstoul
#define scvsprintf  vswprintf
#define scstrstr    wcsstr
#define scprintf    wprintf

#ifdef _WIN32
#define WCHAR_SIZE 2
#define WCHAR_SHIFT_MUL 1
#define MAX_CHAR 0xFFFF
#else
#define WCHAR_SIZE 4
#define WCHAR_SHIFT_MUL 2
#define MAX_CHAR 0xFFFFFFFF
#endif

#define _SC(a) L##a


#define scisspace   iswspace
#define scisdigit   iswdigit
#define scisprint   iswprint
#define scisxdigit  iswxdigit
#define scisalpha   iswalpha
#define sciscntrl   iswcntrl
#define scisalnum   iswalnum


#define ps_rsl(l) ((l)<<WCHAR_SHIFT_MUL)

#else
typedef char PSChar;
#define _SC(a) a
#define scstrcmp    strcmp
#ifdef _MSC_VER
#define scsprintf   _snprintf
#else
#define scsprintf   snprintf
#endif
#define scstrlen    strlen
#define scstrtod    strtod
#ifdef _PS64
#ifdef _MSC_VER
#define scstrtol    _strtoi64
#else
#define scstrtol    strtoll
#endif
#else
#define scstrtol    strtol
#endif
#define scstrtoul   strtoul
#define scvsprintf  vsnprintf
#define scstrstr    strstr
#define scisspace   isspace
#define scisdigit   isdigit
#define scisprint   isprint
#define scisxdigit  isxdigit
#define sciscntrl   iscntrl
#define scisalpha   isalpha
#define scisalnum   isalnum
#define scprintf    printf
#define MAX_CHAR 0xFF

#define ps_rsl(l) (l)

#endif

#ifdef _PS64
#define _PRINT_INT_PREC _SC("ll")
#define _PRINT_INT_FMT _SC("%lld")
#else
#define _PRINT_INT_FMT _SC("%d")
#endif
