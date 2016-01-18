/* 
** Client Process Monitor Server
**
** @file cpmsrv.h
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

#ifndef CPMSRV_H
#define	CPMSRV_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <uthash.h>
#include <ntimer.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CPM_TAG_LEN             128
#define CPM_SUBSECT_LEN         128
#define CPM_KEY_LEN             (CPM_TAG_LEN+1+CPM_SUBSECT_LEN) /* including FS in middle */
    
/* Process flags */
#define CPM_F_AUTO_START          1      /* bit for auto start                      */
#define CPM_F_EXTENSIVE_CHECK     2     /* do extensive checks for process existance */
    
    
#define NDRX_CLTTAG                 "NDRX_CLTTAG" /* Tag format string         */
#define NDRX_CLTSUBSECT             "NDRX_CLTSUBSECT" /* Subsect format string */
    
#define S_FS                        0x1c /* Field seperator */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * Definition of client processes (full command line & all settings)
 */
struct cpm_process
{   
    char key[CPM_KEY_LEN+1]; /* tag<FS>subsect */
    
    char tag[CPM_TAG_LEN+1];
    char subsect[CPM_SUBSECT_LEN+1];
    
    pid_t pid;  /* pid of the process */
    int is_running; /* is process running? */
    int exit_status; /* Exit status... */
    int consecutive_fail; /*  Number of consecutive fails */    
    
    char command_line[PATH_MAX+1+CPM_TAG_LEN+CPM_SUBSECT_LEN];
    char env[PATH_MAX+1];
    n_timer_t last_stat_chg; /* Last time status changes */
    long flags;              /* flags of the process */
    
    
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct cpm_process cpm_process_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


#ifdef	__cplusplus
}
#endif

#endif	/* CPMSRV_H */

