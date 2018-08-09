/**
 * @brief Basic PScript tests...
 *
 * @file atmiclt27.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include "test027.h"
#include <pscript.h>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <cgreen/cgreen.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

void printfunc(HPSCRIPTVM v,const PSChar *s,...)
{
        va_list vl;
        va_start(vl, s);
        vfprintf(stdout, s, vl);
        va_end(vl);
}


void errorfunc(HPSCRIPTVM v,const PSChar *s,...)
{
        va_list vl;
        va_start(vl, s);
        vfprintf(stderr, s, vl);
        va_end(vl);
}

//int main(int argc, char* argv[])
Ensure(basic_script_func_call)
{
    HPSCRIPTVM v;
    v = ps_open(1024); //creates a VM with initial stack size 1024

    const PSChar *err;
    ps_setprintfunc(v,printfunc,errorfunc);

    char *script = "function testHello(name){return name+\" Hello World\";}";
    const PSChar *s;
    //do some stuff with pscript here
    if (PS_FAILED(ps_compilebuffer(v, script, strlen(script), "hello.psc", PSTrue)))
    {
        printf("Failed to compile...\n");

        if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
        {
            printf(_SC("Error [%s]\n"),err);
            assert_true(0); //FAIL
            return;
        }
    }
    else
    {
       ps_pushroottable(v);
    }

    /* Load the function */
    if (PS_FAILED(ps_call(v,1,PSTrue, PSTrue)))
    {
        printf("Failed to load...\n");
        ps_getlasterror(v);
        if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
        {
            printf(_SC("Error [%s]\n"),err);
            assert_true(0); //FAIL
            return;
        }
    }

    /* Finally call the stuff .*/
    ps_pushroottable(v);
    ps_pushstring(v,"testHello",-1);
    ps_get(v,-2); //get the function from the root table
    /* what is this? */
    ps_pushroottable(v);
    ps_pushstring(v, "Jimbo", -1);
    
    //Use this no params given to script
    //ps_push(v,-4);

    if (PS_FAILED(ps_call(v,2,PSTrue, PSTrue)))
    {
        printf("Failed to call...\n");
        ps_getlasterror(v);
        if(PS_SUCCEEDED(ps_getstring(v,-1,&err)))
        {
              printf(_SC("Error [%s]\n"),err);
              assert_true(0); //FAIL
              return;
        }
    }
    
    ps_getstring(v,-1,&s);
    printf("Got result: [%s]\n", s);
    assert_string_equal(s, "Jimbo Hello World");
    
    ps_pop(v,3); //pops the roottable and the function

    ps_close(v);
}


TestSuite *pscrit_test_all(void)
{
    TestSuite *suite = create_test_suite();
    add_test(suite, basic_script_func_call);

    return suite;
}

/*
 * Main test case entry
 */
int main(int argc, char** argv)
{

    int ret;
    TestSuite *suite = create_test_suite();

    add_suite(suite, pscrit_test_all());


    if (argc > 1) {
        ret=run_single_test(suite,argv[1],create_text_reporter());
    }
    else
    {
        ret=run_test_suite(suite, create_text_reporter());
    }

    destroy_test_suite(suite);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
