/**
 * @brief Debug commons
 *  TOOD: mark as internal -> do not include in distribution
 *
 * @file ndebugcmn.h
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
#ifndef NDEBUGCMN_H
#define	NDEBUGCMN_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <unistd.h>
#include <thlock.h>
#include <exhash.h>
#include <sys_unix.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_LOG_MODULE_LEN     4   /**< Module name field length       */
    
#define NDRX_LOG_FPROC        0x00001 /**< This is process level logger */
#define NDRX_LOG_FOSHSTDERR   0x00004 /**< This is OS handler, stderr   */
#define NDRX_LOG_FOSHSTDOUT   0x00008 /**< This is OS handler, stdout   */
#define NDRX_LOG_FLOCK        0x00010 /**< Perform locking when write to file */
        
#define NDRX_LOG_OSSTDERR     "/dev/stderr" /**< This OS handler, stderr*/
#define NDRX_LOG_OSSTDOUT     "/dev/stdout" /**< This OS handler, stdout*/

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*
 * Early logs written to memory
 */
typedef struct ndrx_memlogger ndrx_memlogger_t;
struct ndrx_memlogger
{
    int level;
    char line[PATH_MAX+1];
    ndrx_memlogger_t *next, *prev;
};

/* Create main debug structure */
typedef struct
{
    int   level;
    void *dbg_f_ptr;   /**< Ptr to file sink, opaque                         */
    char filename[PATH_MAX];
    char filename_th_template[PATH_MAX]; /**< template for thread logging... */
    pid_t pid;
    /** Hashed hostname */
    unsigned long hostnamecrc32;
    int buf_lines; 
    int buffer_size;
    int lines_written;      
    char module[NDRX_LOG_MODULE_LEN+1]; /**< 4 symbols of the module        */
    int is_user;            /**< set to 1 if we run in user log mode, 2 if request file */
    char code;              /**< code of the logger                         */
    char iflags[16];        /**< integration flags                          */
    int is_threaded;        /**< are we separating logs by threads?         */
    int is_mkdir;           /**< shall we create directory if we get ENOFILE err */
    unsigned threadnr;      /**< thread number to which we are logging      */
    long flags;             /**< logger code initially                      */
    long swait;             /**< sync wait for close log files, ms          */
    ndrx_memlogger_t *memlog;
    unsigned version;        /**< This is settigns version (inherit by thread/req */
} ndrx_debug_t;

/**
 * This is opaque interface for detecting current LCF command 
 */
typedef struct
{
    unsigned shmcfgver_lcf;         /**< Current version in shared mem of the config */
} ndrx_lcf_shmcfg_ver_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/** Current shared memory published configuration version */
extern NDRX_API volatile ndrx_lcf_shmcfg_ver_t *ndrx_G_shmcfg_ver;
/** Last checked shared mem cfg version                   */
extern NDRX_API volatile unsigned              ndrx_G_shmcfgver_chk;
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* NDEBUGCMN_H */

/* vim: set ts=4 sw=4 et smartindent: */
