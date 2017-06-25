/* 
** Memory checking routines
**
** @file exmemck.h
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
#ifndef EXMEMCK_H
#define EXMEMCK_H


#if defined(__cplusplus)
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <exhash.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/* So we need two structures here:
 * 1. Configuration
 * 2. Per process statistics 
 */

/**
 * Memory check config
 */
struct exmemck_config
{
    char mask[PATH_MAX+1]; 
    
    long mem_limit;
    int percent_diff_allow;
    char dlft_mask[PATH_MAX+1]; 
    int interval_start_prcnt;
    int interval_stop_prcnt;
    long flags;

    EX_hash_handle hh;
};
typedef struct exmemck_config exmemck_config_t;

/**
 * Statistics entry point...
 */
struct exmemck_statentry
{
    long rss;
    long vsz;
};
typedef struct exmemck_statentry exmemck_statentry_t;

/**
 * Definition of client processes (full command line & all settings)
 */
struct exmemck_process
{   
    int pid;                   /* which pid we are monitoring       */
    
    char psout[PATH_MAX+1];     /* ps string we are monitoring      */
    
    exmemck_config_t *p_config; /* when removing config, exmemck_process shall be removed too */
    exmemck_statentry_t *stats; /* Array of statistics               */
    int nr_of_stats;           /* Number array elements...          */
    
    EX_hash_handle hh;         /* makes this structure hashable     */
};
typedef struct exmemck_process exmemck_process_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


#if defined(__cplusplus)
}
#endif


#endif
