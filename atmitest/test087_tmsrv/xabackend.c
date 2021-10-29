/**
 * @brief XA tracking switch may produce different output results for different
 *  configuration, with purpose of diagnostics of the state machines.
 *
 * @file xatracesw.c
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
#include <errno.h>
#include <ndrstandard.h>
#include <xa.h>
#include <userlog.h>
#include <tmenv.h>
#include <thlock.h>

#include "ndebug.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#ifdef TEST_RM1

#define TEST_LOGFILE    "lib1.log"
#define TEST_RETS       "lib1.rets"
#define TRM             "RM1"

#else

#define TEST_LOGFILE    "lib2.log"
#define TEST_RETS       "lib2.rets"
#define TRM             "RM2"

#endif

#define MAX_LEN     1024    /**< Max command len                        */
#define NR_SETTINGS 4       /**< Number of settings used                */

#define SETTING_FUNC        0
#define SETTING_RET1        1
#define SETTING_CNT         2
#define SETTING_RET2        3

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate MUTEX_LOCKDECL(M_init);
__thread int M_is_open = 0;
__thread int M_rmid = -1;
__thread int M_is_reg = 0; /* Dynamic registration done? */
/*---------------------------Prototypes---------------------------------*/

static int xa_open_entry(char *xa_info, int rmid, long flags);
static int xa_close_entry(char *xa_info, int rmid, long flags);
static int xa_start_entry(XID *xid, int rmid, long flags);
static int xa_end_entry(XID *xid, int rmid, long flags);
static int xa_rollback_entry(XID *xid, int rmid, long flags);
static int xa_prepare_entry(XID *xid, int rmid, long flags);
static int xa_commit_entry(XID *xid, int rmid, long flags);
static int xa_recover_entry(XID *xid, long count, int rmid, long flags);
static int xa_forget_entry(XID *xid, int rmid, long flags);
static int xa_complete_entry(int *handle, int *retval, int rmid, long flags);

/* Also needs to configure the flags
 * we might want to perform testing when JOIN is available
 * and we might want check the cases when JOIN is disabled.
 */
struct xa_switch_t exxaswitch = 
{ 
    .name = "exxaswitch",
    .flags = TMNOFLAGS,
    .version = 0,
    .xa_open_entry = xa_open_entry,
    .xa_close_entry = xa_close_entry,
    .xa_start_entry = xa_start_entry,
    .xa_end_entry = xa_end_entry,
    .xa_rollback_entry = xa_rollback_entry,
    .xa_prepare_entry = xa_prepare_entry,
    .xa_commit_entry = xa_commit_entry,
    .xa_recover_entry = xa_recover_entry,
    .xa_forget_entry = xa_forget_entry,
    .xa_complete_entry = xa_complete_entry
};

/**
 * Get command code
 * @param func function that asks for return code
 * @param cntr counter of try
 * @return XA return code
 */
static int get_return_code(const char *func, int *cntr)
{
    FILE* fp;
    char buffer[MAX_LEN];
    int matched = 0;
    char *setting, *saveptr1;
    char *all_settings[4];
    int i, line=0, ret;

    fp = fopen(TEST_RETS, "r");
    
    if (fp == NULL)
    {
      perror("Failed to open return file: " TEST_RETS);
      exit(1);
    }

    while (fgets(buffer, MAX_LEN - 1, fp))
    {
        /* Remove trailing newline */
        buffer[strcspn(buffer, "\n")] = 0;
        buffer[strcspn(buffer, "\r")] = 0;
    
        /* format  is
         * func_name;ret1;attempts;ret2
         * where func_name is matched by func
         * ret1 is returned while cntr < attempts
         * ret2 if returned if cntr>= attempts
         */
        for (i=0, setting=strtok_r(buffer, ":", &saveptr1); NULL!=setting && i<NR_SETTINGS; i++, setting=strtok_r(NULL, ":", &saveptr1))
        {
            all_settings[i] = setting;
        }
        
        if (i!=NR_SETTINGS)
        {
            userlog( TRM ": Invalid settings, line %d expect args %d got %d", 
                    line, NR_SETTINGS, i);
            exit(-1);
        }
        
        if (0==strcmp(all_settings[SETTING_FUNC], func))
        {
            matched=1;
            break;
        }
        
        line++;
    }
    
    fclose(fp);
    
    if (!matched)
    {
        userlog(TRM ": func not found [%s]", func);
        exit(-1);
    }
    
    /* start to process the command */
    
    if (*cntr < atoi(all_settings[SETTING_CNT]))
    {
        *cntr = *cntr + 1;
        NDRX_LOG(log_error, "%s counter: %d", func, *cntr);
        ret=atoi(all_settings[SETTING_RET1]);
    }
    else
    {
        ret=atoi(all_settings[SETTING_RET2]);
    }
    
    userlog(TRM ": FUNC [%s] return %d", func, ret);
    
    return ret;
}


#define ATTEMPT(closed_OK) do {\
        static int attempt = 0;\
        if (!closed_OK && !M_is_open) \
        {\
            return XAER_RMERR;\
        }\
        userlog(TRM ": %s attempt=%d rmid=%d flags=%ld (TMASYNC=%d TMONEPHASE=%d "\
        "TMFAIL=%d TMNOWAIT=%d TMRESUME=%d TMSUCCESS=%d TMSUSPEND=%d TMSTARTRSCAN=%d TMENDRSCAN=%d TMMULTIPLE=%d TMJOIN=%d TMMIGRATE=%d)",\
            __func__, attempt, rmid, flags,\
            !!(flags & TMASYNC),\
            !!(flags & TMONEPHASE),\
            !!(flags & TMFAIL),\
            !!(flags & TMNOWAIT),\
            !!(flags & TMRESUME),\
            !!(flags & TMSUCCESS),\
            !!(flags & TMSUSPEND),\
            !!(flags & TMSTARTRSCAN),\
            !!(flags & TMENDRSCAN),\
            !!(flags & TMMULTIPLE),\
            !!(flags & TMJOIN),\
            !!(flags & TMMIGRATE));\
        return get_return_code(__func__, &attempt);\
    } while (0)

static int xa_open_entry(char *xa_info, int rmid, long flags)
{
    static int first=EXTRUE;

#ifdef TEST_NOJOIN
    /* mark that suspend no required by this resource... */
    if (first)
    {
        MUTEX_LOCK_V(M_init);
        if (first)
        {
            ndrx_xa_nojoin(EXTRUE);
            first=EXFALSE;
        }
        MUTEX_UNLOCK_V(M_init);
    }
#endif
    
    if (M_is_open)
    {
        return XAER_RMERR;
    }

    M_is_open = 1;
    M_rmid = rmid;
             
    /* return XA_OK; */
    
    ATTEMPT(EXFALSE);
}

static int xa_close_entry(char *xa_info, int rmid, long flags)
{   
    if (!M_is_open)
    {
        return XAER_RMERR;
    }
    
    M_is_open = 0;
    /* return XA_OK; */
    
    ATTEMPT(EXTRUE);
}

static int xa_start_entry(XID *xid, int rmid, long flags)
{    
    ATTEMPT(EXFALSE);
}

static int xa_end_entry(XID *xid, int rmid, long flags)
{
    ATTEMPT(EXFALSE);
}

static int xa_rollback_entry(XID *xid, int rmid, long flags)
{
    ATTEMPT(EXFALSE);
}

static int xa_prepare_entry(XID *xid, int rmid, long flags)
{
    ATTEMPT(EXFALSE);
}

static int xa_commit_entry(XID *xid, int rmid, long flags)
{
    ATTEMPT(EXFALSE);
}

static int xa_recover_entry(XID *xid, long count, int rmid, long flags)
{
    ATTEMPT(EXFALSE);
}

static int xa_forget_entry(XID *xid, int rmid, long flags)
{
    ATTEMPT(EXFALSE);
}

static int xa_complete_entry(int *handle, int *retval, int rmid, long flags)
{
    ATTEMPT(EXFALSE);
}

/* vim: set ts=4 sw=4 et smartindent: */
