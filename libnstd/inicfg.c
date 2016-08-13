/* 
** Enduro/X advanced configuration driver
**
** @file inicfg.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ini.h>
#include <inicfg.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Create new config handler
 * @return ptr to config handler or NULL
 */
public ndrx_inicfg_t * ndrx_inicfg_new(void)
{
    ndrx_inicfg_t *ret = NULL;
    
    if (NULL==(ret = calloc(1, sizeof(ndrx_inicfg_t))))
    {
        NDRX_LOG(log_error, "Failed to alloc: ndrx_inicfg_t!");
        goto out;
    }
    
out:
    NDRX_LOG(log_debug, "ndrx_inicfg_new returns %p", ret);
    return ret;
}

/**
 * Iterate over the resource and mark as not checked.
 * @param cfg
 * @param resource
 * @return 
 */
private void cfg_mark_not_loaded(ndrx_inicfg_t *cfg, char *resource)
{
    ndrx_inicfg_file_t *f, *ftmp;
    
    HASH_ITER(hh, cfg->cfgfile, f, ftmp)
    {
        if (0==strcmp(f->resource, resource))
        {
            f->not_refreshed = TRUE;
        }
    }
    
}

/**
 * Load or update resource
 * @param cfg config handler
 * @param resource folder/file to load
 * @param fullsection_start_with list of sections which we are interested in
 * @return 
 */
public int ndrx_inicfg_update(ndrx_inicfg_t *cfg, char *resource, char **fullsection_start_with)
{
    int ret = SUCCEED;
    
    cfg_mark_not_loaded(cfg, resource);
    
    /* Check the type (folder or file) */
    
out:
    return ret;
}

/**
 * If file reloaded, then populate sub-sections
 * @param fullfile full path to file
 * @return 
 */
public int ndrx_inicfg_merge_subsect(char *fullfile)
{
    int ret = SUCCEED;
    
out:
    return ret;
}

/**
 * Get he list of key/value entries for the section
 * @param cfg
 * @param section
 * @return 
 */
public ndrx_inicfg_section_keyval_t* ndrx_inicfg_get(ndrx_inicfg_t *cfg, char *fullsection)
{
    return NULL;
}

/**
 * Get the values from subsection
 * @param cfg
 * @param section
 * @param subsect
 * @return 
 */
public ndrx_inicfg_section_keyval_t* ndrx_inicfg_get_subsect(ndrx_inicfg_t *cfg, char *section, char *subsect)
{
    return NULL;
}

/**
 * Iterate over the sections & return the matched image
 * @param cfg
 * @param fullsection_starts_with
 * @return List of sections
 */
public ndrx_inicfg_section_keyval_t* ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, char **fullsection_starts_with)
{
    return NULL;
}

/**
 * Free up the list of key/values loaded
 * @param keyval
 */
public void ndrx_inicfg_keyval_free(ndrx_inicfg_section_keyval_t *keyval)
{
    
}

/**
 * Free the memory of file
 * @param cfg
 * @param fullfile
 * @return 
 */
public void ndrx_inicfg_file_free(ndrx_inicfg_t *cfg, char *fullfile)
{
    
}

/**
 * Free the whole config
 * @param cfg config handler will become invalid after this operation
 * @return 
 */
public void ndrx_inicfg_free(ndrx_inicfg_t *cfg)
{
    
}

