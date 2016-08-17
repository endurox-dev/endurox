/* 
** Enduro/X ini config driver
**
** @file inicfg.h
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
#ifndef _INICFG_H
#define	_INICFG_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrxdcmn.h>
#include <stdint.h>
#include <ntimer.h>
#include <uthash.h>
/*---------------------------Externs------------------------------------*/
#define NDRX_INICFG_SECTION_MAX  128    
#define NDRX_INICFG_SUBSECT_SPERATOR '/' /* seperate sub-sections with */
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * This is key/value entry (linked list)
 */
typedef struct ndrx_inicfg_section_keyval ndrx_inicfg_section_keyval_t;
struct ndrx_inicfg_section_keyval
{
    char section[NDRX_INICFG_SECTION_MAX]; /* section */
    char *key; /* key for ini */
    char *val; /* value for ini */
    
    UT_hash_handle hh;         /* makes this structure hashable */
};

/**
 * This is section handler
 */    
struct ndrx_inicfg_section
{
    char section[NDRX_INICFG_SECTION_MAX]; /* section */
    
    ndrx_inicfg_section_keyval_t *values; /* list of values */
    
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct ndrx_inicfg_section ndrx_inicfg_section_t;

/**
 * Config handler
 */
struct ndrx_inicfg_file
{
    /* full path */
    char fullname[PATH_MAX+1];
    /* original path */
    char resource[PATH_MAX+1];
    /* time stamp when read */
    
    ndrx_inicfg_section_t *sections;
    
    int not_refreshed; /* marked as not refreshed (to kill after reload) */
    UT_hash_handle hh;         /* makes this structure hashable */
};

typedef struct ndrx_inicfg_file ndrx_inicfg_file_t;


/**
 * Config handle
 */
struct ndrx_inicfg
{
    ndrx_inicfg_file_t *cfgfile;
};

typedef struct ndrx_inicfg ndrx_inicfg_t;




/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
#ifdef	__cplusplus
}
#endif

#endif	/* _INICFG_H */

