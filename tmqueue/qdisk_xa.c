/**
 * @brief Q XA Backend
 *   Prepare the folders & subfolders
 *   So we will have following directory structure:
 *   - active
 *   - prepared
 *   - committed
 *   Initially file will be named after the XID
 *   Once it becomes committed, it is named after message_id
 *   If we have a update to Q (try counter for example)
 *   Then we issue new transaction file with different command inside, but it contains
 *   the message_id
 *   Once do the commit and it is not and message, but update file, then
 *   we update message file.
 *   If the command is delete, then we unlink the file.
 *   Once Queue record is completed (rolled back or committed) we shall send ACK
 *   of COMMAND BLOCK back to queue server via TPCALL command to QSPACE server.
 *   this will allow to synchronize internal status of the messages.
 *   Initially (active) file is named is named after XID. When doing commit, it is
 *   renamed to msg_id.
 *   If we restore the system after the restart, then committed & prepare directory is scanned.
 *   If msg is committed + there is command in prepared, then it is marked as locked.
 *   When scanning prepared directory, we shall read the msg_id from files.
 *   We shall support multiple TMQs running over single Q space, but they each should,
 *   manage it's own set of queued messages.
 *   The queued file names shall contain [QSPACE].[SERVERID].[MSG_ID|XID]
 *   To post the updates to proper TMQ, it should advertise QSPACE.[SERVER_ID]
 *   In case of Active-Active env, servers only on node shall be run.
 *   On the other node we could put in standby qspace+server_id on the same shared dir.
 *
 * @file qdisk_xa.c
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
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <ndrstandard.h>
#include <nstopwatch.h>

#include <xa.h>
#include <atmi_int.h>
#include <unistd.h>

#include "userlog.h"
#include "tmqueue.h"
#include "nstdutil.h"
#include "Exfields.h"
#include "atmi_tls.h"
#include "tmqd.h"
#include <qcommon.h>
#include <xa_cmn.h>
#include <ubfutil.h>
#include "qtran.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic char ndrx_G_qspace[XATMI_SERVICE_NAME_LENGTH+1];   /**< Name of the queue space  */
expublic char ndrx_G_qspacesvc[XATMI_SERVICE_NAME_LENGTH+1];/**< real service name      */
/*---------------------------Statics------------------------------------*/

exprivate char M_folder[PATH_MAX+1] = {EXEOS}; /**< Where to store the q data         */
exprivate char M_folder_active[PATH_MAX+1] = {EXEOS}; /**< Active transactions        */
exprivate char M_folder_prepared[PATH_MAX+1] = {EXEOS}; /**< Prepared transactions    */
exprivate char M_folder_committed[PATH_MAX+1] = {EXEOS}; /**< Committed transactions  */
exprivate int volatile M_folder_set = EXFALSE;   /**< init flag                     */
exprivate MUTEX_LOCKDECL(M_folder_lock); /**< protect against race codition during path make*/
exprivate MUTEX_LOCKDECL(M_init);   /**< init lock      */

exprivate int M_is_tmqueue = EXFALSE;   /**< is this process a tmqueue ?        */

exprivate  int (*M_p_tmq_setup_cmdheader_dum)(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags);
exprivate int (*M_p_tmq_dum_add)(char *tmxid);
exprivate int (*M_p_tmq_unlock_msg)(union tmq_upd_block *b);
/*---------------------------Prototypes---------------------------------*/

expublic int xa_open_entry_stat(char *xa_info, int rmid, long flags);
expublic int xa_close_entry_stat(char *xa_info, int rmid, long flags);
expublic int xa_start_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_end_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_rollback_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_prepare_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_commit_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags);
expublic int xa_forget_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags);

expublic int xa_open_entry_dyn(char *xa_info, int rmid, long flags);
expublic int xa_close_entry_dyn(char *xa_info, int rmid, long flags);
expublic int xa_start_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_end_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_rollback_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_prepare_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_commit_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_recover_entry_dyn(XID *xid, long count, int rmid, long flags);
expublic int xa_forget_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_complete_entry_dyn(int *handle, int *retval, int rmid, long flags);

expublic int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
expublic int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
expublic int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags);
expublic int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags);

exprivate int read_tx_block(FILE *f, char *block, int len, char *fname, char *dbg_msg, 
        int *err, int *tmq_err);
exprivate int read_tx_from_file(char *fname, char *block, int len, int *err,
        int *tmq_err);

exprivate void dirent_free(struct dirent **namelist, int n);

exprivate int xa_rollback_entry_tmq(char *tmxid, long flags);
exprivate int xa_prepare_entry_tmq(char *tmxid, long flags);
exprivate int xa_commit_entry_tmq(char *tmxid, long flags);
exprivate int write_to_tx_file(char *block, int len, char *cust_tmxid, int *int_diag);

struct xa_switch_t ndrxqstatsw = 
{ 
    .name = "ndrxqstatsw",
    .flags = TMNOFLAGS,
    .version = 0,
    .xa_open_entry = xa_open_entry_stat,
    .xa_close_entry = xa_close_entry_stat,
    .xa_start_entry = xa_start_entry_stat,
    .xa_end_entry = xa_end_entry_stat,
    .xa_rollback_entry = xa_rollback_entry_stat,
    .xa_prepare_entry = xa_prepare_entry_stat,
    .xa_commit_entry = xa_commit_entry_stat,
    .xa_recover_entry = xa_recover_entry_stat,
    .xa_forget_entry = xa_forget_entry_stat,
    .xa_complete_entry = xa_complete_entry_stat
};

struct xa_switch_t ndrxqdynsw = 
{ 
    .name = "ndrxqdynsw",
    .flags = TMREGISTER,
    .version = 0,
    .xa_open_entry = xa_open_entry_dyn,
    .xa_close_entry = xa_close_entry_dyn,
    .xa_start_entry = xa_start_entry_dyn,
    .xa_end_entry = xa_end_entry_dyn,
    .xa_rollback_entry = xa_rollback_entry_dyn,
    .xa_prepare_entry = xa_prepare_entry_dyn,
    .xa_commit_entry = xa_commit_entry_dyn,
    .xa_recover_entry = xa_recover_entry_dyn,
    .xa_forget_entry = xa_forget_entry_dyn,
    .xa_complete_entry = xa_complete_entry_dyn
};

/**
 * Mark the current instance as part or not as part of tmqueue
 * @param setting EXTRUE/EXFALSE
 */
expublic void tmq_set_tmqueue(
    int setting
    , int (*p_tmq_setup_cmdheader_dum)(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags)
    , int (*p_tmq_dum_add)(char *tmxid)
    , int (*p_tmq_unlock_msg)(union tmq_upd_block *b)
)
{
    M_is_tmqueue = setting;
    M_p_tmq_setup_cmdheader_dum = p_tmq_setup_cmdheader_dum;
    M_p_tmq_dum_add = p_tmq_dum_add;
    M_p_tmq_unlock_msg = p_tmq_unlock_msg;
    
    NDRX_LOG(log_debug, "qdisk_xa config: M_is_tmqueue=%d "
        "M_p_tmq_setup_cmdheader_dum=%p M_p_tmq_dum_add=%p M_p_tmq_unlock_msg=%p",
        M_is_tmqueue, M_p_tmq_setup_cmdheader_dum, M_p_tmq_dum_add, M_p_tmq_unlock_msg);
}

/**
 * Set filename base
 * @param xid
 * @param rmid
 * @return 
 */
exprivate char *set_filename_base(XID *xid)
{
    atmi_xa_serialize_xid(xid, G_atmi_tls->qdisk_tls->filename_base);    
    return G_atmi_tls->qdisk_tls->filename_base;
}

/**
 * Set base xid
 * @param tmxid xid string
 * @return ptr to buffer
 */
exprivate char *set_filename_base_tmxid(char *tmxid)
{
    /* may overlap in dynamic reg mode */
    if (tmxid!=(char *)G_atmi_tls->qdisk_tls->filename_base)
    {
        NDRX_STRCPY_SAFE(G_atmi_tls->qdisk_tls->filename_base, tmxid);
    }

    return G_atmi_tls->qdisk_tls->filename_base;
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
 * Return max file number
 * @return 0 (no files), >=1 got something
 */
exprivate int get_filenames_max(void)
{
    int i=0;
    char filename_active[PATH_MAX+1];
    char filename_prepared[PATH_MAX+1];
    
    while(1)
    {
        snprintf(filename_active, sizeof(filename_active), "%s/%s-%03d", 
                M_folder_active, G_atmi_tls->qdisk_tls->filename_base, i+1);
        snprintf(filename_prepared, sizeof(filename_prepared), "%s/%s-%03d", 
                M_folder_prepared, G_atmi_tls->qdisk_tls->filename_base, i+1);
        NDRX_LOG(log_debug, "Testing act: [%s] prep: [%s]", filename_active, 
                filename_prepared);
        if (ndrx_file_exists(filename_active) || 
                ndrx_file_exists(filename_prepared))
        {
            i++;
        }
        else
        {
            break;
        }
    }
    
    NDRX_LOG(log_info, "max file names %d", i);
    return i;
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

    if (EXSUCCEED!=rename(f, t))
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
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_file(union tmq_upd_block *p_upd, char *fname1, 
        char *fname2, char fcmd)
{
    
    int ret = EXSUCCEED;
    BFLDOCC occ;
    char name1[PATH_MAX+1];
    char name2[PATH_MAX+1];
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

            if (EXSUCCEED!=unlink(files[occ]))
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

                if (EXSUCCEED!=ndrx_fsync_dsync(files[occ], G_atmi_env.xa_fsync_flags))
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

        if (EXSUCCEED!=rename(name1, name2))
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

        if (EXSUCCEED!=ndrx_fsync_dsync(name2, G_atmi_env.xa_fsync_flags))
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
    if (EXSUCCEED!=M_p_tmq_unlock_msg(p_upd))
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
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_files_upd(tmq_msg_upd_t *p_upd, char *fname1, 
        char *fname2, char fcmd)
{
    union tmq_upd_block block;
    
    memset(&block, 0, sizeof(block));
    
    memcpy(&block.upd, p_upd, sizeof(*p_upd));
    
    return tmq_finalize_file(&block, fname1, fname2, fcmd);
}

/**
 * Finalize files by header block
 * @param p_hdr
 * @param fname1 filename1 to unlink
 * @param fname2 filename2 to unlink
 * @param fcmd file commmand code
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tmq_finalize_files_hdr(tmq_cmdheader_t *p_hdr, char *fname1, 
        char *fname2, char fcmd)
{
    union tmq_upd_block block;
    
    memset(&block, 0, sizeof(block));
    memcpy(&block.hdr, p_hdr, sizeof(*p_hdr));
    
    return tmq_finalize_file(&block, fname1, fname2, fcmd);
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
    
    NDRX_LOG(log_error, "Q data directory: [%s]", xa_info);
    
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
        NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder, strerror(err));
        
        if (err!=EEXIST)
        {
            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", M_folder, strerror(err));
            return XAER_RMERR;
        }
    }
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_active, NDRX_DIR_PERM)) && ret!=EEXIST )
    {
        int err = errno;
        NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_active, strerror(err));
        
        if (err!=EEXIST)
        {
            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", M_folder_active, strerror(err));
            return XAER_RMERR;
        }
    }
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_prepared, NDRX_DIR_PERM)) && ret!=EEXIST )
    {
        int err = errno;
        NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_prepared, strerror(err));
        
        if (err!=EEXIST)
        {
            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", M_folder_prepared, strerror(err));
            return XAER_RMERR;
        }
    }
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_committed, NDRX_DIR_PERM)) && ret!=EEXIST )
    {
        int err = errno;
        NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_committed, strerror(err));
        
        if (err!=EEXIST)
        {
            userlog("xa_open_entry() Q driver: failed to create directory "
                    "[%s] - [%s]!", M_folder_committed, strerror(err));
            return XAER_RMERR;
        }
    }
    
    NDRX_LOG(log_info, "Prepared M_folder=[%s]", M_folder);
    NDRX_LOG(log_info, "Prepared M_folder_active=[%s]", M_folder_active);
    NDRX_LOG(log_info, "Prepared M_folder_prepared=[%s]", M_folder_prepared);
    NDRX_LOG(log_info, "Prepared M_folder_committed=[%s]", M_folder_committed);
    
    
    return XA_OK;
}

/**
 * Open API.
 * Now keeps the settings of the queue space too.
 * @param sw Current switch
 * @param xa_info New format: dir="/path_to_dir",qspace='SAMPLESPACE' (escaped)
 * @param rmid
 * @param flags
 * @return XA_ return codes
 */
expublic int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    int ret = XA_OK, err, i;
    static int first = EXTRUE;
    char *info_tmp = NULL;
    char *p, *val;
    
    /* mark that suspend no required by this resource... */
    if (first)
    {
        MUTEX_LOCK_V(M_init);
        if (first)
        {
            ndrx_xa_nosuspend(EXTRUE);
            first=EXFALSE;
        }
        MUTEX_UNLOCK_V(M_init);
    }
    
    if (G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_warn, "xa_open_entry() - already open!");
        return XA_OK;
    }
    
    G_atmi_tls->qdisk_tls=NDRX_FPMALLOC(sizeof(ndrx_qdisk_tls_t), 0);
            
    if (NULL==G_atmi_tls->qdisk_tls)
    {
        int err=errno;
        
        NDRX_LOG(log_warn, "xa_open_entry() - failed to malloc TLS data: %s",
                strerror(err));
        return XAER_RMERR;
    }

    G_atmi_tls->qdisk_tls->is_reg=EXFALSE;
    G_atmi_tls->qdisk_tls->filename_base[0]=EXEOS;
    G_atmi_tls->qdisk_tls->filename_active[0]=EXEOS;
    G_atmi_tls->qdisk_tls->filename_prepared[0]=EXEOS;

    G_atmi_tls->qdisk_tls->recover_namelist = NULL;
    G_atmi_tls->qdisk_tls->recover_open=EXFALSE;
    G_atmi_tls->qdisk_tls->recover_i=EXFAIL;
    G_atmi_tls->qdisk_tls->recover_last_loaded=EXFALSE;
            
    G_atmi_tls->qdisk_is_open = EXTRUE;
    G_atmi_tls->qdisk_rmid = rmid;
    
#define UNLOCK_OUT MUTEX_UNLOCK_V(M_folder_lock);\
                ret=XAER_RMERR;\
                goto out;
                
    /* Load only once? */
    if (!M_folder_set)
    {
        /* LOCK & Check */
        MUTEX_LOCK_V(M_folder_lock);
        
        if (!M_folder_set)
        {
            
            info_tmp = NDRX_STRDUP(xa_info);
            
            if (NULL==info_tmp)
            {
                err=errno;
                NDRX_LOG(log_error, "Failed to strdup: %s", strerror(err));
                userlog("Failed to strdup: %s", strerror(err));
                UNLOCK_OUT;
            }
            
#define ARGS_DELIM      ","
#define ARGS_QUOTE      "'\""
#define ARG_DIR         "datadir"
#define ARG_QSPACE      "qspace"
            
            /* preserve values in quotes... as atomic values */
            for (p = ndrx_strtokblk ( info_tmp, ARGS_DELIM, ARGS_QUOTE), i=0; 
                    NULL!=p;
                    p = ndrx_strtokblk (NULL, ARGS_DELIM, ARGS_QUOTE), i++)
            {
                if (NULL!=(val = strchr(p, '=')))
                {
                    *val = EXEOS;
                    val++;
                }
                
                /* set data dir. */
                if (0==strcmp(ARG_DIR, p))
                {
                    /* Do parse of the string... */
                    ret=xa_open_entry_mkdir(val);
                    
                    if (EXSUCCEED!=ret)
                    {
                        NDRX_LOG(log_error, "Failed to prepare data directory [%s]", val);
                        UNLOCK_OUT;
                    }   
                }
                else if (0==strcmp(ARG_QSPACE, p))
                {
                    NDRX_STRCPY_SAFE(ndrx_G_qspace, val);
                }
                /* all other ignored... */
            }
            
            if (EXEOS==ndrx_G_qspace[0])
            {
                NDRX_LOG(log_error, "[%s] setting not found in open string!", ARG_QSPACE);
                UNLOCK_OUT;
            }
            
            if (EXEOS==M_folder[0])
            {
                NDRX_LOG(log_error, "[%s] setting not found in open string!", ARG_DIR);
                UNLOCK_OUT;
            }
            
            snprintf(ndrx_G_qspacesvc, sizeof(ndrx_G_qspacesvc),
                    NDRX_SVC_QSPACE, ndrx_G_qspace);

            NDRX_LOG(log_debug, "Qspace set to: [%s]", ndrx_G_qspace);
            NDRX_LOG(log_debug, "Qspace svc set to: [%s]", ndrx_G_qspacesvc);
            M_folder_set=EXTRUE;
            
        }   
        MUTEX_UNLOCK_V(M_folder_lock);
        
    }
out:
    
    if (NULL!=info_tmp)
    {
        NDRX_FREE(info_tmp);
    }
    return ret;
}
/**
 * Close entry
 * @param sw
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    NDRX_LOG(log_error, "xa_close_entry() called");
    
    if (NULL!=G_atmi_tls->qdisk_tls)
    {
        NDRX_FPFREE(G_atmi_tls->qdisk_tls);
        G_atmi_tls->qdisk_tls=NULL;
    }
    
    G_atmi_tls->qdisk_is_open = EXFALSE;
    
    return XA_OK;
}

/**
 * TMQ queue internal version of transaction starting
 * @return XA error code
 */
exprivate int xa_start_entry_tmq(char *tmxid, long flags)
{
    int locke = EXFALSE;
    qtran_log_t * p_tl = NULL;
    int ret = XA_OK;
    
    set_filename_base_tmxid(tmxid);
    
    /* Firstly try to locate the tran */
    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, &locke);

    if ( (flags & TMJOIN) || (flags & TMRESUME) )
    {
        if (NULL==p_tl && !locke)
        {
            NDRX_LOG(log_error, "Xid [%s] TMJOIN/TMRESUME but tran not found",
                    tmxid);
            ret = XAER_NOTA;
            goto out;
        }

        NDRX_LOG(log_info, "Xid [%s] join OK", tmxid);
    }
    else
    {
        /* this is new tran */
        if (NULL!=p_tl || locke)
        {
            NDRX_LOG(log_error, "Cannot start Xid [%s] already in progress",
                    tmxid);
            ret = XAER_DUPID;
            goto out;
        }
        else
        {
            
            if (EXSUCCEED!=tmq_log_start(tmxid))
            {
                NDRX_LOG(log_error, "Failed to start transaction for tmxid [%s]",
                        tmxid);
                ret = XAER_RMERR;
                goto out;
            }
            NDRX_LOG(log_info, "Queue transaction Xid [%s] started OK", tmxid);
        }
    }
    
out:
    
    if (NULL!=p_tl && !locke)
    {
        tmq_log_unlock(p_tl);
    }

    return ret;
}

/**
 * Minimal XA call to tmqueue, used by external processes.
 * @param tmxid serialized xid
 * @param cmd TMQ_CMD* command code
 * @return XA error code
 */
exprivate int ndrx_xa_qminicall(char *tmxid, char cmd)
{
    long rsplen;
    UBFH *p_ub = NULL;
    long ret = XA_OK;
   
    p_ub = (UBFH *)tpalloc("UBF", "", 1024 );
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to allocate notif buffer");
        ret = XAER_RMERR;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup EX_QMSGID!");
        ret = XAER_RMERR;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, TMXID, 0, tmxid, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMXID!");
        ret = XAER_RMERR;
        goto out;
    }
    
    NDRX_LOG(log_info, "Calling QSPACE [%s] for tmxid [%s], command %c",
                ndrx_G_qspacesvc, tmxid, cmd);
    
    ndrx_debug_dump_UBF(log_info, "calling Q space with", p_ub);

    if (EXFAIL == tpcall(ndrx_G_qspacesvc, (char *)p_ub, 0L, (char **)&p_ub, 
        &rsplen, TPNOTRAN))
    {
        NDRX_LOG(log_error, "%s failed: %s", ndrx_G_qspacesvc, tpstrerror(tperrno));
        
        /* best guess -> not available */
        ret = XAER_RMFAIL;
        
        /* anyway if have detailed response, use bellow. */
    }
    
    ndrx_debug_dump_UBF(log_info, "Reply from RM", p_ub);
    
    /* try to get the result of the OP */
    if (Bpres(p_ub, TMTXRMERRCODE, 0) &&
            EXSUCCEED!=Bget(p_ub, TMTXRMERRCODE, 0, (char *)&ret, 0L))
    {
        NDRX_LOG(log_debug, "Failed to get TMTXRMERRCODE: %s", Bstrerror(Berror));
        ret = XAER_RMERR;
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    NDRX_LOG(log_info, "returns %d", ret);
    
    return ret;
}

/**
 * Serve the transaction call
 * @param p_ub UBF buffer
 * @param cmd tmq command code
 * @return XA error code
 */
expublic int ndrx_xa_qminiservce(UBFH *p_ub, char cmd)
{
    long ret = XA_OK;
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1];
    
    BFLDLEN len = sizeof(tmxid);
    
    if (EXSUCCEED!=Bget(p_ub, TMXID, 0, tmxid, &len))
    {
        NDRX_LOG(log_error, "Failed to get TMXID!");
        ret = XAER_INVAL;
        goto out;
    }
    
    switch (cmd)
    {
        case TMQ_CMD_STARTTRAN:
            ret = xa_start_entry_tmq(tmxid, 0);
            break;
        case TMQ_CMD_ABORTTRAN:
            ret = xa_rollback_entry_tmq(tmxid, 0);
            break;
        case TMQ_CMD_PREPARETRAN:
            ret = xa_prepare_entry_tmq(tmxid, 0);
            break;
        case TMQ_CMD_COMMITRAN:
            ret = xa_commit_entry_tmq(tmxid, 0);
            break;
        default:
            NDRX_LOG(log_error, "Invalid command code [%c]", cmd);
            ret = XAER_INVAL;
            break;
    }
    
out:
            
    NDRX_LOG(log_info, "returns XA status: %d", ret);
    
    if (EXSUCCEED!=Bchg(p_ub, TMTXRMERRCODE, 0, (char *)&ret, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMTXRMERRCODE: %s", 
                Bstrerror(Berror));
        ret = XAER_RMERR;
    }

    return ret;
}


/**
 * Set the file name of transaciton file (the base)
 * If exists and join - ok. Otherwise fail.
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *tmxid;
    int ret = XA_OK;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_start_entry() - XA not open!");
        ret = XAER_RMERR;
        goto out;
    }
    
    tmxid = set_filename_base(xid);
    
    /* if we are tmq -> for join, perform lookup.
     * for start check, if tran exists -> error, if not exists, start
     * if doing from other process call the tmqueue for start/join (just chk)
     */
    
    if (M_is_tmqueue)
    {
        ret = xa_start_entry_tmq(tmxid, flags);
    }
    else if (! ( (flags & TMJOIN) || (flags & TMRESUME) ))
    {
        /* in case if no join from external process
         * request the tmsrv to start the transaction
         * also we are interested in return code
         */
        ret=ndrx_xa_qminicall(tmxid, TMQ_CMD_STARTTRAN);
    }
    
out:

    return ret;
}

/**
 * XA call end entry. Nothing special to do here.
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_end_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (G_atmi_tls->qdisk_tls->is_reg)
    {
        if (EXSUCCEED!=ax_unreg(rmid, 0))
        {
            NDRX_LOG(log_error, "ERROR! xa_end_entry() - "
                    "ax_unreg() fail!");
            return XAER_RMERR;
        }
        
        G_atmi_tls->qdisk_tls->is_reg = EXFALSE;
    }
    
    /* no special processing for tran */
    
out:

    return XA_OK;
}

/**
 * Internal TMQ version of rollback
 * @param tmxid xid serialized
 * @param flags any flags
 * @return  XA error code
 */
exprivate int xa_rollback_entry_tmq(char *tmxid, long flags)
{
    union tmq_upd_block b;
    char *fn = "xa_rollback_entry_tmq";
    qtran_log_cmd_t *el, *elt;
    
    int locke = EXFALSE;
    qtran_log_t * p_tl = NULL;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_rollback_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    set_filename_base_tmxid(tmxid);
    
    /* Firstly try to locate the tran */
    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, &locke);
    
    if (NULL==p_tl)
    {
        if (locke)
        {
            NDRX_LOG(log_error, "Q transaction [%s] locked", tmxid);
            return XAER_RMFAIL;
        }
        else
        {
            NDRX_LOG(log_error, "Q transaction [%s] does not exists", tmxid);
            return XAER_NOTA;
        }
    }
    
    p_tl->txstage = XA_TX_STAGE_ABORTING;
    p_tl->is_abort_only=EXTRUE;
    
    /* Process files according to the log... */
    DL_FOREACH_SAFE(p_tl->cmds, el, elt)
    {
        char *fname = NULL;
        
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
            NDRX_LOG(log_info, "%s: issuing delete command...", fn);
            b.hdr.command_code = TMQ_STORCMD_DEL;
        }
        else
        {
            NDRX_LOG(log_info, "%s: unlock command...", fn);

            b.hdr.command_code = TMQ_STORCMD_UNLOCK;
        }

        /* if tmq server is not working at this moment
         * then we cannot complete the rollback
         */
        if (EXSUCCEED!=tmq_finalize_files_hdr(&b.hdr, fname, 
                NULL, TMQ_FILECMD_UNLINK))
        {
            NDRX_LOG(log_error, "Failed to unlink [%s]", fname);
            continue;
        }
        
        /* Finally remove command entry */
        DL_DELETE(p_tl->cmds, el);
        NDRX_FPFREE(el);
        NDRX_LOG(log_debug, "Abort [%s] OK", fname);
        
    }
    
    if (NULL!=p_tl->cmds)
    {
        NDRX_LOG(log_error, "Failed to abort Q transaction [%s] -> commands exists",
                tmxid);
        return XAER_RMERR;
    }
    
    /* delete message from log */
    tmq_remove_logfree(p_tl, EXTRUE);
    
    return XA_OK;
}

/**
 * Process local or remote call
 * @return 
 */
expublic int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *tmxid;
    int ret = XA_OK;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_rollback_entry() - XA not open!");
        ret = XAER_RMERR;
        goto out;
    }
    
    tmxid = set_filename_base(xid);
    
    if (M_is_tmqueue)
    {
        ret = xa_rollback_entry_tmq(tmxid, flags);
    }
    else
    {
        ret=ndrx_xa_qminicall(tmxid, TMQ_CMD_ABORTTRAN);
    }
    
out:

    return ret;
}

/**
 * Prepare transactions (according to TL log)
 * @return XA status
 */
exprivate int xa_prepare_entry_tmq(char *tmxid, long flags)
{
    int locke = EXFALSE;
    qtran_log_t * p_tl = NULL;
    int ret = XA_OK;
    int did_move=EXFALSE;
    qtran_log_cmd_t *el, *elt;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_prepare_entry_tmq() - XA not open!");
        ret=XAER_RMERR;
        goto out;
    }
    
    set_filename_base_tmxid(tmxid);
    
    /* Firstly try to locate the tran */
    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, &locke);
    
    if (NULL==p_tl)
    {
        if (locke)
        {
            NDRX_LOG(log_error, "Q transaction [%s] locked", tmxid);
            ret=XAER_RMFAIL;
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Q transaction [%s] does not exists", tmxid);
            ret=XAER_NOTA;
            goto out;
        }
    }
    
    if (p_tl->is_abort_only)
    {
        NDRX_LOG(log_error, "Q transaction [%s] is abort only!", tmxid);
        ret = XAER_RMERR;
        goto out;
    }
    
    if (XA_TX_STAGE_ACTIVE!=p_tl->txstage)
    {
        NDRX_LOG(log_error, "Q transaction [%s] expected stage %hd (active) got %hd",
                tmxid, XA_TX_STAGE_ACTIVE, p_tl->txstage);
        
        ret = XAER_RMERR;
        p_tl->is_abort_only=EXTRUE;
        goto out;
    }
    
    p_tl->txstage = XA_TX_STAGE_PREPARING;
    
    /* TODO:  
     * Read active records -> as normal. If file cannot be read, create dummy
     * record for given tran. And thus if having active record or prepared+active 
     * at startup will automatically mean that transaction is abort-only. 
     * 
     * At the startup, all abort-only transaction shall be aborted
     * 
     * If doing prepare with empty RO transaction, inject dummy command prior
     * (transaction marking) so that at restart we have transaction log and
     * thus we can respond commit OK, thought might be optional as TMSRV
     * prepared resources with XAER_NOTA transaction would assume that committed OK
     */
    
    if (NULL==p_tl->cmds)
    {
        /* do not write the file, as we will use existing on disk */
        if (EXSUCCEED!=M_p_tmq_dum_add(p_tl->tmxid))
        {
            ret = XAER_RMERR;
            p_tl->is_abort_only=EXTRUE;
            goto out;
        }
        
        NDRX_LOG(log_debug, "Dummy marker added");
        
    }
    
    /* process command by command to stage to prepared ... */
    DL_FOREACH_SAFE(p_tl->cmds, el, elt)
    {
        if (EXSUCCEED!=file_move(el->seqno, M_folder_active, M_folder_prepared))
        {
            NDRX_LOG(log_error, "Q tran tmxid [%s] seq %d failed to prepare (file move)",
                    tmxid, el->seqno);
            p_tl->is_abort_only=EXTRUE;
            ret=XAER_RMERR;
            goto out;
        }
        
        el->cmd_status = XA_RM_STATUS_PREP;
        NDRX_LOG(log_info, "tmxid [%s] seq %d prepared OK", tmxid, el->seqno);
        did_move=EXTRUE;
    }
    
    if (did_move)
    {
        /* sync the  */
        if (EXSUCCEED!=ndrx_fsync_dsync(M_folder_prepared, G_atmi_env.xa_fsync_flags))
        {
            NDRX_LOG(log_error, "Failed to dsync [%s]", M_folder_prepared);
            
            /* mark transaction for abort only! */
            p_tl->is_abort_only=EXTRUE;
            ret=XAER_RMERR;
            goto out;
        }
    }
    
    /* If all OK, switch transaction to prepared */
    p_tl->txstage = XA_TX_STAGE_PREPARED;
    
out:
            
    if (NULL!=p_tl && !locke)
    {
        tmq_log_unlock(p_tl);
    }

    return ret;
}

/**
 * XA prepare entry call
 * Basically we move all messages for active to prepared folder (still named by
 * xid_str)
 * 
 * @return 
 */
expublic int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *tmxid;
    int ret = XA_OK;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_prepare_entry() - XA not open!");
        ret = XAER_RMERR;
        goto out;
    }
    
    tmxid = set_filename_base(xid);
    
    if (M_is_tmqueue)
    {
        ret = xa_prepare_entry_tmq(tmxid, flags);
    }
    else
    {
        ret=ndrx_xa_qminicall(tmxid, TMQ_CMD_PREPARETRAN);
    }
    
out:

    return ret;
}

/**
 * Commit the transaction.
 * Move messages from prepared folder to files names with msgid
 * @param tmxid serialized branch xid
 * @param flags not used
 * @return XA error code
 */
exprivate int xa_commit_entry_tmq(char *tmxid, long flags)
{
    char msgid_str[TMMSGIDLEN_STR+1];
    FILE *f = NULL;
    char *fname;
    char *fname_msg;
    int err, tmq_err;
    int locke = EXFALSE;
    qtran_log_t * p_tl = NULL;
    int ret = XA_OK;
    qtran_log_cmd_t *el, *elt;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - XA not open!");
        return XAER_RMERR;
    }

    set_filename_base_tmxid(tmxid);
    
    /* Firstly try to locate the tran */
    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, &locke);
    
    if (NULL==p_tl)
    {
        if (locke)
        {
            NDRX_LOG(log_error, "Q transaction [%s] locked", tmxid);
            ret=XAER_RMFAIL;
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Q transaction [%s] does not exists", tmxid);
            ret=XAER_NOTA;
            goto out;
        }
    }
    
    if (p_tl->is_abort_only)
    {
        NDRX_LOG(log_error, "Q transaction [%s] is abort only!", tmxid);
        ret = XAER_RMERR;
        goto out;
    }
    
    if (XA_TX_STAGE_PREPARED!=p_tl->txstage)
    {
        NDRX_LOG(log_error, "Q transaction [%s] expected stage %hd (prepared) got %hd",
                tmxid, XA_TX_STAGE_PREPARED, p_tl->txstage);
        
        ret = XAER_RMERR;
        p_tl->is_abort_only=EXTRUE;
        goto out;
    }
    
    p_tl->txstage = XA_TX_STAGE_COMMITTING;
    
     /* process command by command to stage to prepared ... */
    DL_FOREACH_SAFE(p_tl->cmds, el, elt)
    {
        /* run on prepared folder */
        fname = get_filename_i(el->seqno, M_folder_prepared, 0);
            
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
            
            if (EXSUCCEED!=tmq_finalize_files_hdr(&el->b.hdr, fname,
                    to_filename, TMQ_FILECMD_RENAME))
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
            
            if (NULL==(f = NDRX_FOPEN(fname_msg, "r+b")))
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
            if (sizeof(msg_to_upd)!=(ret_len=fwrite((char *)&msg_to_upd, 1, 
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
            
            if (EXSUCCEED!=tmq_finalize_files_upd(&el->b.upd, fname, NULL,
                    TMQ_FILECMD_UNLINK))
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
            if (EXSUCCEED!=tmq_finalize_files_hdr(&el->b.hdr, fname_msg, fname, 
                    TMQ_FILECMD_UNLINK))
            {
                ret = XAER_RMFAIL;
                goto out;
            }
        }
        else if (TMQ_STORCMD_DUM == el->b.hdr.command_code)
        {
            NDRX_LOG(log_info, "Dummy command to remove: [%s]", fname);
            
            /* Remove the message (it must be marked for delete)
             */
            if (EXSUCCEED!=tmq_finalize_files_hdr(&el->b.hdr, fname, NULL, 
                    TMQ_FILECMD_UNLINK))
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
        NDRX_LOG(log_debug, "Commit [%s] sequence OK", tmxid, el->seqno);
        DL_DELETE(p_tl->cmds, el);
        NDRX_FPFREE(el);
        
    }
    
    /* delete message from log */
    tmq_remove_logfree(p_tl, EXTRUE);
    p_tl = NULL;
    
    NDRX_LOG(log_info, "tmxid [%s] all sequences committed ok", tmxid);
    
out:

    NDRX_LOG(log_info, "Commit returns %d", ret);

    /* unlock if tran is still alive */
    if (NULL!=p_tl && !locke)
    {
        tmq_log_unlock(p_tl);
    }

    return ret;
}


/**
 * Local or remove transaction commit
 * @return XA error 
 */
expublic int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *tmxid;
    int ret = XA_OK;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - XA not open!");
        ret = XAER_RMERR;
        goto out;
    }
    
    tmxid = set_filename_base(xid);
    
    if (M_is_tmqueue)
    {
        ret = xa_commit_entry_tmq(tmxid, flags);
    }
    else
    {
        ret=ndrx_xa_qminicall(tmxid, TMQ_CMD_COMMITRAN);
    }
    
out:

    return ret;
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
    if (NULL==(f = NDRX_FOPEN(fname, "r+b")))
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
    if (G_atmi_env.test_qdisk_write_fail || LEN!=(ret_len=fwrite( ((char *)PTR) + OFFSET, 1, LEN, f)))\
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
    if (EXSUCCEED!=fflush(f))\
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
    
    if (NULL==(f = NDRX_FOPEN(G_atmi_tls->qdisk_tls->filename_active, mode_str)))
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
    if (EXSUCCEED!=ndrx_fsync_fsync(f, G_atmi_env.xa_fsync_flags))
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
            unlink(G_atmi_tls->qdisk_tls->filename_active);
        }
        
        NDRX_FCLOSE(f);
    }

    /* mark abort only, if hade issues with disk */
    if (EXSUCCEED!=ret && NULL!=tmxid)
    {
        tmq_log_set_abort_only(tmxid);
    }

    return ret;
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
 * Write the message data to TX file
 * @param msg message (the structure is projected on larger memory block to fit in the whole msg
 * @param int_diag internal diagnostics, flags
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_write_cmd_newmsg(tmq_msg_t *msg, int *int_diag)
{
    int ret = EXSUCCEED;
    char tmp[TMMSGIDLEN_STR+1];
    size_t len;
    /*
    uint64_t lockt =  msg->lockthreadid;
    
     do not want to lock be written out to files: 
    msg->lockthreadid = 0;
    NO NO NO !!!! We still need a lock!
    */
    
     /* detect the actual len... */
    len = tmq_get_block_len((char *)msg);
    
    NDRX_DUMP(log_debug, "Writing new message to disk", 
                (char *)msg, len);
    
    if (EXSUCCEED!=write_to_tx_file((char *)msg, len, NULL, int_diag))
    {
        NDRX_LOG(log_error, "tmq_storage_write_cmd_newmsg() failed for msg %s", 
                tmq_msgid_serialize(msg->hdr.msgid, tmp));
        EXFAIL_OUT(ret);
    }
    /*
    msg->lockthreadid = lockt;
     */
    
    NDRX_LOG(log_info, "Message [%s] written ok to active TX file", 
            tmq_msgid_serialize(msg->hdr.msgid, tmp));
    
out:

    return ret;
}

/**
 * Get the data size from any block
 * @param data generic data block 
 * @return data size or 0 on error (should not happen anyway)
 */
expublic size_t tmq_get_block_len(char *data)
{
    size_t ret = 0;
    
    tmq_cmdheader_t *p_hdr = (tmq_cmdheader_t *)data;
    tmq_msg_t *p_msg;
    tmq_msg_del_t *p_del;
    tmq_msg_upd_t *p_upd;
    tmq_msg_unl_t *p_unl;
    tmq_msg_dum_t *p_dum;
    
    switch (p_hdr->command_code)
    {
        case TMQ_STORCMD_NEWMSG:
            p_msg = (tmq_msg_t *)data;
            ret = sizeof(*p_msg) + p_msg->len;
            break;
        case TMQ_STORCMD_UPD:
            ret = sizeof(*p_upd);
            break;
        case TMQ_STORCMD_DEL:
            ret = sizeof(*p_del);
            break;
        case TMQ_STORCMD_UNLOCK:
            ret = sizeof(*p_unl);
            break;
        case TMQ_STORCMD_DUM:
            ret = sizeof(*p_dum);
            break;
        default:
            NDRX_LOG(log_error, "Unknown command code: %c", p_hdr->command_code);
            break;
    }
    
    return ret;
}

/**
 * Delete/Update message block write
 * @param p_block ptr to union of commands
 * @param cust_tmxid custom transaction id (not part of global tran)
 * @param int_diag internal diagnostics, flags
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_write_cmd_block(char *data, char *descr, char *cust_tmxid, int *int_diag)
{
    int ret = EXSUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    size_t len;
    tmq_cmdheader_t *p_hdr = (tmq_cmdheader_t *)data;
    
    /* detect the actual len... */
    len = tmq_get_block_len((char *)data);
    
    NDRX_LOG(log_info, "Writing command block: %s msg [%s]", descr, 
            tmq_msgid_serialize(p_hdr->msgid, msgid_str));
    
    NDRX_DUMP(log_debug, "Writing command block to disk", 
                (char *)data, len);
    
    if (EXSUCCEED!=write_to_tx_file((char *)data, len, cust_tmxid, int_diag))
    {
        NDRX_LOG(log_error, "tmq_storage_write_cmd_block() failed for msg %s", 
                tmq_msgid_serialize(p_hdr->msgid, msgid_str));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;

}

/**
 * Return msgid from filename
 * @param filename_in
 * @param msgid_out
 * @return SUCCEED/FAIL
 */
exprivate int tmq_get_msgid_from_filename(char *filename_in, char *msgid_out)
{
    int ret = EXSUCCEED;
    
    tmq_msgid_deserialize(filename_in, msgid_out);
    
out:
    return ret;    
}

/** continue with dirent free */
#define DIRENT_CONTINUE \
            NDRX_FREE(namelist[n]);\
            continue;\
/**
 * Restore messages from storage device.
 * TODO: File naming include 03d so that multiple tasks per file sorts alphabetically.
 * Any active transactions get's aborted automatically.
 * @param process_block callback function to process the data block
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_get_blocks(int (*process_block)(char *tmxid, 
        union tmq_block **p_block, int state, int seqno), short nodeid, short srvid)
{
    int ret = EXSUCCEED;
    struct dirent **namelist = NULL;
    int n, seqno;
    int j;
    char *p;
    union tmq_block *p_block = NULL;
    FILE *f = NULL;
    char filename[PATH_MAX+1];
    char tmxid[PATH_MAX+1];
    int err, tmq_err;
    int read;
    int state;
    qtran_log_t *p_tl;
    
    /* prepared & active messages are all locked
     * Also if delete command is found in active, this means that committed
     * message is locked. 
     * In case if there was concurrent tmsrv operation, then it might move file
     * from active to prepare and we have finished with prepared, and start to
     * scan active. Thus we might not see this file. Thus to solve this issue,
     * the prepared folder is scanned twice. In the second time only markings
     * are put that message is busy.
     */
    char *folders[] = {M_folder_committed, M_folder_prepared, M_folder_active};
    short msg_nodeid, msg_srvid;
    char msgid[TMMSGIDLEN];
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! tmq_storage_get_blocks() - XA not open!");
        return XAER_RMERR;
    }
    
    for (j = 0; j < N_DIM(folders); j++)
    {
        
        switch (j)
        {
            case 0:
                /* committed */
                state=TMQ_TXSTATE_COMMITTED;
                break;
            case 1:
            case 3:
                /* prepared */
                state=TMQ_TXSTATE_PREPARED;
                break;
            case 2:
                /* active */
                state=TMQ_TXSTATE_ACTIVE;
                break;
        }
        
        n = scandir(folders[j], &namelist, 0, alphasort);
        if (n < 0)
        {
           NDRX_LOG(log_error, "Failed to scan q directory [%s]: %s", 
                   folders[j], strerror(errno));
           EXFAIL_OUT(ret);
        }
        
        while (n--)
        {
            if (0==strcmp(namelist[n]->d_name, ".") || 
                0==strcmp(namelist[n]->d_name, ".."))
            {
                DIRENT_CONTINUE;
            }

            /* filter nodeid & serverid from the filename... */
            if (0==j) /*  For committed folder we can detect stuff from filename */
            {
                /* early filter... */
                if (EXSUCCEED!=tmq_get_msgid_from_filename(namelist[n]->d_name, msgid))
                {
                    EXFAIL_OUT(ret);
                }
                
                tmq_msgid_get_info(msgid, &msg_nodeid, &msg_srvid);
                
                NDRX_LOG(log_info, "our nodeid/srvid %hd/%hd msg: %hd/%hd",
                        nodeid, srvid, msg_nodeid, msg_srvid);

                if (nodeid!=msg_nodeid || srvid!=msg_srvid)
                {
                    NDRX_LOG(log_warn, "our nodeid/srvid %hd/%hd msg: %hd/%hd - IGNORE",
                        nodeid, srvid, msg_nodeid, msg_srvid);
                    DIRENT_CONTINUE;
                }
            }
            
            snprintf(filename, sizeof(filename), "%s/%s", folders[j], 
                    namelist[n]->d_name);
            
            
            /* In case of active or prepared messages,
             * extract the sequence number and create transaction
             * Active messages we do not as there might be incomplete
             * contents and really what only matters is to rollback
             * abort only transactions at startup.
             */
            NDRX_LOG(log_warn, "Loading [%s]", filename);
            
            /* Read header */            
            if (NULL==(p_block = NDRX_MALLOC(sizeof(*p_block))))
            {
                NDRX_LOG(log_error, "Failed to alloc [%s]: %s", 
                   filename, strerror(errno));
                EXFAIL_OUT(ret);
            }
            
            /* read only if it is committed or prepared */
            if (j>0)
            {
                NDRX_STRCPY_SAFE(tmxid, namelist[n]->d_name);
                
                p = strrchr(tmxid, '-');
                
                if (NULL==p)
                {
                    NDRX_LOG(log_error, "Invalid file name [%s] missing dash!", 
                            filename);
                    userlog("Invalid file name [%s] missing dash!", 
                            filename);
                    NDRX_FREE((char *)p_block);
                    p_block=NULL;
                    DIRENT_CONTINUE;
                }
                
                *p=EXEOS;
                p++;
                
                seqno = atoi(p);
                
                /* get or create a transaction! */
                p_tl = tmq_log_start_or_get(tmxid);
                
                if (NULL==p_tl)
                {
                    NDRX_LOG(log_error, "Failed to get transaction object for [%s] seqno %d",
                            tmxid, seqno);
                    NDRX_FREE((char *)p_block);
                    p_block=NULL;
                    EXFAIL_OUT(ret);
                }
                
                if (2==j)
                {
                    /* mark transaction as abort only! */
                    p_tl->is_abort_only=EXTRUE;
                    p_tl->txstage=XA_TX_STAGE_ACTIVE;
                }
                else
                {
                    p_tl->txstage=XA_TX_STAGE_PREPARED;
                }
                
                /* unlock tran */
                tmq_log_unlock(p_tl);
            }

            if (j<2)
            {
                if (NULL==(f=NDRX_FOPEN(filename, "rb")))
                {
                    err = errno;
                    NDRX_LOG(log_error, "Failed to open for read [%s]: %s", 
                       filename, strerror(err));
                    userlog("Failed to open for read [%s]: %s", 
                       filename, strerror(err));
                    NDRX_FREE((char *)p_block);
                    p_block=NULL;

                    EXFAIL_OUT(ret);
                }

                /* here we read maximum header size.
                 * For smaller messages (fixed struct messages) all data is read
                 */
                if (EXFAIL==(read=read_tx_block(f, (char *)p_block, sizeof(*p_block), 
                        filename, "tmq_storage_get_blocks", &err, &tmq_err)))
                {
                    NDRX_LOG(log_error, "ERROR! Failed to read [%s] hdr (%d bytes) tmqerr: %d: %s - cannot start, resolve manually", 
                       filename, sizeof(*p_block), tmq_err, (err==0?"EOF":strerror(err)) );
                    userlog("ERROR! Failed to read [%s] hdr (%d bytes) tmqerr: %d: %s - cannot start, resolve manually", 
                       filename, sizeof(*p_block), tmq_err, (err==0?"EOF":strerror(err)) );

                    /* skip & continue with next */
                    NDRX_FCLOSE(f);
                    f=NULL;

                    NDRX_FREE((char *)p_block);
                    p_block = NULL;

                    EXFAIL_OUT(ret);
                }

                /* Late filter 
                 * Not sure what will happen if file will be processed/removed
                 * by other server if for example we boot up...read the folder
                 * but other server on same qspace/folder will remove the file
                 * So better use different folder for each server...!
                 */
                if (nodeid!=p_block->hdr.nodeid || srvid!=p_block->hdr.srvid)
                {
                    NDRX_LOG(log_warn, "our nodeid/srvid %hd/%hd msg: %hd/%hd - IGNORE",
                        nodeid, srvid, p_block->hdr.nodeid, p_block->hdr.srvid);

                    NDRX_FREE((char *)p_block);
                    p_block = NULL;
                    NDRX_FCLOSE(f);

                    DIRENT_CONTINUE;
                }

                NDRX_DUMP(log_debug, "Got command block", p_block, read);

                /* if it is message, the re-alloc  */
                if (TMQ_STORCMD_NEWMSG==p_block->hdr.command_code)
                {
                    int bytes_extra;
                    int bytes_to_read;
                    if (NULL==(p_block = NDRX_REALLOC(p_block, sizeof(tmq_msg_t) + p_block->msg.len)))
                    {
                        NDRX_LOG(log_error, "Failed to alloc [%d]: %s", 
                           (sizeof(tmq_msg_t) + p_block->msg.len), strerror(errno));
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
                    bytes_extra = sizeof(*p_block)-EXOFFSET(tmq_msg_t, msg);
                    bytes_to_read = p_block->msg.len - bytes_extra;

                    NDRX_LOG(log_info, "bytes_extra=%d bytes_to_read=%d", 
                            bytes_extra, bytes_to_read);

                    if (bytes_to_read > 0)
                    {
                        if (EXFAIL==read_tx_block(f, 
                                p_block->msg.msg+bytes_extra, 
                                bytes_to_read, filename, "tmq_storage_get_blocks 2", &err, &tmq_err))
                        {
                            NDRX_LOG(log_error, "ERROR! Failed to read [%s] %d bytes: %s - cannot start, resolve manually", 
                               filename, bytes_to_read, strerror(err));

                            userlog("ERROR! Failed to read [%s] %d bytes: %s - cannot start, resolve manually", 
                                    filename, bytes_to_read, strerror(err));
                                            /* skip & continue with next */
                            NDRX_FCLOSE(f);
                            f=NULL;
                            NDRX_FREE((char *)p_block);
                            p_block = NULL;

                            EXFAIL_OUT(ret);
                        }
                    }
                    else
                    {
                        NDRX_LOG(log_info, "Full message already read by command block!");
                    }

                    /* any message not committed automatically means locked */
                    if (0!=j)
                    {
                        /* if message is active or prepared, then message is locked */
                        p_block->msg.lockthreadid = ndrx_gettid();
                    }
                    else
                    {
                        p_block->msg.lockthreadid = 0;
                    }

                    NDRX_DUMP(6, "Read message from disk", 
                            p_block->msg.msg, p_block->msg.len);
                }

                NDRX_FCLOSE(f);
                f=NULL;
            }
            else
            {
                /* just create dummy entry for active transactions */
                M_p_tmq_setup_cmdheader_dum(&p_block->hdr, NULL, tpgetnodeid(), 
                        0, ndrx_G_qspace, 0);
                p_block->hdr.command_code = TMQ_STORCMD_DUM;
            }
            
            /* Process message block 
             * It is up to caller to free the mem & make null
             */
            if (EXSUCCEED!=process_block(tmxid, &p_block, state, seqno))
            {
                NDRX_LOG(log_error, "Failed to process block!");
                EXFAIL_OUT(ret);
            }

            NDRX_FREE(namelist[n]);
        }
        NDRX_FREE(namelist);
        namelist = NULL;
    }
    
out:

    /* this will check for struct init or not init */
    if (NULL!=namelist)
    {
        dirent_free(namelist, n);
        namelist=NULL;
    }

    if (NULL!=p_block)
    {
        NDRX_FREE((char *)p_block);
    }

    /* close the resources */
    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }
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

/** continue with dirent free */
#define RECOVER_CONTINUE \
            NDRX_FREE(G_atmi_tls->qdisk_tls->recover_namelist[G_atmi_tls->qdisk_tls->recover_i]);\
            continue;\
   
/** Close cursor */
#define RECOVER_CLOSE_CURSOR \
        /* reset any stuff left open from previous scan... */\
        if (NULL!=G_atmi_tls->qdisk_tls->recover_namelist)\
        {\
            dirent_free(G_atmi_tls->qdisk_tls->recover_namelist, G_atmi_tls->qdisk_tls->recover_i);\
            G_atmi_tls->qdisk_tls->recover_namelist = NULL;\
        }\
        G_atmi_tls->qdisk_tls->recover_open=EXFALSE;\
        G_atmi_tls->qdisk_tls->recover_i=EXFAIL;\
        G_atmi_tls->qdisk_tls->recover_last_loaded=EXFALSE;\

/**
 * Lists currently prepared transactions
 * NOTE! Currently messages does not store RMID.
 * This means that one directory cannot shared between several 
 * Might have race condition with in-progress prepare. But I think it is
 * is not a big problem. As anyway we normally will run abort or commit.
 * And commit will require that all is prepared.
 * @param sw XA switch
 * @param xid where to unload the xids
 * @param count positions avaialble
 * @param rmid RM id
 * @param flags
 * @return error or nr Xids recovered
 */
expublic int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags)
{
    int ret = XA_OK;
    int err;
    XID xtmp;
    char *p, *fname;
    int current_unload_pos=0; /* where to unload the stuff.. */
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_recover_entry() - XA not open!");
        ret=XAER_PROTO;
        goto out;
    }
    
    if (NULL==xid && count >0)
    {
        NDRX_LOG(log_error, "ERROR: xid is NULL and count >0");
        ret=XAER_INVAL;
        goto out;
    }
    
    if (!G_atmi_tls->qdisk_tls->recover_open && ! (flags & TMSTARTRSCAN))
    {
        NDRX_LOG(log_error, "ERROR: Scan not open and TMSTARTRSCAN not specified");
        ret=XAER_INVAL;
        goto out;
    }
    
    /* close the scan */
    if (flags & TMSTARTRSCAN)
    {
        /* if was not open, no problem.. */
        RECOVER_CLOSE_CURSOR;
    }
    
    /* start the scan if requested so ...  */
    if (flags & TMSTARTRSCAN)
    {
        G_atmi_tls->qdisk_tls->recover_i = scandir(M_folder_prepared, 
                &G_atmi_tls->qdisk_tls->recover_namelist, 0, alphasort);
        
        if (G_atmi_tls->qdisk_tls->recover_i < 0)
        {
            err=errno;
            NDRX_LOG(log_error, "Failed to scan q directory [%s]: %s", 
                    M_folder_prepared, strerror(err));
            userlog("Failed to scan q directory [%s]: %s", 
                    M_folder_prepared, strerror(err));
            ret=XAER_RMERR;
            goto out;
        }
        
        G_atmi_tls->qdisk_tls->recover_open=EXTRUE;
    }
    
    /* nothing to return */
    if (NULL==G_atmi_tls->qdisk_tls->recover_namelist)
    {
        ret=0;
        goto out;
    }
    
    /** start to unload xids, we got to match the same names */
    while ((count - current_unload_pos) > 0 && 
            G_atmi_tls->qdisk_tls->recover_i--)
    {
        fname = G_atmi_tls->qdisk_tls->recover_namelist[G_atmi_tls->qdisk_tls->recover_i]->d_name;
        
        if (0==strcmp(fname, ".") || 
            0==strcmp(fname, ".."))
        {
            RECOVER_CONTINUE;
        }
        
        p = strchr(fname, '-');
        
        /* invalid file name, skip... */
        if (NULL==p)
        {
            NDRX_LOG(log_error, "Invalid prepared name [%s] - skip", fname);
            RECOVER_CONTINUE;
        }
                
        /* terminate so that we have good name */
        *p = EXEOS;
        p++;
        
        if (NULL==atmi_xa_deserialize_xid((unsigned char *)fname, &xtmp))
        {
            NDRX_LOG(log_error, "Failed to deserialize xid: %s - skip", fname);
            RECOVER_CONTINUE;
        }
        
        /* TODO: if we will support several RMIDs in the same folder
         * then we need to read the file block and check the RMID
         * currently file blocks does not contain RMID. Thus that would
         * require to be appended.
         */
        
        /* check is it duplicate? */
        if ( (current_unload_pos>0 &&
                0==memcmp(&xid[current_unload_pos-1], &xtmp, sizeof(XID)))
                /* if it was processed in previous scan: 
                 */
                || (G_atmi_tls->qdisk_tls->recover_last_loaded 
                        && 0==memcmp(&G_atmi_tls->qdisk_tls->recover_last, &xtmp, sizeof(XID)))
                )
        {
            NDRX_LOG(log_debug, "Got part [%s] of xid [%s]", p, fname);
            RECOVER_CONTINUE;
        }
            
        /* Okey unload the xid finally */
        memcpy(&xid[current_unload_pos], &xtmp, sizeof(XID));
        
        
        NDRX_LOG(log_debug, "Xid [%s] unload to position %d", fname, current_unload_pos);
        ret++;
        current_unload_pos++;
        RECOVER_CONTINUE;
    }
    
    if (ret>0)
    {
        /* save the last xid for reetry skipping.. */
        memcpy(&G_atmi_tls->qdisk_tls->recover_last, &xid[ret-1], sizeof(XID));
        G_atmi_tls->qdisk_tls->recover_last_loaded=EXTRUE;
    }
    
out:

    /* terminate the scan */
    NDRX_LOG(log_debug, "recover: count=%ld, ret=%d", count, ret);

    if (    ret>=0 
            && ( (flags & TMENDRSCAN)  || ret < count)
            && G_atmi_tls->qdisk_tls->recover_open
            )
    {
        NDRX_LOG(log_debug, "recover: closing cursor");
        
        /* if was not open, no problem.. */
        RECOVER_CLOSE_CURSOR;
    }

    return ret; /* no transactions found */
}

/**
 * CURRENTLY NOT USED!!!
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_forget_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "ERROR! xa_forget_entry() - not using!!");
    return XAER_RMERR;
}

/**
 * CURRENTLY NOT USED!!!
 * @param sw
 * @param handle
 * @param retval
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags)
{
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_complete_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "ERROR! xa_complete_entry() - not using!!");
    return XAER_RMERR;
}


/* Static entries */
expublic int xa_open_entry_stat( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxqstatsw, xa_info, rmid, flags);
}
expublic int xa_close_entry_stat(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxqstatsw, xa_info, rmid, flags);
}
expublic int xa_start_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_end_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_rollback_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_prepare_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_commit_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxqstatsw, xid, count, rmid, flags);
}
expublic int xa_forget_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxqstatsw, handle, retval, rmid, flags);
}

/* Dynamic entries */
expublic int xa_open_entry_dyn( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxqdynsw, xa_info, rmid, flags);
}
expublic int xa_close_entry_dyn(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxqdynsw, xa_info, rmid, flags);
}
expublic int xa_start_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_end_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_rollback_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_prepare_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_commit_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_recover_entry_dyn(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxqdynsw, xid, count, rmid, flags);
}
expublic int xa_forget_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_complete_entry_dyn(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxqdynsw, handle, retval, rmid, flags);
}
/* vim: set ts=4 sw=4 et smartindent: */
