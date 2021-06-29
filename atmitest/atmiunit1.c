/**
 * @brief Main ATMI unit test dispatcher.
 *
 * @file atmiunit1.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
 * -----------------------------------------------------------------------------
 * AGPL license:
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <ndrx_config.h>
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

Ensure(test000_system)
{
    int ret;
    ret=system_dbg("test000_system/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test001_basiccall)
{
    int ret;
    ret=system_dbg("test001_basiccall/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test002_basicforward)
{
    int ret;
    ret=system_dbg("test002_basicforward/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test003_basicconvers)
{
    int ret;
    ret=system_dbg("test003_basicconvers/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test004_basicevent)
{
    int ret;
    ret=system_dbg("test004_basicevent/run.sh");
    assert_equal(ret, EXSUCCEED);

}

Ensure(test005_convconfload)
{
    int ret;
    ret=system_dbg("test005_convconfload/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test006_ulog)
{
    int ret;
    ret=system_dbg("test006_ulog/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test007_advertise)
{
    int ret;
    ret=system_dbg("test007_advertise/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test008_extensions)
{
    int ret;
    ret=system_dbg("test008_extensions/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test009_srvdie)
{
    int ret;
    ret=system_dbg("test009_srvdie/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test010_strtracecond)
{
    int ret;
    ret=system_dbg("test010_strtracecond/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test011_tout)
{
    int ret;
    ret=system_dbg("test011_tout/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test012_admqfull)
{
    int ret;
    ret=system_dbg("test012_admqfull/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test001_basiccall_dom)
{
    int ret;
    ret=system_dbg("test001_basiccall/run-dom.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test002_basicforward_dom)
{
    int ret;
    ret=system_dbg("test002_basicforward/run-dom.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test003_basicconvers_dom)
{
    int ret;
    ret=system_dbg("test003_basicconvers/run-dom.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test004_basicevent_dom)
{
    int ret;
    ret=system_dbg("test004_basicevent/run-dom.sh");
    assert_equal(ret, EXSUCCEED);

}

Ensure(test013_procnorsp)
{
    int ret;
    ret=system_dbg("test013_procnorsp/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test015_threads)
{
    int ret;
    ret=system_dbg("test015_threads/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test017_srvthread)
{
    int ret;
    ret=system_dbg("test017_srvthread/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test018_tpacalltout)
{
    int ret;
    ret=system_dbg("test018_tpacalltout/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test021_xafull)
{
    int ret;
    ret=system_dbg("test021_xafull/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test022_typedstring)
{
    int ret;
    ret=system_dbg("test022_typedstring/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test023_typedcarray)
{
    int ret;
    ret=system_dbg("test023_typedcarray/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test024_json)
{
    int ret;
    ret=system_dbg("test024_json/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test025_cpmsrv)
{
    int ret;
    ret=system_dbg("test025_cpmsrv/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test026_typedjson)
{
    int ret;
    ret=system_dbg("test026_typedjson/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test027_pscript)
{
    int ret;
    ret=system_dbg("test027_pscript/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test028_tmq)
{
    int ret;
    ret=system_dbg("test028_tmq/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test029_inicfg)
{
    int ret;
    ret=system_dbg("test029_inicfg/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test030_cconfsrv)
{
    int ret;
    ret=system_dbg("test030_cconfsrv/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test031_logging)
{
    int ret;
    ret=system_dbg("test031_logging/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test032_oapi)
{
    int ret;
    ret=system_dbg("test032_oapi/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test033_provision)
{
    int ret;
    ret=system_dbg("test033_provision/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test034_shmreuse)
{
    int ret;
    ret=system_dbg("test034_shmreuse/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test035_hkeep)
{
    int ret;
    ret=system_dbg("test035_hkeep/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test036_tprecover)
{
    int ret;
    ret=system_dbg("test036_tprecover/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test037_down)
{
    int ret;
    ret=system_dbg("test037_down/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test038_tpnotify)
{
    int ret;
    ret=system_dbg("test038_tpnotify/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test039_tpbroadcast)
{
    int ret;
    ret=system_dbg("test039_tpbroadcast/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test040_typedview)
{
    int ret;
    ret=system_dbg("test040_typedview/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test040_typedview_dom)
{
    int ret;
    ret=system_dbg("test040_typedview/run-dom.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test041_bigmsg)
{
    int ret;
    ret=system_dbg("test041_bigmsg/run.sh");
    assert_equal(ret, EXSUCCEED);
}
Ensure(test042_bignet)
{
    int ret;
    ret=system_dbg("test042_bignet/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test043_encrypt)
{
    int ret;
    ret=system_dbg("test043_encrypt/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test044_ping)
{
    int ret;
    ret=system_dbg("test044_ping/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test045_tpcallnoblock)
{
    int ret;
    ret=system_dbg("test045_tpcallnoblock/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test046_twopasscfg)
{
    int ret;
    ret=system_dbg("test046_twopasscfg/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test047_oradb)
{
    int ret;
    ret=system_dbg("test047_oradb/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test048_cache)
{
    int ret;
    ret=system_dbg("test048_cache/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test049_masksvc)
{
    int ret;
    ret=system_dbg("test049_masksvc/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test050_ubfdb)
{
    int ret;
    ret=system_dbg("test050_ubfdb/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test051_settout)
{
    int ret;
    ret=system_dbg("test051_settout/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test052_minstart)
{
    int ret;
    ret=system_dbg("test052_minstart/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test053_logoff)
{
    int ret;
    ret=system_dbg("test053_logoff/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test054_svwrap)
{
    int ret;
    ret=system_dbg("test054_svwrap/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test055_envs)
{
    int ret;
    ret=system_dbg("test055_envs/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test056_tpimpexp)
{
    int ret;
    ret=system_dbg("test056_tpimpexp/run.sh");
    assert_equal(ret, EXSUCCEED);
}


Ensure(test057_invlcmd)
{
    int ret;
    ret=system_dbg("test057_invlcmd/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test058_systemv)
{
    int ret;
    ret=system_dbg("test058_systemv/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test059_pq)
{
    int ret;
    ret=system_dbg("test059_pq/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test060_ndxdfork)
{
    int ret;
    ret=system_dbg("test060_ndxdfork/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test061_ndxddup)
{
    int ret;
    ret=system_dbg("test061_ndxddup/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test062_memlimits)
{
    int ret;
    ret=system_dbg("test062_memlimits/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test063_cpmrange)
{
    int ret;
    ret=system_dbg("test063_cpmrange/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test064_bufswitch)
{
    int ret;
    ret=system_dbg("test064_bufswitch/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test065_tpcancel)
{
    int ret;
    ret=system_dbg("test065_tpcancel/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test066_tmstartserver)
{
    int ret;
    ret=system_dbg("test066_tmstartserver/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test067_postgres)
{
    int ret;
    ret=system_dbg("test067_postgres/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test068_tpadm)
{
    int ret;
    ret=system_dbg("test068_tpadm/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test069_wnormal)
{
    int ret;
    ret=system_dbg("test069_wnormal/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test070_ubfhook)
{
    int ret;
    ret=system_dbg("test070_ubfhook/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test071_buildtools)
{
    int ret;
    ret=system_dbg("test071_buildtools/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test072_qos)
{
    int ret;
    ret=system_dbg("test072_qos/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test073_netact)
{
    int ret;
    ret=system_dbg("test073_netact/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test074_sanitulog)
{
    int ret;
    ret=system_dbg("test074_sanitulog/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test075_dispthread)
{
    int ret;
    ret=system_dbg("test075_dispthread/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test076_dispthreadxa)
{
    int ret;
    ret=system_dbg("test076_dispthreadxa/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test077_diedslowstrt)
{
    int ret;
    ret=system_dbg("test077_diedslowstrt/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test078_tpacallmix)
{
    int ret;
    ret=system_dbg("test078_tpacallmix/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test079_cliquote)
{
    int ret;
    ret=system_dbg("test079_cliquote/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test080_tpexit)
{
    int ret;
    ret=system_dbg("test080_tpexit/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test081_lcf)
{
    int ret;
    ret=system_dbg("test081_lcf/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test082_autotran)
{
    int ret;
    ret=system_dbg("test082_autotran/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test083_ddrsyntax)
{
    int ret;
    ret=system_dbg("test083_ddrsyntax/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test084_ddr)
{
    int ret;
    ret=system_dbg("test084_ddr/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test085_prio)
{
    int ret;
    ret=system_dbg("test085_prio/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test086_tmqlimit)
{
    int ret;
    ret=system_dbg("test086_tmqlimit/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test087_tmsrv)
{
    int ret;
    ret=system_dbg("test087_tmsrv/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test088_addlog)
{
    int ret;
    ret=system_dbg("test088_addlog/run.sh");
    assert_equal(ret, EXSUCCEED);
}

Ensure(test089_tmrecover)
{
    int ret;
    ret=system_dbg("test089_tmrecover/run.sh");
    assert_equal(ret, EXSUCCEED);
}

TestSuite *atmi_test_all(void)
{
    TestSuite *suite = create_test_suite();
    add_test(suite, test000_system);
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

#ifndef EX_OS_CYGWIN
#ifdef SYS64BIT
    /* We should skip this on non 64bit machines! */
    add_test(suite, test012_admqfull);
#endif
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
    add_test(suite, test022_typedstring);
    add_test(suite, test023_typedcarray);
    add_test(suite, test024_json);
    add_test(suite, test025_cpmsrv);
    add_test(suite, test026_typedjson);
#ifndef NDRX_DISABLEPSCRIPT
    add_test(suite, test027_pscript);
#endif
    add_test(suite, test028_tmq);
    add_test(suite, test029_inicfg);
    add_test(suite, test030_cconfsrv);
    add_test(suite, test031_logging);
    add_test(suite, test032_oapi);
    
#ifndef NDRX_DISABLEPSCRIPT
    add_test(suite, test033_provision);
#endif    
    
    /* Feature #139 mvitolin, 09/05/2017 */
    add_test(suite, test034_shmreuse);
    
    /* mvitolin Bug #112, 18/05/2017 */
    add_test(suite, test035_hkeep);
    /* mvitolin Bug #110, 22/05/2017 */
    add_test(suite, test036_tprecover);
    
    /* Bug #133 */
    add_test(suite, test037_down);
    
    add_test(suite, test038_tpnotify);
    add_test(suite, test039_tpbroadcast);
    
    add_test(suite, test040_typedview);
    
    add_test(suite, test040_typedview_dom);

#ifdef SYS64BIT
#if defined(EX_OS_LINUX)
    add_test(suite, test041_bigmsg);
    add_test(suite,test042_bignet);
#endif
#endif

    add_test(suite,test043_encrypt);
    add_test(suite,test044_ping);
    add_test(suite,test045_tpcallnoblock);
    add_test(suite,test046_twopasscfg);
#ifdef NDRX_ENABLE_TEST47
    add_test(suite,test047_oradb);
#endif
    add_test(suite,test048_cache);
    add_test(suite,test049_masksvc);
    add_test(suite,test050_ubfdb);
    add_test(suite,test051_settout);
    add_test(suite,test052_minstart);
    add_test(suite,test053_logoff);
    add_test(suite,test054_svwrap);
    add_test(suite,test055_envs);
    add_test(suite,test056_tpimpexp);
    add_test(suite,test057_invlcmd);
    
#ifdef EX_USE_SYSVQ
    /* for SVAPOLL not needed, as no rqaddr principle */
    add_test(suite, test058_systemv);
#endif
    
    add_test(suite, test059_pq);
    add_test(suite, test060_ndxdfork);
    add_test(suite, test061_ndxddup);
/* for sanitizer the memory layout does not match the
 * test cases, thus cannot test...
 */
#ifndef NDRX_SANITIZE
/* seems like we get incorrect vms readings ... */
#ifndef EX_OS_DARWIN
    add_test(suite, test062_memlimits);
#endif
#endif
    add_test(suite, test063_cpmrange);
    add_test(suite, test064_bufswitch);
    add_test(suite, test065_tpcancel);
    add_test(suite, test066_tmstartserver);
    
#ifdef NDRX_USE_POSTGRES
    add_test(suite, test067_postgres);
#endif
    
    add_test(suite, test068_tpadm);
    add_test(suite, test069_wnormal);
    add_test(suite, test070_ubfhook);
    add_test(suite, test071_buildtools);
    add_test(suite, test072_qos);
    add_test(suite, test073_netact);

#ifndef EX_OS_DARWIN
    add_test(suite, test074_sanitulog);
#endif
    
    add_test(suite, test075_dispthread);
    add_test(suite, test076_dispthreadxa);
    add_test(suite, test077_diedslowstrt);
    add_test(suite, test078_tpacallmix);
    add_test(suite, test079_cliquote);
    add_test(suite, test080_tpexit);
    add_test(suite, test081_lcf);
    add_test(suite, test082_autotran);
    add_test(suite, test083_ddrsyntax);
    add_test(suite, test084_ddr);
    add_test(suite, test085_prio);
    add_test(suite, test086_tmqlimit);
    add_test(suite, test087_tmsrv);
    add_test(suite, test088_addlog);
    add_test(suite, test089_tmrecover);
    
    return suite;
}

/**
 * Main test case entry
 */
int main(int argc, char** argv) {

    int ret;

    TestSuite *suite = create_test_suite();

    add_suite(suite, atmi_test_all());


    if (argc > 1)
    {
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
