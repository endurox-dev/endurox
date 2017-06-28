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
#include <nstopwatch.h>
#include <regex.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define EXMEMCK_STATUS_OK              0x0000   /* OK, no memory leaks detected */
#define EXMEMCK_STATUS_LEAKY_RSS       0x0001   /* Leak detected, RSS           */
#define EXMEMCK_STATUS_LEAKY_VSZ       0x0002   /* Leak detected, VSZ           */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

typedef struct exmemck_process exmemck_process_t;
    
/* So we need two structures here:
 * 1. Configuration
 * 2. Per process statistics 
 */
    
struct exmemck_settings
{
    long mem_limit;
    int percent_diff_allow;
    int interval_start_prcnt;
    int interval_stop_prcnt;
    long flags;
    int interval_mon;       /* Interval into which monitor memory (with out exit).. */
    char negative_mask[PATH_MAX];   /* Negative mask (must not match), optional */
    int min_values;              /* minimum values for each halve */
    /* Have a callback for status notification */
    void (*pf_proc_exit) (exmemck_process_t *proc);
    void (*pf_proc_leaky) (exmemck_process_t *proc);
   
};

typedef struct exmemck_settings exmemck_settings_t;

/**
 * Memory check config
 */
struct exmemck_config
{
    char mask[PATH_MAX+1]; 
    char dlft_mask[PATH_MAX+1];
    regex_t mask_regex;         /* compiled mask */
    int neg_mask_used;          /* Is negative mask used? */
    regex_t neg_mask_regex;     /* compiled negative mask */
    
    exmemck_settings_t settings;
    ndrx_stopwatch_t mon_watch; /*monitor stopwatch if set interval mon*/

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
    exmemck_statentry_t *stats; /* Array of statistics              */
    int nr_of_stats;           /* Number array elements...          */
    
    int status;                 /* Calculated status                */
    
    int avg_first_count;       /* number of records for first halve */
    long avg_first_halve_rss;   /* first halve average bytes        */
    long avg_second_halve_rss;  /* second halve average bytes       */
    
    int avg_second_count;       /* number of records for second halve */
    long avg_first_halve_vsz;   /* first halve average bytes        */
    long avg_second_halve_vsz;  /* second halve average bytes       */
    
    double rss_increase_prcnt;
    double vsz_increase_prcnt;
    
    int proc_exists;        /* check for process existence... */
    
    EX_hash_handle hh;         /* makes this structure hashable     */
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int ndrx_memck_add(char *mask,  char *dlft_mask, 
        exmemck_settings_t *p_settings);
extern NDRX_API int ndrx_memck_rm(char *mask);
extern NDRX_API void ndrx_memck_rmall(void);
extern NDRX_API exmemck_process_t* ndrx_memck_getstats_single(pid_t pid);
extern NDRX_API exmemck_process_t* ndrx_memck_getstats(void);
extern NDRX_API void ndrx_memck_reset(char *mask);
extern NDRX_API void ndrx_memck_reset_pid(pid_t pid);
extern NDRX_API int ndrx_memck_tick(void);

#if defined(__cplusplus)
}
#endif


#endif
