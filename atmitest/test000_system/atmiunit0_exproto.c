/**
 * @brief Protocol converter unit tests
 *
 * @file atmiunit0_exproto.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
#include "ndebug.h"
#include "xatmi.h"
#include "atmi_int.h"
#include <fdatatype.h>
#include <nstdutil.h>
#include <typed_buf.h>
#include <extest.h>
#include <atmi_int.h>
#include <exproto.h>
#include <atmi_tls.h>
#include <exassert.h>
#include <nstd_shm.h>
/**
 * Basic preparation before the test
 */
exprivate void basic_setup(void)
{
    
}

exprivate void basic_teardown(void)
{
    
}

/**
 * Perform network call (wrapper from c->net->c)
 * Function is used to validate transportation of different kind of ATMI buffers
 * @param req_buf input ATMI buffer
 * @param req_len input len
 * @param obuf output buffer ATMI
 * @param olen output buffer len
 * @param mbufsz mutli-buffer size (output tlv)
 * @param netbufsz network output buffer size
 * @param cback_size C buffer size when parsing msg in (from net)
 * @return EXSUCCEED/EXFAIL
 */
exprivate int netcallconv(char *req_buf, long req_len, char **obuf, long *olen, 
        long mbufsz, long netbufsz, long cback_size)
{
    int ret = EXSUCCEED;
    tp_command_call_t *call=NULL;
    tp_command_call_t *callpars=NULL;
    char smallbuf[sizeof(cmd_br_net_call_t) + sizeof(char *)];
    cmd_br_net_call_t *netcall = (cmd_br_net_call_t *)smallbuf;
    long call_len_org;
    long netcall_len_org;
    long proto_len;
    char *proto_out=NULL;
    char *cbuf_back = NULL;
    long max_struct = 0;
    ndrx_stopwatch_t w;
    time_t t;
    /* prepare call, dynamic buffer */
    call = (tp_command_call_t *)NDRX_MALLOC(mbufsz);
    
    if (NULL==call)
    {
        NDRX_LOG(log_error, "Failed to malloc call: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    memset(call, 0, sizeof(*call));
    /* netcall buf bytes are set to dynamic call buffer */
    memcpy(netcall->buf, &call, sizeof(char *));
    
    call->data_len=mbufsz-sizeof(*call);
    
    /* prepare MBUF for output */
    if (EXSUCCEED!=ndrx_mbuf_prepare_outgoing (req_buf, req_len, call->data, 
            &call->data_len, 0, 0))
    {
        NDRX_LOG(log_error, "Failed to prepare outgoing data: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    call_len_org=call->data_len;
    netcall_len_org = netcall->len=sizeof(*call)+call_len_org;
    
    if (NULL==(proto_out=NDRX_MALLOC(netbufsz)))
    {
        NDRX_LOG(log_error, "Failed to malloc %ld bytes: %s", netbufsz, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* setup some minimum constant values: */
    
    ndrx_stopwatch_reset(&w);
    t=time(NULL);
    
    call->timer = w;
    call->cd = 999;
    call->timestamp = t;
    call->command_id=ATMI_COMMAND_TPCALL;
    NDRX_STRCPY_SAFE(call->name, "HELLOSVC");
    
    netcall->br_magic=BR_NET_CALL_MAGIC;
    netcall->command_id=ATMI_COMMAND_TPCALL;
    netcall->msg_type=BR_NET_CALL_MSG_TYPE_ATMI;
    
    proto_len=0;
    if (EXSUCCEED!=exproto_ex2proto((char *)netcall, sizeof(*call)+call->data_len, 
	proto_out, &proto_len, netbufsz))
    {
        NDRX_LOG(log_error, "Failed to convert to convert to net-proto");
        EXFAIL_OUT(ret);
    }
    
    cbuf_back = NDRX_MALLOC(cback_size);
    
    if (NULL==cbuf_back)
    {
        NDRX_LOG(log_error, "Failed to malloc back parse buffer: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    memset(cbuf_back, 0, sizeof(cmd_br_net_call_t) + sizeof(tp_command_call_t));
    netcall = (cmd_br_net_call_t *)cbuf_back;
            
    callpars = (tp_command_call_t *)(cbuf_back + sizeof(cmd_br_net_call_t));
    
    if (EXFAIL==exproto_proto2ex(proto_out, proto_len, 
        cbuf_back, &max_struct, cback_size))
    {
        NDRX_LOG(log_error, "Failed to convert proto -> EX");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=ndrx_mbuf_prepare_incoming (call->data, call->data_len, obuf, olen, 
            0, 0))
    {
        NDRX_LOG(log_error, "Failed to prepare incoming buffers");
        EXFAIL_OUT(ret);
    }
    
    /* minimalistic verification: */
    NDRX_ASSERT_VAL_OUT((netcall->br_magic==BR_NET_CALL_MAGIC), "Invalid netcall->br_magic %ld vs %ld", 
            netcall->br_magic, BR_NET_CALL_MAGIC);
    
    NDRX_ASSERT_VAL_OUT((netcall->command_id==ATMI_COMMAND_TPCALL), "%d vs %d", 
            netcall->command_id, ATMI_COMMAND_TPCALL);
    NDRX_ASSERT_VAL_OUT((netcall->msg_type==BR_NET_CALL_MSG_TYPE_ATMI), "%x vs %x", 
            (int)netcall->command_id, (int)ATMI_COMMAND_TPCALL);
    
    NDRX_ASSERT_VAL_OUT((netcall->len==netcall_len_org), "%ld vs %ld", 
            netcall->len, netcall_len_org);
    NDRX_ASSERT_VAL_OUT((callpars->data_len==call_len_org), "%ld vs %ld", 
            callpars->data_len, call_len_org);
    
    NDRX_ASSERT_VAL_OUT((callpars->cd==999), "%d vs %d", callpars->cd, 999);
    NDRX_ASSERT_VAL_OUT((0==strcmp(callpars->name, "HELLOSVC")), "%s vs %s", callpars->name, "HELLOSVC");
    
    NDRX_ASSERT_VAL_OUT((callpars->command_id==ATMI_COMMAND_TPCALL), "%hd vs %hd", 
            callpars->command_id, ATMI_COMMAND_TPCALL);
    
    NDRX_ASSERT_VAL_OUT((callpars->timer.t.tv_nsec==call->timer.t.tv_nsec), "%ld vs %ld", 
            (long)callpars->timer.t.tv_nsec, (long)call->timer.t.tv_nsec);
    
    NDRX_ASSERT_VAL_OUT((callpars->timer.t.tv_sec==call->timer.t.tv_sec), "%ld vs %ld",
            (long)callpars->timer.t.tv_sec, (long)call->timer.t.tv_sec);
    
    NDRX_ASSERT_VAL_OUT((callpars->timestamp==call->timestamp), "%ld vs %ld",
            (long)callpars->timestamp, (long)call->timestamp);
    
out:
    
    if (NULL!=call)
    {
        NDRX_FREE(call);
    }

    if (NULL!=proto_out)
    {
        NDRX_FREE(proto_out);
    }

    if (NULL!=cbuf_back)
    {
        NDRX_FREE(cbuf_back);
    }

    return ret;    
}

/**
 * Convert UBF call two way
 */
Ensure(test_proto_ubfcall)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", 0, 2024);
    UBFH *p_ci = (UBFH *)tpalloc("UBF", 0, 2024);
    UBFH *p_ub5 = NULL;
    UBFH *p_ci5 = NULL;
    struct UBTESTVIEW1 *vptr;
    struct UBTESTVIEW1 *vptr5;
    char *vptr_2;
    char *vptr5_2;
    long olen;
    
    assert_not_equal(p_ub, NULL);
    
    /* have some ptr to VIEW... */
    
    ATMI_TLS_ENTRY;
    
    vptr=(struct UBTESTVIEW1 *)tpalloc("VIEW", "UBTESTVIEW1", sizeof(struct UBTESTVIEW1));
    assert_not_equal(vptr, NULL);
    
    vptr_2=tpalloc("CARRAY", NULL, 567);
    assert_not_equal(vptr_2, NULL);
    
    memset(vptr_2, 'Z', 567);
    vptr_2[0]='A';
    vptr_2[499]='A';
    
    extest_init_UBTESTVIEW1(vptr);
    
    /* set call infos */
    assert_equal(Bchg(p_ci, T_STRING_6_FLD, 4, "HELLO CALL INFO", 0), EXSUCCEED);
    
    /* load primary data.., including sub-ubf buf */
    extest_ubf_set_up_dummy_data(p_ub, EXTEST_PROC_UBF | EXTEST_PROC_VIEW);
    
    assert_equal(tpsetcallinfo((char *)p_ub, p_ci, 0), EXSUCCEED);
    
    
    /* Load view ptr..., at pos 0 we will have NULL buffer ... */
    assert_equal(Bchg(p_ub, T_PTR_FLD, 1, (char *)&vptr, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_PTR_FLD, 2, (char *)&vptr_2, 0), EXSUCCEED);
    
    fprintf(stdout, "YOPT ORG p_ub:\n");
    Bprint(p_ub);
    
    olen=0;
    assert_equal(netcallconv((char *)p_ub, 0, (char **)&p_ub5, &olen, 9000, 9000, 9000), EXSUCCEED);
            
    fprintf(stdout, "YOPT p_ub5:\n");
    Bprint(p_ub5);
    
    /* read ptr */
    assert_equal(Bget(p_ub5, T_PTR_FLD, 1, (char *)&vptr5, 0), EXSUCCEED);
    assert_equal(Bget(p_ub5, T_PTR_FLD, 2, (char *)&vptr5_2, 0), EXSUCCEED);
    
    /* ptr, as it will be different */
    Bdel(p_ub, T_PTR_FLD, 2);
    Bdel(p_ub5, T_PTR_FLD, 2);
    Bdel(p_ub, T_PTR_FLD, 1);
    Bdel(p_ub5, T_PTR_FLD, 1);
    
    
    assert_equal(Bcmp(p_ub, p_ub5), 0);
    
    
    fprintf(stdout, "YOPT vptr:\n");
    Bvprint((char *)vptr, "UBTESTVIEW1");
    
    fprintf(stdout, "YOPT vptr5:\n");
    Bvprint((char *)vptr5, "UBTESTVIEW1");
    
    assert_equal(Bvcmp((char *)vptr, "UBTESTVIEW1", (char *)vptr5, "UBTESTVIEW1"), 0);
    
    
    assert_equal(memcmp(vptr_2, vptr5_2, 567), 0);
    
    /* read the call infos ! */
    assert_equal(tpgetcallinfo((char *)p_ub5, &p_ci5, 0), EXSUCCEED);
    assert_equal(Bcmp(p_ci, p_ci5), 0);
    

    tpfree((char *)p_ub);
    tpfree((char *)p_ub5);
    
    tpfree((char *)p_ci);
    tpfree((char *)p_ci5);
 
}

Ensure(test_proto_nullcall)
{
    char *null_buf = NULL;
    long olen = 0;
    char *null_out = NULL;
    ATMI_TLS_ENTRY;
    
    assert_equal(netcallconv(null_buf, 0, (char **)&null_out, &olen, 1024, 1024, 1024), 
            EXSUCCEED);
    
    assert_equal(null_buf, NULL);
    assert_equal(null_out, NULL);
    assert_equal(olen, 0);
    
}

Ensure(test_proto_carraycall)
{
    char *carray_buf = NULL;
    long olen = 0;
    char *carray_out = NULL;
    char rndbuf[1028];
    
    ATMI_TLS_ENTRY; /* otherwise mbuf cannot work with NULL buffers */
    
    carray_buf=tpalloc("CARRAY", NULL, 1028);
    
    memcpy(carray_buf, rndbuf, sizeof(rndbuf));
    
    assert_equal(netcallconv(carray_buf, 1028, (char **)&carray_out, &olen, 2000, 2000, 2000), 
            EXSUCCEED);
    assert_equal(memcmp(carray_buf, carray_out, sizeof(rndbuf)), 0);
    
    tpfree(carray_buf);
    tpfree(carray_out);
}

/**
 * Perform no-space tests (i.e. buffer too for the operation)
 * Check that any of converting parts go over the allocated memory...
 */
Ensure(test_proto_nospace)
{
    int i,j,k;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", 0, 2048);
    UBFH *buf_out;
    long olen;
    int ret = EXSUCCEED;
    int callsize=sizeof(cmd_br_net_call_t)+sizeof(tp_command_call_t);
    
    assert_not_equal(p_ub, NULL);

    extest_ubf_set_up_dummy_data(p_ub, EXTEST_PROC_UBF | EXTEST_PROC_VIEW);
    
    for (i=callsize; i<3000; i+=56)
        for (j=callsize; j<3000; j+=56)
            for (k=callsize; k<3000; k+=56)
            {
                /* try 3x combinations... */
                olen=0;
                buf_out=NULL;
                netcallconv((char *)p_ub, 0, (char **)&buf_out, &olen, i, j, k);
                
                if (NULL!=buf_out)
                {
                    NDRX_ASSERT_UBF_OUT((0==Bcmp(p_ub, buf_out)), "Buffer does not match");
                    
                    tpfree((char *)buf_out);
                }
                
            }
    
out:
            
    assert_equal(ret, EXSUCCEED);
            
}

#define LEVEL_MAX   100
ndrx_shm_t M_testmem;


int brute_force_protocol(int level)
{
    int ret = EXSUCCEED;
    int i;
    
    if (LEVEL_MAX==level)
    {
        char obuf[1024];
        long len;
        
        /* just run the asan... */
        exproto_proto2ex((char *)M_testmem.mem, LEVEL_MAX, obuf, &len, 1024);
    }
    else for (i=M_testmem.mem[level]; i<255; i++)
    {
        M_testmem.mem[level]=(char) (i & 0xff);
        brute_force_protocol(level+1);
    }

out:
    return ret;
}

/**
 * Needs to have shm, so that we can restart when looped with out
 * log files.
 */
Ensure(test_proto_rndparse)
{
    int i;
    
    memset(&M_testmem, 0, sizeof(M_testmem));
    M_testmem.size=LEVEL_MAX;
    M_testmem.key = 0x00aabbcc;
    
    NDRX_STRCPY_SAFE(M_testmem.path, "/test");
    
    
    if (EXSUCCEED==ndrx_shm_open(&M_testmem, EXFALSE))
    {
        /* start here... */
        fprintf(stderr, "Creating new...\n");
        memset(M_testmem.mem, 0, LEVEL_MAX);
    }
    else
    {
        fprintf(stderr, "Continuing...\n");
        assert_equal(ndrx_shm_open(&M_testmem, EXTRUE), EXSUCCEED);
    }
            
    /*
    srand(time(NULL));
    
    for (i=0; i<LEVEL_MAX; i++)
    {
        M_msg[i] = (char)(rand() % 255);
    }*/
    
    
    assert_equal(brute_force_protocol(0), EXSUCCEED);
}

/**
 * Check the time sync conv operations
 */
Ensure(test_proto_timesync)
{
    cmd_br_time_sync_t *ptr_tmsg;
    char smallbuf[sizeof(cmd_br_net_call_t) + sizeof(cmd_br_time_sync_t)];
    cmd_br_net_call_t *netcall = (cmd_br_net_call_t *)smallbuf;
    char proto_out[1024];
    long proto_len;
    long max_struct = 0;
    cmd_br_time_sync_t *tmsg_back;
    cmd_br_time_sync_t tmsg;
    char *tmp_ptr;

    memset(&tmsg, 0, sizeof(tmsg));
    
    tmsg.call.msg_type=NDRXD_CALL_TYPE_BRBCLOCK;
    tmsg.call.command = NDRXD_COM_BRCLOCK_RQ;
    tmsg.call.caller_nodeid=1;
    tmsg.call.msg_src = NDRXD_SRC_ADMIN;
    tmsg.call.magic=NDRX_MAGIC;
    
    NDRX_STRCPY_SAFE(tmsg.call.reply_queue, "Helloqueue");
    
    /* set local time */
    ndrx_stopwatch_reset(&tmsg.time);
    
    netcall->br_magic=BR_NET_CALL_MAGIC;
    netcall->command_id=NDRXD_COM_BRCLOCK_RQ;
    netcall->msg_type=BR_NET_CALL_MSG_TYPE_NDRXD;
        
    tmp_ptr = (char *)&tmsg;
    memcpy(netcall->buf, &tmp_ptr, sizeof(char *));
    assert_not_equal(&tmsg, NULL);
    assert_equal(*(cmd_br_time_sync_t **)netcall->buf, &tmsg);
    
    ptr_tmsg = *((cmd_br_time_sync_t **)netcall->buf);
    
    assert_equal(ptr_tmsg, &tmsg);
    assert_equal(ptr_tmsg->call.command, tmsg.call.command);
    
    proto_len=0;
    assert_not_equal(exproto_ex2proto((char *)netcall, 0, proto_out, &proto_len, 
            sizeof(proto_out)), EXFAIL);
    
    memset(smallbuf, 0, sizeof(smallbuf));
    assert_not_equal(exproto_proto2ex(proto_out, proto_len, smallbuf, 
            &max_struct, sizeof(smallbuf)), EXFAIL);
    
    /* OK check the fields... */
    tmsg_back = (cmd_br_time_sync_t *)netcall->buf;
    
    assert_equal(netcall->br_magic, BR_NET_CALL_MAGIC);
    assert_equal(netcall->command_id, NDRXD_COM_BRCLOCK_RQ);
    assert_equal(netcall->msg_type, BR_NET_CALL_MSG_TYPE_NDRXD);
    
    /* check call fields*/
    assert_equal(tmsg_back->call.command, NDRXD_COM_BRCLOCK_RQ);
    assert_equal(tmsg_back->call.caller_nodeid, 1);
    assert_equal(tmsg_back->call.msg_type, NDRXD_CALL_TYPE_BRBCLOCK);
    assert_equal(tmsg_back->call.msg_src, NDRXD_SRC_ADMIN);
    assert_equal(tmsg_back->call.magic, NDRX_MAGIC);
    assert_string_equal(tmsg_back->call.reply_queue, "Helloqueue");
    
    /* finally check the payload */
    assert_equal(ndrx_stopwatch_diff(&tmsg.time, &tmsg_back->time), 0);
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *atmiunit0_exproto(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_proto_ubfcall);
    add_test(suite, test_proto_nullcall);
    add_test(suite, test_proto_carraycall);
    add_test(suite, test_proto_nospace);
    /* add_test(suite, test_proto_rndparse); - too long...*/
    
    add_test(suite, test_proto_timesync);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
