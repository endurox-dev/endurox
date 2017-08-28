/* 
** TPLog routines at ATMI level
**
** @file atmi_tplog.c
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
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <xa_cmn.h>
#include <Exfields.h>
#include <ubfutil.h>

#include "tperror.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Compare with current logger & setup it up if needed
 * @param new_file
 * @return 
 */
exprivate int tplog_compare_set_file(char *new_file)
{
    int changed = EXFALSE;
    int have_reqfile;
    char cur_filename[PATH_MAX];
    /* get the current file (if any we have) */
    have_reqfile = tploggetreqfile(cur_filename, sizeof(cur_filename));
    
    if (have_reqfile && 0==strcmp(new_file, cur_filename))
    {
        NDRX_LOG(log_warn, "Already logging to [%s] - not changing...", cur_filename);
        changed=EXFALSE;
    }
    else
    {
        /* just set it up - new file */
        tplogsetreqfile_direct(new_file);
        changed=EXTRUE;
    }
    
    return changed;
}

/**
 * Print UBF buffer to logger
 * @param lev logging level to start print at
 * @param title title of the dump
 * @param p_ub UBF buffer
 */
expublic void ndrx_tplogprintubf(int lev, char *title, UBFH *p_ub)
{
    ndrx_debug_t * dbg = debug_get_tp_ptr();
    if (dbg->level>=lev)
    {
        TP_LOG(lev, "%s", title);
        Bfprint(p_ub, dbg->dbg_f_ptr);
    }
}

/**
 * Set the request file.
 * @param data optional, will search for filename here (XATMI buffer, UBF type
 * @param filename if file name not found in data, then use this one.
 *          and if type allows, then install this file name into buffer.
 * @param filesvc Service name to call for requesting the filename
 * @return SUCCEED/FAIL
 */
expublic int ndrx_tplogsetreqfile(char **data, char *filename, char *filesvc)
{
    int ret = EXSUCCEED;
    char btype[16] = {EXEOS};
    char stype[16] = {EXEOS};
    
    char ubf_filename[PATH_MAX] = {EXEOS};
    
    int buf_len;
    UBFH **p_ub = NULL;
    
    /* scenario 1 - have buffer:
     * Check for field existence, if exists, then get value
     * - compare with filename (maybe need switch) and do update UBF
     * - compare with current filename, if different then do switch the file
     */
    if (NULL!=*data)
    {
        if (EXFAIL==ndrx_tptypes(*data, btype, stype))
        {
            EXFAIL_OUT(ret);
        }
        
        /* buffer is ok */
        if (0==strcmp(btype, "UBF") || 0==strcmp(btype, "FML") || 
                0==strcmp(btype, "FML32"))
        {
            p_ub = (UBFH **)data;
            buf_len = sizeof(ubf_filename);
            
            if (Bpres(*p_ub, EX_NREQLOGFILE, 0))
            {
                if (EXSUCCEED!=Bget(*p_ub, EX_NREQLOGFILE, 0, ubf_filename, &buf_len))
                {
                    NDRX_LOG(log_error, "Failed to get EX_NREQLOGFILE: %s", 
                            Bstrerror(Berror));
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get EX_NREQLOGFILE: %s", 
                            Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }
                
                /* Field exists, compare with current */
                
                if (NULL!=filename && EXEOS!=filename[0])
                {
                    /* set new file */
                    tplog_compare_set_file(filename);
                    
                    if (0!=strcmp(ubf_filename, filename))
                    {
                        /* update UBF */
                        if (EXSUCCEED!=Bchg(*p_ub, EX_NREQLOGFILE, 0, filename, 0L))
                        {
                            NDRX_LOG(log_error, "Failed to set EX_NREQLOGFILE: %s", 
                                    Bstrerror(Berror));
                            
                            ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set EX_NREQLOGFILE: %s", 
                                Bstrerror(Berror));
                            
                            EXFAIL_OUT(ret);
                        }
                    }
                }/* if have file name given in... */
                else if (EXEOS!=ubf_filename[0])
                {
                    /* Have file name in buffer */
                    tplog_compare_set_file(ubf_filename);
                }
                else
                {
                    NDRX_LOG(log_warn, "Cannot set request log file: "
                            "no name in buffer, no name in 'filename'!");
                    ndrx_TPset_error_msg(TPEINVAL, "Cannot set request log file: "
                            "no name in buffer, no name in 'filename'!");
                    EXFAIL_OUT(ret);
                }
            }
            else if (NULL!=filename && EXEOS!=filename[0])
            {
                /* field does not exists, thus maybe need to install it */
                tplog_compare_set_file(filename);
                
                /* set stuff in buffer */
                if (EXSUCCEED!=Bchg(*p_ub, EX_NREQLOGFILE, 0, filename, 0L))
                {
                    NDRX_LOG(log_error, "Failed to set EX_NREQLOGFILE: %s", 
                            Bstrerror(Berror));

                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set EX_NREQLOGFILE: %s", 
                        Bstrerror(Berror));

                    EXFAIL_OUT(ret);
                }
            }
            else if (NULL!=filesvc && EXEOS!=filesvc[0])
            {
                long rsplen;
                NDRX_LOG(log_debug, "About to call [%s] for new request "
                        "file log name", filesvc);
                
                if (EXFAIL == tpcall(filesvc, (char *)*data, 0L, (char **)data, &rsplen,TPNOTRAN))
                {
                    NDRX_LOG(log_error, "%s failed: %s", filesvc, tpstrerror(tperrno));
                    /* tperror already set */
                    EXFAIL_OUT(ret);
                }
                else
                {
                    /* call self again... 
                     * Only now with out request service
                     */
                    if (EXSUCCEED!=ndrx_tplogsetreqfile(data, filename, NULL))
                    {
                        EXFAIL_OUT(ret);
                    }
                }
            }
            else
            {
                NDRX_LOG(log_warn, "Cannot set request log file: "
                            "empty name in buffer, no name in 'filename'!");
                ndrx_TPset_error_msg(TPEINVAL, "Cannot set request log file: "
                        "empty name in buffer, no name in 'filename'!");
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_debug, "Buffer no UBF - cannot test request file");
            tplog_compare_set_file(filename);
        }
    }
    /*
     * Scenario 2: just file name
     */
    else if (NULL!=filename && EXEOS!=filename[0])
    {
        /* only have file name */
        tplog_compare_set_file(filename);
    }
    else
    {
        NDRX_LOG(log_warn, "Cannot set request log file: "
                            "no buffer and no name in 'filename'!");
        ndrx_TPset_error_msg(TPEINVAL, "Cannot set request log file: "
                "no buffer and no name in 'filename'!");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Get the request file from buffer (if buffer is ok)
 * The error will be returned if there is not file or buffer invalid.
 * @param data
 * @param filename
 * @return 
 */
expublic int ndrx_tploggetbufreqfile(char *data, char *filename, int bufsize)
{
    int ret = EXSUCCEED;
    char btype[16] = {EXEOS};
    char stype[16] = {EXEOS};
    UBFH *p_ub;
    int buf_len;
    
    if (NULL!=data)
    {
        if (EXFAIL==ndrx_tptypes(data, btype, stype))
        {
            EXFAIL_OUT(ret);
        }
        
        /* buffer is ok */
        if (0==strcmp(btype, "UBF") || 0==strcmp(btype, "FML") || 
                0==strcmp(btype, "FML32"))
        {
            p_ub = (UBFH *)data;
            buf_len = bufsize;
            
            if (Bpres(p_ub, EX_NREQLOGFILE, 0))
            {
                if (EXSUCCEED!=Bget(p_ub, EX_NREQLOGFILE, 0, filename, &buf_len))
                {
                    NDRX_LOG(log_error, "Failed to get EX_NREQLOGFILE: %s", 
                            Bstrerror(Berror));
                    ndrx_TPset_error_fmt(TPENOENT, "Failed to get EX_NREQLOGFILE: %s", 
                            Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                } 
            }
            else
            {
                ndrx_TPset_error_fmt(TPENOENT, "No file exists: %s", 
                            Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            ndrx_TPset_error_fmt(TPEINVAL, "Not UBF buffer: %s", 
                            Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Null buffer: %s", 
                            Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    
out:
    return ret;
}

/**
 * Remove request file from buffer
 * @param data
 * @param filename
 * @return 
 */
expublic int ndrx_tplogdelbufreqfile(char *data)
{
    int ret = EXSUCCEED;
    char btype[16] = {EXEOS};
    char stype[16] = {EXEOS};
    UBFH *p_ub;
    
    if (NULL!=data)
    {
        if (EXFAIL==ndrx_tptypes(data, btype, stype))
        {
            EXFAIL_OUT(ret);
        }
        
        /* buffer is ok */
        if (0==strcmp(btype, "UBF") || 0==strcmp(btype, "FML") || 
                0==strcmp(btype, "FML32"))
        {
            p_ub = (UBFH *)data;
            
            if (Bpres(p_ub, EX_NREQLOGFILE, 0))
            {
                if (EXSUCCEED!=Bdel(p_ub, EX_NREQLOGFILE, 0))
                {
                    NDRX_LOG(log_error, "Failed to get EX_NREQLOGFILE: %s", 
                            Bstrerror(Berror));
                    ndrx_TPset_error_fmt(TPENOENT, "Failed to get EX_NREQLOGFILE: %s", 
                            Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                } 
            }
            else
            {
                ndrx_TPset_error_fmt(TPENOENT, "No file exists: %s", 
                            Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            ndrx_TPset_error_fmt(TPEINVAL, "Not UBF buffer: %s", 
                            Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Null buffer: %s", 
                            Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}
