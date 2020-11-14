/**
 * @brief tpimport()/tpexport() function tests - client
 *
 * @file atmiclt56_embbuf.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <atmi.h>
#include <atmi_int.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include <ubfutil.h>
#include "test56.h"
#include "t56.h"
#include <exbase64.h>
#include <exassert.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Test one loop
 * @return 
 */
exprivate int test_impexp_testemb_int(void)
{
    int ret = EXSUCCEED;
    
    UBFH *p_ub1=NULL;
    UBFH *p_ub2=NULL;
    UBFH *p_ub3=NULL;
    struct MYVIEW56 *v=NULL;
    struct MYVIEW56 *v2=NULL;
    struct MYVIEW56 *v3=NULL;
    
    UBFH *p_ub4=NULL;
    UBFH *p_ub5=NULL;
    
    BVIEWFLD vf;
    char json_ubf_out[56000];
    long olen;
    long rsplen;
    char vtyp[16];
    char vsubtyp[16];
    
    p_ub1 = (UBFH *)tpalloc("UBF", 0, 56000); 
    NDRX_ASSERT_TP_OUT((NULL!=p_ub1), "Failed to alloc p_ub1");
    
    p_ub2 = (UBFH *)tpalloc("UBF", 0, 56000); 
    NDRX_ASSERT_TP_OUT((NULL!=p_ub2), "Failed to alloc p_ub2");
    
    p_ub3 = (UBFH *)tpalloc("UBF", 0, 56000); 
    NDRX_ASSERT_TP_OUT((NULL!=p_ub3), "Failed to alloc p_ub3");
    
    
    v = (struct MYVIEW56 *)tpalloc("VIEW", "MYVIEW56", sizeof(struct MYVIEW56));
    
    
    memset(v, 0, sizeof(*v));
    
    NDRX_STRCPY_SAFE(v->tstring1, "HELLO");
    v->tchar1='A';
    v->tshort1=77;
    v->tlong1=123123;
    v->tchar1='X';
    v->tfloat1=99;
    v->tdouble1=99999;
    v->tcarray1[0]=0; /* well seems like we have a bug. default empty view value is ' ' space */
    v->tcarray1[1]=1;
    v->tcarray1[2]=2;
    
    NDRX_ASSERT_TP_OUT((NULL!=p_ub2), "Failed to alloc MYVIEW56");
    
    /* Load some fields in sub-buffer  */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub2, T_STRING_FLD, 3, "HELLO STRING", 0)), 
            "Failed to set T_STRING to buf2");
    
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==CBchg(p_ub2, T_LONG_FLD, 10, "777", 0, BFLD_STRING)), 
            "Failed to set T_LONG_FLD to buf2");
    
    vf.data = (char *)v;
    vf.vflags=0;
    NDRX_STRCPY_SAFE(vf.vname, "MYVIEW56");
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub2, T_VIEW_FLD, 5, (char *)&vf, 0)), 
            "Failed to set T_VIEW_FLD[5]");
    
    /* test non array */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub2, T_VIEW_2_FLD, 0, (char *)&vf, 0)), 
            "Failed to set T_VIEW_2_FLD[0]");
    
    /* Load some UBF... */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==CBchg(p_ub3, T_STRING_FLD, 5, "HELLO 3/5", 0, BFLD_STRING)), 
            "Failed to set T_STRING_FLD to buf3");
    
    /* at third level */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub2, T_UBF_FLD, 2, (char *)p_ub3, 0)), 
            "Failed to set T_UBF_FLD[2]");
    
    /* Load buffer into main p_ub1 */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub1, T_UBF_FLD, 3, (char *)p_ub2, 0)), 
            "Failed to set T_UBF_FLD[3]");
    
    /* test non array ... */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub1, T_UBF_2_FLD, 0, (char *)p_ub2, 0)), 
            "Failed to set T_UBF_2_FLD[0]");
    
    /* unload some pointers: */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub1, T_PTR_FLD, 0, (char *)&p_ub2, 0)), 
            "Failed to set T_PTR_FLD[0]");
    
    /* some view too... */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub1, T_PTR_FLD, 1, (char *)&v, 0)), 
            "Failed to set T_PTR_FLD[1]");
    
    /* test non array ... */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bchg(p_ub1, T_PTR_2_FLD, 0, (char *)&v, 0)), 
            "Failed to set T_PTR_2_FLD[0]");
    
    memset(json_ubf_out, 0, sizeof(json_ubf_out));
    olen = sizeof(json_ubf_out);
    
    if ( EXFAIL == tpexport((char *)p_ub1, 
                            0,
                            json_ubf_out, 
                            &olen, 
                            0L) )
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to export JSON UBF!!!!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, 
             "JSON UBF exported. Return json_ubf_out=[%s] olen=[%ld]", 
             json_ubf_out, olen);
    
    
    /* load the buffer */
    if ( EXFAIL == tpimport(json_ubf_out, 
                                (long)strlen(json_ubf_out), 
                                (char **)&p_ub4, 
                                &rsplen, 
                                0L) )
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to import JSON UBF/EMB!!!!");
        EXFAIL_OUT(ret);
    }
    
    /* read the pointer and delete them... */
    
    /* Read the pointer: */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub4, T_PTR_FLD, 0, (char *)&p_ub5, 0)), 
            "Failed to set T_PTR_FLD[0]");
    
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub4, T_PTR_FLD, 1, (char *)&v2, NULL)), 
            "Failed to set T_PTR_FLD[1]");
    
    /* test non array ... */
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bget(p_ub4, T_PTR_2_FLD, 0, (char *)&v3, NULL)), 
            "Failed to set T_PTR_2_FLD[0]");
    
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bdel(p_ub4, T_PTR_FLD, 1)), "Failed to delete p_ub4 T_PTR_FLD[1]");
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bdel(p_ub4, T_PTR_FLD, 0)), "Failed to delete p_ub4 T_PTR_FLD[0]");
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bdel(p_ub4, T_PTR_2_FLD, 0)), "Failed to delete p_ub4 T_PTR_2_FLD[0]");
    
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bdel(p_ub1, T_PTR_FLD, 1)), "Failed to delete p_ub1 T_PTR_FLD[1]");
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bdel(p_ub1, T_PTR_FLD, 0)), "Failed to delete p_ub1 T_PTR_FLD[0]");
    NDRX_ASSERT_UBF_OUT((EXSUCCEED==Bdel(p_ub1, T_PTR_2_FLD, 0)), "Failed to delete p_ub1 T_PTR_2_FLD[0]");
    
    
    /* run compare: */
    NDRX_ASSERT_UBF_OUT(Bcmp(p_ub1, p_ub4)==0, "Failed to compare p_ub1 vs p_ub4");
    NDRX_ASSERT_UBF_OUT(Bcmp(p_ub2, p_ub5)==0, "Failed to compare p_ub2 vs p_ub5");
    
    
    /* check that that v2 has view type */
    
    if (EXFAIL==tptypes((char *)v2, vtyp, vsubtyp))
    {
        NDRX_LOG(log_error, "TESTERROR: got error for tptypes on v2 %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_ASSERT_VAL_OUT(0==strcmp(vtyp, "VIEW"), "Expected VIEW");
    NDRX_ASSERT_VAL_OUT(0==strcmp(vsubtyp, "MYVIEW56"), "Expected MYVIEW56");
    
    
    /* check that that v3 has view type */
    
    if (EXFAIL==tptypes((char *)v3, vtyp, vsubtyp))
    {
        NDRX_LOG(log_error, "TESTERROR: got error for tptypes on v3 %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_ASSERT_VAL_OUT(0==strcmp(vtyp, "VIEW"), "Expected VIEW");
    NDRX_ASSERT_VAL_OUT(0==strcmp(vsubtyp, "MYVIEW56"), "Expected MYVIEW56");
    
    NDRX_ASSERT_UBF_OUT(Bvcmp((char *)v, "MYVIEW56", (char *)v2, "MYVIEW56")==0, 
            "Failed to compare v vs v2");
    NDRX_ASSERT_UBF_OUT(Bvcmp((char *)v, "MYVIEW56", (char *)v3, "MYVIEW56")==0, 
            "Failed to compare v vs v3");
    
out:
   
    if (NULL!=p_ub1)
    {
        tpfree((char *)p_ub1);
    }

    if (NULL!=p_ub2)
    {
        tpfree((char *)p_ub2);
    }

    if (NULL!=p_ub3)
    {
        tpfree((char *)p_ub3);
    }

    if (NULL!=p_ub4)
    {
        tpfree((char *)p_ub4);
    }

    if (NULL!=p_ub5)
    {
        tpfree((char *)p_ub5);
    }

    if (NULL!=v)
    {
        tpfree((char *)v);
    }

    if (NULL!=v2)
    {
        tpfree((char *)v2);
    }

    if (NULL!=v3)
    {
        tpfree((char *)v3);
    }
             
    return ret;
}

/**
 * Test embedded buffer build/parse for UBF
 * @return EXSUCCEED/EXFAIL
 */
expublic int test_impexp_testemb(void)
{
    int ret=EXSUCCEED;
    
    int i;
    for (i=0; i<10000; i++)
    {
        
        if (EXSUCCEED!=test_impexp_testemb_int())
        {
            NDRX_LOG(log_error, "TESTERROR at %d loop", i);
            EXFAIL_OUT(ret);
        }
        
    }
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
