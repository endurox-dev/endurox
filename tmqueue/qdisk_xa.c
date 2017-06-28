/* 
** Q XA Backend 
**
** Prepare the folders & subfolders 
** So we will have following directory structure:
** - active
** - prepared
** - committed
** 
** Initialy file will be named after the XID
** Once it becomes committed, it is named after message_id
** 
** If we have a update to Q (try counter for example)
** Then we issue new transaction file with different command inside, but it contains
** the message_id 
** 
** Once do the commit and it is not and message, but update file, then
** we update message file.
** 
** If the command is delete, then we unlink the file.
** 
** Once Queue record is completed (rolled back or committed) we shall send ACK
** of COMMAND BLOCK back to queue server via TPCALL command to QSPACE server.
** this will allow to synchornize internal status of the messages. 
** 
** Initially (active) file is named is named after XID. When doing commit, it is
** renamed to msg_id.
** 
** If we restore the system after the restart, then committed & prepare directory is scanned.
** If msg is committed + there is command in prepared, then it is marked as locked.
** When scanning prepared directory, we shall read the msg_id from files.
** 
** We shall support multiple TMQs running over single Q space, but they each should,
** manage it's own set of queued messages.
** The queued file names shall contain [QSPACE].[SERVERID].[MSG_ID|XID]
** To post the updates to proper TMQ, it should advertise QSPACE.[SERVER_ID]
** In case of Active-Active env, servers only on node shall be run.
** 
** On the other node we could put in standby qspace+server_id on the same shared dir.
** 
**
** @file qdisk_xa.c
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
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

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
#include <qcommon.h>
#include <xa_cmn.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/* Shared between threads: */
exprivate int M_is_open = EXFALSE;
exprivate int M_rmid = EXFAIL;

exprivate char M_folder[PATH_MAX] = {EXEOS}; /* Where to store the q data */
exprivate char M_folder_active[PATH_MAX] = {EXEOS}; /* Active transactions */
exprivate char M_folder_prepared[PATH_MAX] = {EXEOS}; /* Prepared transactions */
exprivate char M_folder_committed[PATH_MAX] = {EXEOS}; /* Committed transactions */

/* Per thread data: */
exprivate __thread int M_is_reg = EXFALSE; /* Dynamic registration done? */
/*
 * Due to fact that we might have multiple queued messages per resource manager
 * we will name the transaction files by this scheme:
 * - <XID_STR>-1|2|3|4|..|N
 * we will start the processing from N back to 1 so that if we crash and retry
 * the operation, we can handle all messages in system.
 */
exprivate __thread char M_filename_base[PATH_MAX+1] = {EXEOS}; /* base name of the file */
exprivate __thread char M_filename_active[PATH_MAX+1] = {EXEOS}; /* active file name */
exprivate __thread char M_filename_prepared[PATH_MAX+1] = {EXEOS}; /* prepared file name */
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

exprivate int read_tx_block(FILE *f, char *block, int len);
exprivate int read_tx_from_file(char *fname, char *block, int len);

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
 * Set filename base
 * @param xid
 * @param rmid
 * @return 
 */
exprivate char *set_filename_base(XID *xid, int rmid)
{
    atmi_xa_serialize_xid(xid, M_filename_base);
    
    NDRX_LOG(log_debug, "Base file name built [%s]", M_filename_base);
    
    return M_filename_base;
}

/**
 * Set the real filename (this includes the number postfix.)
 * We need to test file if existance in:
 * - active
 * - prepared folders
 * 
 * if file exists, we increment the counter. We start from 1.
 * 
 * @param xid
 * @param rmid
 * @param filename where to store the output file name
 */
exprivate void set_filenames(void)
{
    int i;
    
    for (i=1;;i++)
    {
        sprintf(M_filename_active, "%s/%s-%03d", M_folder_active, M_filename_base, i);
        sprintf(M_filename_prepared, "%s/%s-%03d", M_folder_prepared, M_filename_base, i);
        
        if (!ndrx_file_exists(M_filename_active) && 
                !ndrx_file_exists(M_filename_prepared))
        {
            break;
        }
    }
    NDRX_LOG(log_info, "Filenames set to: [%s] [%s]", 
                M_filename_active, M_filename_prepared);
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
        sprintf(filename_active, "%s/%s-%03d", M_folder_active, M_filename_base, i+1);
        sprintf(filename_prepared, "%s/%s-%03d", M_folder_prepared, M_filename_base, i+1);
        NDRX_LOG(log_debug, "Testing act: [%s] prep: [%s]", filename_active, filename_prepared);
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
    
    sprintf(filename[slot], "%s/%s-%03d", folder, M_filename_base, i);
    
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
    
    sprintf(buf, "%s/%s", M_folder_committed, fname);
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
        
    NDRX_LOG(log_error, "Rename [%s]->[%s]", 
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
 * @return SUCCEED/FAIL
 */
exprivate int file_move_final(char *from_filename, char *to_filename_only)
{
    int ret = EXSUCCEED;
    
    char *to_filename = get_file_name_final(to_filename_only);
    
    NDRX_LOG(log_debug, "Rename [%s] -> [%s]", from_filename, to_filename);
    
    if (EXSUCCEED!=rename(from_filename, to_filename))
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to rename [%s]->[%s]: %s", 
                from_filename, to_filename, strerror(err));
        userlog("Failed to rename [%s]->[%s]: %s", 
                from_filename, to_filename, strerror(err));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}


/**
 * Unlink the file
 * @param filename full file name (with path)
 * @return 
 */
exprivate int file_unlink(char *filename)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_info, "Unlinking [%s]", filename);
    if (EXSUCCEED!=unlink(filename))
    {
        NDRX_LOG(log_error, "Failed to unlink [%s]: %s", 
                filename, strerror(errno));
        
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}


/**
 * Send notification to tmqueue server so that we have finished this
 * particular message & we can unlock that for further processing
 * @param p_hdr
 * @return 
 */
exprivate int send_unlock_notif(union tmq_upd_block *p_upd)
{
    int ret = EXSUCCEED;
    long rsplen;
    char cmd = TMQ_CMD_NOTIFY;
    char tmp[TMMSGIDLEN_STR+1];
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", 1024);
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to allocate notif buffer");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_DATA, 0, (char *)p_upd, sizeof(*p_upd)))
    {
        NDRX_LOG(log_error, "Failed to setup EX_DATA!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup EX_QMSGID!");
        EXFAIL_OUT(ret);
    }
    
    /* We need to send also internal command (what we are doing with the struct) */
    
    NDRX_LOG(log_info, "Calling QSPACE [%s] for msgid_str [%s] unlock",
                p_upd->hdr.qspace, tmq_msgid_serialize(p_upd->hdr.msgid, tmp));
    
    ndrx_debug_dump_UBF(log_info, "calling Q space with", p_ub);
    
    /*Call The TMQ- server */
    sprintf(svcnm, NDRX_SVC_TMQ, (long)p_upd->hdr.nodeid, (int)p_upd->hdr.srvid);

    NDRX_LOG(log_debug, "About to notify [%s]", svcnm);
            
    if (p_upd->hdr.flags & TPQASYNC)
    {
        if (EXFAIL == tpacall(svcnm, (char *)p_ub, 0L, TPNOTRAN))
        {
            NDRX_LOG(log_error, "%s failed: %s", svcnm, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    else if (EXFAIL == tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPNOTRAN))
    {
        NDRX_LOG(log_error, "%s failed: %s", svcnm, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Send update notification to Q server
 * @param p_upd
 * @return 
 */
exprivate int send_unlock_notif_upd(tmq_msg_upd_t *p_upd)
{
    union tmq_upd_block block;
    
    memset(&block, 0, sizeof(block));
    
    memcpy(&block.upd, p_upd, sizeof(*p_upd));
    
    return send_unlock_notif(&block);
}

/**
 * Used for message commit/delete
 * @param p_hdr
 * @return 
 */
exprivate int send_unlock_notif_hdr(tmq_cmdheader_t *p_hdr)
{
    union tmq_upd_block block;
    
    memset(&block, 0, sizeof(block));
    
    memcpy(&block.hdr, p_hdr, sizeof(*p_hdr));
    
    return send_unlock_notif(&block);
}

/**
 * Open API
 * @param sw
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    int ret = EXSUCCEED;
    if (M_is_open)
    {
        NDRX_LOG(log_warn, "xa_open_entry() - already open!");
        return XA_OK;
    }
#define DIR_PERM 0755
    M_is_open = EXTRUE;
    M_rmid = rmid;
    
    /* The xa_info is directory, where to store the data...*/
    strncpy(M_folder, xa_info, sizeof(M_folder)-2);
    M_folder[sizeof(M_folder)-1] = EXEOS;
    
    NDRX_LOG(log_error, "Q data directory: [%s]", xa_info);
    
    /* The xa_info is directory, where to store the data...*/
    strncpy(M_folder_active, xa_info, sizeof(M_folder_active)-8);
    M_folder_active[sizeof(M_folder_active)-7] = EXEOS;
    strcat(M_folder_active, "/active");
    
    strncpy(M_folder_prepared, xa_info, sizeof(M_folder_prepared)-10);
    M_folder_prepared[sizeof(M_folder_prepared)-9] = EXEOS;
    strcat(M_folder_prepared, "/prepared");
    
    strncpy(M_folder_committed, xa_info, sizeof(M_folder_committed)-11);
    M_folder_committed[sizeof(M_folder_committed)-10] = EXEOS;
    strcat(M_folder_committed, "/committed");
    
    /* Test the directories */
    if (EXSUCCEED!=(ret=mkdir(M_folder, DIR_PERM)) && ret!=EEXIST )
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
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_active, DIR_PERM)) && ret!=EEXIST )
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
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_prepared, DIR_PERM)) && ret!=EEXIST )
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
    
    if (EXSUCCEED!=(ret=mkdir(M_folder_committed, DIR_PERM)) && ret!=EEXIST )
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
    
             
    return XA_OK;
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
    
    M_is_open = EXFALSE;
    return XA_OK;
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
    set_filename_base(xid, rmid);
    
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_start_entry() - XA not open!");
        return XAER_RMERR;
    }
    return XA_OK;
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
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_end_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (M_is_reg)
    {
        if (EXSUCCEED!=ax_unreg(rmid, 0))
        {
            NDRX_LOG(log_error, "ERROR! xa_end_entry() - "
                    "ax_unreg() fail!");
            return XAER_RMERR;
        }
        
        M_is_reg = EXFALSE;
    }
    
out:

    return XA_OK;
}

/**
 * Remove any transaction file (we might have multiple here
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    int i, j;
    int names_max;
    char *fname;
    union tmq_block b;
    char *folders[2] = {M_folder_active, M_folder_prepared};
    char *fn = "xa_rollback_entry";

    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_rollback_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    set_filename_base(xid, rmid);
    names_max = get_filenames_max();
    
    NDRX_LOG(log_info, "%s: %d", fn, names_max);
    
    /* send notification, that message is removed, but firstly we need to 
     * understand what kind of message it was.
     * - If new msg: send delete to server
     * - If any otgher: send simple unlock
     */
    for (i=names_max; i>=1; i--)
    {
        for (j = 0; j<2; j++)
        {
            fname = get_filename_i(i, folders[j], 0);
            
            if (ndrx_file_exists(fname))
            {
                NDRX_LOG(log_debug, "%s: Processing file exists [%s]", fn, fname);
                if (EXSUCCEED==read_tx_from_file(fname, (char *)&b, sizeof(b)))
                {
                    /* Send the notification */
                    if (TMQ_STORCMD_NEWMSG == b.hdr.command_code)
                    {
                        NDRX_LOG(log_info, "%s: delete command...", fn);
                        b.hdr.command_code = TMQ_STORCMD_DEL;
                    }
                    else
                    {
                        NDRX_LOG(log_info, "%s: unlock command...", fn);
                        
                        b.hdr.command_code = TMQ_STORCMD_UNLOCK;
                    }

                    send_unlock_notif_hdr(&b.hdr);
                }
                file_unlink(fname);
            }
            else
            {
                NDRX_LOG(log_debug, "%s: File [%s] does not exists", fn, fname);
            }
                
        }
    }
    
    return XA_OK;
}

/**
 * XA prepare entry call
 * Basically we move all messages for active to prepared folder (still named by
 * xid_str)
 * 
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    int i;
    int names_max;

    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_prepare_entry() - XA not open!");
        return XAER_RMERR;
    }

    set_filename_base(xid, rmid);
    names_max = get_filenames_max();
    
    for (i=names_max; i>=1; i--)
    {
        if (EXSUCCEED!=file_move(i, M_folder_active, M_folder_prepared))
        {
            return XAER_RMERR;
        }
    }
    
    return XA_OK;
    
}

/**
 * Commit the transaction.
 * For new transaction:
 * - Rename to msgid_str
 * 
 * For update transactions:
 * - Update the message on disk with update data + remove update command msg.
 * 
 * For delete transactions:
 * - Remove file from disk + remove delete msg.
 * 
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    union tmq_block block;
    char msgid_str[TMMSGIDLEN_STR+1];
    int i;
    int names_max;
    FILE *f = NULL;
    char *fname;
    char *fname_msg;
    
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - XA not open!");
        return XAER_RMERR;
    }

    set_filename_base(xid, rmid);
    names_max = get_filenames_max();
    
    for (i=names_max; i>=1; i--)
    {
        
        fname=get_filename_i(i, M_folder_prepared, 0);
        
        if (EXSUCCEED!=read_tx_from_file(fname, (char *)&block, sizeof(block)))
        {
            NDRX_LOG(log_error, "ERROR! xa_commit_entry() - failed to read data block!");
            goto xa_err;
        }

        /* Do the task... */
        if (TMQ_STORCMD_NEWMSG == block.hdr.command_code)
        {
            if (EXSUCCEED!=file_move_final(fname, 
                    tmq_msgid_serialize(block.hdr.msgid, msgid_str)))
            {
                goto xa_err;
            }
            
            if (EXSUCCEED!=send_unlock_notif_hdr(&block.hdr))
            {
                goto xa_err;
            }
        }
        else if (TMQ_STORCMD_UPD == block.hdr.command_code)
        {
            tmq_msg_t msg_to_upd; /* Message to update */
            int ret_len;
            
            fname_msg = get_file_name_final(tmq_msgid_serialize(block.hdr.msgid, msgid_str));
            NDRX_LOG(log_info, "Updating message file: [%s]", fname_msg);
            if (NULL==(f = NDRX_FOPEN(fname_msg, "r+b")))
            {
                int err = errno;
                NDRX_LOG(log_error, "ERROR! xa_commit_entry() - failed to open file[%s]: %s!", 
                        fname_msg, strerror(err));

                userlog( "ERROR! xa_commit_entry() - failed to open file[%s]: %s!", 
                        fname_msg, strerror(err));
                goto xa_err;
            }
            
            if (EXSUCCEED!=read_tx_block(f, (char *)&msg_to_upd, sizeof(msg_to_upd)))
            {
                NDRX_LOG(log_error, "ERROR! xa_commit_entry() - failed to read data block!");
                goto xa_err;
            }
            
            /* seek the start */
            if (EXSUCCEED!=fseek (f, 0 , SEEK_SET ))
            {
                NDRX_LOG(log_error, "Seekset failed: %s", strerror(errno));
                goto xa_err;
            }
            
            UPD_MSG((&msg_to_upd), (&block.upd));
            
            /* Write th block */
            if (sizeof(msg_to_upd)!=(ret_len=fwrite((char *)&msg_to_upd, 1, sizeof(msg_to_upd), f)))
            {
                int err = errno;
                NDRX_LOG(log_error, "ERROR! Filed to write to msg file [%s]: "
                        "req_len=%d, written=%d: %s", fname_msg,
                        sizeof(msg_to_upd), ret_len, strerror(err));

                userlog("ERROR! Filed to write to msg file[%s]: req_len=%d, "
                        "written=%d: %s",
                        fname_msg, sizeof(msg_to_upd), ret_len, strerror(err));

                goto xa_err;
            }
            NDRX_FCLOSE(f);
            f = NULL;
            
            /* remove the update file */
            NDRX_LOG(log_info, "Removing update command file: [%s]", fname);
            
            if (EXSUCCEED!=unlink(fname))
            {
                NDRX_LOG(log_error, "Failed to remove update file [%s]: %s", 
                        fname, strerror(errno));
            }
            
            if (EXSUCCEED!=send_unlock_notif_upd(&block.upd))
            {
                goto xa_err;
            }
            
        }
        else if (TMQ_STORCMD_DEL == block.hdr.command_code)
        {
            fname_msg = get_file_name_final(tmq_msgid_serialize(block.hdr.msgid, msgid_str));
            NDRX_LOG(log_info, "Removing message file: [%s]", fname_msg);
            
            if (EXSUCCEED!=unlink(fname_msg))
            {
                NDRX_LOG(log_error, "Failed to remove update file [%s]: %s", 
                        fname_msg, strerror(errno));
            }
            
            NDRX_LOG(log_info, "Removing delete command file file: [%s]", fname);
            
            if (EXSUCCEED!=unlink(fname))
            {
                NDRX_LOG(log_error, "Failed to remove update file [%s]: %s", 
                        fname, strerror(errno));
            }
            
            /* Remove the message (it must be marked for delete)
             */
            if (EXSUCCEED!=send_unlock_notif_hdr(&block.hdr))
            {
                goto xa_err;
            }
        }
        else
        {
            NDRX_LOG(log_error, "ERROR! xa_commit_entry() - invalid command [%c]!",
                    block.hdr.command_code);

            
            goto xa_err;
        }
    }
    
    NDRX_LOG(log_info, "Committed ok");
    
    return XA_OK;
    
xa_err:
    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }

    NDRX_LOG(log_info, "Commit failed");
    return XAER_RMERR;
}

/**
 * Reads the header block
 * @param block
 * @param p_len
 * @return 
 */
exprivate int read_tx_block(FILE *f, char *block, int len)
{
    int act_read;
    int ret = EXSUCCEED;
    
    if (len!=(act_read=fread(block, 1, len, f)))
    {
        int err = errno;
        
        NDRX_LOG(log_error, "ERROR! Filed to read tx file: req_read=%d, read=%d: %s",
                len, act_read, strerror(err));
        
        userlog("ERROR! Filed to read tx file: req_read=%d, read=%d: %s",
                len, act_read, strerror(err));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Read the block file file
 * @param fname full path to file
 * @param block buffer where to store the data block
 * @param len length to read
 * @return 
 */
exprivate int read_tx_from_file(char *fname, char *block, int len)
{
    int ret = EXSUCCEED;
    FILE *f = NULL;
    
    if (NULL==(f = NDRX_FOPEN(fname, "r+b")))
    {
        int err = errno;
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - failed to open file[%s]: %s!", 
                fname, strerror(err));

        userlog( "ERROR! xa_commit_entry() - failed to open file[%s]: %s!", 
                fname, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    ret = read_tx_block(f, block, len);
    
out:

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }
    
    return ret;
}

/**
 * Write data to transaction file
 * @param block
 * @param len
 * @return SUCCEED/FAIL
 */
exprivate int write_to_tx_file(char *block, int len)
{
    int ret = EXSUCCEED;
    XID xid;
    size_t ret_len;
    FILE *f = NULL;
    int ax_ret;
    
    if (ndrx_get_G_atmi_env()->xa_sw->flags & TMREGISTER && !M_is_reg)
    {
        ax_ret = ax_reg(M_rmid, &xid, 0);
                
        if (TM_JOIN!=ax_ret && TM_OK!=ax_ret)
        {
            NDRX_LOG(log_error, "ERROR! xa_reg() failed!");
            EXFAIL_OUT(ret);
        }
        
        if (XA_OK!=xa_start_entry(ndrx_get_G_atmi_env()->xa_sw, &xid, M_rmid, 0))
        {
            NDRX_LOG(log_error, "ERROR! xa_start_entry() failed!");
            EXFAIL_OUT(ret);
        }
        
        M_is_reg = EXTRUE;
    }
    
    set_filenames();
    
    /* Open file for write... */
    NDRX_LOG(log_info, "Writting command file: [%s]", M_filename_active);
    if (NULL==(f = NDRX_FOPEN(M_filename_active, "a+b")))
    {
        int err = errno;
        NDRX_LOG(log_error, "ERROR! write_to_tx_file() - failed to open file[%s]: %s!", 
                M_filename_active, strerror(err));
        
        userlog( "ERROR! write_to_tx_file() - failed to open file[%s]: %s!", 
                M_filename_active, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* Write th block */
    if (len!=(ret_len=fwrite(block, 1, len, f)))
    {
        int err = errno;
        NDRX_LOG(log_error, "ERROR! Filed to write to tx file: req_len=%d, written=%d: %s",
                len, ret_len, strerror(err));
        
        userlog("ERROR! Filed to write to tx file: req_len=%d, written=%d: %s",
                len, ret_len, strerror(err));
        
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
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
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_write_cmd_newmsg(tmq_msg_t *msg)
{
    int ret = EXSUCCEED;
    char tmp[TMMSGIDLEN_STR+1];
    
    /*
    uint64_t lockt =  msg->lockthreadid;
    
     do not want to lock be written out to files: 
    msg->lockthreadid = 0;
    NO NO NO !!!! We still need a lock!
    */
    
    NDRX_DUMP(log_debug, "Writing new message to disk", 
                (char *)msg, sizeof(*msg)+msg->len);
    
    if (EXSUCCEED!=write_to_tx_file((char *)msg, sizeof(*msg)+msg->len))
    {
        NDRX_LOG(log_error, "tmq_storage_write_cmd_newmsg() failed for msg %s", 
                tmq_msgid_serialize(msg->hdr.msgid, tmp));
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
 * Delete/Update message block write
 * @param p_block ptr to union of commands
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_write_cmd_block(union tmq_block *p_block, char *descr)
{
    int ret = EXSUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    
    NDRX_LOG(log_info, "Writing command block: %s msg [%s]", descr, 
            tmq_msgid_serialize(p_block->hdr.msgid, msgid_str) );
    
    NDRX_DUMP(log_debug, "Writing command block to disk", 
                (char *)p_block, sizeof(*p_block));
    
    if (EXSUCCEED!=write_to_tx_file((char *)p_block, sizeof(*p_block)))
    {
        NDRX_LOG(log_error, "tmq_storage_write_cmd_block() failed for msg %s", 
                tmq_msgid_serialize(p_block->hdr.msgid, msgid_str));
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

/**
 * Restore messages from storage device.
 * TODO: File naming include 03d so that multiple tasks per file sorts alphabetically.
 * @param process_block callback function to process the data block
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_get_blocks(int (*process_block)(union tmq_block **p_block),
            short nodeid, short srvid)
{
    int ret = EXSUCCEED;
    struct dirent **namelist = NULL;
    int n;
    int j;
    union tmq_block *p_block = NULL;
    FILE *f = NULL;
    char filename[PATH_MAX+1];
    char *folders[2] = {M_folder_committed, M_folder_prepared};
    short msg_nodeid, msg_srvid;
    char msgid[TMMSGIDLEN];
    
    for (j = 0; j < 2; j++)
    {
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
                NDRX_FREE(namelist[n]);
                continue;
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
                    NDRX_FREE(namelist[n]);
                    continue;
                }
            }
            
            sprintf(filename, "%s/%s", folders[j], namelist[n]->d_name);
            NDRX_LOG(log_warn, "Loading [%s]", filename);

            if (NULL==(f=NDRX_FOPEN(filename, "rb")))
            {
                NDRX_LOG(log_error, "Failed to open for read [%s]: %s", 
                   filename, strerror(errno));
                EXFAIL_OUT(ret);
            }

            /* Read header */            
            if (NULL==(p_block = NDRX_MALLOC(sizeof(*p_block))))
            {
                NDRX_LOG(log_error, "Failed to alloc [%s]: %s", 
                   filename, strerror(errno));
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED!=read_tx_block(f, (char *)p_block, sizeof(*p_block)))
            {
                NDRX_LOG(log_error, "Failed to read [%s]: %s", 
                   filename, strerror(errno));
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
                NDRX_FREE(namelist[n]);
                NDRX_FREE((char *)p_block);
                p_block = NULL;
                continue;
            }
            
            NDRX_DUMP(log_debug, "Got command block",  p_block, sizeof(*p_block));
            
            /* if it is message, the re-alloc  */
            if (TMQ_STORCMD_NEWMSG==p_block->hdr.command_code)
            {
                if (NULL==(p_block = NDRX_REALLOC(p_block, sizeof(tmq_msg_t) + p_block->msg.len)))
                {
                    NDRX_LOG(log_error, "Failed to alloc [%d]: %s", 
                       (sizeof(tmq_msg_t) + p_block->msg.len), strerror(errno));
                    EXFAIL_OUT(ret);
                }
                /* Read some more */
                if (EXSUCCEED!=read_tx_block(f, p_block->msg.msg, p_block->msg.len))
                {
                    NDRX_LOG(log_error, "Failed to read [%s]: %s", 
                       filename, strerror(errno));
                    EXFAIL_OUT(ret);
                }
                /* unlock the message */
                p_block->msg.lockthreadid = 0;
                
                NDRX_DUMP(log_debug, "Read message from disk", 
                        p_block->msg.msg, p_block->msg.len);
            }
            
            NDRX_FCLOSE(f);
            f=NULL;
            
            /* Process message block 
             * It is up to caller to free the mem & make null
             */
            if (EXSUCCEED!=process_block(&p_block))
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

    if (NULL!=namelist)
    {
        NDRX_FREE(namelist[n]);
        NDRX_FREE(namelist);
        namelist = NULL;
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
 * CURRENTLY NOT USED!!!
 * @param sw
 * @param xid
 * @param count
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags)
{
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_recover_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "ERROR! xa_recover_entry() - not using!!");
    return XAER_RMERR;
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
    
    if (!M_is_open)
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
    if (!M_is_open)
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
