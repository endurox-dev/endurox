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
** The file format will be following:
** [message_id 32 bytes][command - [i]nc_counter]
** [message_id 32 bytes][command - [u]nlink_msg]
** [message_id 32 bytes][command - [d]data][..all the flags..][try_counter][data block]
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
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <errno.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <ntimer.h>

#include <xa.h>
#include <atmi_int.h>
#include <unistd.h>

#include "userlog.h"
#include "tmqueue.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
private int M_is_open = FALSE;
private int M_is_reg = FALSE; /* Dynamic registration done? */
private int M_rmid = FAIL;
private FILE *M_f = NULL;


private char M_folder[PATH_MAX] = {EOS}; /* Where to store the q data */
private char M_folder_active[PATH_MAX] = {EOS}; /* Active transactions */
private char M_folder_prepared[PATH_MAX] = {EOS}; /* Prepared transactions */
private char M_folder_committed[PATH_MAX] = {EOS}; /* Committed transactions */
/*---------------------------Prototypes---------------------------------*/

public int xa_open_entry_stat(char *xa_info, int rmid, long flags);
public int xa_close_entry_stat(char *xa_info, int rmid, long flags);
public int xa_start_entry_stat(XID *xid, int rmid, long flags);
public int xa_end_entry_stat(XID *xid, int rmid, long flags);
public int xa_rollback_entry_stat(XID *xid, int rmid, long flags);
public int xa_prepare_entry_stat(XID *xid, int rmid, long flags);
public int xa_commit_entry_stat(XID *xid, int rmid, long flags);
public int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags);
public int xa_forget_entry_stat(XID *xid, int rmid, long flags);
public int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags);

public int xa_open_entry_dyn(char *xa_info, int rmid, long flags);
public int xa_close_entry_dyn(char *xa_info, int rmid, long flags);
public int xa_start_entry_dyn(XID *xid, int rmid, long flags);
public int xa_end_entry_dyn(XID *xid, int rmid, long flags);
public int xa_rollback_entry_dyn(XID *xid, int rmid, long flags);
public int xa_prepare_entry_dyn(XID *xid, int rmid, long flags);
public int xa_commit_entry_dyn(XID *xid, int rmid, long flags);
public int xa_recover_entry_dyn(XID *xid, long count, int rmid, long flags);
public int xa_forget_entry_dyn(XID *xid, int rmid, long flags);
public int xa_complete_entry_dyn(int *handle, int *retval, int rmid, long flags);

public int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
public int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
public int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
public int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
public int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
public int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
public int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
public int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags);
public int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
public int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags);



private int read_tx_header(char *block, int len);

struct xa_switch_t ndrxstatsw = 
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

struct xa_switch_t ndrxdynsw = 
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
 * will use NDRX_TEST_RM_DIR env variable...
 */
private char *get_file_name(XID *xid, int rmid, char *folder)
{
    static char buf[2048];
    char xid_str[128];
    
    sprintf(buf, "%s/%s", folder, xid_str);
    NDRX_LOG(log_debug, "Folder built: %s", buf);
    
    return buf;
}

/**
 * Special Q file name
 * @param fname
 * @return 
 */
private char *get_file_name_final(char *fname)
{
    static char buf[2048];
    
    sprintf(buf, "%s/%s", M_folder_committed, fname);
    NDRX_LOG(log_debug, "Folder built: %s", buf);
    
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
private int file_move(XID *xid, int rmid, char *from_folder, char *to_folder)
{
    int ret = SUCCEED;
    
    char from_file[FILENAME_MAX+1] = {EOS};
    char to_file[FILENAME_MAX+1] = {EOS};
    
    strcpy(from_file, get_file_name(xid, rmid, from_folder));
    strcpy(to_file, get_file_name(xid, rmid, to_folder));
    
    if (SUCCEED!=rename(from_file, to_file))
    {
        NDRX_LOG(log_error, "Failed to rename: %s", strerror(errno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Final file move where the final is the exact file name
 * @param xid
 * @param rmid
 * @param from_folder
 * @param to_folder
 * @return 
 */
private int file_move_final(XID *xid, int rmid, char *from_folder, char *filename)
{
    int ret = SUCCEED;
    
    char from_file[FILENAME_MAX+1] = {EOS};
    char to_file[FILENAME_MAX+1] = {EOS};
    
    strcpy(from_file, get_file_name(xid, rmid, from_folder));
    strcpy(to_file, get_file_name_final(to_file));
    
    NDRX_LOG(log_debug, "Rename [%s] -> [%s]", from_file, to_file);
    
    if (SUCCEED!=rename(from_file, to_file))
    {
        NDRX_LOG(log_error, "Failed to rename: %s", strerror(errno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Unlink the transaction file
 * @param xid
 * @param rmid
 * @param folder
 * @return 
 */
private int file_unlink(XID *xid, int rmid, char *folder)
{
    int ret = SUCCEED;
    
    char file[FILENAME_MAX+1] = {EOS};
    
    strcpy(file, get_file_name(xid, rmid, folder));
    
    if (SUCCEED!=unlink(file))
    {
        NDRX_LOG(log_error, "Failed to unlink [%s]: %s", file, strerror(errno));
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Open API
 * @param sw
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
public int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    int ret = SUCCEED;
    if (M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_open_entry() - already open!");
        return XAER_RMERR;
    }
    M_is_open = TRUE;
    M_rmid = rmid;
    
    /* The xa_info is directory, where to store the data...*/
    strncpy(M_folder, xa_info, sizeof(M_folder)-2);
    M_folder[sizeof(M_folder)-1] = EOS;
    
    NDRX_LOG(log_error, "Q data directory: [%s]", xa_info);
    
    /* The xa_info is directory, where to store the data...*/
    strncpy(M_folder_active, xa_info, sizeof(M_folder_active)-8);
    M_folder_active[sizeof(M_folder_active)-7] = EOS;
    strcat(M_folder, "active");
    
    strncpy(M_folder_prepared, xa_info, sizeof(M_folder_prepared)-10);
    M_folder_prepared[sizeof(M_folder_prepared)-9] = EOS;
    strcat(M_folder_prepared, "prepared");
    
    strncpy(M_folder_committed, xa_info, sizeof(M_folder_committed)-11);
    M_folder_committed[sizeof(M_folder_committed)-10] = EOS;
    strcat(M_folder_committed, "committed");
    
    /* Test the directories */
    if (SUCCEED!=(ret=mkdir(M_folder)) && ret!=EEXIST )
    {
        int err = errno;
        NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder, strerror(errno));
        
        userlog("xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder, strerror(errno));
        return XAER_RMERR;
    }
    
    if (SUCCEED!=(ret=mkdir(M_folder_active)) && ret!=EEXIST )
    {
        int err = errno;
        NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_active, strerror(errno));
        
        userlog("xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_active, strerror(errno));
        return XAER_RMERR;
    }
    
    if (SUCCEED!=(ret=mkdir(M_folder_prepared)) && ret!=EEXIST )
    {
        int err = errno;
        NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_prepared, strerror(errno));
        
        userlog("xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_prepared, strerror(errno));
        return XAER_RMERR;
    }
    
    if (SUCCEED!=(ret=mkdir(M_folder_committed)) && ret!=EEXIST )
    {
        int err = errno;
        NDRX_LOG(log_error, "xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_committed, strerror(errno));
        
        userlog("xa_open_entry() Q driver: failed to create directory "
                "[%s] - [%s]!", M_folder_committed, strerror(errno));
        return XAER_RMERR;
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
public int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    NDRX_LOG(log_error, "xa_close_entry() called");
    
    if (!M_is_open)
    {
        /* Ignore this error...
        NDRX_LOG(log_error, "TESTERROR!!! xa_close_entry() - already closed!");
         */
        return XAER_RMERR;
    }
    
    M_is_open = FALSE;
    return XA_OK;
}

/**
 * Open text file in RMID folder. Create file by TXID.
 * Check for file existance. If start & not exists - ok .
 * If exists and join - ok. Otherwise fail.
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
public int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *file = get_file_name(xid, rmid, "active");
    
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_start_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    /* Open file for write... */
    if (NULL==(M_f = fopen(file, "a+b")))
    {
        int err = errno;
        NDRX_LOG(log_error, "ERROR! xa_start_entry() - failed to open file: %s!", 
                strerror(errno));
        
        userlog( "ERROR! xa_start_entry() - failed to open file: %s!", strerror(errno));
        return XAER_RMERR;
    }
    
    return XA_OK;
}

public int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_end_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (NULL==M_f)
    {
        NDRX_LOG(log_error, "ERROR! xa_end_entry() - "
                "transaction already closed: %s!", 
                strerror(errno));
        return XAER_RMERR;
    }
    
    if (M_is_reg)
    {
        if (SUCCEED!=ax_unreg(rmid, 0))
        {
            NDRX_LOG(log_error, "ERROR! xa_end_entry() - "
                    "ax_unreg() fail!");
            return XAER_RMERR;
        }
        
        M_is_reg = FALSE;
    }
    
out:
    if (M_f)
    {
        fclose(M_f);
        M_f = NULL;
    }
    return XA_OK;
}

public int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_rollback_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (SUCCEED!=file_unlink(xid, rmid, M_folder_active) && 
            SUCCEED!=file_unlink(xid, rmid, M_folder_prepared))
    {
        return XAER_NOTA;
    }
    
    return XA_OK;
}

public int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_prepare_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (SUCCEED!=file_move(xid, rmid, M_folder_active, M_folder_prepared))
    {
        return XAER_RMERR;
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
public int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    union tmq_block block;
    char msgid_str[TMMSGIDLEN_STR+1];    
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (SUCCEED!=read_tx_header((char *)&block, sizeof(block)))
    {
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - failed to read data block!");
        return XAER_RMERR;
    }
    
    /* Do the task... */
    if (TMQ_CMD_NEWMSG == block.hdr.command_code)
    {
        if (SUCCEED!=file_move_final(xid, rmid, M_folder_prepared, 
                tmq_msgid_serialize(block.hdr.msgid, msgid_str)))
        {
            return XAER_RMERR;
        }
    }
    
    return XA_OK;
}

/**
 * Reads the header block
 * @param block
 * @param p_len
 * @return 
 */
private int read_tx_header(char *block, int len)
{
    int act_read;
    int ret = SUCCEED;
    
    if (len!=(act_read=fread(block, 1, len, M_f)))
    {
        int err = errno;
        
        NDRX_LOG(log_error, "ERROR! Filed to read tx file: req_read=%d, read=%d: %s",
                len, act_read, strerror(err));
        
        userlog("ERROR! Filed to read tx file: req_read=%d, read=%d: %s",
                len, act_read, strerror(err));
        FAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Write data to transaction file
 * @param block
 * @param len
 * @return 
 */
private int write_to_tx_file(char *block, int len)
{
    int ret = SUCCEED;
    XID xid;
    size_t ret_len;
    if (G_atmi_env.xa_sw->flags & TMREGISTER && !M_is_reg)
    {
        if (SUCCEED!=ax_reg(M_rmid, &xid, 0))
        {
            NDRX_LOG(log_error, "ERROR! xa_reg() failed!");
            FAIL_OUT(ret);
        }
        
        if (XA_OK!=xa_start_entry(G_atmi_env.xa_sw, &xid, M_rmid, 0))
        {
            NDRX_LOG(log_error, "ERROR! xa_start_entry() failed!");
            FAIL_OUT(ret);
        }
        
        M_is_reg = TRUE;
    }
    
    if (NULL==M_f)
    {
        NDRX_LOG(log_error, "ERROR! write with no tx file!!!");
        FAIL_OUT(ret);
    }
    /* Write th block */
    if (len!=(ret_len=fwrite(block, 1, len, M_f)))
    {
        int err = errno;
        NDRX_LOG(log_error, "ERROR! Filed to write to tx file: req_len=%d, written=%d: %s",
                len, ret_len, strerror(err));
        
        userlog("ERROR! Filed to write to tx file: req_len=%d, written=%d: %s",
                len, ret_len, strerror(err));
        
        FAIL_OUT(ret);
    }
    
out:
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
public int tmq_storage_write_cmd_newmsg(tmq_msg_t *msg)
{
    int ret = SUCCEED;
    char tmp[TMMSGIDLEN_STR+1];
    
    if (SUCCEED!=write_to_tx_file((char *)msg, sizeof(*msg)+msg->len))
    {
        NDRX_LOG(log_error, "tmq_storage_write_cmd_newmsg() failed for msg %s", 
                tmq_msgid_serialize(msg->hdr.msgid, tmp));
    }
    
    NDRX_LOG(log_info, "Message [%s] written ok to active TX file", 
            tmq_msgid_serialize(msg->hdr.msgid, tmp));
    
out:
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
public int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags)
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
public int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
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
public int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags)
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
public int xa_open_entry_stat( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxstatsw, xa_info, rmid, flags);
}
public int xa_close_entry_stat(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxstatsw, xa_info, rmid, flags);
}
public int xa_start_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxstatsw, xid, rmid, flags);
}
public int xa_end_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxstatsw, xid, rmid, flags);
}
public int xa_rollback_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxstatsw, xid, rmid, flags);
}
public int xa_prepare_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxstatsw, xid, rmid, flags);
}
public int xa_commit_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxstatsw, xid, rmid, flags);
}
public int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxstatsw, xid, count, rmid, flags);
}
public int xa_forget_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxstatsw, xid, rmid, flags);
}
public int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxstatsw, handle, retval, rmid, flags);
}

/* Dynamic entries */
public int xa_open_entry_dyn( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxdynsw, xa_info, rmid, flags);
}
public int xa_close_entry_dyn(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxdynsw, xa_info, rmid, flags);
}
public int xa_start_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxdynsw, xid, rmid, flags);
}
public int xa_end_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxdynsw, xid, rmid, flags);
}
public int xa_rollback_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxdynsw, xid, rmid, flags);
}
public int xa_prepare_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxdynsw, xid, rmid, flags);
}
public int xa_commit_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxdynsw, xid, rmid, flags);
}
public int xa_recover_entry_dyn(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxdynsw, xid, count, rmid, flags);
}
public int xa_forget_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxdynsw, xid, rmid, flags);
}
public int xa_complete_entry_dyn(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxdynsw, handle, retval, rmid, flags);
}
