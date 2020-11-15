/**
 * @brief Manual syntax test of tpimport() for embedded buffers
 *  at next step export/import is performed, thus export shall match the
 *  already tested import format.
 *
 * @file atmiclt56_embbuf_syntax.c
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
#include "expr.h"
#include "odebug.h"
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
 * Quick buffer checking...
 * @param p_ub
 * @param expr
 * @return EXFAIL/EXSUCCEED/EXFAIL
 */
exprivate int q_eval(UBFH *p_ub, char *expr)
{
    int ret = EXSUCCEED;
    char *tree=NULL;
    
    if (NULL==(tree=Bboolco(expr)))
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid check syntax for [%s]: %s", 
                expr, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    ret = Bboolev(p_ub, tree);
    
out:
    
    if (NULL!=tree)
    {
        Btreefree(tree);
    }

    return ret;
    
}

/**
 * Manual buffer import -> validate syntax
 * - Check single & array of UBF
 * - Check single view & array of views
 * - Check single ptr & array of ptrs
 * @return 
 */
expublic int test_impexp_testemb_syntax(void)
{
    int ret = EXSUCCEED;
    int i;
    long rsplen;
    UBFH *obuf=NULL;
    UBFH *p1_ubf=NULL;
    UBFH *p2_ubf=NULL;
    UBFH *p3_ubf=NULL;
    char *json_ubf_in = 
        "{"
            "\"buftype\":\"UBF\","
            "\"version\":1,"
            "\"data\":"
            "{"
                "\"T_SHORT_FLD\":55,"
                /* Single sub-UBF: */
                "\"T_UBF_FLD\":"
                "{"
                    "\"T_STRING_FLD\":\"HELLO WORLD INNER\""
                "},"
                /* Array of sub-UBFs */
                "\"T_UBF_2_FLD\": ["
                    "{"
                        "\"T_STRING_FLD\":\"HELLO WORLD INNER 1\""
                    "},"
                    "{"
                        "\"T_STRING_FLD\":\"HELLO WORLD INNER 2\""
                    "},"
                    "{"
                    "}"
                "],"
                /* Test single view import */
                "\"T_VIEW_FLD\":"
                "{"
                   "\"MYVIEW56\":"
                   "{"
                       "\"tshort1\":1,"
                       "\"tlong1\":2,"
                       "\"tchar1\":\"A\","
                       "\"tfloat1\":1,"
                       "\"tdouble1\":21,"
                       "\"tstring1\":\"ABC\","
                       "\"tcarray1\":\"SEVMTE8AAAAAAA==\""
                   "}"
                "},"
                /* Test view array: */
                "\"T_VIEW_2_FLD\": ["
                    "{"
                       "\"MYVIEW56\":"
                       "{"
                           "\"tshort1\":1,"
                           "\"tlong1\":2,"
                           "\"tchar1\":\"A\","
                           "\"tfloat1\":1,"
                           "\"tdouble1\":21,"
                           "\"tstring1\":\"ABC_2\","
                           "\"tcarray1\":\"SEVMTE8AAAAAAA==\""
                       "}"
                    "},"
                    "{"
                       "\"\":{}"
                    "},"
                    "{"
                       "\"MYVIEW56\":"
                       "{"
                           "\"tshort1\":1,"
                           "\"tlong1\":2,"
                           "\"tchar1\":\"A\","
                           "\"tfloat1\":1,"
                           "\"tdouble1\":21,"
                           "\"tstring1\":\"ABC_3\","
                           "\"tcarray1\":\"SEVMTE8AAAAAAA==\""
                       "}"
                    "}"
                "],"
                /* Test single ptr: */
                "\"T_PTR_FLD\":"
                "{"
                    "\"buftype\":\"UBF\","
                    "\"version\":1,"
                    "\"data\":"
                    "{"
                        "\"T_SHORT_FLD\":1765,"
                        "\"T_LONG_FLD\":[115,2],"
                        "\"T_CHAR_FLD\":\"A\","
                        "\"T_FLOAT_FLD\":1,"
                        "\"T_DOUBLE_FLD\":[1111.220000,333,444],"
                        "\"T_STRING_FLD\":\"HELLO WORLD\","
                        "\"T_CARRAY_FLD\":\"AAECA0hFTExPIEJJTkFSWQQFAA==\""
                    "}"
                "},"
                /* Test array of ptr: */
                "\"T_PTR_2_FLD\": ["
                    "{"
                        "\"buftype\":\"UBF\","
                        "\"version\":1,"
                        "\"data\":"
                        "{"
                            "\"T_SHORT_FLD\":1765,"
                            "\"T_LONG_FLD\":[1111,2],"
                            "\"T_CHAR_FLD\":\"A\","
                            "\"T_FLOAT_FLD\":1,"
                            "\"T_DOUBLE_FLD\":[1111.220000,333,444],"
                            "\"T_STRING_FLD\":\"HELLO WORLD 22\","
                            "\"T_CARRAY_FLD\":\"AAECA0hFTExPIEJJTkFSWQQFAA==\""
                        "}"
                    "},"
                    "{"
                        "\"buftype\":\"UBF\","
                        "\"version\":1,"
                        "\"data\":"
                        "{"
                            "\"T_SHORT_FLD\":1765,"
                            "\"T_LONG_FLD\":[4444,2],"
                            "\"T_CHAR_FLD\":\"A\","
                            "\"T_FLOAT_FLD\":1,"
                            "\"T_DOUBLE_FLD\":[1111.220000,333,444],"
                            "\"T_STRING_FLD\":\"HELLO WORLD 44\","
                            "\"T_CARRAY_FLD\":\"AAECA0hFTExPIEJJTkFSWQQFAA==\""
                        "}"
                    "}"
                "]"
            "}"
        "}";
    
    NDRX_LOG(log_info, "JSON UBF IN: [%s]", json_ubf_in);

    if (NULL == (obuf = (UBFH *)tpalloc("UBF", NULL, NDRX_MSGSIZEMAX)))
    {
        NDRX_LOG(log_error, "Failed to allocate UBFH %ld bytes: %s",
                 NDRX_MSGSIZEMAX, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    for (i=0; i<10000; i++)
    {
        rsplen=0L;
        if ( EXFAIL == tpimport(json_ubf_in, 
                                (long)strlen(json_ubf_in), 
                                (char **)&obuf, 
                                &rsplen, 
                                0L) )
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to import JSON UBF!!!!");
            EXFAIL_OUT(ret);
        }
        
        ndrx_debug_dump_UBF(log_debug, "JSON UBF imported. Return obuf", (UBFH *)obuf);
        
        /* check the values */
        
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(obuf, "T_SHORT_FLD==55"), "Expected root level parsed");
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(obuf, "T_UBF_FLD.T_STRING_FLD=='HELLO WORLD INNER'"), 
                "Expected single UBF parsed");
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(obuf, "T_UBF_2_FLD[0].T_STRING_FLD=='HELLO WORLD INNER 1'"), 
                "Array UBF[0] inval value");
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(obuf, "T_UBF_2_FLD[1].T_STRING_FLD=='HELLO WORLD INNER 2'"), 
                "Array UBF[1] inval value");
        
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(obuf, "T_VIEW_FLD[0].tstring1=='ABC'"), 
                "Single T_VIEW_FLD[0].tstring1 inval value");
        
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(obuf, "T_VIEW_2_FLD[2].tstring1=='ABC_3'"), 
                "Array T_VIEW_2_FLD[2].tstring1 inval value");
        
        /* read of the pointer allocs */
        NDRX_ASSERT_UBF_OUT(EXSUCCEED==Bget(obuf, T_PTR_FLD, 0, (char *)&p1_ubf, 0L), 
                "Failed to get T_PTR_FLD");
        
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(p1_ubf, "T_STRING_FLD=='HELLO WORLD' && T_LONG_FLD[0]==115 && T_LONG_FLD[1]==2"), 
                "Invalid p1_ubf");
        
        NDRX_ASSERT_UBF_OUT(EXSUCCEED==Bget(obuf, T_PTR_2_FLD, 0, (char *)&p2_ubf, 0L), 
                "Failed to get T_PTR_2_FLD[0]");
        
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(p2_ubf, "T_STRING_FLD=='HELLO WORLD 22' && T_LONG_FLD[0]==1111 && T_LONG_FLD[1]==2"), 
                "Invalid p2_ubf");
        
        NDRX_ASSERT_UBF_OUT(EXSUCCEED==Bget(obuf, T_PTR_2_FLD, 1, (char *)&p3_ubf, 0L), 
                "Failed to get T_PTR_2_FLD[1]");
        
        NDRX_ASSERT_VAL_OUT(EXTRUE==q_eval(p3_ubf, "T_STRING_FLD=='HELLO WORLD 44' && T_LONG_FLD[0]==4444 && T_LONG_FLD[1]==2"), 
                "Invalid p3_ubf");
        
        /* delete the buffer */
        tpfree((char *)obuf);
        
        tpfree((char *)p1_ubf);
        tpfree((char *)p2_ubf);
        tpfree((char *)p3_ubf);
        
        obuf=NULL;
    }
    
out:

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
