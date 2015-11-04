/* 
** Main ATMI unit test dispatcher.
**
** @file atmiunit1.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <cgreen/cgreen.h>
#include "atmiuni1.h"

#include <ndrstandard.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

int system_dbg(char *cmd)
{
	int ret;
	fprintf(stderr, "************ RUNNING TEST: [%s] *********************\n", cmd);
	ret=system(cmd);
	fprintf(stderr, "************ FINISHED TEST: [%s] with %d ************\n", cmd, ret);
	return ret;
}

void test001_basiccall(void)
{
    int ret;
    ret=system_dbg("test001_basiccall/run.sh");
    assert_equal(ret, SUCCEED);
}

void test002_basicforward(void)
{
    int ret;
    ret=system_dbg("test002_basicforward/run.sh");
    assert_equal(ret, SUCCEED);
}

void test003_basicconvers(void)
{
    int ret;
    ret=system_dbg("test003_basicconvers/run.sh");
    assert_equal(ret, SUCCEED);
}

void test004_basicevent(void)
{
    int ret;
    ret=system_dbg("test004_basicevent/run.sh");
    assert_equal(ret, SUCCEED);

}

void test005_convconfload(void)
{
    int ret;
    ret=system_dbg("test005_convconfload/run.sh");
    assert_equal(ret, SUCCEED);
}

void test006_ulog(void)
{
    int ret;
    ret=system_dbg("test006_ulog/run.sh");
    assert_equal(ret, SUCCEED);
}

void test007_advertise(void)
{
    int ret;
    ret=system_dbg("test007_advertise/run.sh");
    assert_equal(ret, SUCCEED);
}

void test008_extensions(void)
{
    int ret;
    ret=system_dbg("test008_extensions/run.sh");
    assert_equal(ret, SUCCEED);
}

void test009_srvdie(void)
{
    int ret;
    ret=system_dbg("test009_srvdie/run.sh");
    assert_equal(ret, SUCCEED);
}

void test010_strtracecond(void)
{
    int ret;
    ret=system_dbg("test010_strtracecond/run.sh");
    assert_equal(ret, SUCCEED);
}

void test011_tout(void)
{
    int ret;
    ret=system_dbg("test011_tout/run.sh");
    assert_equal(ret, SUCCEED);
}

void test012_admqfull(void)
{
    int ret;
    ret=system_dbg("test012_admqfull/run.sh");
    assert_equal(ret, SUCCEED);
}

void test001_basiccall_dom(void)
{
    int ret;
    ret=system_dbg("test001_basiccall/run-dom.sh");
    assert_equal(ret, SUCCEED);
}

void test002_basicforward_dom(void)
{
    int ret;
    ret=system_dbg("test002_basicforward/run-dom.sh");
    assert_equal(ret, SUCCEED);
}

void test003_basicconvers_dom(void)
{
    int ret;
    ret=system_dbg("test003_basicconvers/run-dom.sh");
    assert_equal(ret, SUCCEED);
}

void test004_basicevent_dom(void)
{
    int ret;
    ret=system_dbg("test004_basicevent/run-dom.sh");
    assert_equal(ret, SUCCEED);

}

void test013_procnorsp(void)
{
    int ret;
    ret=system_dbg("test013_procnorsp/run.sh");
    assert_equal(ret, SUCCEED);
}

void test015_threads(void)
{
    int ret;
    ret=system_dbg("test015_threads/run.sh");
    assert_equal(ret, SUCCEED);
}

void test017_srvthread(void)
{
    int ret;
    ret=system_dbg("test017_srvthread/run.sh");
    assert_equal(ret, SUCCEED);
}

void test018_tpacalltout(void)
{
    int ret;
    ret=system_dbg("test018_tpacalltout/run.sh");
    assert_equal(ret, SUCCEED);
}

void test021_xafull(void)
{
    int ret;
    ret=system_dbg("test021_xafull/run.sh");
    assert_equal(ret, SUCCEED);
}

TestSuite *atmi_test_all(void)
{
    TestSuite *suite = create_test_suite();
    add_test(suite, test001_basiccall);
    add_test(suite, test002_basicforward);
    add_test(suite, test003_basicconvers);
    add_test(suite, test004_basicevent);
    add_test(suite, test005_convconfload);
    add_test(suite, test006_ulog);
    add_test(suite, test007_advertise);
    add_test(suite, test008_extensions);
    add_test(suite, test009_srvdie);
    add_test(suite, test010_strtracecond);
    add_test(suite, test011_tout);
#ifdef SYS64BIT
    /* We should skip this on non 64bit machines! */
    add_test(suite, test012_admqfull);
#endif
    add_test(suite, test001_basiccall_dom);
    add_test(suite, test002_basicforward_dom);
    add_test(suite, test003_basicconvers_dom);
    add_test(suite, test004_basicevent_dom);
    add_test(suite, test013_procnorsp);
    add_test(suite, test015_threads);
    add_test(suite, test017_srvthread);
    add_test(suite, test018_tpacalltout);
    add_test(suite, test021_xafull);
    
    return suite;
}

/*
 * Main test case entry
 */
int main(int argc, char** argv) {

    TestSuite *suite = create_test_suite();

    add_suite(suite, atmi_test_all());


    if (argc > 1) {
        return run_single_test(suite,argv[1],create_text_reporter());
    }

    return run_test_suite(suite, create_text_reporter());
}

