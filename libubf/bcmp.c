/* 
 * @brief Implementation of Bcmp and Bsubset
 *   TODO: Might increase speed if we would use Bfind?
 *
 * @file bcmp.c
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

/*---------------------------Includes-----------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#include <ubf.h>
#include <ubf_int.h>	/* Internal headers for UBF... */
#include <fdatatype.h>
#include <ferror.h>
#include <fieldtable.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <cf.h>
#include <ubf_impl.h>

#include "userlog.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Compare two UBF buffers
 * @param p_ubf1 buffer 1 to compare
 * @param p_ubf2 buffer 2 to compare with 1
 * @return  0 - buffers are equal
 * -1 - first ubf1 field is less than ubf2, value of 1 is less than 2
 *      ubf1 have less fields than ubf2
 * 1 - vice versa
 */
expublic int ndrx_Bcmp(UBFH *p_ubf1, UBFH *p_ubf2)
{
    int ret = EXSUCCEED;
    Bnext_state_t state1;
    Bnext_state_t state2;
    BFLDID bfldid1;
    BFLDOCC occ1;

    BFLDID bfldid2;
    BFLDOCC occ2;

    BFLDLEN len1;
    BFLDLEN len2;
    
    int ret1;
    int ret2;
    
    char *buf1;
    char *buf2;
    
    dtype_ext1_t *typ;
    int typcode;
    
    memset(&state1, 0, sizeof(state1));
    memset(&state2, 0, sizeof(state2));
    
    bfldid1 = BFIRSTFLDID;
    bfldid2 = BFIRSTFLDID;
    while (1)
    {
        ret1=ndrx_Bnext(&state1, p_ubf1, &bfldid1, &occ1, NULL, &len1, &buf1);
        ret2=ndrx_Bnext(&state2, p_ubf2, &bfldid2, &occ2, NULL, &len2, &buf2);
        
        if (EXFAIL==ret1)
        {
            /* error must be set */
            UBF_LOG(log_debug, "buffer1 Bnext failed");
            EXFAIL_OUT(ret);
        }

        if (EXFAIL==ret2)
        {
            /* error must be set */
            UBF_LOG(log_debug, "buffer2 Bnext failed");
            EXFAIL_OUT(ret);
        }

        if (EXTRUE!=ret1 && EXTRUE!=ret2)
        {
            UBF_LOG(log_debug, "EOF reached buffers %p vs %p equal",
                    p_ubf1, p_ubf2);
            
            ret = 0;
            goto out;
        }
        else if (EXTRUE!=ret1 && EXTRUE==ret2)
        {
            ret=-1;
            goto out;
        }
        else if (EXTRUE==ret1 && EXTRUE!=ret2)
        {
            ret=1;
            goto out;
        }
        /* compare IDs */
        else if (bfldid1 < bfldid2)
        {
            ret=-1;
            goto out;
        }
        else if (bfldid1 > bfldid2)
        {
            ret=1;
            goto out;
        }
        
        /* ok, ids are equal, lets compare by value */
        
        typcode = Bfldtype(bfldid1);
        
        if (typcode > BFLD_MAX || typcode < BFLD_MIN)
        {
            ret=-11;
            userlog("Invalid type id found in buffer %p: %d - corrupted UBF buffer?", 
                    p_ubf1, typcode);
            UBF_LOG(log_error, "Invalid type id found in buffer %p: %d "
                    "- corrupted UBF buffer?",  p_ubf1, typcode)
            /* set error */
            ndrx_Bset_error_fmt(BNOTFLD, "Invalid type id found in buffer %p: %d "
                    "- corrupted UBF buffer?", p_ubf1, typcode);
            
            goto out;
        }

        typ = &G_dtype_ext1_map[typcode];

        ret = typ->p_cmp(typ, buf1, len1, buf2, len2, UBF_CMP_MODE_STD);
        
        if (ret < 0)
        {
            /* so fields are different */
            ret = -1;
            goto out;
        }
        else if (ret > 0)
        {
            ret = 1;
            goto out;
        }
    }
    
out:

    return ret;
}

/**
 * Compares the buffer and returns TRUE if all p_ubf2 fields and values 
 * are present in ubf1
 * @param p_ubf1 haystack
 * @param p_ubf2 needle
 * @return EXFALSE not a subset, EXTRUE is subset, EXFAIL (error occurred)
 */
expublic int ndrx_Bsubset(UBFH *p_ubf1, UBFH *p_ubf2)
{
    /* so basically we loop over the p_ubf2 and check every field obf ubf2
     * presence in ubf1 and compare their values... */
    
    int ret = EXSUCCEED;
    Bnext_state_t state2;

    BFLDID bfldid2;
    BFLDOCC occ2;

    BFLDLEN len1;
    BFLDLEN len2;

    char *buf1 = NULL;
    char *buf2 = NULL;
    
    int ret2;
    
    dtype_ext1_t *typ;
    int typcode;
    
    
    memset(&state2, 0, sizeof(state2));
    
    bfldid2 = BFIRSTFLDID;
    while (1)
    {
        ret2=ndrx_Bnext(&state2, p_ubf2, &bfldid2, &occ2, NULL, &len2, &buf2);    
        
        if (0==ret2)
        {
            /* ok we have EOF of buffer, equals */
            ret=EXTRUE;
            goto out;
        }
        else if (EXFAIL==ret2)
        {
            /* something is have failed, Bnext have set error  */
            EXFAIL_OUT(ret);
        }
        
        /* got the field, now read from haystack */
        
        buf1=ndrx_Bfind(p_ubf1, bfldid2, occ2, &len1, NULL);
        
        if (NULL==buf1)
        {
            if (BNOTPRES!=Berror)
            {
                UBF_LOG(log_error, "Failed to get [%d] occ [%d] from haystack buffer", 
                        bfldid2, occ2);
                EXFAIL_OUT(ret);
            }
            
            ndrx_Bunset_error();
            /* in this case needle is not part of haystack */
            ret = EXFALSE;
            goto out;
        }
        
        /* we have got the value, now compare boths  */
        
        typcode = Bfldtype(bfldid2);
        
        if (typcode > BFLD_MAX || typcode < BFLD_MIN)
        {
            ret=-1;
            userlog("Invalid type id found in buffer %p: %d - corrupted UBF buffer?", 
                    p_ubf1, typcode);
            UBF_LOG(log_error, "Invalid type id found in buffer %p: %d "
                    "- corrupted UBF buffer?",  p_ubf1, typcode)
            /* set error */
            ndrx_Bset_error_fmt(BNOTFLD, "Invalid type id found in buffer %p: %d "
                    "- corrupted UBF buffer?", p_ubf1, typcode);
            
            goto out;
        }

        
        typ = &G_dtype_ext1_map[typcode];
        
        ret=typ->p_cmp(typ, buf1, len1, buf2, len2, 0);
        
        if (EXFALSE==ret)
        {
            UBF_LOG(log_debug, "fields are different, not a subset");
            ret=EXFALSE;
            goto out;
        }
        else if (EXFAIL==ret)
        {
            UBF_LOG(log_error, "error comparing fields");
            ret=-1;
            goto out;
        }
    }
    
out:
    return ret;
}

