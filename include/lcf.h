/**
 * @brief LCF API (published)
 *
 * @file lcf.h
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
#ifndef LCF_H_
#define LCF_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <limits.h>
#include <sys/types.h>
#include <nstopwatch.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/** Number of pages used to flip-flop the active DDR routing SHM area   */
#define NDRX_LCF_FEEDBACK_BUF          64   /**< ASCII feedback buffer from command */
#define NDRX_LCF_ADMINCMD_MAX          32   /**< Admin command max lenght     */
#define NDRX_LCF_ADMINDSCR_MAX         128  /**< Max description for admin cmd */

#define NDRX_LCF_FLAG_PID           0x00000001   /**< Interpret PID as regexp, xadmin */
#define NDRX_LCF_FLAG_BIN           0x00000002   /**< Interpret BINNAME as regexp, xadmin */
#define NDRX_LCF_FLAG_ALL           0x00000004   /**< Apply to all processes, xadmin  */
#define NDRX_LCF_FLAG_ARGA          0x00000008   /**< Arg A required, xadmin reg, chk args   */
#define NDRX_LCF_FLAG_ARGB          0x00000010   /**< Arg B required, xadmin reg, chk args   */
#define NDRX_LCF_FLAG_DOSTARTUP     0x00000020   /**< Execute command at startup, xadmin, xadmin reg default+ */
#define NDRX_LCF_FLAG_DOSTARTUPEXP  0x00000040   /**< Execute at startup, having w expiry, xadmin reg default+ */
#define NDRX_LCF_FLAG_DOREX         0x00000080   /**< Check BIN or PID by regexp */
    
#define NDRX_LCF_FLAG_FBACK_CODE    0x00000100   /**< Feedback code is loaded  */
#define NDRX_LCF_FLAG_FBACK_MSG     0x00000200    /**< Feedback msg  is loaded  */
    
/**< Callback reg struct version (ndrx_lcf_reg_func_t)  */
#define NDRX_LCF_CCMD_VERSION            1
/**< Xadmin command version (ndrx_lcf_reg_xadmin_t)     */
#define NDRX_LCF_XCMD_VERSION            1
/**< LCF Command version   (ndrx_lcf_command_t)         */
#define NDRX_LCF_LCMD_VERSION            1 
    
#define NDRX_LCF_CMD_MIN                0   /**< minimum accepted command   */
#define NDRX_LCF_CMD_DISABLE            0   /**< Command is disabled        */
#define NDRX_LCF_CMD_LOGROTATE          1   /**< Perfrom logrotated         */
#define NDRX_LCF_CMD_LOGCHG             2   /**< Change logger params       */
#define NDRX_LCF_CMD_MAX_PROD           999 /**< Maximum product command    */    
#define NDRX_LCF_CMD_MIN_CUST           1000 /**< Minimum user command code */
#define NDRX_LCF_CMD_MAX_CUST           1999 /**< Maximum user command code */
    
#define NDRX_LCF_CMDSTR_DISABLE         "disable"
#define NDRX_LCF_CMDSTR_LOGROTATE       "logrotate"
#define NDRX_LCF_CMDSTR_LOGCHG          "logchg"    
    
#define NDRX_LCF_SLOT_LOGROTATE         0   /**< Default command slot for logrotate */
#define NDRX_LCF_SLOT_LOGCHG            1   /**< Default slot for log re-configure  */

#define NDRX_NAME_MAX			64  /**< Name max	*/
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/* LCF will provide shared memory block */

/**
 * Actual command for LCF data
 * For command registration:
 * - Have a plugin call to register the commands (id, name, arg A / B / none / descr).
 *    these will appear in xadmin lcf <command>
 * - Have a plugin to export a callbacks for the LCF command. So that user
 * application may implement handlers
 */
typedef struct
{
    /* The command: */
    int    version;         /**< Version number of the c struct                         */
    unsigned  cmdversion;   /**< command version, so that if we switch the logs check that
                             * command is not changed (i.e. we want to update the stats) */
    
    char cmdstr[NDRX_LCF_ADMINCMD_MAX+1]; /**< Command code $ xadmin lcf <code>     */
    
    ndrx_stopwatch_t publtim;/**< Time when command was published                    */
    int    command;         /**< Command code                                        */
    char   arg_a[PATH_MAX+1]; /**< Argument a                                        */
    char   arg_b[NDRX_NAME_MAX+1]; /**< Argument b, shorter to save some memor       */
    long   flags;           /**< LCF Command flags                                   */
    
    /* To whom: */
    char   procid[NDRX_NAME_MAX];  /**< PID or program name                          */
    
    /* To metrics: */
    int    applied;         /**< binaries applied the command                        */
    int    failed;          /**< either regexp failed, or the target callback failed */
    int    seen;            /**< Number of processes seen, but not matched           */
    long   fbackcode;       /**< Feedback code from last who executed, user changed  */
    char   fbackmsg[NDRX_LCF_FEEDBACK_BUF];    /**< Feedback message, user chnaged   */
    
} ndrx_lcf_command_t;


/**
 * Standard library configuration managed by lcf
 */
typedef struct
{
    int version;    /**< API version            */
    int command;    /**< lcf comand code        */
    char cmdstr[NDRX_LCF_ADMINCMD_MAX+1]; /**< For debug purposes     */
    
    /** API receives copy (snapshoot) of mem block 
     * @param cmd this is constant and must not be changed by process
     *  it is data from shared memory directly.
     * @param flags output flags about feedback: NDRX_LCF_FLAG_FBACK*. On input value is 0.
     * @return EXSUCCEED/EXFAIL
     */
    int (*pf_callback)(ndrx_lcf_command_t *cmd, long *p_flags);
    
} ndrx_lcf_reg_func_t;

/**
 * Register command with xadmin
 */
typedef struct
{
    int version;    /**< API version                                              */
    char cmdstr[NDRX_LCF_ADMINCMD_MAX+1]; /**< Command code $ xadmin lcf <code>     */
    int command;    /**< lcf comand code                                          */
    char helpstr[NDRX_LCF_ADMINDSCR_MAX+1]; /**< Help text for command              */
    long dfltflags;     /**< ARGA, ARGB, DOSTARTUP, DOSTARTUPDEL                      */
    int dfltslot;   /**< Default slot numbere wher command shall be installed     */
} ndrx_lcf_reg_xadmin_t;


/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_lcf_xadmin_add(ndrx_lcf_reg_xadmin_t *xcmd);
extern NDRX_API int ndrx_lcf_func_add(ndrx_lcf_reg_func_t *cfunc);
extern NDRX_API int ndrx_lcf_publish(int slot, ndrx_lcf_command_t *cmd);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
