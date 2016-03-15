/*
	see copyright notice in pscript.h
*/
#include "pspcheader.h"
void *ps_vm_malloc(PSUnsignedInteger size){	return malloc(size); }

void *ps_vm_realloc(void *p, PSUnsignedInteger oldsize, PSUnsignedInteger size){ return realloc(p, size); }

void ps_vm_free(void *p, PSUnsignedInteger size){	free(p); }
