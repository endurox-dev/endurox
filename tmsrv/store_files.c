/**
 * @brief Storage interace - file-system
 *
 * @file store_files.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmsrv.h"
#include "../libatmisrv/srv_int.h"
#include <xa_cmn.h>
#include <atmi_int.h>
#include <sys_test.h>
#include <userlog.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Simualte error, if we have to
 */
#define NDRX_TEST_LOCKLOSS do {\
        if (ndrx_G_systest_lockloss)\
        {\
            _Nset_error_fmt(NESTALE, "Stale file handle");\
            EXFAIL_OUT(ret);\
        }\
    } while (0)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/** 
 * init interface 
 * @param sw storage interface
 * @param p_tmsrv_cfg Configuration used by TM.
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_init(ndrx_tms_storage_t *sw, tmsrv_cfg_t *p_tmsrv_cfg)
{
    return EXSUCCEED;
}

/** 
 * un-init the interface
 * @param sw switch
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_uninit(ndrx_tms_storage_t *sw)
{
    return EXSUCCEED;
}

/**
 * open transaction file
 * @param sw switch
 * @param p_tl transaction log (struct)
 * @param fname transaction file name. For no DB, will contain <connref>/TRN-<vnodeid>-<rmid>-<srvid>-<txid>
 * @param mode open mode, "a" for new transaction, "a+" for recovery
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_open(ndrx_tms_storage_t  *sw, 
    atmi_xa_log_t* p_tl, char *fname, char *mode)
{
    int ret = EXSUCCEED;

    /* reset error handle 
     * this allows to overwrite last error
     */
    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    if (NULL==(p_tl->f=NDRX_FOPEN(p_tl->fname, mode)))
    {
        _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "fopen() errno=%d: %s", errno, strerror(errno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * close storage file
 * @param sw switch
 * @param p_tl transaction log (struct)
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_close(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl)
{
    int ret = EXSUCCEED;
    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    if (NULL!=p_tl->f)
    {
        if (0!=NDRX_FCLOSE(p_tl->f))
        {
            _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "fclose() errno=%d: %s", errno, strerror(errno));
            EXFAIL_OUT(ret);
        }
        p_tl->f = NULL;
    }

out:
    return ret;
}

/**
 * Remove transaction log
 * @param sw switch
 * @param p_tl transaction log (struct)
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_unlink(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl)
{
    int ret = EXSUCCEED;
    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    if (EXSUCCEED!=unlink(p_tl->fname))
    {
        _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "unlink() errno=%d: %s", errno, strerror(errno));
        EXFAIL_OUT(ret);
    }
    p_tl->f = NULL;

out:
    return ret;
}

/**
 * write async to file. No guarantees are made
 * that records will persist.
 * @param sw switch
 * @param p_tl transaction log (struct)
 * @param cmdid command id
 * @param tstamp timestamp
 * @param buf status buffer to write
 * @param len number of bytes to write
 * @param sync if 1, then sync to disk
 * @return nr bytes wrote, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_write(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl, 
        char cmdid, long long tstamp, char *buf, size_t len, int sync)
{
    int ret=EXSUCCEED;

    _Nunset_error();

    NDRX_TEST_LOCKLOSS;
    
    ret = fwrite(buf, 1, len, p_tl->f);

    if (ret!=len)
    {
        _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "fwrite() fail errno=%d: %s", errno, strerror(errno));
        EXFAIL_OUT(ret);
    }

    /* flush the stuff... */
    if (EXSUCCEED!=fflush(p_tl->f))
    {
        int err=errno;
        userlog("ERROR! Failed to fflush(): %s", strerror(err));
        _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "fflush() fail errno=%d: %s", errno, strerror(errno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/** 
 * Start reading of the transaction log for the given file name.
 * @param sw switch
 * @param p_tl transaction log (struct). The fname must be filled in the log struct
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_read_start(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl)
{
    int ret = EXSUCCEED;

    _Nunset_error();

    /* On mac and freebsd seem that reading happens at end of the
     * file if file was opened with a+
     */
    if (EXSUCCEED!=fseek(p_tl->f, 0, SEEK_SET))
    {
        _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "fseek() fail errno=%d: %s", 
            errno, strerror(errno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * Read next record from the transaction log
 * @param sw switch
 * @param p_tl transaction log (struct)
 * @return 0 >= succeed (number of bytes read), -1 on error (Nerror is set
 */
expublic int ndrx_tms_file_storage_read_next(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl, 
    char *buf, size_t bufsz)
{
    int ret = EXSUCCEED;
    char *ret_read = fgets(buf, bufsz, p_tl->f);

    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    if (NULL==ret_read)
    {
        if (feof(p_tl->f))
        {
            _Nset_error_fmt(NEEOF, "EOF reached of %p", p_tl->f);
        }
        else
        {
            int err = errno;
            _Nset_error_fmt(ndrx_Nerrno2nerror(err), "fgets() fail errno=%d: %s", 
                errno, strerror(err));
        }
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * End reading of the transaction log
 * @param sw switch
 * @param p_tl transaction log (struct)
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_read_end(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl)
{
    _Nunset_error();
    return EXSUCCEED;
}

/** 
 * list transactions in the storage, start
 * @param sw switch
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_list_start(ndrx_tms_storage_t *sw)
{
    
    dir = opendir(M_folder_prepared);

    if (dir == NULL) {

        NDRX_LOG(log_error, "opendir [%s] failed: %s", M_folder_prepared, strerror(errno));
        EXFAIL_OUT(ret);
    }

}

/** 
 * list transactions in the storage, next
 * This returns file names of the transactions
 * @param sw switch
 * @param buf buffer to write the transaction name
 * @param bufsz size of the buffer
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_list_next(ndrx_tms_storage_t *sw, char *buf, size_t bufsz)
{
    return EXFAIL;
}

/** 
 * list transactions in the storage, end
 * @param sw switch
 * @return 0 on success, -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_list_end(ndrx_tms_storage_t *sw)
{
    return EXFAIL;
}

/**
 * Check if storage file exists
 * @param sw switch
 * @param fname file name
 * @param p_tl return basic parameters of the transaction (such as creation time and update time).
 *  optional, may be NULL.
 * @return 1 - exists, 0 on false (ok, but does not exist), -1 on error (Nerror is set)
 */
expublic int ndrx_tms_file_storage_exists(ndrx_tms_storage_t *sw, char *fname,  atmi_xa_log_t* p_tl)
{
    return EXFAIL;
}

/* vim: set ts=4 sw=4 et smartindent: */
