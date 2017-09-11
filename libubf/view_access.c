/* 
** Dynamic VIEW access functions
**
** @file view_access.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

#include <ndrstandard.h>
#include <ubfview.h>
#include <ndebug.h>

#include <userlog.h>
#include <view_cmn.h>
#include <atmi_tls.h>
#include <cf.h>
#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Return the VIEW field according to user type
 * In case of NULL value, we do not return the given occurrence.
 * If the C_count is less than given occurrence and BVNEXT_NOTNULL is set, then
 * field not found will be returned.
 * 
 * @param cstruct instance of the view object
 * @param view view name
 * @param cname field name in view
 * @param occ array occurrence of the field (if count > 0)
 * @param buf user buffer to install data to
 * @param len on input user buffer len, on output bytes written to (for carray only)
 * on input for string too.
 * @param usrtype of buf see BFLD_* types
 * @param mode BVACCESS_NOTNULL
 * @return 0 on success, or -1 on fail. 
 * 
 * The following errors possible:
 * - BBADVIEW view not found
 * - BNOCNAME field not found
 * - BNOTPRES field is NULL
 * - BEINVAL cstruct/view/cname/buf is NULL
 * - BEINVAL - invalid usrtype
 * - BEINVAL - invalid occurrence
 * - BNOSPACE - no space in buffer (the data is larger than user buf specified)
 */

expublic int ndrx_CBvget_int(char *cstruct, ndrx_typedview_t *v,
	ndrx_typedview_field_t *f, BFLDOCC occ, char *buf, BFLDLEN *len, 
			     int usrtype, long flags)
{
    int ret = EXSUCCEED;
    int dim_size = f->fldsize/f->count;
    char *fld_offs = cstruct+f->offset+occ*dim_size;
    char *cvn_buf;
    short *C_count;
    short C_count_stor;
    unsigned short *L_length;
    unsigned short L_length_stor;

    UBF_LOG(log_debug, "%s enter, get %s.%s occ=%d", __func__,
            v->vname, f->cname, occ);

    NDRX_VIEW_COUNT_SETUP;

    if (flags & BVACCESS_NOTNULL)
    {
        if (ndrx_Bvnull_int(v, f, occ, cstruct))
        {
            NDRX_LOG(log_debug, "Field is NULL");
            ndrx_Bset_error_fmt(BNOTPRES, "%s.%s occ=%d is NULL", 
                             v->vname, f->cname, occ);
            EXFAIL_OUT(ret);
        }

        if (*C_count<occ+1)
        {
            NDRX_LOG(log_debug, "%s.%s count field is set to %hu, "
                    "but requesting occ=%d (zero based) - NOT PRES",
                     v->vname, f->cname, *C_count, occ);
            ndrx_Bset_error_fmt(BNOTPRES, "%s.%s count field is set to %hu, "
                    "but requesting occ=%d (zero based) - NOT PRES",
                     v->vname, f->cname, *C_count, occ);
            EXFAIL_OUT(ret);
        }
    }

    /* Will request type convert now */
    NDRX_VIEW_LEN_SETUP(occ, dim_size);

    cvn_buf = ndrx_ubf_convert(f->typecode_full, CNV_DIR_OUT, fld_offs, *L_length,
                                usrtype, buf, len);
    if (NULL==cvn_buf)
    {
        UBF_LOG(log_error, "%s: failed to convert data!", __func__);
        /* Error should be provided by conversation function */
        EXFAIL_OUT(ret);
    }
	 
out:
    UBF_LOG(log_debug, "%s return %d", __func__, ret);

    return ret;
}

/**
 * Wrapper for function for ndrx_CBvget_int, just to resolve the objects
 * @param cstruct
 * @param view
 * @param cname
 * @param occ
 * @param buf
 * @param len
 * @param usrtype
 * @param mode
 * @return 
 */
expublic int ndrx_CBvget(char *cstruct, char *view, char *cname, BFLDOCC occ, 
			 char *buf, BFLDLEN *len, int usrtype, long flags)
{
    int ret = EXFALSE;
    ndrx_typedview_t *v = NULL;
    ndrx_typedview_field_t *f = NULL;

    /* resolve view & field, call ndrx_Bvnull_int */

    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }

    if (NULL==(f = ndrx_view_get_field(v, cname)))
    {
        ndrx_Bset_error_fmt(BNOCNAME, "Field [%s] of view [%s] not found!", 
                cname, v->vname);
        EXFAIL_OUT(ret);
    }

    if (occ > f->count-1 || occ<0)
    {
        ndrx_Bset_error_fmt(BEINVAL, "Invalid occurrence requested for field "
                "%s.%s, count=%d occ=%d (zero base)",
                v->vname, f->cname, f->count, occ);
        EXFAIL_OUT(ret);
    }

    if (EXFAIL==(ret=ndrx_CBvget_int(cstruct, v, f, occ, buf, len, 
                                     usrtype, flags)))
    {
        /* error must be set */
        NDRX_LOG(log_error, "ndrx_CBvget_int failed");
        EXFAIL_OUT(ret);
    }
	
out:
	
    return ret;
}

/**
 * Return the VIEW field according to user type
 * If "C" (count flag) was used, and is less than occ+1, then C_<field> is incremented
 * to occ+1.
 * 
 * In case if "L" (length) was present and this is carray buffer, then L flag will
 * be set
 * 
 * @param cstruct instance of the view object
 * @param view view name
 * @param cname field name in view
 * @param occ array occurrence of the field (if count > 0)
 * @param buf data to set
 * @param len used only for carray, to indicate the length of the data
 * @param usrtype of buf see BFLD_* types
 * @return 0 on success, or -1 on fail. 
 * 
 * The following errors possible:
 * - BBADVIEW view not found
 * - BNOCNAME field not found
 * - BNOTPRES field not present (invalid occ)
 * - BEINVAL cstruct/view/cname/buf is NULL
 * - BEINVAL invalid usrtype
 * - BNOSPACE the view field is shorter than data received
 */
expublic int ndrx_CBvchg_int(char *cstruct, ndrx_typedview_t *v, 
		ndrx_typedview_field_t *f, BFLDOCC occ, char *buf, 
			     BFLDLEN len, int usrtype)
{
	int ret = EXSUCCEED;
	int dim_size = f->fldsize/f->count;
	char *fld_offs = cstruct+f->offset+occ*dim_size;
	char *cvn_buf;
	short *C_count;
	short C_count_stor;
	unsigned short *L_length;
	unsigned short L_length_stor;
   
	BFLDLEN setlen;
	UBF_LOG(log_debug, "%s enter, get %s.%s occ=%d", __func__,
		v->vname, f->cname, occ);
	
	NDRX_VIEW_COUNT_SETUP;
	
	/* Length output buffer */
	NDRX_VIEW_LEN_SETUP(occ, dim_size);

	setlen = dim_size;
	
	cvn_buf = ndrx_ubf_convert(usrtype, CNV_DIR_OUT, buf, len,
                                    f->typecode_full, fld_offs, &setlen);
	
	if (NULL==cvn_buf)
        {
            UBF_LOG(log_error, "%s: failed to convert data!", __func__);
            /* Error should be provided by conversation function */
            EXFAIL_OUT(ret);
        }
        
        if (occ+1 > *C_count)
        {
            *C_count = occ+1;
        }
	
	*L_length = setlen;
	
out:	
	UBF_LOG(log_debug, "%s return %d", __func__, ret);
	
	return ret;
}

/**
 * Set the field wrapper for internal method above.
 * @param cstruct
 * @param view
 * @param cname
 * @param occ
 * @param buf
 * @param len
 * @param usrtype
 * @return 
 */
expublic int ndrx_CBvchg(char *cstruct, char *view, char *cname, BFLDOCC occ, 
			 char *buf, BFLDLEN len, int usrtype)
{
    int ret = EXFALSE;
    ndrx_typedview_t *v = NULL;
    ndrx_typedview_field_t *f = NULL;

    /* resolve view & field, call ndrx_Bvnull_int */

    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }

    if (NULL==(f = ndrx_view_get_field(v, cname)))
    {
        ndrx_Bset_error_fmt(BNOCNAME, "Field [%s] of view [%s] not found!", 
                cname, v->vname);
        EXFAIL_OUT(ret);
    }

    if (occ > f->count-1 || occ<0)
    {
        ndrx_Bset_error_fmt(BEINVAL, "Invalid occurrence requested for field "
                "%s.%s, count=%d occ=%d (zero base)",
                v->vname, f->cname, f->count, occ);
        EXFAIL_OUT(ret);
    }

    if (EXFAIL==(ret=ndrx_CBvchg_int(cstruct, v, f, occ, buf, len, usrtype)))
    {
        /* error must be set */
        NDRX_LOG(log_error, "ndrx_CBvchg_int failed");
        EXFAIL_OUT(ret);
    }
	
out:
    return ret;
}

/**
 * Return size of the given view in bytes
 * @param view view name
 * @return >=0 on success, -1 on FAIL.
 * Errors:
 * BBADVIEW view not found
 * BEINVAL view field is NULL
 */
expublic long ndrx_Bvsizeof(char *view)
{
    long ret;

    ndrx_typedview_t *v = NULL;

    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }

    ret = v->ssize;
	
out:
    return ret;
}

/**
 * Return the occurrences set in buffer. This will either return C_ count field
 * set, or will return max array size, this does not test field against NULL or not.
 * anything, 
 * @param cstruct instance of the view object
 * @param view view name
 * @param cname field name in view
 * @param maxocc number of occurrences for view filed
 * @param realoccs by using set the count set in variable, it tests the array
 * until the end. And it detects the last array element which is not NULL and 
 * from start till last used (even in the middle there is NULL) is assumed as
 * it is real occs.
 * @return occurrences (1 based)
 *  The following errors possible:
 * - BBADVIEW view not found
 * - BNOCNAME field not found
 * - BEINVAL cstruct/view/cname/buf is NULL
 * 
 */
expublic BFLDOCC ndrx_Bvoccur_int(char *cstruct, ndrx_typedview_t *v, 
		ndrx_typedview_field_t *f, BFLDOCC *maxocc, BFLDOCC *realocc)
{
    BFLDOCC ret;
    short *C_count;
    short C_count_stor;
    int i;
        
    NDRX_VIEW_COUNT_SETUP;

    *maxocc=f->count;

    ret = *C_count;
    
    /* search for number of real occs... 
     */
    *realocc = f->count;
    
    /* scan from the end until we reach first non NULL */
    for (i=f->count-1; i>=0; i--)
    {
        if (!ndrx_Bvnull_int(v, f, i, cstruct))
        {
            break;
        }
    }
    
    *realocc = i+1;
	
out:
    NDRX_LOG(log_debug, "%s returns %d maxocc=%d", __func__, ret, maxocc);
    return ret;
}

/**
 * Wrapper for ndrx_Bvoccur_int
 * @param cstruct
 * @param view
 * @param cname
 * @param maxocc
 * @param realocc
 * @return 
 */
expublic BFLDOCC ndrx_Bvoccur(char *cstruct, char *view, char *cname, 
        BFLDOCC *maxocc, BFLDOCC *realocc)
{
    BFLDOCC ret = EXSUCCEED;
    ndrx_typedview_t *v = NULL;
    ndrx_typedview_field_t *f = NULL;

    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }

    if (NULL==(f = ndrx_view_get_field(v, cname)))
    {
        ndrx_Bset_error_fmt(BNOCNAME, "Field [%s] of view [%s] not found!", 
                cname, v->vname);
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==(ret = ndrx_Bvoccur_int(cstruct, v, f, maxocc, realocc)))
    {
        NDRX_LOG(log_error, "ndrx_Bvoccur_int failed");
    }
    
out:
    return ret;    
}

/**
 * Set C_ occurrence indicator field, if "C" flag was set
 * If flag is not present, function succeeds and does not return error, but
 * data also is not changed.
 * @param cstruct instance of the view object
 * @param view view name
 * @param cname field name in view
 * @param occ occurrences (non zero based)
 * @return 0 on success or -1 on failure
 *  The following errors possible:
 * - BBADVIEW view not found
 * - BNOCNAME field not found
 * - BEINVAL cstruct/view/cname/buf is NULL
 * - BNOTPRES occ out of bounds
 */
expublic int ndrx_Bvsetoccur(char *cstruct, char *view, char *cname, BFLDOCC occ)
{	
    int ret = EXSUCCEED;
    ndrx_typedview_t *v = NULL;
    ndrx_typedview_field_t *f = NULL;
    short *C_count;
    short C_count_stor;

    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }

    if (NULL==(f = ndrx_view_get_field(v, cname)))
    {
        ndrx_Bset_error_fmt(BNOCNAME, "Field [%s] of view [%s] not found!", 
                cname, v->vname);
        EXFAIL_OUT(ret);
    }

    if (occ>f->count || occ<0)
    {
        ndrx_Bset_error_fmt(BEINVAL, "%s: invalid occ %d max: %d, min: 0",
                         __func__, occ, occ>f->count);
        EXFAIL_OUT(ret);
    }

    NDRX_VIEW_COUNT_SETUP;

    *C_count = occ;
	
out:
    return ret;
}

/**
 * This will iterate over the view buffer and will return the value in buf/len
 * optionally.
 * @param state Save state for scanning. Save is initialised when view is not NULL
 * @param cstruct instance of the view object
 * @param view view name if not NULL, then start scan
 * @param cname output cname field, must be NDRX_VIEW_CNAME_LEN + 1 in size.
 * @param fldtype return the type of the field
 * @param occ occurrence return
 * @param is_null return TRUE if value is NULL (tested only if BNEXT_NOTNULL not set)
 * @param buf data copied to buf (optional if not NULL)
 * @param len buffer len (if set on input validate the input len) on output bytes copied
 * @param usrtype in case if not -1, the data will be converted to destination type
 * otherwise raw data is copied out...
 * including NULL, BVACCESS_NOTNULL - return only non NULL values, if array have NULL
 * in the middle it will be skipped and next value will be returned. Also this follows
 * the Count flags, thus if count is set less than max count, then less elements will
 * be tested and returned.
 * 
 * In case if BVACCESS_NOTNULL is not set, the array length indicators still will be
 * used
 * 
 * @return On success 1 is returned, on EOF 0, on failure -1
 */
expublic int ndrx_Bvnext (Bvnext_state_t *state, char *cstruct, char *view, char *cname,
		int *fldtype, BFLDOCC *occ, int *is_null,
		char *buf, BFLDLEN *len, long flags, int usrtype)
{
    int ret = EXSUCCEED;
    int gotone = EXFALSE;
    int skip_null;
    BFLDOCC occs, maxoccs, realoccs;
    char *cnv_buf;
    char *fld_offs;
    unsigned short *L_length;
    unsigned short L_length_stor;

    ndrx_typedview_t *v;
    ndrx_typedview_field_t *f;
    ndrx_typedview_field_t *ft;
    
    v = (ndrx_typedview_t *) state->v;
    f = (ndrx_typedview_field_t *) state->vel;
    ft = (ndrx_typedview_field_t *) state->velt;

    do 
    {
        skip_null = EXFALSE;
        /* find first */
        if (NULL!=view)
        {
            UBF_LOG(log_debug, "Starting to scan view: %s", view);
            memset(state, 0, sizeof(Bvnext_state_t));

            /* Resolve the view */
            if (NULL==(v = ndrx_view_get_view(view)))
            {
                ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
                EXFAIL_OUT(ret);
            }
            
            DL_FOREACH_SAFE(v->fields, f, ft)
            {
                gotone = EXTRUE;
                break;
            }
            
            if (!gotone)
            {
                UBF_LOG(log_debug, "View scan EOF");
                ret = 0;
                goto out;
            }
            
        }
        else if (state->non_null_occs > state->occ)
        {
            UBF_LOG(log_debug, "%s: total non null count: %d "
                    "current occ: %d - process array elem",
                    __func__, state->non_null_occs, state->occ);
        }
        else
        {
            state->occ = 0; /* reset occurrences */
            
            /* find next */
            DL_FOREACH_SAFE_CONT(v->fields, f, ft)
            {
                gotone = EXTRUE;
                break;
            }
            
            if (!gotone)
            {
                UBF_LOG(log_debug, "View scan EOF");
                ret = 0;
                goto out;
            }
            
            if (flags & BVACCESS_NOTNULL)
            {
                occs = ndrx_Bvoccur_int(cstruct, v, f, &maxoccs, &realoccs);
                
                if (EXFAIL==occs)
                {
                    UBF_LOG(log_error, "%s: Failed to get occurrences!", __func__);
                    EXFAIL_OUT(ret);
                }
                
                state->non_null_occs = realoccs;
            }
            else
            {
                state->non_null_occs = occs;
            }
            
            UBF_LOG(log_debug, "%s: Next: got: %d %s, non_null_occs=%d", __func__, 
                    gotone, f->cname, state->non_null_occs);
            
            if (0==state->non_null_occs)
            {
                UBF_LOG(log_debug, "Field is NULL or all occs are NULL, skip to next");
                gotone = EXFALSE;
                skip_null = EXTRUE;
            }
            else
            {
                state->dim_size = f->fldsize/f->count;
            }
                
        }
    
        if (gotone)
        {
            *is_null = ndrx_Bvnull_int(v, f, state->occ, cstruct);
            *fldtype = f->typecode_full;
            
            strncpy(cname, f->cname, NDRX_VIEW_CNAME_LEN);
            cname[NDRX_VIEW_CNAME_LEN] = EXEOS;
            *occ = state->occ;
            
            UBF_LOG(log_debug, "Element found: %s.%s index: %d type: %d is_null: %d", 
                    v->vname, cname, *occ, *fldtype, *is_null);
            
            if (NULL!=buf)
            {
                
                fld_offs = cstruct+f->offset+state->occ*state->dim_size;
                
                /* Will request type convert now */
                NDRX_VIEW_LEN_SETUP(state->occ, state->dim_size);
    
    
                UBF_LOG(log_debug, "User buffer is not null (%p) loading data, usrtype: %d",
                        buf, usrtype);
                
                if (EXFAIL==usrtype)
                {
                    UBF_LOG(log_debug, "User type set to -1, assuming user buffer "
                            "same as in struct - direct return");
                    
                    cnv_buf = ndrx_ubf_convert(*fldtype, CNV_DIR_OUT, fld_offs, *L_length,
                                *fldtype, buf, len);
                    if (NULL==cnv_buf)
                    {
                        UBF_LOG(log_error, "%s: failed to convert data!", __func__);
                        /* Error should be provided by conversation function */
                        EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    UBF_LOG(log_debug, "Converting from %d to %d", 
                            *fldtype, usrtype);
                    
                    cnv_buf = ndrx_ubf_convert(*fldtype, CNV_DIR_OUT, fld_offs, *L_length,
                                usrtype, buf, len);
                    if (NULL==cnv_buf)
                    {
                        UBF_LOG(log_error, "%s: failed to convert data!", __func__);
                        /* Error should be provided by conversation function */
                        EXFAIL_OUT(ret);
                    }
                }
            }
        }
        
        /* if all ok, increment occ. */
        state->occ++;
    
    } while (skip_null);
    
out:
    /* fill up the state: */

    state->v = v;
    state->vel = f;
    state->velt = ft;
    
    
    return ret;
}

