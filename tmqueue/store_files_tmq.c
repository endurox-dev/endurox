/**
 * @brief Storage interace - file-system
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

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <userlog.h>

#include "qdisk_xa.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate char M_folder_active[PATH_MAX+1] = {EXEOS}; /**< Active transactions        */
exprivate char M_folder_prepared[PATH_MAX+1] = {EXEOS}; /**< Prepared transactions    */
exprivate char M_folder_committed[PATH_MAX+1] = {EXEOS}; /**< Committed transactions  */

/*---------------------------Prototypes---------------------------------*/

exprivate int ndrx_tmq_file_storage_init(ndrx_tmq_storage_t *sw, 
    ndrx_tmq_qdisk_xa_cfg_t *p_tmq_cfg);
exprivate int ndrx_tmq_file_storage_uninit(ndrx_tmq_storage_t *sw);
exprivate void *ndrx_tmq_file_storage_list_start(ndrx_tmq_storage_t *sw, int mode);
exprivate int ndrx_tmq_file_storage_list_next(ndrx_tmq_storage_t *sw, 
    void *cursor, char *ref, size_t refsz);
exprivate int ndrx_tmq_file_storage_list_end(ndrx_tmq_storage_t *sw, void *cursor);
exprivate int ndrx_tmq_file_storage_prep_exists(ndrx_tmq_storage_t *sw, char *tmxid);
exprivate int ndrx_tmq_file_storage_read_block(ndrx_tmq_storage_t *sw,
    char *ref, char *buf, size_t bufsz, int mode);
exprivate int ndrx_tmq_file_storage_rollback_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl);
exprivate int ndrx_tmq_file_storage_commit_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl);
exprivate int ndrx_tmq_file_storage_prepare_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl);
exprivate int ndrx_tmq_file_storage_write_block(char *block, int len,
    char *cust_tmxid, int *int_diag);

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
    // Implementation of initialization
    // ...

    return EXSUCCEED;
}

// Function to uninitialize the storage interface
exprivate int ndrx_tmq_file_storage_uninit(ndrx_tmq_storage_t *sw)
{
    // Implementation of uninitialization
    // ...

    return EXSUCCEED;
}

// Function to list messages in the store
exprivate void *ndrx_tmq_file_storage_list_start(ndrx_tmq_storage_t *sw, int mode)
{
    // Implementation of list start
    // ...

    return NULL;  // Replace with actual cursor
}

exprivate int ndrx_tmq_file_storage_list_next(ndrx_tmq_storage_t *sw, 
    void *cursor, char *ref, size_t refsz)
{
    // Implementation of list next
    // ...

    return EXSUCCEED;
}

exprivate int ndrx_tmq_file_storage_list_end(ndrx_tmq_storage_t *sw, void *cursor)
{
    // Implementation of list end
    // ...

    return EXSUCCEED;
}

exprivate int ndrx_tmq_file_storage_prep_exists(ndrx_tmq_storage_t *sw, char *tmxid)
{
    // Implementation of prep exists
    // ...

    return EXSUCCEED;
}

exprivate int ndrx_tmq_file_storage_read_block(ndrx_tmq_storage_t *sw,
    char *ref, char *buf, size_t bufsz, int mode)
{
    // Implementation of read block
    // ...

    return 0;  // Replace with actual number of bytes read or EXFAIL
}

    exprivate int ndrx_tmq_file_storage_rollback_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl)
{
    // Implementation of rollback commands
    // ...

    return EXSUCCEED;
}

exprivate int ndrx_tmq_file_storage_commit_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl)
{
    // Implementation of commit commands
    // ...

    return EXSUCCEED;
}

exprivate int ndrx_tmq_file_storage_prepare_cmds(ndrx_tmq_storage_t *sw, qtran_log_t *p_tl)
{
    // Implementation of prepare commands
    // ...

    return EXSUCCEED;
}

exprivate int ndrx_tmq_file_storage_write_block(char *block, int len,
    char *cust_tmxid, int *int_diag)
{
    // Check for NULL pointers or invalid arguments
    if (block == NULL || len <= 0) {
        // Handle error
        return EXFAIL;
    }

    // Implementation of writing block to the store
    // ...

    return EXSUCCEED;
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
    static __thread char filename[2][PATH_MAX+1];
    
    snprintf(filename[slot], sizeof(filename[0]), "%s/%s-%03d", folder, 
        G_atmi_tls->qdisk_tls->filename_base, i);
    
    return filename[slot];
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
 *  this is needed to avoid infinte loop when tlog have gone async with disk
 * @param p_hdr
 * @param fname1 filename 1 to unlink (priority 1 - after this message is unblocked)
 * @param fname2 filename 2 to unlink (priority 2)
 * @param fcmd file cmd, if U, then fname1 & 2 is both unlinks, if R, then f1 src name, f2 dest name
 * @param tcmd transaction command
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_file(union tmq_upd_block *p_upd, char *fname1, 
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
    if (!tcmd->no_unlock && EXSUCCEED!=M_p_tmq_unlock_msg(p_upd))
    {
        ret=XAER_RMERR;
        goto out;
    }

out:
    return ret;
}

/**
 * Finalize files by update block
 * @param p_upd
 * @param fname1 filename1 to unlink
 * @param fname2 filename2 to unlink
 * @param fcmd file cmd
 * @param tcmd transaction command
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_files_upd(tmq_msg_upd_t *p_upd, char *fname1, 
        char *fname2, char fcmd, qtran_log_cmd_t *tcmd)
{
    union tmq_upd_block block;
    
    memset(&block, 0, sizeof(block));
    
    memcpy(&block.upd, p_upd, sizeof(*p_upd));
    
    return tmq_finalize_file(&block, fname1, fname2, fcmd, tcmd);
}

/**
 * Finalize files by header block
 * @param p_hdr
 * @param fname1 filename1 to unlink
 * @param fname2 filename2 to unlink
 * @param fcmd file commmand code
 * @param tcmd tran command
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_files_hdr(tmq_cmdheader_t *p_hdr, char *fname1, 
        char *fname2, char fcmd, qtran_log_cmd_t *tcmd)
{
    union tmq_upd_block block;
    
    memset(&block, 0, sizeof(block));
    memcpy(&block.hdr, p_hdr, sizeof(*p_hdr));
    
    return tmq_finalize_file(&block, fname1, fname2, fcmd, tcmd);
}

/**
 * Create required folders
 * @param xa_info root folder
 * @return EXSUCCEED/RM ERR
 */
expublic int xa_open_entry_mkdir(char *xa_info)
{
    int ret;
    /* The xa_info is directory, where to store the data...*/
    NDRX_STRNCPY(M_folder, xa_info, sizeof(M_folder)-2);
    M_folder[sizeof(M_folder)-1] = EXEOS;
    
    NDRX_LOG(log_info, "Q data directory: [%s]", xa_info);
    
    /* The xa_info is directory, where to store the data...*/
    NDRX_STRNCPY(M_folder_active, xa_info, sizeof(M_folder_active)-8);
    M_folder_active[sizeof(M_folder_active)-7] = EXEOS;
    NDRX_STRCAT_S(M_folder_active, sizeof(M_folder_active), "/active");
    
    NDRX_STRNCPY(M_folder_prepared, xa_info, sizeof(M_folder_prepared)-10);
    M_folder_prepared[sizeof(M_folder_prepared)-9] = EXEOS;
    NDRX_STRCAT_S(M_folder_prepared, sizeof(M_folder_prepared), "/prepared");
    
    NDRX_STRNCPY(M_folder_committed, xa_info, sizeof(M_folder_committed)-11);
    M_folder_committed[sizeof(M_folder_committed)-10] = EXEOS;
    NDRX_STRCAT_S(M_folder_committed, sizeof(M_folder_committed), "/committed");
    
    /* Test the directories */
    if (EXSUCCEED!=(ret=mkdir(M_folder, NDRX_DIR_PERM)) && ret!=EEXIST )
    {
        int err = errno;

        if (err!=EEXIST)
        {
            NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder, strerror(err));

            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", M_folder, strerror(err));
            return XAER_RMERR;
        }
        else
        {
            NDRX_LOG(log_info, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder, strerror(err));
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
            return XAER_RMERR;
        }
        else
        {
            NDRX_LOG(log_info, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_active, strerror(err));
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
            return XAER_RMERR;
        }
        else
        {
            NDRX_LOG(log_info, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_prepared, strerror(err));
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
            return XAER_RMERR;
        }
        else
        {
            NDRX_LOG(log_info, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_committed, strerror(err));
        }
    }
    
    NDRX_LOG(log_info, "Prepared M_folder=[%s]", M_folder);
    NDRX_LOG(log_info, "Prepared M_folder_active=[%s]", M_folder_active);
    NDRX_LOG(log_info, "Prepared M_folder_prepared=[%s]", M_folder_prepared);
    NDRX_LOG(log_info, "Prepared M_folder_committed=[%s]", M_folder_committed);
    
    
    return XA_OK;
}
/* vim: set ts=4 sw=4 et smartindent: */
