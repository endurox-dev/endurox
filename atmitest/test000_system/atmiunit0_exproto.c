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
 * Convert UBF call two way
 */
Ensure(test_proto_ubfcall)
{
    /* allocate FB & load data, convert out... (i.e.) alloc the tpcall struct */
    char buf[6048];
    char proto_out[6048];
    long proto_len;
    tp_command_call_t *call = (tp_command_call_t *)(buf + sizeof(cmd_br_net_call_t));
    cmd_br_net_call_t *netcall = (cmd_br_net_call_t *)buf;
    ndrx_stopwatch_t w;
    time_t t;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", 0, 2024);
    UBFH *p_ci = (UBFH *)tpalloc("UBF", 0, 2024);
    UBFH *p_ub5 = NULL;
    UBFH *p_ci5 = NULL;
    long olen;
    long call_len_org;
    struct UBTESTVIEW1 *vptr;
    struct UBTESTVIEW1 *vptr5;
    
    assert_not_equal(p_ub, NULL);
    
    /* have some ptr to VIEW... */
    
    ATMI_TLS_ENTRY;
    
    vptr=(struct UBTESTVIEW1 *)tpalloc("VIEW", "UBTESTVIEW1", sizeof(struct UBTESTVIEW1));
    assert_not_equal(vptr, NULL);
    
    extest_init_UBTESTVIEW1(vptr);
    
    /* reset call header */
    memset(call, 0, sizeof(*call));
    call->command_id = ATMI_COMMAND_TPCALL;
    NDRX_STRCPY_SAFE(call->name, "HELLOSVC");
    
    ndrx_stopwatch_reset(&w);
    t=time(NULL);
    
    call->timer = w;
    call->cd = 999;
    call->timestamp = t;
    call->buffer_type_id = BUF_TYPE_UBF;
    netcall->br_magic=BR_NET_CALL_MAGIC;
    netcall->command_id=ATMI_COMMAND_TPCALL;
    netcall->msg_type=BR_NET_CALL_MSG_TYPE_ATMI;
    
    /* set call infos */
    assert_equal(Bchg(p_ci, T_STRING_6_FLD, 4, "HELLO CALL INFO", 0), EXSUCCEED);
    
    /* load primary data.., including sub-ubf buf */
    extest_ubf_set_up_dummy_data(p_ub, EXTEST_PROC_UBF | EXTEST_PROC_VIEW);
    
    assert_equal(tpsetcallinfo((char *)p_ub, p_ci, 0), EXSUCCEED);
    
    
    /* Load view ptr... */
    assert_equal(Bchg(p_ub, T_PTR_FLD, 0, (char *)&vptr, 0), EXSUCCEED);
    
    call->data_len=sizeof(buf)-sizeof(*call)-sizeof(*netcall);
    assert_equal(ndrx_mbuf_prepare_outgoing ((char *)p_ub, 0, call->data, 
            &call->data_len, 0, 0), EXSUCCEED);
    
    call_len_org=call->data_len;
    netcall->len=sizeof(*call)+call_len_org;
    
    NDRX_LOG(log_error, "YOPT len %ld call len: %ld sizeofcall: %d", 
            netcall->len, call_len_org, sizeof(*call));
    
    
    ndrx_mbuf_tlv_debug(call->data, call->data_len);
    
    
    fprintf(stdout, "YOPT ORG p_ub:\n");
    Bprint(p_ub);
    
    
    proto_len = 0;
    /* try to serialize */
    assert_equal(exproto_ex2proto(buf, sizeof(*call)+call->data_len, 
	proto_out, &proto_len, sizeof(proto_out)), EXSUCCEED);
    
    
    memset(buf, 0, sizeof(buf));
    
    NDRX_LOG(log_error, "YOPT DATA PTR: %p", buf);
    /* deserialize the buffer back... */
    assert_not_equal(exproto_proto2ex(proto_out, proto_len, 
        buf, &proto_len, sizeof(buf)), EXFAIL);
    
    
    ndrx_mbuf_tlv_debug(call->data, call->data_len);
    
    
    NDRX_LOG(log_debug, "protolen: %ld", proto_len);
    
    /* Check the output... */
    assert_equal(netcall->br_magic, BR_NET_CALL_MAGIC);
    assert_equal(netcall->command_id, ATMI_COMMAND_TPCALL);
    assert_equal(netcall->msg_type, BR_NET_CALL_MSG_TYPE_ATMI);
    assert_equal(netcall->len, sizeof(*call)+call->data_len);
    assert_equal(call->data_len, call_len_org);
    
    assert_equal(call->cd, 999);
    assert_string_equal(call->name, "HELLOSVC");
    assert_equal(call->command_id, ATMI_COMMAND_TPCALL);
    assert_equal(call->timer.t.tv_nsec, call->timer.t.tv_nsec);
    assert_equal(call->timer.t.tv_sec, call->timer.t.tv_sec);
    assert_equal(call->timestamp, call->timestamp);
    
    /* TODO: read the buffer? */
    olen=0;
    assert_equal(ndrx_mbuf_prepare_incoming (call->data, call->data_len, 
            (char **)&p_ub5, &olen, 0, 0), EXSUCCEED);
    
    fprintf(stdout, "YOPT p_ub:\n");
    Bprint(p_ub);
    fprintf(stdout, "YOPT p_ub5:\n");
    Bprint(p_ub5);
    
    /* read ptr */
    assert_equal(Bget(p_ub5, T_PTR_FLD, 0, (char *)&vptr5, 0), EXSUCCEED);
    
    /* ptr, as it will be different */
    Bdel(p_ub, T_PTR_FLD, 0);
    Bdel(p_ub5, T_PTR_FLD, 0);
    
    assert_equal(Bcmp(p_ub, p_ub5), 0);
    
    
    fprintf(stdout, "YOPT vptr:\n");
    Bvprint((char *)vptr, "UBTESTVIEW1");
    
    fprintf(stdout, "YOPT vptr5:\n");
    Bvprint((char *)vptr5, "UBTESTVIEW1");
    
    assert_equal(Bvcmp((char *)vptr, "UBTESTVIEW1", (char *)vptr5, "UBTESTVIEW1"), 0);
    
    /* read the call infos ! */
    assert_equal(tpgetcallinfo((char *)p_ub5, &p_ci5, 0), EXSUCCEED);
    assert_equal(Bcmp(p_ci, p_ci5), 0);
    

    tpfree((char *)p_ub);
    tpfree((char *)p_ub5);
    
    tpfree((char *)p_ci);
    tpfree((char *)p_ci5);
 
}

/**
 * Standard library tests
 * @return
 */
TestSuite *atmiunit0_exproto(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_proto_ubfcall);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
