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

#include "xatmi.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define R_IS_NEXT_EOS(X) (EXFAIL==*(X+1) || EXFAIL==*(X+2) || EXFAIL==*(X+3))
#define DEBUG_STR_MAX   512 /**< Max debug string len                   */

#define VIEW_FLD_FOUND  1
#define VIEW_FLD_CNAME_PARSED  2

#define STATE_NONE  1
#define STATE_OCC  2
#define STATE_OCCANY  3
#define STATE_VIEW  4

#define IS_VALID_ID(X)  ( X>='a' && X<='z' || X>='0' && X<='9' || X>='A' && X<='Z' || X=='_')
#define IS_VALID_NUM(X)  ( X>='0' && X<='9' )

#define RESOLVE_FIELD   tmp[j]=EXEOS;\
if (is_view)\
{\
  rfldid->cname=NDRX_STRDUP(tmp);\
  if (NULL==rfldid->cname)\
  {\
    int err;\
    err=errno;\
    UBF_LOG(log_error, "Failed to malloc: %s", strerror(errno));\
    ndrx_Bset_error_fmt(BEUNIX, "Failed to malloc: %s", strerror(errno));\
    EXFAIL_OUT(ret);\
  }\
  is_view=VIEW_FLD_CNAME_PARSED;\
}\
else \
{\
    if (BBADFLDID==(parsedid=Bfldid (tmp)))\
    {\
        UBF_LOG(log_error, "Failed to resolve [%s] nrfld=%d", tmp, nrflds);\
        EXFAIL_OUT(ret);\
    }\
    if (BFLD_VIEW==Bfldtype(parsedid))\
    {\
        is_view=VIEW_FLD_FOUND;\
    } /* cache last field */\
    rfldid->bfldid=parsedid;\
}\
j=0;\
nrflds++;

/** 
 * Add field to ndrx_ubf_rfldid_t, so lets build full version in any case 
 * Also needs to test the case if we parsed the view field, then 
 * nothing to add to growlist
 */
#define RESOLVE_ADD   if (VIEW_FLD_CNAME_PARSED!=is_view)\
{\
  ndrx_growlist_append(&(rfldid->fldidocc), &parsedid);\
  ndrx_growlist_append(&(rfldid->fldidocc), &parsedocc);\
  rfldid->bfldid=parsedid;\
  rfldid->occ=parsedocc;\
}\
else \
{\
    rfldid->cname_occ=parsedocc;\
}\
j=0;

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Free up the rfield parsed data
 * @param rfldid recursive field id
 */
expublic void ndrx_ubf_rfldid_free(ndrx_ubf_rfldid_t *rfldid)
{
    if (NULL!=rfldid->cname)
    {
        NDRX_FREE(rfldid->cname);
        rfldid->cname=NULL;
    }
    
    if (NULL!=rfldid->fldnm)
    {
        NDRX_FREE(rfldid->fldnm);
        rfldid->fldnm=NULL;
    }
    
    ndrx_growlist_free(&(rfldid->fldidocc));
}

/**
 * Parse the field reference
 * @param rfldidstr field reference: fld[occ].fld[occ].fld.fld[occ]
 * @param rfldid parsed reference
 * @param bfldid leaf field id
 * @param occ leaf occurrence
 * @return EXFAIL (error), EXSUCCEED (ok)
 */
expublic int ndrx_ubf_rfldid_parse(char *rfldidstr, ndrx_ubf_rfldid_t *rfldid)
{
    int ret = EXSUCCEED;
    int i, j, len, state, terminator;
    char tmp[PATH_MAX];
    BFLDID parsedid=BBADFLDID;
    BFLDOCC parsedocc;
    int *rfldidseq;
    int nrflds=0;
    int is_view=EXFALSE;
        
    UBF_LOG(log_debug, "Parsing field id sequence: [%s]", rfldidstr);
    ndrx_growlist_init(&(rfldid->fldidocc), 10, sizeof(int));
    rfldid->cname = NULL;
    rfldid->fldnm = NDRX_STRDUP(rfldidstr);
    
    if (NULL==rfldid->fldnm)
    {
        int err = errno;
        
        ndrx_Bset_error_fmt(BEUNIX, "Failed to malloc rfldidstr: %s", strerror(err));
        UBF_LOG(log_error, "Failed to malloc rfldidstr: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    len=strlen(rfldidstr);
    
    if (len>sizeof(tmp)-1)
    {
        ndrx_Bset_error_fmt(BSYNTAX, "Field id too max len %d max %d: [%s] ",
                len, sizeof(tmp)-1, rfldidstr);
        UBF_LOG(log_error, "Field id too max len %d max %d: [%s] ",
                len, sizeof(tmp)-1, rfldidstr);
        EXFAIL_OUT(ret);
    }
    
    state=STATE_NONE;
    j=0;
    for (i=0; i<len+1; i++)
    {
        if (STATE_NONE==state)
        {
            if (0x0 ==rfldidstr[i])
            {
                if (j>0)
                {
                    /* finish field, got 0 occ */
                    tmp[j]=EXEOS;
                    RESOLVE_FIELD;
                    parsedocc=0;
                    RESOLVE_ADD;
                }
                
                /* we are done... */
                break;
            }
            
            /* error! sub-fields of view not supported */
            if (VIEW_FLD_CNAME_PARSED==is_view)
            {
                ndrx_Bset_error_fmt(BSYNTAX, "Subfield for view-field not expected: [%s] nrfld=%d pos=%d", 
                        rfldidstr, nrflds, i);
                UBF_LOG(log_error, "Subfield for view-field not expected: [%s] nrfld=%d pos=%d", 
                        rfldidstr, nrflds, i);
                EXFAIL_OUT(ret);
            }
            
            if (IS_VALID_ID(rfldidstr[i]))
            {
                tmp[j]=rfldidstr[i];
                j++;
            }
            else if ('['==rfldidstr[i])
            {
                /* finish field, count occ */
                tmp[j]=EXEOS;
                RESOLVE_FIELD;
                state=STATE_OCC;
            }
            else if ('.'==rfldidstr[i])
            {
                /* finish field, got 0 occ */
                tmp[j]=EXEOS;
                RESOLVE_FIELD;
                parsedocc=0;
                RESOLVE_ADD;
            }
            else
            {
                /* error! invalid character expected C identifier, '.', or '[' */
                ndrx_Bset_error_fmt(BSYNTAX, "Invalid character found [%c] in [%s] pos=%d",
                        rfldidstr[i], rfldidstr, i);
                UBF_LOG(log_error, "Invalid character found [%c] in [%s] pos=%d",
                        rfldidstr[i], rfldidstr, i);
                EXFAIL_OUT(ret);
            }
        }
        else if (STATE_OCC==state)
        {
            if (IS_VALID_NUM(rfldidstr[i]))
            {
                tmp[j]=rfldidstr[i];
                j++;
            }
            else if ('?'==rfldidstr[i])
            {
                
                tmp[j]=rfldidstr[i];
                j++;
                if (j>0)
                {
                    tmp[j]=EXEOS;
                    ndrx_Bset_error_fmt(BSYNTAX, "Invalid occurrence: [%s] at pos=%d", 
                            tmp, i);
                    UBF_LOG(log_error, "Invalid occurrence: [%s] at pos=%d", 
                            tmp, i);
                    EXFAIL_OUT(ret);
                }
                
                parsedocc=-2; /* any occurrence */
                state=STATE_OCCANY;
            }
            else if (']'==rfldidstr[i])
            {
                /* Finish off the field + occ */
                tmp[j]=EXEOS;
                parsedocc=atoi(tmp);
                RESOLVE_ADD;
                state=STATE_NONE;
            }
            else if (0x0 ==rfldidstr[i])
            {
                tmp[j]=rfldidstr[i];
                ndrx_Bset_error_fmt(BSYNTAX, "Unclosed occurrence [%s] at pos=%d", 
                        tmp[j], i);
                UBF_LOG(log_error, "Unclosed occurrence [%s] at pos=%d", 
                        tmp[j], i);
                EXFAIL_OUT(ret);
            }
            else
            {
                /* error! Invalid character, expected number, '?' or ']' */
                tmp[j]=rfldidstr[i];
                j++;
                tmp[j]=EXEOS;
                ndrx_Bset_error_fmt(BSYNTAX, "Invalid occurrence: [%s] at pos=%d", 
                            tmp, i);
                UBF_LOG(log_error, "Invalid occurrence: [%s] at pos=%d", 
                        tmp, i);
                EXFAIL_OUT(ret);
            }
        }
        else if (STATE_OCCANY==state)
        {
            if (']'==rfldidstr[i])
            {
                /* Finish off the field + occ, already parsed.. */
                RESOLVE_ADD;
                state=STATE_NONE;
            }
            else if (0x0 ==rfldidstr[i])
            {
                tmp[j]=rfldidstr[i];
                ndrx_Bset_error_fmt(BSYNTAX, "Unclosed occurrence [%s] at pos=%d", 
                        tmp[j], i);
                UBF_LOG(log_error, "Unclosed occurrence [%s] at pos=%d", 
                        tmp[j], i);
                EXFAIL_OUT(ret);
            }
            else
            {
                /* error! Invalid character, expected [?] */
                tmp[j]=rfldidstr[i];
                tmp[j]=EXEOS;
                ndrx_Bset_error_fmt(BSYNTAX, "Invalid any occurrence: [%s] at pos=%d", 
                            tmp, i);
                UBF_LOG(log_error, "Invalid any occurrence: [%s] at pos=%d", 
                        tmp, i);
                EXFAIL_OUT(ret);
            }
        }
    }
    
    /* Terminate the memory sequence... */
    
    /* Unload the field... */
    terminator=-1;
    ndrx_growlist_append(&(rfldid->fldidocc), &terminator);
    
    rfldidseq=(int *)rfldid->fldidocc.mem;
    
    /* get the last two identifiers.. */
    if (rfldid->fldidocc.maxindexused<2)
    {
         ndrx_Bset_error_fmt(BSYNTAX, "Empty field name parsed (%d)!",
                 rfldid->fldidocc.maxindexused);
        UBF_LOG(log_error, "Empty field name parsed (%d)!",
                rfldid->fldidocc.maxindexused);
        EXFAIL_OUT(ret);
    }
    
    rfldid->nrflds=nrflds;
    
out:

    /* free up un-needed resources */
    if (EXSUCCEED!=ret)
    {
        ndrx_ubf_rfldid_free(rfldid);
    }

    UBF_LOG(log_debug, "returns %d bfldid=%d, occ=%d, nrflds=%d, cname=[%s]", 
        ret, rfldid->bfldid, rfldid->occ, nrflds, rfldid->cname?rfldid->cname:"(null)");
    return ret;
}

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
