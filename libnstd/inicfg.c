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
            f->refreshed = TRUE;
        }
    }
}

/**
 * Remove any config file which is not reloaded
 * @param cfg
 * @param resource
 */
private void cfg_remove_not_marked(ndrx_inicfg_t *cfg)
{
    ndrx_inicfg_file_t *f, *ftmp;
    
    HASH_ITER(hh, cfg->cfgfile, f, ftmp)
    {
        if (!f->refreshed)
        {
            ndrx_inicfg_file_free(cfg, f->fullname);
        }
    }
}

/**
 * Get new qhash entry + add it to hash.
 * @param qname
 * @return 
 */
private ndrx_inicfg_section_t * cfg_section_new(ndrx_inicfg_file_t *cf, char *section)
{
    ndrx_inicfg_section_t * ret = calloc(1, sizeof(ndrx_inicfg_section_t));
    
    if (NULL==ret)
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_section_t: %s", strerror(err));
        goto out;
    }
    
    strcpy(ret->section, section);
    
    HASH_ADD_STR( cf->sections, section, ret );
out:
    return ret;
}

/**
 * Get QHASH record for particular q
 * @param qname
 * @return 
 */
private ndrx_inicfg_section_t * cfg_section_get(ndrx_inicfg_file_t *cf, char *section)
{
    ndrx_inicfg_section_t * ret = NULL;
   
    HASH_FIND_STR( cf->sections, section, ret);
    
    if (NULL==ret)
    {
        ret = cfg_section_new(cf, section);
    }
    
    return ret;
}

/**
 * Add stuff to file content hash
 * @param cf_ptr
 * @param section
 * @param name
 * @param value
 * @return 
 */
private int handler(void* cf_ptr, void *vsection_start_with, const char* section, const char* name,
                   const char* value)
{
    int ret = 1;
    int value_len;
    ndrx_inicfg_file_t *cf = (ndrx_inicfg_file_t*)cf_ptr;
    char **section_start_with = (char **)vsection_start_with;
    char *p;
    int needed = TRUE;
    
    ndrx_inicfg_section_t *mem_section = NULL;
    ndrx_inicfg_section_keyval_t * mem_value = NULL;
    
    /* check do we need this section at all */
    
    if (NULL!=section_start_with)
    {
        needed = FALSE;
        
        while (NULL!=*section_start_with)
        {
            int len = NDRX_MIN(strlen(*section_start_with), strlen(section));
            
            if (0 == strcmp(*section_start_with, section))
            {
                needed = TRUE;
                break;
            }
            section_start_with++;
        }
    }
    
    /* section not needed. */
    if (!needed)
    {
        goto out;
    }
    
    /* add/get section */
    mem_section = cfg_section_get(cf, (char *)section);
    if (NULL==mem_section)
    {
        ret = 0;
        goto out;
    }
    
    mem_value = calloc(1, sizeof(ndrx_inicfg_section_keyval_t));
    
    if (NULL==mem_value)
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_section_t: %s", strerror(err));
        goto out;
    }
    
    strncpy(mem_value->section, section, sizeof(mem_value->section)-1);
    mem_value->section[sizeof(mem_value->section)-1] = EOS;
    
    /* Process the key */
    if (NULL==(mem_value->key = strdup(name)))
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc mem_value->key: %s", strerror(err));
        ret = 0;
        goto out;
    }
    
    if (NULL==(mem_value->val = strdup(value)))
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc mem_value->val: %s", strerror(err));
        ret = 0;
        goto out;
    }    
    
    value_len = strlen(mem_value->val) + PATH_MAX + 1;
    
    if (NULL==(mem_value->val = realloc(mem_value->val, value_len)))
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc mem_value->val (new size: %d): %s", 
                value_len, strerror(err));
        ret = 0;
        goto out;
    }
    /* replace the env... */
    ndrx_str_env_subs_len(mem_value->val, value_len);
    value_len = strlen(mem_value->val) + 1;
    
    /* realloc to exact size */
    if (NULL==(mem_value->val = realloc(mem_value->val, value_len)))
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to truncate mem_value->val to %d: %s", 
                value_len, strerror(err));

        ret = 0;
        goto out;
    }
    
    /* Add stuff to the section */
    HASH_ADD_STR(mem_section->values, key, mem_value);
    
out:
    return ret;
}


/**
 * Load single file into config
 * @param cfg
 * @param resource
 * @param section_start_with
 * @return 
 */
public int ndrx_inicfg_load_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    ndrx_inicfg_file_t *cf = NULL;
    int ret = SUCCEED;
    char fn[] = "ndrx_inicfg_load_single_file";
    
    if (NULL==(cf = malloc(sizeof(ndrx_inicfg_file_t))))
    {
        _Nset_error_fmt(NEMALLOC, "%s: Failed to malloc ndrx_inicfg_file_t: %s", 
                fn, strerror(errno));
        FAIL_OUT(ret);
    }
    
    /* copy off resource */
    strncpy(cf->resource, resource, sizeof(cf->resource)-1);
    cf->resource[sizeof(cf->resource)-1] = EOS;
    
    /* copy off fullname of file */
    strncpy(cf->fullname, fullname, sizeof(cf->fullname)-1);
    cf->fullname[sizeof(cf->fullname)-1] = EOS;
    
    cf->refreshed = FALSE;
    
    if (SUCCEED!=stat(fullname, &cf->attr))
    {
        _Nset_error_fmt(NEUNIX, "%s: stat() failed for [%s]:%s", 
                fn, fullname, strerror(errno));
        FAIL_OUT(ret);    
    }

    /* start to parse the file by inih */
    if (SUCCEED!=(ret=ini_parse(fullname, handler, (void *)cf, (void *)section_start_with)))
    {
        _Nset_error_fmt(NEINVALINI, "%s: Invalid ini file: [%s] error on line: %d", 
                fn, fullname, ret);
        FAIL_OUT(ret);
    }
    
    HASH_ADD_STR( cfg->cfgfile, fullname, cf );
    
out:
    return ret;
}

/**
 * Get the single file
 * @param cfg
 * @param fullname
 * @return 
 */
private ndrx_inicfg_file_t* cfg_single_file_get(ndrx_inicfg_t *cfg, char *fullname)
{
    ndrx_inicfg_file_t * ret = NULL;
   
    HASH_FIND_STR( cfg->cfgfile, fullname, ret);
    
    return ret;
}


/**
 * Check the file for changes, if found in hash & 
 * @param cfg
 * @param resource
 * @param fullname
 * @param section_start_with
 * @return 
 */
public int ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    int ret = SUCCEED;
    struct stat attr; 
    char fn[] = "ndrx_inicfg_update_single_file";
    int ferr = 0;
    
    /* try to get the file handler (resolve) */
    ndrx_inicfg_file_t *cf = cfg_single_file_get(cfg, fullname);
    
    if (SUCCEED!=stat(fullname, &attr))
    {
        /* check the error. */
        ferr = errno;
    }
    
    if (NULL!=cf && SUCCEED==ferr && 
            0!=memcmp(&attr.st_mtime, &cf->attr.st_mtime, sizeof(attr.st_mtime)))
    {
        /* reload the file - kill the old one, and load again */
        ndrx_inicfg_file_free(cfg, fullname);
        if (SUCCEED!=ndrx_inicfg_load_single_file(cfg, resource, fullname, 
                section_start_with))
        {
            FAIL_OUT(ret);
        }
    }
    else if (NULL!=cf && SUCCEED==ferr)
    {
        /* config not changed, mark as refreshed */
        cf->refreshed = TRUE;
        goto out;
    }
    else if (NULL==cf && SUCCEED==ferr)
    {
        /* config does not exists, but file exists - load */
        if (SUCCEED!=ndrx_inicfg_load_single_file(cfg, resource, fullname, 
                section_start_with))
        {
            FAIL_OUT(ret);
        }
    }
    else if (NULL!=cf && SUCCEED!=ferr)
    {
        /* config exits, file not, kill the config  */
        ndrx_inicfg_file_free(cfg, fullname);
    }
    
out:
    return ret;    
}

/**
 * 
 * @param cfg
 * @param files - malloced array (NULL terminated) with config files (also malloced strings)
 * @return 
 */
public int ndrx_inicfg_set_files(ndrx_inicfg_t *cfg, char **files)
{
    return SUCCEED;
}


/**
 * Set ini files by environments
 * @param cfg
 * @return 
 */
public int ndrx_inicfg_set_files_by_env(ndrx_inicfg_t *cfg)
{
    int ret = SUCCEED;
    
    return ret;
}

/**
 * Reload the config, use the globals to search for value...
 * @param cfg
 * @param section_start_with
 * @return 
 */
public int ndrx_inicfg_reload(ndrx_inicfg_t *cfg, char **section_start_with)
{
    
}

/**
 * Load or update resource
 * @param cfg config handler
 * @param resource folder/file to load
 * @param section_start_with list of sections which we are interested in
 * @return 
 */
public int ndrx_inicfg_update(ndrx_inicfg_t *cfg, char *resource, char **section_start_with)
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
                resource, section_start_with))
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
                           elt->qname, section_start_with))
                   {
                       FAIL_OUT(ret);
                   }
               }         
           }
        }
        
        ndrx_string_list_free(flist);
    }
    
    cfg_remove_not_marked(cfg);
    
out:
    return ret;
}

/**
 * Get he list of key/value entries for the section (fills up the hash with keys/val)
 * @param cfg
 * @param section
 * @param out
 * @return 
 */
public void ndrx_inicfg_get(ndrx_inicfg_t *cfg, char **resources, char *section, ndrx_inicfg_section_keyval_t **out)
{
    /* Loop over all resources, and check that these are present in  
     * resources var (or resources is NULL) 
     * in that case resolve from all resources found in system.
     */
    
    
    /* HASH FOR EACH: cfg->cfgfile */
    
    
    return;
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
public ndrx_inicfg_section_keyval_t* ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, char **section_start_with)
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

