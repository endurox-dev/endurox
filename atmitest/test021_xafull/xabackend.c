/* 
 * @brief XA testing backend
 *   Comments:
 *   - Will assume that our resource is file system.
 *   - The file names shall match the transaction XID.
 *   - Content will be custom one.
 *   - Transaction will travel via folders in this order:
 *   - active - active transactions
 *   - prepared - prepared
 *   - committed - committed txns
 *   - aborted - transactions killed
 *   TODO: Might want testing with RETRY
 *
 * @file xabackend.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>

#include <xa.h>
#include <atmi_int.h>
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
__thread int M_is_open = EXFALSE;
__thread int M_is_reg = EXFALSE; /* Dynamic registration done? */
__thread int M_rmid = EXFAIL;
__thread FILE *M_f = NULL;
/*---------------------------Prototypes---------------------------------*/

expublic int xa_open_entry_stat(char *xa_info, int rmid, long flags);
expublic int xa_close_entry_stat(char *xa_info, int rmid, long flags);
expublic int xa_start_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_start_entry_statstartfail(XID *xid, int rmid, long flags);
expublic int xa_end_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_rollback_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_prepare_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_prepare_entry_stat105(XID *xid, int rmid, long flags);
expublic int xa_commit_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_commit_entry_stat_tryok(XID *xid, int rmid, long flags);
expublic int xa_commit_entry_stat_tryfail(XID *xid, int rmid, long flags);
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

struct xa_switch_t ndrxstatsw_startfail = 
{ 
    .name = "ndrxstatsw_startfail",
    .flags = TMNOFLAGS,
    .version = 0,
    .xa_open_entry = xa_open_entry_stat,
    .xa_close_entry = xa_close_entry_stat,
    .xa_start_entry = xa_start_entry_statstartfail,
    .xa_end_entry = xa_end_entry_stat,
    .xa_rollback_entry = xa_rollback_entry_stat,
    .xa_prepare_entry = xa_prepare_entry_stat,
    .xa_commit_entry = xa_commit_entry_stat,
    .xa_recover_entry = xa_recover_entry_stat,
    .xa_forget_entry = xa_forget_entry_stat,
    .xa_complete_entry = xa_complete_entry_stat
};

struct xa_switch_t ndrxstatswtryfail = 
{ 
    .name = "ndrxstatswtryok",
    .flags = TMNOFLAGS,
    .version = 0,
    .xa_open_entry = xa_open_entry_stat,
    .xa_close_entry = xa_close_entry_stat,
    .xa_start_entry = xa_start_entry_stat,
    .xa_end_entry = xa_end_entry_stat,
    .xa_rollback_entry = xa_rollback_entry_stat,
    .xa_prepare_entry = xa_prepare_entry_stat,
    .xa_commit_entry = xa_commit_entry_stat_tryfail,
    .xa_recover_entry = xa_recover_entry_stat,
    .xa_forget_entry = xa_forget_entry_stat,
    .xa_complete_entry = xa_complete_entry_stat
};

struct xa_switch_t ndrxstatswtryok = 
{ 
    .name = "ndrxstatswtryok",
    .flags = TMNOFLAGS,
    .version = 0,
    .xa_open_entry = xa_open_entry_stat,
    .xa_close_entry = xa_close_entry_stat,
    .xa_start_entry = xa_start_entry_stat,
    .xa_end_entry = xa_end_entry_stat,
    .xa_rollback_entry = xa_rollback_entry_stat,
    .xa_prepare_entry = xa_prepare_entry_stat,
    .xa_commit_entry = xa_commit_entry_stat_tryok,
    .xa_recover_entry = xa_recover_entry_stat,
    .xa_forget_entry = xa_forget_entry_stat,
    .xa_complete_entry = xa_complete_entry_stat
};


struct xa_switch_t ndrxstatsw105 = 
{ 
    .name = "ndrxstatsw105",
    .flags = TMNOFLAGS,
    .version = 0,
    .xa_open_entry = xa_open_entry_stat,
    .xa_close_entry = xa_close_entry_stat,
    .xa_start_entry = xa_start_entry_stat,
    .xa_end_entry = xa_end_entry_stat,
    .xa_rollback_entry = xa_rollback_entry_stat,
    .xa_prepare_entry = xa_prepare_entry_stat105,
    .xa_commit_entry = xa_commit_entry_stat,
    .xa_recover_entry = xa_recover_entry_stat,
    .xa_forget_entry = xa_forget_entry_stat,
    .xa_complete_entry = xa_complete_entry_stat
};

struct xa_switch_t ndrxstatsw = 
{ 
    .name = "ndrxstatsw",
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
    .name = "ndrxdynsw",
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
exprivate char *get_file_name(XID *xid, int rmid, char *folder)
{
    static char buf[2048];
    char xid_str[128];
    static int first = EXTRUE;
    static char test_root[FILENAME_MAX+1] = {EXEOS};
    
    if (first)
    {
        NDRX_STRCPY_SAFE(test_root, getenv("NDRX_TEST_RM_DIR"));
        first = EXFALSE;
    }
    
    atmi_xa_serialize_xid(xid, xid_str);
    
    snprintf(buf, sizeof(buf), "%s/%s/%s", test_root, folder, xid_str);
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
exprivate int file_move(XID *xid, int rmid, char *from_folder, char *to_folder)
{
    int ret = EXSUCCEED;
    
    char from_file[FILENAME_MAX+1] = {EXEOS};
    char to_file[FILENAME_MAX+1] = {EXEOS};
    
    NDRX_STRCPY_SAFE(from_file, get_file_name(xid, rmid, from_folder));
    NDRX_STRCPY_SAFE(to_file, get_file_name(xid, rmid, to_folder));
    
    
    if (EXSUCCEED!=rename(from_file, to_file))
    {
        NDRX_LOG(log_error, "Failed to rename: %s", strerror(errno));
        ret=EXFAIL;
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
expublic int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    if (M_is_open)
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_open_entry() - already open!");
        return XAER_RMERR;
    }
    M_is_open = EXTRUE;
    M_rmid = rmid;
             
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
    
    if (!M_is_open)
    {
        /* Ignore this error...
        NDRX_LOG(log_error, "TESTERROR!!! xa_close_entry() - already closed!");
         */
        return XAER_RMERR;
    }
    
    M_is_open = EXFALSE;
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
expublic int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *file = get_file_name(xid, rmid, "active");
    
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_start_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if ((flags & TMJOIN) || (flags & TMRESUME))
    {
        if (!ndrx_file_exists(file))
        {
            NDRX_LOG(log_error, "TMJOIN or TMRESUME but transaction does not exits (file: [%s])!",
			file);
            return XAER_NOTA;
        }
    }
    
    /* Open file for write... */
    if (NULL==(M_f = NDRX_FOPEN(file, "a")))
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_start_entry() - failed to open file: %s!", 
                strerror(errno));
        return XAER_RMERR;
    }
    
    return XA_OK;
}

expublic int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_end_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (NULL==M_f)
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_end_entry() - "
                "transaction already closed: %s!", 
                strerror(errno));
        return XAER_RMERR;
    }
    
    if (M_is_reg)
    {
        if (EXSUCCEED!=ax_unreg(rmid, 0))
        {
            NDRX_LOG(log_error, "TESTERROR!!! xa_end_entry() - "
                    "ax_unreg() fail!");
            return XAER_RMERR;
        }
        
        M_is_reg = EXFALSE;
    }
    
out:
    if (M_f)
    {
        NDRX_FCLOSE(M_f);
        M_f = NULL;
    }
    return XA_OK;
}

expublic int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_rollback_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (EXSUCCEED!=file_move(xid, rmid, "active", "aborted") &&
            EXSUCCEED!=file_move(xid, rmid, "prepared", "aborted"))
    {
        return XAER_NOTA;
    }
    
    return XA_OK;
}

expublic int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_prepare_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (EXSUCCEED!=file_move(xid, rmid, "active", "prepared"))
    {
        return XAER_RMERR;
    }
    
    return XA_OK;
    
}

expublic int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_commit_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    
    if (EXSUCCEED!=file_move(xid, rmid, "prepared", "committed"))
    {
        return XAER_RMERR;
    }
    
    return XA_OK;
}

/**
 * Write some stuff to transaction!!!
 * @param buf
 * @return 
 */
expublic int __write_to_tx_file(char *buf)
{
    int ret = EXSUCCEED;
    XID xid;
    int len = strlen(buf);
    
    if (G_atmi_env.xa_sw->flags & TMREGISTER && !M_is_reg)
    {
        if (EXSUCCEED!=ax_reg(M_rmid, &xid, 0))
        {
            NDRX_LOG(log_error, "TESTERROR!!! xa_reg() failed!");
            ret=EXFAIL;
            goto out;
        }
        
        if (XA_OK!=xa_start_entry(G_atmi_env.xa_sw, &xid, M_rmid, 0))
        {
            NDRX_LOG(log_error, "TESTERROR!!! xa_start_entry() failed!");
            ret=EXFAIL;
            goto out;
        }
        
        M_is_reg = EXTRUE;
    }
    
    if (NULL==M_f)
    {
        NDRX_LOG(log_error, "TESTERROR!!! write with no tx file!!!");
        ret=EXFAIL;
        goto out;
    }
    
    if (fprintf(M_f, "%s", buf) < len)
    {
        NDRX_LOG(log_error, "TESTERROR!!! Failed to write to transaction!");
        ret=EXFAIL;
        goto out;
    }
    
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
expublic int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags)
{
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "TESTERROR!!! xa_recover_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "WARNING!!! xa_recover_entry() - STUB!!");
    return 0; /* 0 transactions found... */
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
        NDRX_LOG(log_error, "TESTERROR!!! xa_forget_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "TESTERROR!!! xa_forget_entry() - not implemented!!");
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
        NDRX_LOG(log_error, "TESTERROR!!! xa_complete_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "TESTERROR!!! xa_complete_entry() - not using!!");
    return XAER_RMERR;
}


/* Static entries */
expublic int xa_open_entry_stat( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxstatsw, xa_info, rmid, flags);
}
expublic int xa_close_entry_stat(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxstatsw, xa_info, rmid, flags);
}
expublic int xa_start_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxstatsw, xid, rmid, flags);
}

/**
 * For test case #160, retry to reconnect...
 * So on every 3 start we return SUCCEED;
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_start_entry_statstartfail(XID *xid, int rmid, long flags)
{
    static __thread int cntr = 0;
    cntr++;
    
    if (0!=cntr%3)
    {
        NDRX_LOG(log_error, "start FAIL (%d) - close connection too...", cntr);
        M_is_open = EXFALSE;
        return XAER_RMERR;
    }
    else
    {
        return xa_start_entry(&ndrxstatsw, xid, rmid, flags);
    }
    
}

expublic int xa_end_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxstatsw, xid, rmid, flags);
}
expublic int xa_rollback_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxstatsw, xid, rmid, flags);
}
expublic int xa_prepare_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxstatsw, xid, rmid, flags);
}

/**
 * Test case for bug 105 - abort after transaction is prepared...
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_prepare_entry_stat105(XID *xid, int rmid, long flags)
{
    int ret =  xa_prepare_entry(&ndrxstatsw, xid, rmid, flags);
    
    /* seems have issues with freebsd ... abort();*/
    exit(EXFAIL);
}

expublic int xa_commit_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxstatsw, xid, rmid, flags);
}

/**
 * Bug #123 - test try counter
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_commit_entry_stat_tryok(XID *xid, int rmid, long flags)
{
    static int try=0;
    char *fn = "xa_commit_entry_stat_tryok";
    
    try++;
    
    if (try > 10 || 2==rmid)
    {
        NDRX_LOG(log_error, "%s: try %d - continue,", fn, try);
        return xa_commit_entry(&ndrxstatsw, xid, rmid, flags);
    }
    else
    {
        NDRX_LOG(log_error, "%s: try %d - ret err", fn, try);
        return XA_RETRY;
    }
    
}

/**
 * Bug #123 - test try counter
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_commit_entry_stat_tryfail(XID *xid, int rmid, long flags)
{
    static int try=0;
    char *fn = "xa_commit_entry_stat_tryfail";
    try++;

    if (try > 30 || 2==rmid)
    {
        NDRX_LOG(log_error, "%s: try %d - continue", fn, try);
        return xa_commit_entry(&ndrxstatsw, xid, rmid, flags);
    }
    else
    {
        NDRX_LOG(log_error, "%s: try %d - ret err", fn, try);
        return XA_RETRY;
    }
}

expublic int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxstatsw, xid, count, rmid, flags);
}
expublic int xa_forget_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxstatsw, xid, rmid, flags);
}
expublic int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxstatsw, handle, retval, rmid, flags);
}

/* Dynamic entries */
expublic int xa_open_entry_dyn( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxdynsw, xa_info, rmid, flags);
}
expublic int xa_close_entry_dyn(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxdynsw, xa_info, rmid, flags);
}
expublic int xa_start_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxdynsw, xid, rmid, flags);
}
expublic int xa_end_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxdynsw, xid, rmid, flags);
}
expublic int xa_rollback_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxdynsw, xid, rmid, flags);
}
expublic int xa_prepare_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxdynsw, xid, rmid, flags);
}
expublic int xa_commit_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxdynsw, xid, rmid, flags);
}
expublic int xa_recover_entry_dyn(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxdynsw, xid, count, rmid, flags);
}
expublic int xa_forget_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxdynsw, xid, rmid, flags);
}
expublic int xa_complete_entry_dyn(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxdynsw, handle, retval, rmid, flags);
}
