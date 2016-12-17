/*
    see copyright notice in pscript.h
*/
#include "pspcheader.h"
#ifndef PS_EXCLUDE_DEFAULT_MEMFUNCTIONS
void *ps_vm_malloc(PSUnsignedInteger size){ return malloc(size); }

void *ps_vm_realloc(void *p, PSUnsignedInteger PS_UNUSED_ARG(oldsize), PSUnsignedInteger size){ return realloc(p, size); }

void ps_vm_free(void *p, PSUnsignedInteger PS_UNUSED_ARG(size)){ free(p); }
#endif
