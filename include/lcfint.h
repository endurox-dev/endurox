/**
 * @brief Latent command framework. Part of the Enduro/X standard library
 *  internal part.
 * 
 * @file lcfint.h
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
#ifndef LCFINT_H__
#define LCFINT_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <nstopwatch.h>
#include <lcf.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_LCF_DDR_PAGES             2 
#define NDRX_LCF_RESVR                 4096 /**< Reserved space for future commands and seamless update        */
#define NDRX_LCFCNT_DEFAULT            20   /**< Default count of LCF commands                                 */
#define NDRX_LCF_STARTCMDEXP           60   /**< Number of secs for exiting command to be expired for new proc */

    
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
    int  svqreadersmax;    /**< Max number of concurrent lckrds*/
    int  lcfreadersmax; /**< Max number of concurrent lckrds*/
    
    int  lcfmax;        /**< Max number of LCF commands     */
    key_t ipckey;       /**< Semphoare key                  */
    int  startcmdexp;   /**< Startup command delay          */
    int lcf_norun;    /**< LCF is disabled                */
} ndrx_nstd_libconfig_t;

/**
 * This is local list of commands seen & processed
 * During the program startup we do no process any commands
 * Except logrotates 
 */
typedef struct
{
    int    cmdversion;         /**< command version, so that if we switch the logs check that
                                * command is not changed (i.e. we want to update the stats) */
    ndrx_stopwatch_t publtim;/**< Time when command was published                    */
    int    command;         /**< Command code                                        */
} ndrx_lcf_command_seen_t;

/**
 * Shared memory settings
 */
typedef struct
{
    unsigned shmcfgver_lcf;     /**< Monitor the version, if changed run LCF with readlock */
    int use_ddr;            /**< Should DDR be used by callers                         */
    long ddr_page;          /**< DDR page number  0 or 1, version not changes, using long for align */
    char reserved[NDRX_LCF_RESVR];  /**< reserved space for future updates             */
    
    /**< Array of LCF commands */
    ndrx_lcf_command_t commands[0];
} ndrx_lcf_shmcfg_t;

/**
 * Needs hash for function registration data -> hash by name 
 */
typedef struct 
{
    int command;    /**< lcf comand code        */
    ndrx_lcf_reg_func_t cfunc;
    EX_hash_handle hh; /**< makes this structure hashable               */
} ndrx_lcf_reg_funch_t;

/**
 * Needs hash with callback data, hash by command code       
 * This is used by xadmin lookups
 */
typedef struct 
{
    char cmdstr[NDRX_LCF_ADMINCMD_MAX+1]; /**< Command code */
    ndrx_lcf_reg_xadmin_t xcmd;
    EX_hash_handle hh; /**< makes this structure hashable               */
} ndrx_lcf_reg_xadminh_t;

/*---------------------------Globals------------------------------------*/
extern NDRX_API ndrx_lcf_shmcfg_t *ndrx_G_shmcfg;
extern NDRX_API ndrx_nstd_libconfig_t ndrx_G_libnstd_cfg;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API ndrx_nstd_libconfig_t ndrx_G_libnstd_cfg;

extern NDRX_API int ndrx_lcf_init(void);
extern NDRX_API void ndrx_lcf_detach(void);

/** Register callback for command code */
extern NDRX_API int ndrx_lcf_xadmin_add_int(ndrx_lcf_reg_xadmin_t *xcmd);
extern NDRX_API int ndrx_lcf_func_add_int(ndrx_lcf_reg_func_t *cfunc);
extern NDRX_API ndrx_lcf_reg_xadminh_t* ndrx_lcf_xadmin_find_int(char *cmdstr);
extern NDRX_API int ndrx_lcf_publish_int(int slot, ndrx_lcf_command_t *cmd);
extern NDRX_API void ndrx_lcf_xadmin_list(void (*pf_callback)(ndrx_lcf_reg_xadminh_t *xcmd));

extern NDRX_API int ndrx_lcf_read_lock(void);
extern NDRX_API int ndrx_lcf_read_unlock(void);
extern NDRX_API int ndrx_lcf_supported_int(void);
extern NDRX_API ndrx_lcf_reg_xadminh_t* ndrx_lcf_xadmin_find_int(char *cmdstr);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
