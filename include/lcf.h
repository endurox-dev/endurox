/**
 * @brief Latent command framework. Part of the Enduro/X standard library
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
#define NDRX_LCF_DDR_PAGES              2
#define NDRX_LCF_RESVR                 4096 /**< Reserved space for future commands and seamless update */
#define NDRX_LCFCNT_DEFAULT            20  /**< Default count of LCF commands       */
#define NDRX_LCF_FEEDBACK_BUF           64  /**< ASCII feedback buffer from command */

#define NDRX_LCF_FLAG_PIDREX           0x00000001   /**< Interpret PID as regexp */
#define NDRX_LCF_FLAG_BINREX                  0x00000001   /**< Interpret PID as regexp */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/* LCF will provide generic settings library settings for internal use */
    
/**
 * Standard library configuration managed by lcf
 */
typedef struct
{
    char *qprefix;      /**< Queue prefix used by mappings  */
    long queuesmax;     /**< Max number of queues           */
    int  readersmax;    /**< Max number of concurrent lckrds*/
    int  lcfmax;        /**< Max number of LCF commands     */
    key_t ipckey;       /**< Semphoare key                  */
} ndrx_nstd_libconfig_t;

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
    int    version;         /**< command version, so that if we switch the logs check that
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
 * Shared memory settings
 */
typedef struct
{
    unsigned version;       /**< Monitor the version, if changed run LCF with readlock */
    int use_ddr;            /**< Should DDR be used by callers                         */
    long ddr_page;          /**< DDR page number  0 or 1, version not changes, using long for align */
    char reserved[NDRX_LCF_RESVR];  /**< reserved space for future updates             */
    
    /**< Array of LCF commands */
    ndrx_lcf_command_t commands[0];
} ndrx_lcf_shmcfg_t;
    

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern ndrx_nstd_libconfig_t ndrx_G_libnstd_cfg;

extern int ndrx_lcf_init(void);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
