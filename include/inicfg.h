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
#include <sys/stat.h>
#include <ndrxdcmn.h>
#include <stdint.h>
#include <ntimer.h>
#include <uthash.h>
#include <sys_unix.h>
/*---------------------------Externs------------------------------------*/
#define NDRX_INICFG_SUBSECT_SPERATOR '/' /* seperate sub-sections with */
#define NDRX_INICFG_SUBSECT_SPERATOR_STR "/" /* seperate sub-sections with */
#define NDRX_INICFG_RESOURCES_MAX   5
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * This is key/value entry (linked list)
 */
typedef struct ndrx_inicfg_section_keyval ndrx_inicfg_section_keyval_t;
struct ndrx_inicfg_section_keyval
{
    char *section; /* section */
    char *key; /* key for ini */
    char *val; /* value for ini */
    /* TODO: */
    int subsection_level; /* the level of subsections (we can overwrite
                           *  data with deeper sections) */
    
    UT_hash_handle hh;         /* makes this structure hashable */
};

/**
 * This is section handler
 */    
struct ndrx_inicfg_section
{
    char *section; /* section */
    
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
    
    struct stat attr; /* time stamp when read */
    
    ndrx_inicfg_section_t *sections;
    
    int refreshed; /* marked as not refreshed (to kill after reload) */
    UT_hash_handle hh;         /* makes this structure hashable */
};

typedef struct ndrx_inicfg_file ndrx_inicfg_file_t;


/**
 * Config handle
 */
struct ndrx_inicfg
{
    /* resource files (if set to EOS, not used) */
    /* List of resources */
    string_hash_t *resource_hash;
    ndrx_inicfg_file_t *cfgfile;
};

typedef struct ndrx_inicfg ndrx_inicfg_t;


/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API  ndrx_inicfg_t * ndrx_inicfg_new(void);
extern NDRX_API  int ndrx_inicfg_load_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with);
extern NDRX_API  int ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with);
extern NDRX_API  int ndrx_inicfg_add(ndrx_inicfg_t *cfg, char *resource, char **section_start_with);
extern NDRX_API  int ndrx_inicfg_reload(ndrx_inicfg_t *cfg, char **section_start_with);
extern NDRX_API  int ndrx_keyval_hash_add(ndrx_inicfg_section_keyval_t **h, 
            ndrx_inicfg_section_keyval_t *src);
extern NDRX_API  ndrx_inicfg_section_keyval_t * ndrx_keyval_hash_get(
        ndrx_inicfg_section_keyval_t *h, char *key);
extern NDRX_API  void ndrx_keyval_hash_free(ndrx_inicfg_section_keyval_t *h);
extern NDRX_API  int ndrx_inicfg_resolve(ndrx_inicfg_t *cfg, char **resources, char *section, 
        ndrx_inicfg_section_keyval_t **out);
extern NDRX_API  int ndrx_inicfg_get_subsect(ndrx_inicfg_t *cfg, 
        char **resources, char *section, ndrx_inicfg_section_keyval_t **out);
extern NDRX_API  int ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, 
        char **resources,
        char **section_start_with, 
        ndrx_inicfg_section_t **out);
extern NDRX_API  void ndrx_inicfg_sections_free(ndrx_inicfg_section_t *sections);
extern NDRX_API  void ndrx_inicfg_file_free(ndrx_inicfg_t *cfg, ndrx_inicfg_file_t *cfgfile);
extern NDRX_API  void ndrx_inicfg_free(ndrx_inicfg_t *cfg);

#ifdef	__cplusplus
}
#endif

#endif	/* _INICFG_H */

