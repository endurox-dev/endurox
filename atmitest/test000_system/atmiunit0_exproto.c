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
    char buf[2048];
    char proto_out[2048];
    long proto_len;
    tp_command_call_t *call = (tp_command_call_t *)(buf + sizeof(cmd_br_net_call_t));
    cmd_br_net_call_t *netcall = (cmd_br_net_call_t *)buf;
    ndrx_stopwatch_t w;
    time_t t;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", 0, 1024);
    UBFH *p_ub5 = NULL;
    long olen;
    long call_len_org;
    assert_not_equal(p_ub, NULL);
    
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
    
    /* Load some buffer fields (standard mode currently */
    extest_ubf_set_up_dummy_data(p_ub, 0);
    call->data_len=sizeof(buf)-sizeof(*call)-sizeof(*netcall);
    assert_equal(ndrx_mbuf_prepare_outgoing ((char *)p_ub, 0, call->data, 
            &call->data_len, 0, 0), EXSUCCEED);
    
    call_len_org=call->data_len;
    netcall->len=sizeof(*call)+call_len_org;
    
    NDRX_LOG(log_error, "YOPT len %ld call len: %ld", 
            netcall->len, call_len_org);
    
    
    ndrx_mbuf_tlv_debug(call->data, call->data_len);
    
    proto_len = 0;
    /* try to serialize */
    assert_equal(exproto_ex2proto(buf, sizeof(*call)+call->data_len, 
	proto_out, &proto_len, sizeof(proto_out)), EXSUCCEED);
    
    
    memset(buf, 0, sizeof(buf));
    
    /* deserialize the buffer back... */
    assert_equal(exproto_proto2ex(proto_out, proto_len, 
        buf, &proto_len, sizeof(buf)), EXSUCCEED);
    
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
    
    Bprint(p_ub);
    Bprint(p_ub5);
    assert_equal(Bcmp(p_ub, p_ub5), 0);
    
    tpfree((char *)p_ub);
    tpfree((char *)p_ub5);
 
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
