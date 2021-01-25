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
#include <linux/limits.h>
#include <sys/types.h>
#include <nstopwatch.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/** Number of pages used to flip-flop the active DDR routing SHM area   */
#define NDRX_LCF_DDR_PAGES             2 
#define NDRX_LCF_RESVR                 4096 /**< Reserved space for future commands and seamless update        */
#define NDRX_LCFCNT_DEFAULT            20   /**< Default count of LCF commands                                 */
#define NDRX_LCF_STARTCMDEXP           60   /**< Number of secs for exiting command to be expired for new proc */
#define NDRX_LCF_FEEDBACK_BUF          64   /**< ASCII feedback buffer from command */
#define NDRX_LCF_ADMINCMD_MAX          64   /**< Admin command max lenght     */
#define NDRX_LCF_ADMINDSCR_MAX         128  /**< Max description for admin cmd */

#define NDRX_LCF_FLAG_PIDREX        0x00000001   /**< Interpret PID as regexp */
#define NDRX_LCF_FLAG_BINREX        0x00000002   /**< Interpret BINNAME as regexp */
#define NDRX_LCF_FLAG_ALL           0x00000004   /**< Apply to all processes  */
#define NDRX_LCF_FLAG_ARGA          0x00000008   /**< Arg A required          */
#define NDRX_LCF_FLAG_ARGB          0x00000010   /**< Arg B required          */
#define NDRX_LCF_FLAG_DOSTARTUP     0x00000020   /**< Execute command at startup */
#define NDRX_LCF_FLAG_DOSTARTUPEXP  0x00000040   /**< Execute at startup, having w expiry */
#define NDRX_LCF_FLAG_FBACKCODE     0x00000100   /**< Feedback code loaded    */
#define NDRX_LCF_FLAG_FBACKMSG      0x00000200   /**< Feedback message loaded */
    
    
#define NDRX_LCF_CMD_VERSION            1   /**< Current version number */
#define NDRX_LCF_CMD_MIN                0   /**< minimum accepted command   */
#define NDRX_LCF_CMD_DISABLED           0   /**< Command is disabled        */
#define NDRX_LCF_CMD_LOGROTATE          1   /**< Perfrom logrotated         */
#define NDRX_LCF_CMD_LOGCHG             2   /**< Change logger params       */
#define NDRX_LCF_CMD_MAX_PROD           999 /**< Maximum product command    */    
    
#define NDRX_LCF_CMD_MIN_CUST           1000 /**< Minimum user command code */
    
#define NDRX_LCF_CMD_MAX_CUST           1999 /**< Maximum user command code */
    
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
    ndrx_stopwatch_t publtim;/**< Time when command was published                    */
    int    command;         /**< Command code                                        */
    char   arg1[PATH_MAX];  /**< Argument 1                                          */
    char   arg2[NAME_MAX];  /**< Argument 2, shorter to save some memory             */
    long   flags;           /**< LCF Command flags                                   */
    
    /* To whom: */
    char   pid[NAME_MAX];   /**< str/regexp buffer for checking the PID              */
    char   bin[NAME_MAX];   /**< str/regexp buffer for checking the binary name      */
    
    /* To metrics: */
    
    int    applied;         /**< binaries applied the command                        */
    int    failed;          /**< either regexp failed, or the target callback failed */
    int    seen;            /**< Number of processes seen, but not matched           */
    long   fbackcode;       /**< Feedback code from last who executed                */
    char   fbackmsg[NDRX_LCF_FEEDBACK_BUF];    /**< Feedback message from who executed*/
    
} ndrx_lcf_command_t;


/**
 * Standard library configuration managed by lcf
 */
typedef struct
{
    int version;    /**< API version            */
    int command;    /**< lcf comand code        */
    
    /** API receives copy (snapshoot) of mem block 
     * @param cmd memory snapshoot
     * @param flags output flags about feedback: NDRX_LCF_FLAG_FBACK*
     * @return EXSUCCEED/EXFAIL
     */
    int (*pf_callback)(ndrx_lcf_command_t *cmd, long *flags);
    
} ndrx_lcf_reg_func_t;

/**
 * Register command with xadmin
 */
typedef struct
{
    int version;    /**< API version                                              */
    char cmdstr[NDRX_LCF_ADMINCMD_MAX]; /**< Command code $ xadmin lcf <code>     */
    int command;    /**< lcf comand code                                          */
    char helpstr[NDRX_LCF_ADMINDSCR_MAX]; /**< Help text for command              */
    long flags;     /**< ARGA, ARGB, DOSTARTUP, DOSTARTUPDEL                      */
    int dfltslot;   /**< Default slot numbere wher command shall be installed     */
} ndrx_lcf_reg_xadmin_t;


/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/



#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
