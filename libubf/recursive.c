/**
 * @brief Recursive UBF API
 *
 * @file recursive.c
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

/*---------------------------Includes-----------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <ubf.h>
#include <ubf_int.h>	/* Internal headers for UBF... */
#include <fdatatype.h>
#include <ferror.h>
#include <fieldtable.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <cf.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define R_IS_NEXT_EOS(X) (EXFAIL==*(X+1) || EXFAIL==*(X+2) || EXFAIL==*(X+3))
#define DEBUG_STR_MAX   512 /**< Max debug string len                   */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/


/**
 * Find the buffer for the final FLDID/OCC
 * i.e. fld1,occ1,fld2,occ2,fld3,occ3.
 * Thus our function is to return the UBF buffer presented by fld2.
 * During the search ensure that sub-buffers are UBFs actually
 * @param p_ub parent UBF buffer to search for field
 * @param fldidocc sequence of fld1,occ1,fld2,occ2,fld3,occ3.
 * @param fldid_leaf terminating field e.g. fld3
 * @param occ_leaf terminating field e.g. occ3
 * @return PTR to UBF buffer found
 */
exprivate UBFH * ndrx_ubf_R_find(UBFH *p_ub, BFLDID *fldidocc, 
        BFLDID *fldid_leaf, BFLDOCC *occ_leaf, BFLDLEN *len)
{
    int ret = EXSUCCEED;
    BFLDID bfldid;
    BFLDID occ;
    int pos=0;
    int typ;
    
    /* lookup ahead do we have EOF */
    while (!R_IS_NEXT_EOS(fldidocc))
    {
        /* first is fld id */
        bfldid=*fldidocc;
        
        /* second is occurrence */
        fldidocc++;
        
        if (EXFAIL==*fldidocc)
        {
            UBF_LOG(log_error, "Invalid recursive field identifier sequence, "
                    "expected occ, got -1 at pos %d", pos);
            ndrx_Bset_error_fmt(BEINVAL, "Invalid recursive field identifier sequence, "
                    "expected occ, got -1 at pos %d", pos);
            EXFAIL_OUT(ret);
        }
        
        occ=*fldidocc;
        
        /* find the buffer */
        typ = Bfldtype(bfldid);
        if (BFLD_UBF!=typ)
        {
            
            UBF_LOG(log_error, "Expected BFLD_UBF (%d) at position %d in "
                    "sequence but got: %d type", BFLD_UBF, pos, typ);
            ndrx_Bset_error_fmt(BTYPERR, "Expected BFLD_UBF (%d) at "
                    "position %d in sequence but got: %d type", BFLD_UBF, pos, typ);
            EXFAIL_OUT(ret);
        }
        
        p_ub = (UBFH *)ndrx_Bfind(p_ub, bfldid, occ, len, NULL);
        
        if (NULL==p_ub)
        {
            UBF_LOG(log_error, "Buffer not found at position of field sequence %d", pos);
            EXFAIL_OUT(ret);
        }
        
        pos++;
        
        /* step to next pair */
        fldidocc+=1;
        
    }
    
    if (NULL!=p_ub)
    {
        if (EXFAIL==*fldidocc)
        {
            UBF_LOG(log_error, "Field ID not present at position %d in sequence (-1 found)",
                    pos);
            ndrx_Bset_error_fmt(BEINVAL, "Field ID not present at position %d in sequence (-1 found)",
                    pos);
            EXFAIL_OUT(ret);
        }
        
        *fldid_leaf=*fldidocc;
        
        fldidocc++;
        
        if (EXFAIL==*fldidocc)
        {
            UBF_LOG(log_error, "Occurrence not present at position %d in sequence (-1 found)",
                    pos);
            ndrx_Bset_error_fmt(BEINVAL, "Occurrence not present at position %d in sequence (-1 found)",
                    pos);
            EXFAIL_OUT(ret);
        }
        
        *occ_leaf=*fldidocc;    
    }
    
    UBF_LOG(log_debug, "Leaf fldid=%d occ=%d", *fldid_leaf, *occ_leaf);
    
out:
    
    UBF_LOG(log_debug, "Returning status=%d p_ub=%p", ret, p_ub);
    
    return p_ub;
}

/**
 * Construct the full debug string as the path 
 * FIELD1[OCC1].FIELD2[OCC2]
 * @param fldidocc
 * @param debug_buf
 * @param debug_buf_len
 * @return 
 */
exprivate void ndrx_ubf_sequence_str(BFLDID *fldidocc, 
        char *debug_buf, size_t debug_buf_len)
{
    int pos=0;
    char *nam;
    char tmp[128];
    debug_buf[0]=EXEOS;
    
    while (EXFAIL!=*fldidocc)
    {
        if (0==pos%2)
        {
            /* field id: */
            nam=Bfname(*fldidocc);
            
            if (pos>0)
            {
                NDRX_STRCAT_S(debug_buf, debug_buf_len, ".");
            }
            NDRX_STRCAT_S(debug_buf, debug_buf_len, nam);
        }
        else
        {
            NDRX_STRCAT_S(debug_buf, debug_buf_len, "[");
            snprintf(tmp, sizeof(tmp), "%d", *fldidocc);
            NDRX_STRCAT_S(debug_buf, debug_buf_len, tmp);
            NDRX_STRCAT_S(debug_buf, debug_buf_len, "]");
        }
        
        fldidocc++;
        pos++;
    }
}

/**
 * Recursive field field get
 * @param p_ub parent UBF buffer
 * @param fldidocc array of: <field_id1>,<occ1>,..<field_idN><occN>,-1
 * @param buf buffer where to return the value
 * @param len in input buffer len, on output bytes written
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_RBget (UBFH * p_ub, BFLDID *fldidocc,
                            char * buf, BFLDLEN * buflen)
{
    
    int ret = EXSUCCEED;
    
    BFLDID bfldid;
    BFLDOCC occ;
    BFLDLEN len_data;
    char debugbuf[DEBUG_STR_MAX]="";
    
    p_ub=ndrx_ubf_R_find(p_ub, fldidocc, &bfldid, &occ, &len_data);
    
    if (NULL==p_ub)
    {
        if (debug_get_ubf_level() > log_info)
        {
            ndrx_ubf_sequence_str(fldidocc, debugbuf, sizeof(debugbuf));
            UBF_LOG(log_info, "Field not found, sequence: %s", debugbuf);
        }
        
        EXFAIL_OUT(ret);
    }
    
    ret=Bget(p_ub, bfldid, occ, buf, buflen);
    
out:
    return ret;
}


/**
 * Recursive field field get, with convert
 * @param p_ub parent UBF buffer
 * @param fldidocc array of: <field_id1>,<occ1>,..<field_idN><occN>,-1
 * @param buf buffer where to return the value
 * @param len in input buffer len, on output bytes written
 * @param usrtype user type to convert data to
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_RCBget (UBFH * p_ub, BFLDID *fldidocc,
                            char * buf, BFLDLEN * len, int usrtype)
{
    int ret = EXSUCCEED;
    BFLDID bfldid;
    BFLDOCC occ;
    BFLDLEN len_data;
    char debugbuf[DEBUG_STR_MAX]="";
    
    p_ub=ndrx_ubf_R_find(p_ub, fldidocc, &bfldid, &occ, &len_data);
    
    if (NULL==p_ub)
    {
        if (debug_get_ubf_level() > log_info)
        {
            ndrx_ubf_sequence_str(fldidocc, debugbuf, sizeof(debugbuf));
            UBF_LOG(log_info, "Field not found, sequence: %s", debugbuf);
        }
        
        EXFAIL_OUT(ret);
    }
    
    ret=CBget(p_ub, bfldid, occ, buf, len, usrtype);
    
out:
    return ret;
}

/**
 * Recursive find implementation
 * @param p_ub root UBF buffer
 * @param fldidocc field sequence
 * @param p_len data len
 * @return ptr to data
 */
expublic char* ndrx_RBfind (UBFH *p_ub, BFLDID *fldidocc, BFLDLEN *p_len)
{
    char* ret = NULL;
    BFLDID bfldid;
    BFLDOCC occ;
    BFLDLEN len_data;
    char debugbuf[DEBUG_STR_MAX]="";
    
    p_ub=ndrx_ubf_R_find(p_ub, fldidocc, &bfldid, &occ, &len_data);
    
    if (NULL==p_ub)
    {
        if (debug_get_ubf_level() > log_info)
        {
            ndrx_ubf_sequence_str(fldidocc, debugbuf, sizeof(debugbuf));
            UBF_LOG(log_info, "Field not found, sequence: %s", debugbuf);
        }
        
        goto out;
    }
    
    ret=Bfind (p_ub, bfldid, occ, p_len);
    
out:
    return ret;
}

/**
 * Recursive find implementation
 * @param p_ub root UBF buffer
 * @param fldidocc field sequence
 * @param p_len data len
 * @param usrtype user type
 * @return ptr to data
 */
expublic char* ndrx_RCBfind (UBFH *p_ub, BFLDID *fldidocc, BFLDLEN *p_len, int usrtype)
{
    char* ret = NULL;
    BFLDID bfldid;
    BFLDOCC occ;
    BFLDLEN len_data;
    char debugbuf[DEBUG_STR_MAX]="";
    
    p_ub=ndrx_ubf_R_find(p_ub, fldidocc, &bfldid, &occ, &len_data);
    
    if (NULL==p_ub)
    {
        if (debug_get_ubf_level() > log_info)
        {
            ndrx_ubf_sequence_str(fldidocc, debugbuf, sizeof(debugbuf));
            UBF_LOG(log_info, "Field not found, sequence: %s", debugbuf);
        }
        
        goto out;
    }
    
    ret=CBfind (p_ub, bfldid, occ, p_len, usrtype);
    
out:
    return ret;
}

/* TODO: Add RBpres */


/* vim: set ts=4 sw=4 et smartindent: */
