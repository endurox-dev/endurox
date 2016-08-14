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
#include <utlist.h>
#include <nstdutil.h>
#include <ini.h>
#include <inicfg.h>
#include <nerror.h>
#include <sys_unix.h>
#include <errno.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define API_ENTRY {_Nunset_error();}


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
    
    API_ENTRY;
    
    if (NULL==(ret = calloc(1, sizeof(ndrx_inicfg_t))))
    {
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_t: %s", 
                strerror(errno));
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
 * Add stuff to file content hash
 * @param cf_ptr
 * @param section
 * @param name
 * @param value
 * @return 
 */
private int handler(void* cf_ptr, const char* section, const char* name,
                   const char* value)
{
    ndrx_inicfg_file_t *cf = (ndrx_inicfg_file_t*)cf_ptr;
    
    
    /* on error 0 */
    
    /* on success 1 */
    return 0;
}


/**
 * Load single file into config
 * @param cfg
 * @param resource
 * @param fullsection_start_with
 * @return 
 */
public int ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **fullsection_start_with)
{
    ndrx_inicfg_file_t *cf = NULL;
    int ret = SUCCEED;
    
    if (NULL==(cf = malloc(sizeof(ndrx_inicfg_file_t))))
    {
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_file_t: %s", 
                strerror(errno));
        NDRX_LOG(log_error, "Failed to alloc: ndrx_inicfg_file_t!");
    }
    
    /* copy off resource */
    strncpy(cf->resource, resource, sizeof(cf->resource)-1);
    cf->resource[sizeof(cf->resource)-1] = EOS;
    
    /* copy off fullname of file */
    strncpy(cf->fullname, fullname, sizeof(cf->fullname)-1);
    cf->fullname[sizeof(cf->fullname)-1] = EOS;
    
    cf->not_refreshed = FALSE;
    
    /* start to parse the file by inih */
    
    if (SUCCEED!=(ret=ini_parse(fullname, handler, (void *)cf)))
    {
        _Nset_error_fmt(NEINVALINI, "Invalid ini file: [%s] error on line: %d", 
                fullname, ret);
        NDRX_LOG(log_error, "Invalid ini file: [%s] error on line: %d", 
                fullname, ret);
    }
    
out:
    return ret;
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
    
    /* Check the type (folder or file) 
     * If it is not file, then it is a directory (assumption).
     */
    if (ndrx_file_regular(resource))
    {
        NDRX_LOG(log_debug, "Resource: [%s] is regular file", resource);
        if (SUCCEED!=ndrx_inicfg_update_single_file(cfg, resource, 
                resource, fullsection_start_with))
        {
            FAIL_OUT(ret);
        }
    }
    else
    {
        string_list_t* flist = NULL;
        string_list_t* elt = NULL;

        int return_status = SUCCEED;
        
        NDRX_LOG(log_debug, "Resource: [%s] seems like directory "
                "(checking for *.ini, *.cfg, *.conf, *.config)", resource);
        
        if (NULL!=(flist=ndrx_sys_folder_list(resource, &return_status)))
        {
           LL_FOREACH(flist,elt)
           {
               int len = strlen(elt->qname);
               if (    (len >=4 && 0==strcmp(elt->qname+len-4, ".ini")) ||
                       (len >=4 && 0==strcmp(elt->qname+len-4, ".cfg")) ||
                       (len >=5 && 0==strcmp(elt->qname+len-5, ".conf")) ||
                       (len >=7 && 0==strcmp(elt->qname+len-7, ".config"))
                   )
               {
                   if (SUCCEED!=ndrx_inicfg_update_single_file(cfg, resource, 
                           elt->qname, fullsection_start_with))
                   {
                       FAIL_OUT(ret);
                   }
               }         
           }
        }
        
        ndrx_string_list_free(flist);
    }
    
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

