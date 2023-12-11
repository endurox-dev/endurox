/**
 * @brief Storage interface - file system queue storage engine
 *
 * @file store_files_tmq.c
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
#include <utlist.h>
#include <dirent.h>
#include <assert.h>
#include <sys/stat.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <userlog.h>

#include "qdisk_xa.h"
#include <xa.h>
#include <qcommon.h>
#include <sys_test.h>
#include <atmi_tls.h>

/*---------------------------Externs------------------------------------*/
extern int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Used to hold the cursor of the
 * directory listing. Two modes supported.
 */
typedef struct
{
    /* scandir related: */
    int i, cnt, mode;
    struct dirent **namelist;
    char *prev; /** used for duplicate scan */

    /* dir() related */
    DIR *dirp;

} ndrx_tmq_dir_list_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate char M_folder_active[PATH_MAX+1] = {EXEOS}; /**< Active transactions        */
exprivate char M_folder_prepared[PATH_MAX+1] = {EXEOS}; /**< Prepared transactions    */
exprivate char M_folder_committed[PATH_MAX+1] = {EXEOS}; /**< Committed transactions  */

/** thread local shortucts for path operations */
exprivate __thread char M_filename[2][PATH_MAX+1];
/*---------------------------Prototypes---------------------------------*/

exprivate int ndrx_tmq_file_storage_init(ndrx_tmq_storage_t *sw, 
    ndrx_tmq_qdisk_xa_cfg_t *p_tmq_cfg);
exprivate int ndrx_tmq_file_storage_uninit(ndrx_tmq_storage_t *sw);
exprivate int ndrx_tmq_file_storage_list_start(ndrx_tmq_storage_t *sw, void **ret_cursor, int mode);
exprivate int ndrx_tmq_file_storage_list_next(ndrx_tmq_storage_t *sw, 
    void *cursor, char *ref, size_t refsz);
exprivate int ndrx_tmq_file_storage_list_end(ndrx_tmq_storage_t *sw, void *cursor);
exprivate int ndrx_tmq_file_storage_prep_exists(ndrx_tmq_storage_t *sw, char *tmxid);
exprivate int ndrx_tmq_file_storage_comm_exists(ndrx_tmq_storage_t *sw, char *msgid_str);
exprivate int ndrx_tmq_file_storage_read_block(ndrx_tmq_storage_t *sw, 
    short nodeid, short srvid, char *ref,  int seqno,
    union tmq_block **p_block, int mode);
exprivate int ndrx_tmq_file_storage_rollback_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl);
exprivate int ndrx_tmq_file_storage_commit_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl);
exprivate int ndrx_tmq_file_storage_prepare_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl);
exprivate int ndrx_tmq_file_storage_write_block(ndrx_tmq_storage_t *sw, char *block, int len,
    char *cust_tmxid, int *int_diag);

exprivate char *get_filename_i(int i, char *folder, int slot);
exprivate char *mode_to_filename(ndrx_tmq_storage_t *sw, char *ref, int seqno, int mode);
exprivate void dirent_free(struct dirent **namelist, int n);
exprivate int prepare_folders(ndrx_tmq_qdisk_xa_cfg_t *p_tmq_cfg);
exprivate int write_to_tx_file(char *block, int len, char *cust_tmxid, int *int_diag);
exprivate int read_tx_block(FILE *f, char *block, int len, char *fname, 
        char *dbg_msg, int *err, 
        int *tmq_err);
exprivate int tmq_finalize_files_hdr(ndrx_tmq_storage_t *sw, tmq_cmdheader_t *p_hdr, char *fname1, 
        char *fname2, char fcmd, qtran_log_cmd_t *tcmd);
exprivate char * file_move_final_names(char *from_filename, char *to_filename_only);
exprivate char *get_file_name_final(char *fname);
exprivate int tmq_finalize_files_upd(ndrx_tmq_storage_t *sw, tmq_msg_upd_t *p_upd, char *fname1, 
        char *fname2, char fcmd, qtran_log_cmd_t *tcmd);
exprivate int file_move(int i, char *from_folder, char *to_folder);

/** File store switch */
expublic ndrx_tmq_storage_t ndrx_G_tmq_store_files =
{
    .magic=NDRX_TMQ_STOREIF_MAGIC
    ,.name="File-system"
    ,.sw_version=NDRX_TMQ_STOREIF_VERSION

    ,.custom_block1=NULL
    ,.custom_block2=NULL
    ,.custom_block3=NULL
    ,.custom_block4=NULL
    ,.cfg = NULL

    ,.pf_storage_init=ndrx_tmq_file_storage_init
    ,.pf_storage_uninit=ndrx_tmq_file_storage_uninit
    ,.pf_storage_list_start=ndrx_tmq_file_storage_list_start
    ,.pf_storage_list_next=ndrx_tmq_file_storage_list_next
    ,.pf_storage_list_end=ndrx_tmq_file_storage_list_end
    ,.pf_storage_prep_exists=ndrx_tmq_file_storage_prep_exists
    ,.pf_storage_comm_exists=ndrx_tmq_file_storage_comm_exists
    ,.pf_storage_read_block=ndrx_tmq_file_storage_read_block
    ,.pf_storage_rollback_cmds=ndrx_tmq_file_storage_rollback_cmds
    ,.pf_storage_commit_cmds=ndrx_tmq_file_storage_commit_cmds
    ,.pf_storage_prepare_cmds=ndrx_tmq_file_storage_prepare_cmds
    ,.pf_storage_write_block=ndrx_tmq_file_storage_write_block

};

/**
 * Initialize the storage interface
 * For files storage, create necessary directories.
 * @param sw storage interface
 * @param p_tmq_cfg configuration 
 */
exprivate int ndrx_tmq_file_storage_init(ndrx_tmq_storage_t *sw, 
    ndrx_tmq_qdisk_xa_cfg_t *p_tmq_cfg)
{
    int ret=XA_OK;

    if (EXSUCCEED!=prepare_folders(p_tmq_cfg))
    {
        ret=XAER_RMERR;
        goto out;
    }

    sw->cfg = p_tmq_cfg;
    
out:
    return ret;
}

/**
 * Uninitialize the storage interface
 * @param sw storage interface
 * @return EXSUCCEED
*/
exprivate int ndrx_tmq_file_storage_uninit(ndrx_tmq_storage_t *sw)
{
    return EXSUCCEED;
}

/**
 * Start to scan the directory. Two modes will be used.
 * For message loading, readdir() will be used.
 * For active/prepared directories, scandir() will be used, as transaction ordering
 * is required, to avoid duplicates.
 * @param sw storage interface
 * @param mode mode of the list (see NDRX_TMQ_STORAGE_LIST_MODE_* constants)
 *  WARNING: !NDRX_TMQ_STORAGE_LIST_MODE_INCL_DUPS && NDRX_TMQ_STORAGE_LIST_MODE_NO_SORT is not allowed
 * @return XA error code
 */
exprivate int ndrx_tmq_file_storage_list_start(ndrx_tmq_storage_t *sw, void **ret_cursor, int mode)
{
    int ret = XA_OK;
    int err;
    char *dir_to_scan=NULL;
    ndrx_tmq_dir_list_t *cursor=NULL;
    cursor = (ndrx_tmq_dir_list_t *)NDRX_FPMALLOC(sizeof(ndrx_tmq_dir_list_t), 0);

    if (NULL==cursor)
    {
        NDRX_LOG(log_error, "Failed to allocate memory for cursor (%d bytes): %s", 
            sizeof(ndrx_tmq_dir_list_t), strerror(errno));
        ret = XAER_RMERR;
        goto out;
    }

    /* will be needed for looping over */
    cursor->mode = mode;
    cursor->prev = NULL;

    if (NDRX_TMQ_STORAGE_LIST_MODE_ACTIVE & mode)
    {
        dir_to_scan = M_folder_active;
    }
    else if (NDRX_TMQ_STORAGE_LIST_MODE_PREPARED & mode)
    {
        dir_to_scan = M_folder_prepared;
    }
    else if (NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED & mode)
    {
        dir_to_scan = M_folder_committed;
    }
    else
    {
        NDRX_LOG(log_error, "Invalid mode %d", mode);
        ret = XAER_RMERR;
        goto out;
    }

    /* we cannot have unsorted listing without dups */
    assert (( !(NDRX_TMQ_STORAGE_LIST_MODE_INCL_DUPS & mode) && 
        (NDRX_TMQ_STORAGE_LIST_MODE_NO_SORT & mode) ) == EXFALSE);

    if ( (NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED & mode) ||
        (NDRX_TMQ_STORAGE_LIST_MODE_NO_SORT & mode) )
    {
        /* use readdir() */
        cursor->dirp = opendir(dir_to_scan);
    }
    else
    {
        /* use scandir() */
        cursor->i=scandir(dir_to_scan, &cursor->namelist, 0, alphasort);
        cursor->cnt = cursor->i;

        if (cursor->i < 0)
        {
            err=errno;
            NDRX_LOG(log_error, "Failed to scan q directory [%s]: %s", 
                    M_folder_prepared, strerror(err));
            userlog("Failed to scan q directory [%s]: %s", 
                    M_folder_prepared, strerror(err));
            ret=XAER_RMERR;
            goto out;
        }
    }

    *ret_cursor = cursor;

out:
    NDRX_LOG(log_debug, "%s start: [%s] mode %d ret %d",
            __func__, dir_to_scan?dir_to_scan:"(null)", mode, ret);
    return ret;
}

/**
 * Read next file name from the directory.
 * @param sw storage interface
 * @param cursor cursor returned by pf_storage_list_start
 * @param ref buffer to store the tmxid or unique message id
 * @param refsz size of the ref buffer
 * @return EXTRUE (ref loaded), XA_OK (EOF), XAER_* error occurred.
 */
exprivate int ndrx_tmq_file_storage_list_next(ndrx_tmq_storage_t *sw, 
    void *cursor, char *ref, size_t refsz)
{
    ndrx_tmq_dir_list_t *p_cursor=(ndrx_tmq_dir_list_t *)cursor;
    char *p, *fname, *prev=NULL;
    int ret = XA_OK;
    int do_next;

    ref[0]=EXEOS;

    do
    {
        do_next=EXFALSE;
        /* allow fast access, if not sort requested */
        if ( (p_cursor->mode & NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED) ||
            (p_cursor->mode & NDRX_TMQ_STORAGE_LIST_MODE_NO_SORT) )
        {
            /* readdir(). Do not sort out dupes */
            struct dirent *ent;
            ent = readdir(p_cursor->dirp);

            if (NULL==ent)
            {
                /* EOF */
                ret=XA_OK;
                goto out;
            }
            fname=ent->d_name;
        }
        else
        {
            /* return element from the array */
            if (p_cursor->i <= 0)
            {
                /* EOF */
                ret=XA_OK;
                goto out;
            }
            p_cursor->i--;
            fname=p_cursor->namelist[p_cursor->i]->d_name;
        }

        NDRX_LOG(log_debug, "got entry: [%s]", fname);

        if (0==strcmp(fname, ".") || 
            0==strcmp(fname, ".."))
        {
            /* skip the empyt names... */
            do_next=EXTRUE;
            continue;
        }

        /* if we include duplicates, */
        if ( (p_cursor->mode & NDRX_TMQ_STORAGE_LIST_MODE_INCL_DUPS) ||
            (p_cursor->mode & NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED) )
        {
            /* just return the value */
            NDRX_STRCPY_SAFE_DST(ref, fname, refsz);
            ret=EXTRUE;
        }
        else
        {
            /* filter out duplciates (i.e. cut the name at -*, and compare with previous) */
            p = strchr(fname, '-');
            if (NULL==p)
            {
                do_next=EXTRUE;
                continue;
            }

            *p=EXEOS;

            /* check the previous load */
            if (NULL!=prev && 0==strcmp(p_cursor->prev, fname))
            {
                /* bypass it too */
                do_next=EXTRUE;
                continue;
            }

            /* valid ok */
            NDRX_STRCPY_SAFE_DST(ref, fname, refsz);
            ret=EXTRUE;
        }
        /* to deal with dulicates (i.e. suffix is seqno in global txn) */
        p_cursor->prev=fname;
    } while (do_next);

out:
    NDRX_LOG(log_debug, "%s next: [%s] ret %d",
            __func__, ref, ret);
    return ret;
}

/**
 * End the directory listing
 * free up any cursors
 * @param sw storage interface
 * @param cursor cursor returned by pf_storage_list_start
 * @return EXSUCCEED
 */
exprivate int ndrx_tmq_file_storage_list_end(ndrx_tmq_storage_t *sw, void *cursor)
{
    ndrx_tmq_dir_list_t *p_cursor=(ndrx_tmq_dir_list_t *)cursor;

    if ( (p_cursor->mode & NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED)
           || (p_cursor->mode & NDRX_TMQ_STORAGE_LIST_MODE_NO_SORT))
    {
        /* free up dir() */
        if (0!=closedir(p_cursor->dirp))
        {
            int err=errno;
            NDRX_LOG(log_error, "failed to closedir(): %s", strerror(err));
        }
    }
    else
    {
        /* free up scandir output  */
        if (NULL!=p_cursor->namelist)
        {
            dirent_free(p_cursor->namelist, p_cursor->cnt-1);
        }
    }

    memset(p_cursor, 0, sizeof(*p_cursor));

    return EXSUCCEED;
}

/**
 * Check first entry by readdir() (scan for any). We could check for
 * seq001 entry, but not sure would that be always correct
 */
exprivate int ndrx_tmq_file_storage_prep_exists(ndrx_tmq_storage_t *sw, char *tmxid)
{
    char ref[PATH_MAX+1];
    void *cursor=NULL;
    int ret;

    if (XA_OK!=(ret=ndrx_tmq_file_storage_list_start(sw, &cursor, 
        NDRX_TMQ_STORAGE_LIST_MODE_PREPARED|NDRX_TMQ_STORAGE_LIST_MODE_NO_SORT)))
    {
        goto out;
    }

    /* check in the loop presence of the xid */
    while (EXTRUE==(ret=ndrx_tmq_file_storage_list_next(sw, cursor, ref, sizeof(ref))))
    {
        if (0==strcmp(ref, tmxid))
        {
            /* found */
            ret=EXTRUE;
            goto out;
        }
    }

    if (XA_OK==ret)
    {
        /* EOF */
        ret=EXFALSE;
    }
    else
    {
        /* error has occurred */
        goto out;
    }

out:

    if (NULL!=cursor)
    {
        ndrx_tmq_file_storage_list_end(sw, cursor);
    }
    NDRX_LOG(log_debug, "%s: tmxid [%s] ret %d",
            __func__, tmxid, ret);
    return ret;
}

/**
 * Check if message exists in the store
 * @param sw storage interface
 * @param msgid_str message id
 * @return EXTRUE/EXFALSE/EXFAIL
 */
exprivate int ndrx_tmq_file_storage_comm_exists(ndrx_tmq_storage_t *sw, char *msgid_str)
{
   int ret=EXFALSE;
   char tmp[PATH_MAX+1];

   snprintf(tmp, sizeof(tmp), "%s/%s", 
                M_folder_committed, msgid_str);

    ret = access(tmp, 0);

    if (EXSUCCEED!=ret && ENOENT==errno)
    {
        ret=EXFALSE;
        goto out;
    }

    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "Failed to access [%s]: %s", tmp, strerror(errno));
        EXFAIL_OUT(ret);
    }

    ret=EXTRUE;

out:
    NDRX_LOG(log_debug, "%s: msgid_str [%s] ret %d",
            __func__, msgid_str, ret);
    return ret;
}

/**
 * Read command file from the disk. Commands may used as for:
 * - new message, delete message, increment counters and dummy message (for activate txns)
 * @param sw storage interface
 * @param nodeid node id
 * @param srvid server id
 * @param ref reference to the block (msgid id or transaction id)
 * @param seqno sequence number of the block (command sequence)
 * @param p_block allocate and read the block. Null in case of ignore
 * @param mode mode of the list (see NDRX_TMQ_STORAGE_LIST_MODE_* constants)
 * @return EXSUCCEED/EXFAIL.
 */
exprivate int ndrx_tmq_file_storage_read_block(ndrx_tmq_storage_t *sw, 
    short nodeid, short srvid, char *ref, int seqno,
    union tmq_block **p_block, int mode)
{
    int ret = EXSUCCEED;
    FILE *f;
    int err, tmq_err;
    int read;
    int state;
    char *filename = mode_to_filename(sw, ref, seqno, mode);

    /* Read header */            
    if (NULL==(*p_block = NDRX_MALLOC(sizeof(union tmq_block))))
    {
        NDRX_LOG(log_error, "Failed to alloc [%s]: %s", 
            filename, strerror(errno));
        EXFAIL_OUT(ret);
    }
   
    /* we do not read active blocks.., just prepare dummy rec... */
    if (mode & NDRX_TMQ_STORAGE_LIST_MODE_ACTIVE)
    {
        /* just create dummy entry for active transactions */
        sw->cfg->pf_tmq_setup_cmdheader_dum(&(*p_block)->hdr, NULL, tpgetnodeid(), 
                0, ndrx_G_qspace, 0);
        (*p_block)->hdr.command_code = TMQ_STORCMD_DUM;
    }
    else
    {
        if (ndrx_G_systest_lockloss || NULL==(f=NDRX_FOPEN(filename, "rb")))
        {
            err = errno;
            NDRX_LOG(log_error, "Failed to open for read [%s]: %s", 
                filename, strerror(err));
            userlog("Failed to open for read [%s]: %s", 
                filename, strerror(err));
            NDRX_FREE((char *)*p_block);
            *p_block=NULL;

            EXFAIL_OUT(ret);
        }

        /* here we read maximum header size.
         * For smaller messages (fixed struct messages) all data is read
         */
        if (EXFAIL==(read=read_tx_block(f, (char *)(*p_block), sizeof(**p_block), 
                filename, "tmq_storage_get_blocks", &err, &tmq_err)))
        {
            NDRX_LOG(log_error, "ERROR! Failed to read [%s] hdr (%d bytes) tmqerr: %d: %s - cannot start, resolve manually", 
                filename, sizeof(**p_block), tmq_err, (err==0?"EOF":strerror(err)) );
            userlog("ERROR! Failed to read [%s] hdr (%d bytes) tmqerr: %d: %s - cannot start, resolve manually", 
                filename, sizeof(**p_block), tmq_err, (err==0?"EOF":strerror(err)) );

            /* skip & continue with next */
            NDRX_FCLOSE(f);
            f=NULL;

            NDRX_FREE((char *)*p_block);
            *p_block = NULL;

            EXFAIL_OUT(ret);
        }

        /* Late filter 
         * Not sure what will happen if file will be processed/removed
         * by other server if for example we boot up...read the folder
         * but other server on same qspace/folder will remove the file
         * So better use different folder for each server...!
         */
        if (nodeid!=(*p_block)->hdr.nodeid || srvid!=(*p_block)->hdr.srvid)
        {
            NDRX_LOG(log_warn, "our nodeid/srvid %hd/%hd msg: %hd/%hd - IGNORE",
                nodeid, srvid, (*p_block)->hdr.nodeid, (*p_block)->hdr.srvid);

            NDRX_FREE((char *)*p_block);
            *p_block = NULL;
            NDRX_FCLOSE(f);
            goto out;
        }

        NDRX_DUMP(log_debug, "Got command block", *p_block, read);

        /* if it is message, the re-alloc  */
        if (TMQ_STORCMD_NEWMSG==(*p_block)->hdr.command_code)
        {
            int bytes_extra;
            int bytes_to_read;
            if (NULL==((*p_block) = NDRX_REALLOC((*p_block), sizeof(tmq_msg_t) + (*p_block)->msg.len)))
            {
                NDRX_LOG(log_error, "Failed to alloc [%d]: %s", 
                    (sizeof(tmq_msg_t) + (*p_block)->msg.len), strerror(errno));
                EXFAIL_OUT(ret);
            }
            /* Read some more */
            /* Bug #178
             * Under raspberry-pi looks like msg.msg is closer to the start than
             * whole message, and problem is that two bytes gets lost or over
             * written. Thus needs some kind of correction - advance the
             * pointer to msg over the extra bytes we have read.
             * Also needs correction against size to read.
             * This assumes that "tmq_msg_t" is largest structure...
             */
            bytes_extra = sizeof(**p_block)-EXOFFSET(tmq_msg_t, msg);
            bytes_to_read = (*p_block)->msg.len - bytes_extra;

            NDRX_LOG(log_info, "bytes_extra=%d bytes_to_read=%d", 
                    bytes_extra, bytes_to_read);

            if (bytes_to_read > 0)
            {
                if (EXFAIL==read_tx_block(f, 
                        (*p_block)->msg.msg+bytes_extra, 
                        bytes_to_read, filename, "tmq_storage_get_blocks 2", &err, &tmq_err))
                {
                    NDRX_LOG(log_error, "ERROR! Failed to read [%s] %d bytes: %s - cannot start, resolve manually", 
                        filename, bytes_to_read, strerror(err));

                    userlog("ERROR! Failed to read [%s] %d bytes: %s - cannot start, resolve manually", 
                            filename, bytes_to_read, strerror(err));
                                    /* skip & continue with next */
                    NDRX_FCLOSE(f);
                    f=NULL;
                    NDRX_FREE((char *)(*p_block));
                    *p_block = NULL;

                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                NDRX_LOG(log_info, "Full message already read by command block!");
            }

            /* any message not committed automatically means locked */
            if (mode & NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED)
            {
                (*p_block)->msg.lockthreadid = 0;
            }
            else
            {
                /* if message is active or prepared, then message is locked */
                (*p_block)->msg.lockthreadid = ndrx_gettid();
            }

            NDRX_DUMP(6, "Read message from disk", 
                    (*p_block)->msg.msg, (*p_block)->msg.len);
        }

        NDRX_FCLOSE(f);
        f=NULL;
    }

out:

    if ( EXSUCCEED!=ret && NULL!=*p_block)
    {
        NDRX_FREE((char *)(*p_block));
        *p_block=NULL;
    }

    return ret;
}

/**
 * Rollback commands
 * @param sw storage interface
 * @param p_tl transaction log
 * @return XA_OK/XAER*
 */
exprivate int ndrx_tmq_file_storage_rollback_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl)
{
    qtran_log_cmd_t *el, *elt;
    union tmq_upd_block b;
    int ret = XA_OK;

    /* Process files according to the log... */
    DL_FOREACH_SAFE(p_tl->cmds, el, elt)
    {
        char *fname = NULL;
        
#ifdef TXN_TRACE
        userlog("ABORT_ENT: tmxid=[%s] command_code=[%c]",
            tmxid, el->b.hdr.command_code);
        NDRX_LOG(log_error, "ABORT_ENT: tmxid=[%s] command_code=[%c]",
            tmxid, el->b.hdr.command_code);
#endif

        if (XA_RM_STATUS_ACTIVE==el->cmd_status)
        {
            /* run on active folder */
            fname = get_filename_i(el->seqno, M_folder_active, 0);
        }
        else if (XA_RM_STATUS_PREP==el->cmd_status)
        {
            /* run on prepared folder */
            fname = get_filename_i(el->seqno, M_folder_prepared, 0);
        }
        else
        {
            NDRX_LOG(log_error, "Invalid QCMD status %c", el->cmd_status);
            userlog("Invalid QCMD status %c", el->cmd_status);
            continue;
        }
        
        memcpy(&b, &el->b, sizeof(b));
        
        /* Send the notification */
        if (TMQ_STORCMD_DUM == el->b.hdr.command_code)
        {
            /* nothing special here... */
        }
        else if (TMQ_STORCMD_NEWMSG == el->b.hdr.command_code)
        {
            NDRX_LOG(log_info, "%s: issuing delete command...", __func__);
            b.hdr.command_code = TMQ_STORCMD_DEL;
        }
        else
        {
            NDRX_LOG(log_info, "%s: unlock command...", __func__);

            b.hdr.command_code = TMQ_STORCMD_UNLOCK;
        }

        /* if tmq server is not working at this moment
         * then we cannot complete the rollback
         */
        if (EXSUCCEED!=tmq_finalize_files_hdr(sw, &b.hdr, fname, 
                NULL, TMQ_FILECMD_UNLINK, el))
        {
            NDRX_LOG(log_error, "Failed to unlink [%s]", fname);
            continue;
        }
        
        /* Finally remove command entry */
        DL_DELETE(p_tl->cmds, el);
        NDRX_FPFREE(el);
        NDRX_LOG(log_debug, "Abort [%s] OK", fname);
    }
    
out:
    return ret;
}

/**
 * Process commit to the commands
 * @param sw storage interface
 * @param p_tl transaction log
 * @return XA_OK, XAER_*
 */
exprivate int ndrx_tmq_file_storage_commit_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl)
{
    int ret = XA_OK;
    char *fname;
    char *fname_msg;
    char msgid_str[TMMSGIDLEN_STR+1];
    FILE *f = NULL;
    qtran_log_cmd_t *el, *elt;
    int err, tmq_err;

    DL_FOREACH_SAFE(p_tl->cmds, el, elt)
    { 
        /* run on prepared folder */
        fname = get_filename_i(el->seqno, M_folder_prepared, 0);
            
#ifdef TXN_TRACE
        userlog("COMMIT_ENT: tmxid=[%s] command_code=[%c] fname=[%s]",
            tmxid, el->b.hdr.command_code, fname);
        NDRX_LOG(log_error, "COMMIT_ENT: tmxid=[%s] command_code=[%c] fname=[%s]",
            tmxid, el->b.hdr.command_code, fname);
#endif

        /* Do the task... */
        if (TMQ_STORCMD_NEWMSG == el->b.hdr.command_code)
        {
             char *to_filename;
             
            /* also here  move shall be finalized by tmq
             * as if move fails, tmsrv could retry
             * and only after retry to unli
             */
             to_filename = file_move_final_names(fname, 
                    tmq_msgid_serialize(el->b.hdr.msgid, msgid_str));
            
            if (EXSUCCEED!=tmq_finalize_files_hdr(sw, &el->b.hdr, fname,
                    to_filename, TMQ_FILECMD_RENAME, el))
            {
                ret = XAER_RMFAIL;
                goto out;
            }
        }
        else if (TMQ_STORCMD_UPD == el->b.hdr.command_code)
        {
            tmq_msg_t msg_to_upd; /* Message to update */
            int ret_len;
            
            fname_msg = get_file_name_final(tmq_msgid_serialize(el->b.hdr.msgid, 
                    msgid_str));
            NDRX_LOG(log_info, "Updating message file: [%s]", fname_msg);
            
            if (ndrx_G_systest_lockloss || NULL==(f = NDRX_FOPEN(fname_msg, "r+b")))
            {
                int err = errno;
                NDRX_LOG(log_error, "ERROR! xa_commit_entry() - failed to open file[%s]: %s!", 
                        fname_msg, strerror(err));

                userlog( "ERROR! xa_commit_entry() - failed to open file[%s]: %s!", 
                        fname_msg, strerror(err));
                ret = XAER_RMFAIL;
                goto out;
            }
            
            if (EXFAIL==read_tx_block(f, (char *)&msg_to_upd, sizeof(msg_to_upd), 
                    fname_msg, "xa_commit_entry", &err, &tmq_err))
            {
                NDRX_LOG(log_error, "ERROR! xa_commit_entry() - failed to read data block!");
                ret = XAER_RMFAIL;
                goto out;
            }
            
            /* seek the start */
            if (EXSUCCEED!=fseek (f, 0 , SEEK_SET ))
            {
                NDRX_LOG(log_error, "Seekset failed: %s", strerror(errno));
                ret = XAER_RMFAIL;
                goto out;
            }
            
            UPD_MSG((&msg_to_upd), (&el->b.upd));
            
            /* Write th block */
	        /* ndrx_G_systest_lockloss -> IO fence for testing */
            if (ndrx_G_systest_lockloss || sizeof(msg_to_upd)!=(ret_len=fwrite((char *)&msg_to_upd, 1, 
                    sizeof(msg_to_upd), f)))
            {
                int err = errno;
                NDRX_LOG(log_error, "ERROR! Failed to write to msg file [%s]: "
                        "req_len=%d, written=%d: %s", fname_msg,
                        sizeof(msg_to_upd), ret_len, strerror(err));

                userlog("ERROR! Failed to write to msg file[%s]: req_len=%d, "
                        "written=%d: %s",
                        fname_msg, sizeof(msg_to_upd), ret_len, strerror(err));

                ret = XAER_RMFAIL;
                goto out;
            }
            NDRX_FCLOSE(f);
            f = NULL;
            
            /* remove the update file */
            NDRX_LOG(log_info, "Removing update command file: [%s]", fname);
            
            if (EXSUCCEED!=tmq_finalize_files_upd(sw, &el->b.upd, fname, NULL,
                    TMQ_FILECMD_UNLINK, el))
            {
                ret = XAER_RMFAIL;
                goto out;
            }
            
        }
        else if (TMQ_STORCMD_DEL == el->b.hdr.command_code)
        {
            fname_msg = get_file_name_final(tmq_msgid_serialize(el->b.hdr.msgid, 
                    msgid_str));
            
            NDRX_LOG(log_info, "Message file to remove: [%s]", fname_msg);
            NDRX_LOG(log_info, "Command file to remove: [%s]", fname);
            
            /* Remove the message (it must be marked for delete)
             */
            if (EXSUCCEED!=tmq_finalize_files_hdr(sw, &el->b.hdr, fname_msg, fname, 
                    TMQ_FILECMD_UNLINK, el))
            {
                ret = XAER_RMFAIL;
                goto out;
            }
        }
        else if (TMQ_STORCMD_DUM == el->b.hdr.command_code)
        {
            NDRX_LOG(log_info, "Dummy command to remove: [%s]", fname);
            
            /* 
             * Remove the message (it must be marked for delete)
             */
            if (EXSUCCEED!=tmq_finalize_files_hdr(sw, &el->b.hdr, fname, NULL, 
                    TMQ_FILECMD_UNLINK, el))
            {
                ret = XAER_RMFAIL;
                goto out;
            }
        }
        else
        {
            NDRX_LOG(log_error, "ERROR! xa_commit_entry() - invalid command [%c]!",
                    el->b.hdr.command_code);

            ret = XAER_RMERR;
            goto out;
        }
        
        /* msg is unlocked, thus just remove ptr  */
        NDRX_LOG(log_debug, "Commit [%s] sequence OK", p_tl->tmxid, el->seqno);
        DL_DELETE(p_tl->cmds, el);
        NDRX_FPFREE(el);
        
    }
out:
    return ret;
}

/**
 * Prepare commands
 * @param sw storage interface
 * @param p_tl transaction log
 * @return XA_OK, XAER_*
 */
exprivate int ndrx_tmq_file_storage_prepare_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl)
{
    int ret = XA_OK;
    int did_move = EXFALSE;
    qtran_log_cmd_t *el, *elt;

    /* process command by command to stage to prepared ... */
    DL_FOREACH_SAFE(p_tl->cmds, el, elt)
    {
        if (EXSUCCEED!=file_move(el->seqno, M_folder_active, M_folder_prepared))
        {
            NDRX_LOG(log_error, "Q tran tmxid [%s] seq %d failed to prepare (file move)",
                    p_tl->tmxid, el->seqno);
            p_tl->is_abort_only=EXTRUE;
            ret=XAER_RMERR;
            goto out;
        }
        
        el->cmd_status = XA_RM_STATUS_PREP;
        NDRX_LOG(log_info, "tmxid [%s] seq %d prepared OK", p_tl->tmxid, el->seqno);
        did_move=EXTRUE;
    }
    
    if (did_move)
    {
        /* sync the  */
	    /* + write io fence */
        if (ndrx_G_systest_lockloss || 
            EXSUCCEED!=ndrx_fsync_dsync(M_folder_prepared, G_atmi_env.xa_fsync_flags))
        {
            NDRX_LOG(log_error, "Failed to dsync [%s]", M_folder_prepared);
            
            /* mark transaction for abort only! */
            p_tl->is_abort_only=EXTRUE;
            ret=XAER_RMERR;
            goto out;
        }
    }
    
out:
    return ret;
}

/**
 * Write block to the store.
 * This includes: new msg, delete msg, update counters
 * @param sw storage interface
 * @param block block to write
 * @param len length of the block
 * @param cust_tmxid custom transaction id (NULL if not used)
 * @param int_diag internal diagnostic (NULL if not used)
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_tmq_file_storage_write_block(ndrx_tmq_storage_t *sw, 
    char *block, int len, char *cust_tmxid, int *int_diag)
{
    return write_to_tx_file(block, len, cust_tmxid, int_diag);
}

/**
 * Get the next file name for current transaction
 * 
 * @return EXSUCCEED (names set) / EXFAIL (transaction not found)
 */
exprivate int set_filenames(int *p_seqno)
{
    /* get next sequence number of tran 
     * tmxid is encoded in G_atmi_tls->qdisk_tls->filename_base
     * thus lookup the transaction, and get the next number
     */
    int ret = EXSUCCEED;
    int locke=EXFALSE;
    int seqno;
    qtran_log_t * p_tl = tmq_log_get_entry(G_atmi_tls->qdisk_tls->filename_base, 
        NDRX_LOCK_WAIT_TIME, &locke);
    
    if (NULL==p_tl)
    {
        NDRX_LOG(log_error, "Transaction [%s] not found", 
                G_atmi_tls->qdisk_tls->filename_base);
        EXFAIL_OUT(ret);
    }
    
    seqno = tmq_log_next(p_tl);
    
    snprintf(G_atmi_tls->qdisk_tls->filename_active, sizeof(G_atmi_tls->qdisk_tls->filename_active), 
                "%s/%s-%03d", M_folder_active, G_atmi_tls->qdisk_tls->filename_base, seqno);
    
    snprintf(G_atmi_tls->qdisk_tls->filename_prepared, sizeof(G_atmi_tls->qdisk_tls->filename_prepared), 
            "%s/%s-%03d", M_folder_prepared, G_atmi_tls->qdisk_tls->filename_base, seqno);
        
    NDRX_LOG(log_info, "Filenames set to: [%s] [%s] (base: [%s])", 
                G_atmi_tls->qdisk_tls->filename_active, 
                G_atmi_tls->qdisk_tls->filename_prepared,
                G_atmi_tls->qdisk_tls->filename_base);
    
    *p_seqno = seqno;
out:
                
    /* if not recursive lock, then release */
    if (NULL!=p_tl && !locke)
    {
        tmq_log_unlock(p_tl);
    }

    return ret;
}


/**
 * Get the full file name for `i' occurrence 
 * @param i
 * @param folder
 * @return path to file
 */
exprivate char *get_filename_i(int i, char *folder, int slot)
{
    snprintf(M_filename[slot], sizeof(M_filename[0]), "%s/%s-%03d", folder, 
        G_atmi_tls->qdisk_tls->filename_base, i);
    
    return M_filename[slot];
}

/**
 * Convert reference to file name
 * @param ref (tmxid or msgid)
 * @param seqno sequence number
 * @param mode mode of the list (see NDRX_TMQ_STORAGE_LIST_MODE_* constants)
 * @return file name (TLS alloc)
 */
exprivate char *mode_to_filename(ndrx_tmq_storage_t *sw, char *ref, int seqno, int mode)
{
    char *ret = NULL;
    if (mode & NDRX_TMQ_STORAGE_LIST_MODE_ACTIVE)
    {
        snprintf(M_filename[0], sizeof(M_filename[0]), "%s/%s-%03d", M_folder_active, 
            ref, seqno);
        ret=M_filename[0];
    }
    else if (mode & NDRX_TMQ_STORAGE_LIST_MODE_PREPARED)
    {
        snprintf(M_filename[0], sizeof(M_filename[0]), "%s/%s-%03d", M_folder_prepared, 
            ref, seqno);
        ret=M_filename[0];
    }
    else if (mode & NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED)
    {
        snprintf(M_filename[0], sizeof(M_filename[0]), "%s/%s", M_folder_committed, 
            ref);
        ret=M_filename[0];
    }

    assert(ret!=NULL);

    return ret;
}

/**
 * Special Q file name
 * @param fname
 * @return 
 */
exprivate char *get_file_name_final(char *fname)
{
    static __thread char buf[PATH_MAX+1];
    
    snprintf(buf, sizeof(buf), "%s/%s", M_folder_committed, fname);
    NDRX_LOG(log_debug, "Filename built: %s", buf);
    
    return buf;
}

/**
 * Rename file from one folder to another...
 * @param xid
 * @param rmid
 * @param from_folder
 * @param to_folder
 * @return 
 */
exprivate int file_move(int i, char *from_folder, char *to_folder)
{
    int ret = EXSUCCEED;
    char *f;
    char *t;

    f = get_filename_i(i, from_folder, 0);
    t = get_filename_i(i, to_folder, 1);
        
    NDRX_LOG(log_info, "Rename [%s]->[%s]", 
                f,t);

    /* ndrx_G_systest_lockloss -> IO fence for test */
    if (ndrx_G_systest_lockloss || EXSUCCEED!=rename(f, t))
    {
        NDRX_LOG(log_error, "Failed to rename [%s]->[%s]: %s", 
                f,t,
                strerror(errno));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Move the file to committed storage
 * @param from_filename source file name with path
 * @param to_filename_only dest only filename
 * @return final file name
 */
exprivate char * file_move_final_names(char *from_filename, char *to_filename_only)
{
    int ret = EXSUCCEED;
    
    char *to_filename = get_file_name_final(to_filename_only);
    
    NDRX_LOG(log_debug, "Rename [%s] -> [%s]", from_filename, to_filename);

    return to_filename;
}

/**
 * Send notification to tmqueue server so that we have finished this
 * particular message & we can unlock that for further processing
 * TODO: in case of rename if we get noent, then check the destination
 *  if the destination name exists, then assume that rename was fine.
 *  this is needed to avoid infinite loop when tlog have gone async with disk
 * @param sw storange engine
 * @param p_hdr
 * @param fname1 filename 1 to unlink (priority 1 - after this message is unblocked)
 * @param fname2 filename 2 to unlink (priority 2)
 * @param fcmd file cmd, if U, then fname1 & 2 is both unlinks, if R, then f1 src name, f2 dest name
 * @param tcmd transaction command
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_file(ndrx_tmq_storage_t *sw, 
        union tmq_upd_block *p_upd, char *fname1, 
        char *fname2, char fcmd, qtran_log_cmd_t *tcmd)
{
    
    int ret = EXSUCCEED;
    BFLDOCC occ;
    char name1[PATH_MAX+1]="";
    char name2[PATH_MAX+1]="";
    char *p;
    char *files[2];
    
    /* Load args... */
    if (NULL!=fname1)
    {
        NDRX_STRCPY_SAFE(name1, fname1);
        files[0]=name1;
    }
    else
    {
        files[0]=NULL;
    }
    
    if (NULL!=fname2)
    {
        NDRX_STRCPY_SAFE(name2, fname2);
        files[1]=name2;
    }
    else
    {
        files[1]=NULL;
    }    
    
    /* rename -> mandatory. Also if on remove file does not exists, this is the
     *  same as removed OK
     * first unlink -> mandatory
     * second unlink -> optional 
     */
    if (TMQ_FILECMD_UNLINK==fcmd)
    {            
        for (occ=0; occ<N_DIM(files) && NULL!=files[occ]; occ++)
        {
            NDRX_LOG(log_debug, "Unlinking file [%s]", files[occ]);

            /* IO fence test */
            if (ndrx_G_systest_lockloss || EXSUCCEED!=unlink(files[occ]))
            {
                if (ENOENT!=errno)
                {
                    int err = errno;
                    NDRX_LOG(log_error, "Failed to unlinking file [%s] occ %d: %s", 
                            files[occ], occ, strerror(err));
                    userlog("Failed to unlinking file [%s] occ %d: %s", 
                            files[occ], occ, strerror(err));

                    if (0==occ)
                    {
                        ret=XAER_RMERR;
                        goto out;
                    }

                }
            }

            if (0==occ)
            {
                /* get the folder form file name */
                p=strrchr(files[occ], '/');

                if (NULL!=p)
                {
                    *p=EXEOS;
                }

                /* io fence test */
                if (ndrx_G_systest_lockloss || EXSUCCEED!=ndrx_fsync_dsync(files[occ], G_atmi_env.xa_fsync_flags))
                {
                    NDRX_LOG(log_error, "Failed to dsync [%s]", files[occ]);
                    ret=XAER_RMERR;
                    goto out;
                }
            }
        }
    }
    else if (TMQ_FILECMD_RENAME==fcmd)
    {
        if (NULL==files[0] || NULL==files[1])
        {
            NDRX_LOG(log_error, "File 1 or 2 is NULL %p %p - cannot rename",
                    files[0], files[1]);
            ret=XAER_RMERR;
            goto out;
        }

        NDRX_LOG(log_debug, "About to rename: [%s] -> [%s]",
                name1, name2);

        /* wth io fence test: */
        if (ndrx_G_systest_lockloss|| EXSUCCEED!=rename(name1, name2))
        {
            int err = errno;
            
            if (ENOENT==err && ndrx_file_exists(name2))
            {
                NDRX_LOG(log_error, "Failed to rename file [%s] -> [%s] occ %d: "
                        "%s, but dest exists - assume retry", 
                        name1, name2, occ, strerror(err));
            }
            else
            {
                NDRX_LOG(log_error, "Failed to rename file [%s] -> [%s] occ %d: %s", 
                        name1, name2, occ, strerror(err));
                userlog("Failed to rename file [%s] -> [%s] occ %d: %s", 
                        name1, name2, occ, strerror(err));
                ret=XAER_RMERR;
                goto out;
            }
        }

        /* get the folder form file name */
        p=strrchr(name2, '/');

        if (NULL!=p)
        {
            *p=EXEOS;
        }

        /* write io fence */
        if (ndrx_G_systest_lockloss || EXSUCCEED!=ndrx_fsync_dsync(name2, G_atmi_env.xa_fsync_flags))
        {
            NDRX_LOG(log_error, "Failed to dsync [%s]", name2);
            ret=XAER_RMERR;
            goto out;
        }
    }
    else
    {
        NDRX_LOG(log_error, "Unsupported file command %c", fcmd);
        ret=XAER_RMERR;
        goto out;
    }
    
    /* If all OK, lets unlock the message. */
    if (!tcmd->no_unlock && EXSUCCEED!=sw->cfg->pf_tmq_unlock_msg(p_upd))
    {
        ret=XAER_RMERR;
        goto out;
    }

out:
    return ret;
}

/**
 * Finalize files by update block
 * @param sw storage switch
 * @param p_upd
 * @param fname1 filename1 to unlink
 * @param fname2 filename2 to unlink
 * @param fcmd file cmd
 * @param tcmd transaction command
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_files_upd(ndrx_tmq_storage_t *sw,
        tmq_msg_upd_t *p_upd, char *fname1, 
        char *fname2, char fcmd, qtran_log_cmd_t *tcmd)
{
    union tmq_upd_block block;
    
    memset(&block, 0, sizeof(block));
    
    memcpy(&block.upd, p_upd, sizeof(*p_upd));
    
    return tmq_finalize_file(sw, &block, fname1, fname2, fcmd, tcmd);
}

/**
 * Finalize files by header block
 * @param sw storage switch
 * @param p_hdr
 * @param fname1 filename1 to unlink
 * @param fname2 filename2 to unlink
 * @param fcmd file commmand code
 * @param tcmd tran command
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_files_hdr(ndrx_tmq_storage_t *sw,
        tmq_cmdheader_t *p_hdr, char *fname1, 
        char *fname2, char fcmd, qtran_log_cmd_t *tcmd)
{
    union tmq_upd_block block;
    
    memset(&block, 0, sizeof(block));
    memcpy(&block.hdr, p_hdr, sizeof(*p_hdr));
    
    return tmq_finalize_file(sw, &block, fname1, fname2, fcmd, tcmd);
}

/**
 * Create required folders
 * @param p_tmq_cfg tmq
 * @return EXSUCCEED/RM ERR
 */
exprivate int prepare_folders(ndrx_tmq_qdisk_xa_cfg_t *p_tmq_cfg)
{

    int ret = EXSUCCEED;

    NDRX_LOG(log_info, "Q data directory: [%s]", p_tmq_cfg->data_folder);
    
    /* The xa_info is directory, where to store the data...*/
    NDRX_STRNCPY(M_folder_active, p_tmq_cfg->data_folder, sizeof(M_folder_active)-8);
    M_folder_active[sizeof(M_folder_active)-7] = EXEOS;
    NDRX_STRCAT_S(M_folder_active, sizeof(M_folder_active), "/active");
    
    NDRX_STRNCPY(M_folder_prepared, p_tmq_cfg->data_folder, sizeof(M_folder_prepared)-10);
    M_folder_prepared[sizeof(M_folder_prepared)-9] = EXEOS;
    NDRX_STRCAT_S(M_folder_prepared, sizeof(M_folder_prepared), "/prepared");
    
    NDRX_STRNCPY(M_folder_committed, p_tmq_cfg->data_folder, sizeof(M_folder_committed)-11);
    M_folder_committed[sizeof(M_folder_committed)-10] = EXEOS;
    NDRX_STRCAT_S(M_folder_committed, sizeof(M_folder_committed), "/committed");
    
    /* Test the directories */
    if (EXSUCCEED!=(ret=mkdir(p_tmq_cfg->data_folder, NDRX_DIR_PERM)) && ret!=EEXIST )
    {
        int err = errno;

        if (err!=EEXIST)
        {
            NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", p_tmq_cfg->data_folder, strerror(err));

            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", p_tmq_cfg->data_folder, strerror(err));
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_info, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", p_tmq_cfg->data_folder, strerror(err));
            ret=EXSUCCEED;
        }
    }
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_active, NDRX_DIR_PERM)) && ret!=EEXIST )
    {
        int err = errno;
        
        if (err!=EEXIST)
        {
            NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_active, strerror(err));

            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", M_folder_active, strerror(err));
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_info, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_active, strerror(err));
            ret=EXSUCCEED;
        }
    }
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_prepared, NDRX_DIR_PERM)) && ret!=EEXIST )
    {
        int err = errno;
        
        if (err!=EEXIST)
        {
            NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_prepared, strerror(err));
            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", M_folder_prepared, strerror(err));
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_info, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_prepared, strerror(err));
            ret=EXSUCCEED;
        }
    }
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_committed, NDRX_DIR_PERM)) && ret!=EEXIST )
    {
        int err = errno;
        
        if (err!=EEXIST)
        {
            NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_committed, strerror(err));
            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", M_folder_committed, strerror(err));
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_info, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_committed, strerror(err));
            ret=EXSUCCEED;
        }
    }
    
    NDRX_LOG(log_info, "Prepared data root=[%s]", p_tmq_cfg->data_folder);
    NDRX_LOG(log_info, "Prepared M_folder_active=[%s]", M_folder_active);
    NDRX_LOG(log_info, "Prepared M_folder_prepared=[%s]", M_folder_prepared);
    NDRX_LOG(log_info, "Prepared M_folder_committed=[%s]", M_folder_committed);
    
out:
    return ret;
}

/**
 * Free up the current scan of the directory entries
 * used by recover
 */
exprivate void dirent_free(struct dirent **namelist, int n)
{
    while (n>=0)
    {
        NDRX_FREE(namelist[n]);
        n--;
    }
    NDRX_FREE(namelist);
}

#define READ_BLOCK(BUF, LEN) do {\
        if (LEN!=(act_read=fread(BUF, 1, LEN, f)))\
        {\
            *err = ferror(f);\
            \
            if (EXSUCCEED==*err)\
            {\
                *tmq_err = TMQ_ERR_EOF;\
            }\
            NDRX_LOG(log_error, "ERROR! Failed to read tx file (%s: %s): req_read=%d, read=%d: %s, tmq_err: %d",\
                    dbg_msg, fname, LEN, act_read, (*err==0?"EOF":strerror(*err)), *tmq_err);\
            \
            userlog("ERROR! Failed to read tx file (%s: %s): req_read=%d, read=%d: %s, tmq_err: %d",\
                dbg_msg, fname, LEN, act_read, (*err==0?"EOF":strerror(*err)), *tmq_err);\
            EXFAIL_OUT(ret);\
        }\
    } while (0)

/** verify magic */
#define VERIFY_MAGIC(FIELD, MAGIC_VAL, LEN, DESCR, ERROR_CODE) do { \
        /* validate magics and command code... */\
        if (0!=strncmp(FIELD, MAGIC_VAL, LEN))\
        {\
            NDRX_LOG(log_error, "ERROR! file: [%s] invalid %s: expected: [%s] got: [%.*s] command: [%x] error_code: %d", \
                    fname, DESCR, MAGIC_VAL, LEN, FIELD, (unsigned int)p_hdr->command_code, ERROR_CODE);\
            NDRX_DUMP(log_error, "Invalid header", p_hdr, sizeof(tmq_cmdheader_t));\
            userlog("ERROR! file: [%s] invalid %s: expected: [%s] got: [%.*s] command: [%x] error_code: %d", \
                    fname, DESCR, MAGIC_VAL, LEN, FIELD, (unsigned int)p_hdr->command_code, ERROR_CODE);\
            *tmq_err = ERROR_CODE;\
            EXFAIL_OUT(ret);\
        }\
    } while(0)

/**
 * Reads the header block
 * Either full read or partial.
 * - Check header validity if ftell() position is 0.
 * @param block where to unload the data
 * @param p_len for TMQ_STORCMD_NEWMSG this indicates number of mandatory bytes to read
 *  for other message types, validate the read size against the actual structure size.
 * @param fname name for debug
 * @param dbg_msg debug message
 * @param err ptr to error code if failed to read
 * @param tmq_err internal error code
 * @return EXFAIL or number of bytes read
 */
exprivate int read_tx_block(FILE *f, char *block, int len, char *fname, 
        char *dbg_msg, int *err, 
        int *tmq_err)
{
    int act_read;
    int read = 0;
    int ret = EXSUCCEED;
    int offset = 0;
    tmq_cmdheader_t *p_hdr;
    
    *err=0;
    *tmq_err = 0;
    
    if (0==ftell(f))
    {
        assert(len >= sizeof(tmq_cmdheader_t));
        /* read header */
        READ_BLOCK(block, sizeof(tmq_cmdheader_t));
        
        read+=sizeof(tmq_cmdheader_t);
        
        /* validate header */
        p_hdr = (tmq_cmdheader_t *)block;
        
        /* validate magics and command code... */
        VERIFY_MAGIC(p_hdr->magic,  TMQ_MAGICBASE,  TMQ_MAGICBASE_LEN,  "base magic1",  TMQ_ERR_CORRUPT);
        VERIFY_MAGIC(p_hdr->magic2, TMQ_MAGICBASE2, TMQ_MAGICBASE_LEN,  "base magic2",  TMQ_ERR_CORRUPT);
        VERIFY_MAGIC(p_hdr->magic,  TMQ_MAGIC,      TMQ_MAGIC_LEN,      "magic1",       TMQ_ERR_VERSION);
        VERIFY_MAGIC(p_hdr->magic2, TMQ_MAGIC2,     TMQ_MAGIC_LEN,      "magic2",       TMQ_ERR_VERSION);
        
        /* validate command code */
        switch (p_hdr->command_code)
        {
            case TMQ_STORCMD_NEWMSG:
                NDRX_LOG(log_debug, "Command: New message");
                /* as this is biggest message & dynamic size: */
                len = len - sizeof(tmq_cmdheader_t);
                break;
            case TMQ_STORCMD_UPD:
                
                NDRX_LOG(log_debug, "Command: Update message");
                if (len > sizeof(tmq_cmdheader_t))
                {
                    len = sizeof(tmq_msg_upd_t) - sizeof(tmq_cmdheader_t);
                }
                break;
            case TMQ_STORCMD_DEL:
                
                NDRX_LOG(log_debug, "Command: Delete message");
                
                if (len > sizeof(tmq_cmdheader_t))
                {
                    len = sizeof(tmq_msg_del_t) - sizeof(tmq_cmdheader_t);
                }
                break;
            case TMQ_STORCMD_UNLOCK:
                
                NDRX_LOG(log_debug, "Command: Unlock message");
                
                if (len > sizeof(tmq_cmdheader_t))
                {
                    len = sizeof(tmq_msg_unl_t) - sizeof(tmq_cmdheader_t);
                }
                break;
            default:
                NDRX_LOG(log_error, "ERROR! file [%s] invalid command code [%x]", 
                    fname, (unsigned int)p_hdr->command_code);
                NDRX_DUMP(log_error, "Invalid header", p_hdr, sizeof(tmq_cmdheader_t));
                userlog("ERROR! file [%s] invalid command code [%x]", 
                    fname, (unsigned int)p_hdr->command_code);
                *tmq_err = TMQ_ERR_CORRUPT;
                EXFAIL_OUT(ret);

                break;
        }
        
        offset = sizeof(tmq_cmdheader_t);
    }
    
    if (len > 0)
    {
        READ_BLOCK((block+offset), len);
        read+=len;   
    }
    
out:
                
    if (EXSUCCEED==ret)
    {
        return read;
    }
    else
    {    
        return ret;
    }
}

/**
 * Read the block file file
 * @param fname full path to file
 * @param block buffer where to store the data block
 * @param len length to read.
 * @param err error code
 * @param tmq_err internal error code
 * @return EXFAIL or number of bytes read
 */
exprivate int read_tx_from_file(char *fname, char *block, int len, int *err,
        int *tmq_err)
{
    int ret = EXSUCCEED;
    FILE *f = NULL;
    
    *err=0;
    if (ndrx_G_systest_lockloss || NULL==(f = NDRX_FOPEN(fname, "r+b")))
    {
        *err = errno;
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - failed to open file[%s]: %s!", 
                fname, strerror(*err));

        userlog( "ERROR! xa_commit_entry() - failed to open file[%s]: %s!", 
                fname, strerror(*err));
        EXFAIL_OUT(ret);
    }
    
    ret = read_tx_block(f, block, len, fname, "read_tx_from_file", err, tmq_err);
    
out:

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }
    
    return ret;
}

/** Write th block */
#define WRITE_TO_DISK(PTR, OFFSET, LEN) \
    ret_len = 0;\
    if (ndrx_G_systest_lockloss || G_atmi_env.test_qdisk_write_fail || LEN!=(ret_len=fwrite( ((char *)PTR) + OFFSET, 1, LEN, f)))\
    {\
        int err = errno;\
        /* For Q/A purposes - simulate no space error, if requested */\
        if (G_atmi_env.test_qdisk_write_fail)\
        {\
            NDRX_LOG(log_error, "test point: test_qdisk_write_fail TRUE");\
            err = ENOSPC;\
        }\
        NDRX_LOG(log_error, "ERROR! Failed to write to msgblock/tx file [%s]: "\
                            "offset=%d req_len=%d, written=%d: %s",\
                            G_atmi_tls->qdisk_tls->filename_active,\
                            OFFSET, LEN, ret_len, strerror(err));\
        userlog("ERROR! Failed to write msgblock/tx file [%s]: "\
                "offset=%d req_len=%d, written=%d: %s",\
                G_atmi_tls->qdisk_tls->filename_active,\
                OFFSET, LEN, ret_len, strerror(err));\
        EXFAIL_OUT(ret);\
    }

/** flush changes to disk  */
#define WRITE_FLUSH \
    ret_len = 0;\
    if (ndrx_G_systest_lockloss || EXSUCCEED!=fflush(f))\
    {\
        int err = errno;\
        NDRX_LOG(log_error, "ERROR! fflush() on [%s] failed: %s", \
            G_atmi_tls->qdisk_tls->filename_active, strerror(err));\
        userlog("ERROR! fflush() on [%s] failed: %s", \
            G_atmi_tls->qdisk_tls->filename_active, strerror(err));\
        EXFAIL_OUT(ret);\
    }

#define WRITE_REWIND \
        if (EXSUCCEED!=fseek(f, 0L, SEEK_SET))\
        {\
            int err = errno;\
            NDRX_LOG(log_error, "ERROR! fseek() rewind on [%s] failed: %s", \
                G_atmi_tls->qdisk_tls->filename_active, strerror(err));\
            userlog("ERROR! fseek() rewind on [%s] failed: %s", \
                G_atmi_tls->qdisk_tls->filename_active, strerror(err));\
            EXFAIL_OUT(ret);\
        }

/* COMMANDS:
 * - write qmessage
 * - write status/try update
 * - unlink the message
 * 
 * Read only commands:
 * - Query messages (find first & find next)
 */

/**
 * Write data to transaction file.
 * This works only with new files (new transactions)
 * @param block
 * @param len
 * @param cust_tmxid custom tmxid, if not running in global tran
 * @param int_diag internal diagnostics, flags
 * @return SUCCEED/FAIL
 */
exprivate int write_to_tx_file(char *block, int len, char *cust_tmxid, int *int_diag)
{
    int ret = EXSUCCEED;
    XID xid;
    size_t ret_len;
    FILE *f = NULL;
    int ax_ret;
    char mode_str[16];
    tmq_cmdheader_t dum;
    int seqno;
    long xaflags=0;
    char *tmxid=NULL;
    
    NDRX_STRCPY_SAFE(mode_str, "wb");
    
    if (ndrx_get_G_atmi_env()->xa_sw->flags & TMREGISTER && !G_atmi_tls->qdisk_tls->is_reg)
    {
        ax_ret = ax_reg(G_atmi_tls->qdisk_rmid, &xid, 0);
                
        if (TM_JOIN!=ax_ret && TM_OK!=ax_ret)
        {
            if (NULL!=int_diag)
            {
                *int_diag|=TMQ_INT_DIAG_EJOIN;
            }
            
            NDRX_LOG(log_error, "ERROR! xa_reg() failed!");
            EXFAIL_OUT(ret);
        }
        
        if (TM_JOIN==ax_ret)
        {
            xaflags = TMJOIN;
        }
        
        /* done by ax_reg! */
        if (XA_OK!=xa_start_entry(ndrx_get_G_atmi_env()->xa_sw, &xid, G_atmi_tls->qdisk_rmid, xaflags))
        {
            
            if (NULL!=int_diag)
            {
                *int_diag|=TMQ_INT_DIAG_EJOIN;
            }
            
            NDRX_LOG(log_error, "ERROR! xa_start_entry() failed!");
            EXFAIL_OUT(ret);
        }
        
        G_atmi_tls->qdisk_tls->is_reg = EXTRUE;
    }
    
    /* get the current transaction? 
     * If the same thread is locked, then no problem...
     */
    if (EXSUCCEED!=set_filenames(&seqno))
    {
        EXFAIL_OUT(ret);
    }

    if (NULL!=cust_tmxid)
    {
        tmxid = cust_tmxid;
    }
    else
    {
        /* part of global trn */
        tmxid = G_atmi_tls->qdisk_tls->filename_base;
    }
    
    /* Open file for write... */
    NDRX_LOG(log_info, "Writing command file: [%s] mode: [%s] (seqno: %d)", 
        G_atmi_tls->qdisk_tls->filename_active, mode_str, seqno);
    
    /* io fence test. */
    if (ndrx_G_systest_lockloss || NULL==(f = NDRX_FOPEN(G_atmi_tls->qdisk_tls->filename_active, mode_str)))
    {
        int err = errno;
        NDRX_LOG(log_error, "ERROR! write_to_tx_file() - failed to open file[%s]: %s!", 
                G_atmi_tls->qdisk_tls->filename_active, strerror(err));
        
        userlog( "ERROR! write_to_tx_file() - failed to open file[%s]: %s!", 
                G_atmi_tls->qdisk_tls->filename_active, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* single step write..., as temp files now we discard
     * no problem with we get temprorary files incomplete...
     */
    WRITE_TO_DISK(block, 0, len);
    
    WRITE_FLUSH;
    
    /* sync the file, if required so... 
     * file updates are optional..
     */
    if (ndrx_G_systest_lockloss || EXSUCCEED!=ndrx_fsync_fsync(f, G_atmi_env.xa_fsync_flags))
    {
        NDRX_LOG(log_error, "failed to fsync");
        EXFAIL_OUT(ret);
    }
    
    /* 
     * If all OK, add command transaction log 
     */
    if (EXSUCCEED!=tmq_log_addcmd(tmxid, seqno, block, XA_RM_STATUS_ACTIVE))
    {
        NDRX_LOG(log_error, "Failed to add [%s] seqno %d to transaction log",
                G_atmi_tls->qdisk_tls->filename_base, seqno);
        userlog("Failed to add [%s] seqno %d to transaction log",
                G_atmi_tls->qdisk_tls->filename_base, seqno);
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (NULL!=f)
    {
        /* unlink if failed to write to the folder... */
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Unlink: [%s]", G_atmi_tls->qdisk_tls->filename_active);

            /* io fencing test */
            if (ndrx_G_systest_lockloss 
			    || EXSUCCEED!=unlink(G_atmi_tls->qdisk_tls->filename_active))
            {
                 int err = errno;
                 NDRX_LOG(log_error, "Failed to unlink [%s]: %s",
                    G_atmi_tls->qdisk_tls->filename_active, strerror(err));
                 userlog("Failed to unlink [%s]: %s",
                    G_atmi_tls->qdisk_tls->filename_active, strerror(err));
            }
        }
        
        NDRX_FCLOSE(f);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
