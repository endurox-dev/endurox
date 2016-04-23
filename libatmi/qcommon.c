/* 
** Persistent queue commons (used between tmqsrv and "userspace" api)
**
** @file qcommon.c
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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <ubf.h>
#include <ubfutil.h>
#include <Exfields.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define OFSZ(s,e)   OFFSET(s,e), ELEM_SIZE(s,e)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * TPQCTL structure mappings.
 */
static ubf_c_map_t M_tpqctl_map[] = 
{
    {EX_QFLAGS,         0, OFSZ(TPQCTL, flags),         BFLD_LONG}, /* 0 */
    {EX_QDEQ_TIME,      0, OFSZ(TPQCTL, deq_time),      BFLD_LONG}, /* 1 */
    {EX_QPRIORITY,      0, OFSZ(TPQCTL, priority),      BFLD_LONG}, /* 2 */
    {EX_QDIAGNOSTIC,    0, OFSZ(TPQCTL, diagnostic),    BFLD_LONG}, /* 3 */
    {EX_QMSGID,         0, OFSZ(TPQCTL, msgid),         BFLD_CARRAY}, /* 4 */
    {EX_QCORRID,        0, OFSZ(TPQCTL, corrid),        BFLD_STRING}, /* 5 */
    {EX_QREPLYQUEUE,    0, OFSZ(TPQCTL, replyqueue),    BFLD_STRING}, /* 6 */
    {EX_QFAILUREQUEUE,  0, OFSZ(TPQCTL, failurequeue),  BFLD_STRING}, /* 7 */
    {EX_CLTID,          0, OFSZ(TPQCTL, cltid),         BFLD_STRING}, /* 8 */
    {EX_QURCODE,        0, OFSZ(TPQCTL, urcode),        BFLD_LONG}, /* 9 */
    {EX_QAPPKEY,        0, OFSZ(TPQCTL, appkey),        BFLD_LONG}, /* 10 */
    {EX_QDELIVERY_QOS,  0, OFSZ(TPQCTL, delivery_qos),  BFLD_LONG}, /* 11 */
    {EX_QREPLY_QOS,     0, OFSZ(TPQCTL, reply_qos),     BFLD_LONG}, /* 12 */
    {EX_QEXP_TIME,      0, OFSZ(TPQCTL, exp_time),      BFLD_LONG}, /* 13 */
    {EX_QDIAGMSG,       0, OFSZ(TPQCTL, diagmsg),       BFLD_STRING}, /* 14 */
    {BBADFLDID}
};
static long M_tpqctl_req[] = 
{
    UBFUTIL_EXPORT,/* 0 - EX_QFLAGS*/
    UBFUTIL_EXPORT,/* 1 - EX_QDEQ_TIME*/
    UBFUTIL_EXPORT,/* 2 - EX_QPRIORITY*/
    0,             /* 3 - EX_QDIAGNOSTIC*/
    UBFUTIL_EXPORT,/* 4 - EX_QMSGID*/
    UBFUTIL_EXPORT,/* 5 - EX_QCORRID*/
    UBFUTIL_EXPORT,/* 6 - EX_QREPLYQUEUE*/
    UBFUTIL_EXPORT,/* 7 - EX_QFAILUREQUEUE*/
    UBFUTIL_EXPORT,/* 8 - EX_CLTID*/
    UBFUTIL_EXPORT,/* 9 - EX_QURCODE*/
    UBFUTIL_EXPORT,/* 10 - EX_QAPPKEY*/
    UBFUTIL_EXPORT,/* 11 - EX_QDELIVERY_QOS*/
    UBFUTIL_EXPORT,/* 12 - EX_QREPLY_QOS*/
    UBFUTIL_EXPORT,/* 13 - EX_QEXP_TIME*/
    0              /* 14 - EX_QDIAGMSG*/
};


/**
 * Copy the TPQCTL data to buffer, request data
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_to_ubf_req(UBFH *p_ub, TPQCTL *ctl, long *rules)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_req);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_from_ubf_req(UBFH *p_ub, TPQCTL *ctl, long *rules)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, p_ub, ctl, M_tpqctl_req);
    
    return ret;
}
