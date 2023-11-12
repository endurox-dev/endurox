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
#include <dirent.h>

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
exprivate int ndrx_tms_file_storage_init(ndrx_tms_storage_t *sw, tmsrv_cfg_t *p_tmsrv_cfg);
exprivate int ndrx_tms_file_storage_uninit(ndrx_tms_storage_t *sw);
exprivate int ndrx_tms_file_storage_open(ndrx_tms_storage_t  *sw, 
    atmi_xa_log_t* p_tl, char *mode);
exprivate int ndrx_tms_file_storage_close(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl);
exprivate int ndrx_tms_file_storage_unlink(ndrx_tms_storage_t *sw, char *fname);
exprivate int ndrx_tms_file_storage_write(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl, 
        char cmdid, char *buf, size_t len, int sync);
exprivate int ndrx_tms_file_storage_read_start(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl);
exprivate int ndrx_tms_file_storage_read_next(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl, 
    char *buf, size_t bufsz);
exprivate int ndrx_tms_file_storage_read_end(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl);
exprivate int ndrx_tms_file_storage_list_start(ndrx_tms_storage_t *sw);
exprivate int ndrx_tms_file_storage_list_next(ndrx_tms_storage_t *sw, char *buf, size_t bufsz);
exprivate int ndrx_tms_file_storage_list_end(ndrx_tms_storage_t *sw);
exprivate int ndrx_tms_file_storage_exists(ndrx_tms_storage_t *sw, char *fname);
exprivate long ndrx_tms_file_storage_age(ndrx_tms_storage_t *sw, char *fname);

/** File store switch */
expublic ndrx_tms_storage_t ndrx_G_tms_store_files =
{
    .magic = STOREIF_MAGIC,
    .name = "file system",
    .sw_version = TMREGISTER,
    .pf_storage_init = ndrx_tms_file_storage_init,
    .pf_storage_uninit = ndrx_tms_file_storage_uninit,
    .pf_storage_open = ndrx_tms_file_storage_open,
    .pf_storage_close = ndrx_tms_file_storage_close,
    .pf_storage_unlink = ndrx_tms_file_storage_unlink,
    .pf_storage_write = ndrx_tms_file_storage_write,
    .pf_storage_read_start = ndrx_tms_file_storage_read_start,
    .pf_storage_read_next = ndrx_tms_file_storage_read_next,
    .pf_storage_read_end = ndrx_tms_file_storage_read_end,
    .pf_storage_list_start = ndrx_tms_file_storage_list_start,
    .pf_storage_list_next=ndrx_tms_file_storage_list_next,
    .pf_storage_list_end=ndrx_tms_file_storage_list_end,
    .pf_storage_exists=ndrx_tms_file_storage_exists,
    .pf_storage_get_age=ndrx_tms_file_storage_age
};

/** 
 * init interface 
 * @param sw storage interface
 * @param p_tmsrv_cfg Configuration used by TM.
 * @return 0 on success, -1 on error (Nerror is set)
 */
exprivate int ndrx_tms_file_storage_init(ndrx_tms_storage_t *sw, tmsrv_cfg_t *p_tmsrv_cfg)
{
    return EXSUCCEED;
}

/** 
 * un-init the interface
 * @param sw switch
 * @return 0 on success, -1 on error (Nerror is set)
 */
exprivate int ndrx_tms_file_storage_uninit(ndrx_tms_storage_t *sw)
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
exprivate int ndrx_tms_file_storage_open(ndrx_tms_storage_t  *sw, 
    atmi_xa_log_t* p_tl, char *mode)
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
exprivate int ndrx_tms_file_storage_close(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl)
{
    int ret = EXSUCCEED;
    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    if (NULL!=p_tl->f)
    {
        NDRX_FCLOSE(p_tl->f);
        p_tl->f = NULL;
    }

out:
    return ret;
}

/**
 * Remove transaction log
 * @param sw switch
 * @param fname transaction logfile name
 * @return 0 on success, -1 on error (Nerror is set)
 */
exprivate int ndrx_tms_file_storage_unlink(ndrx_tms_storage_t *sw, char *fname)
{
    int ret = EXSUCCEED;
    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    if (EXSUCCEED!=unlink(fname))
    {
        _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "unlink() errno=%d: %s", errno, strerror(errno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * write async to file. No guarantees are made
 * that records will persist.
 * @param sw switch
 * @param p_tl transaction log (struct)
 * @param cmdid command id
 * @param buf status buffer to write
 * @param len number of bytes to write
 * @param sync if 1, then sync to disk
 * @return nr bytes wrote, -1 on error (Nerror is set)
 */
exprivate int ndrx_tms_file_storage_write(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl, 
        char cmdid, char *buf, size_t len, int sync)
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

    /* Do the sync if required */
    if (sync && (EXSUCCEED!=ndrx_fsync_fsync(p_tl->f, G_atmi_env.xa_fsync_flags) ||
        EXSUCCEED!=ndrx_fsync_dsync(G_tmsrv_cfg.tlog_dir, G_atmi_env.xa_fsync_flags)))
    {
         _Nset_error_fmt(NESYNC, "fsync() fail");
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
exprivate int ndrx_tms_file_storage_read_start(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl)
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
 * @return 0 = string read ok, -1 on error (Nerror is set
 */
exprivate int ndrx_tms_file_storage_read_next(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl, 
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
                err, strerror(err));
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
exprivate int ndrx_tms_file_storage_read_end(ndrx_tms_storage_t *sw, atmi_xa_log_t* p_tl)
{
    _Nunset_error();
    return EXSUCCEED;
}

/** 
 * list transactions in the storage, start
 * @param sw switch
 * @return 0 on success, -1 on error (Nerror is set)
 */
exprivate int ndrx_tms_file_storage_list_start(ndrx_tms_storage_t *sw)
{
    int ret=EXSUCCEED;
    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    sw->custom_block1=(void *)opendir(G_tmsrv_cfg.tlog_dir);

    if (sw->custom_block1 == NULL)
    {
        int err=errno;
        _Nset_error_fmt(ndrx_Nerrno2nerror(err), "opendir() fail errno=%d: %s", 
                errno, strerror(err));
        EXFAIL_OUT(ret);
    }

out:

    NDRX_LOG(log_info, "returns, opendir [%s] %p: %d", 
            G_tmsrv_cfg.tlog_dir, sw->custom_block1, ret);

    return ret;
}

/** 
 * list transactions in the storage, next
 * This returns file names of the transactions
 * @param sw switch
 * @param buf buffer to write the transaction name
 * @param bufsz size of the buffer
 * @return 1 on success (loaded dir), 0 (success, EOF), -1 on error (Nerror is set)
 */
exprivate int ndrx_tms_file_storage_list_next(ndrx_tms_storage_t *sw, char *buf, size_t bufsz)
{
    int ret = EXSUCCEED;
    struct dirent *entry;
    char tranmask[256];
    int tranmask_len;

    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    snprintf(tranmask, sizeof(tranmask), "TRN-%ld-%hd-%d-", G_tmsrv_cfg.vnodeid,
            G_atmi_env.xa_rmid, G_server_conf.srv_id);

    tranmask_len = strlen(tranmask);

    errno=0;

    do
    {    
        if (NULL==(entry=readdir((DIR *)sw->custom_block1)))
        {
            if (0!=errno)
            {
                int err=errno;
                _Nset_error_fmt(ndrx_Nerrno2nerror(err), "readdir() fail errno=%d: %s", 
                        errno, strerror(err));
                EXFAIL_OUT(ret);
            }
            break;
        }
        else if (0==strncmp(entry->d_name, tranmask, tranmask_len))
        {
            ret=EXTRUE;
            NDRX_STRCPY_SAFE_DST(buf, entry->d_name, bufsz);
            break;
        }
        /* else we skip the ., .. and other files (other tms...) */

    } while (1);

out:
    return ret;
}

/** 
 * list transactions in the storage, end
 * @param sw switch
 * @return 0 on success, -1 on error (Nerror is set)
 */
exprivate int ndrx_tms_file_storage_list_end(ndrx_tms_storage_t *sw)
{
    int ret = EXSUCCEED;
    _Nunset_error();

    if (NULL!=sw->custom_block1)
    {
        if (0!=closedir((DIR *)sw->custom_block1))
        {
            int err=errno;
            NDRX_LOG(log_error, "failed to closedir(): %s", strerror(err));
        }
        sw->custom_block1 = NULL;
    }
out:
    return ret;
}

/**
 * Check if storage file exists
 * @param sw switch
 * @param fname file name
 * @param p_tl return basic parameters of the transaction (such as creation time and update time).
 *  optional, may be NULL.
 * @return 1 - exists, 0 on false (ok, but does not exist), -1 on error (Nerror is set)
 */
exprivate int ndrx_tms_file_storage_exists(ndrx_tms_storage_t *sw, char *fname)
{
    int ret = EXSUCCEED;

    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    ret = access(fname, 0);

    if (EXSUCCEED!=ret && ENOENT==errno)
    {
        ret=EXFALSE;
        goto out;
    }

    if (EXSUCCEED!=ret)
    {
        _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "access() fail errno=%d: %s", 
                errno, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    ret=EXTRUE;

out:
    return ret;
}

/**
 * How old record (file) is?
 * Used by record housekeeping
 * @param sw switch
 * @param fname file name
 * @return age in seconds, -1 on error (Nerror is set)
 */
exprivate long ndrx_tms_file_storage_age(ndrx_tms_storage_t *sw, char *fname)
{
    long ret = EXSUCCEED;
    long age = -1;

    _Nunset_error();

    NDRX_TEST_LOCKLOSS;

    ret = ndrx_file_age(fname);

    if (ret<0)
    {
        _Nset_error_fmt(ndrx_Nerrno2nerror(errno), "stat() fail errno=%d: %s", 
                errno, strerror(errno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
