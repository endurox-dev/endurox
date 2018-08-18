/**
 * @brief Enduro/X advanced configuration driver
 *   We have a problem with logging here - chicken-egg issue.
 *   We cannot start to log unless debug is initialised. But debug init depends
 *   on ini config. Thus we print here debugs to stderr, if needed.
 *
 * @file inicfg.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
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
#include <ndebug.h>
#include <nstd_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define API_ENTRY {_Nunset_error();}

/* #define INICFG_ENABLE_DEBUG  */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate ndrx_inicfg_t * _ndrx_inicfg_new(int load_global_env);
exprivate int _ndrx_inicfg_reload(ndrx_inicfg_t *cfg, char **section_start_with);
exprivate void cfg_mark_not_loaded(ndrx_inicfg_t *cfg, char *resource);
exprivate void cfg_remove_not_marked(ndrx_inicfg_t *cfg);
exprivate ndrx_inicfg_section_t * cfg_section_new(ndrx_inicfg_section_t **sections_h, char *section);
exprivate ndrx_inicfg_section_t * cfg_section_get(ndrx_inicfg_section_t **sections_h, char *section);
exprivate int handler(void* cf_ptr, void *vsection_start_with, void *cfg_ptr, 
        const char* section, const char* name, const char* value);
exprivate int _ndrx_inicfg_load_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with);
exprivate ndrx_inicfg_file_t* cfg_single_file_get(ndrx_inicfg_t *cfg, char *fullname);
exprivate int _ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with);
exprivate int _ndrx_inicfg_add(ndrx_inicfg_t *cfg, char *resource, char **section_start_with);
exprivate int _ndrx_keyval_hash_add(ndrx_inicfg_section_keyval_t **h, 
            ndrx_inicfg_section_keyval_t *src);
exprivate ndrx_inicfg_section_keyval_t * _ndrx_keyval_hash_get(
        ndrx_inicfg_section_keyval_t *h, char *key);
exprivate void _ndrx_keyval_hash_free(ndrx_inicfg_section_keyval_t *h);
exprivate int _ndrx_inicfg_resolve(ndrx_inicfg_t *cfg, char **resources, char *section, 
        ndrx_inicfg_section_keyval_t **out);
exprivate int _ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, 
        char **resources,
        char **section_start_with, 
        ndrx_inicfg_section_t **out);
exprivate void _ndrx_inicfg_sections_free(ndrx_inicfg_section_t *sections);
exprivate void _ndrx_inicfg_file_free(ndrx_inicfg_t *cfg, ndrx_inicfg_file_t *cfgfile);
exprivate void _ndrx_inicfg_free(ndrx_inicfg_t *cfg);

/**
 * Create new config handler
 * @return ptr to config handler or NULL
 */
exprivate ndrx_inicfg_t * _ndrx_inicfg_new(int load_global_env)
{
    ndrx_inicfg_t *ret = NULL;
    
    if (NULL==(ret = NDRX_CALLOC(1, sizeof(ndrx_inicfg_t))))
    {
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_t: %s", 
                strerror(errno));
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "Failed to alloc: ndrx_inicfg_t!\n");
#endif
        goto out;
    }
    
    ret->load_global_env = load_global_env;
    
    NDRX_LOG_EARLY(log_debug, "%s: load_global_env: %d", __func__, load_global_env);
    
out:
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "ndrx_inicfg_new returns %p\n", ret);
#endif
    return ret;
}

/**
 * Reload the config, use the globals to search for value...
 * Already API version.
 * @param cfg
 * @param section_start_with
 * @return 
 */
exprivate int _ndrx_inicfg_reload(ndrx_inicfg_t *cfg, char **section_start_with)
{
    int i;
    int ret = EXSUCCEED;
    char fn[] = "_ndrx_inicfg_reload";
    string_hash_t * r, *rt;
    
    /* safe iter over the list */
    EXHASH_ITER(hh, cfg->resource_hash, r, rt)
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: Reloading [%s]\n", fn, r->str);
#endif
        if (EXSUCCEED!=_ndrx_inicfg_add(cfg, r->str, section_start_with))
        {
            EXFAIL_OUT(ret);
        }
    }
    
out:

#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: returns %d\n", fn, ret);
#endif  
    return ret;
}

/**
 * Iterate over the resource and mark as not checked.
 * @param cfg
 * @param resource
 * @return 
 */
exprivate void cfg_mark_not_loaded(ndrx_inicfg_t *cfg, char *resource)
{
    ndrx_inicfg_file_t *f, *ftmp;
    
    EXHASH_ITER(hh, cfg->cfgfile, f, ftmp)
    {
        if (0==strcmp(f->resource, resource))
        {
#ifdef INICFG_ENABLE_DEBUG
            fprintf(stderr, "Unrefreshing resource [%s] file [%s]\n", 
                    f->resource, f->fullname);
#endif
        
            f->refreshed = EXFALSE;
        }
    }
}

/**
 * Remove any config file which is not reloaded
 * @param cfg
 * @param resource
 */
exprivate void cfg_remove_not_marked(ndrx_inicfg_t *cfg)
{
    ndrx_inicfg_file_t *f, *ftmp;
    
    EXHASH_ITER(hh, cfg->cfgfile, f, ftmp)
    {
        if (!f->refreshed)
        {
#ifdef INICFG_ENABLE_DEBUG
            fprintf(stderr, "Resource [%s]/file [%s] not refreshed - removing from mem\n", 
                    f->resource, f->fullname);
#endif
            ndrx_inicfg_file_free(cfg, f);
        }
    }
}

/**
 * Get new qhash entry + add it to hash.
 * @param qname
 * @return 
 */
exprivate ndrx_inicfg_section_t * cfg_section_new(ndrx_inicfg_section_t **sections_h, char *section)
{
    ndrx_inicfg_section_t * ret = NDRX_CALLOC(1, sizeof(ndrx_inicfg_section_t));
            
    if (NULL==ret)
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_section_t: %s", strerror(err));
        goto out;
    }
    
    if (NULL==(ret->section=strdup(section)))
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_section_t: "
                "(section) %s", strerror(err));
        ret=NULL;
        goto out;
    }
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "Adding new section [%s]\n", ret->section);
#endif

    EXHASH_ADD_KEYPTR(hh, (*sections_h), ret->section, 
            strlen(ret->section), ret);
    
out:
    return ret;
}

/**
 * Get QHASH record for particular q
 * @param qname
 * @return 
 */
exprivate ndrx_inicfg_section_t * cfg_section_get(ndrx_inicfg_section_t **sections_h, char *section)
{
    ndrx_inicfg_section_t * ret = NULL;
   
    EXHASH_FIND_STR( (*sections_h), section, ret);
    
    if (NULL==ret)
    {
        ret = cfg_section_new(sections_h, section);
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
exprivate int handler(void* cf_ptr, void *vsection_start_with, void *cfg_ptr, 
        const char* section, const char* name, const char* value)
{
    int ret = 1;
    int value_len;
    ndrx_inicfg_file_t *cf = (ndrx_inicfg_file_t*)cf_ptr;
    ndrx_inicfg_t *cfg = (ndrx_inicfg_t *)cfg_ptr;
    char **section_start_with = (char **)vsection_start_with;
    char *p;
    int needed = EXTRUE;
    
    ndrx_inicfg_section_t *mem_section = NULL;
    ndrx_inicfg_section_keyval_t * mem_value = NULL;
    
    /* check do we need this section at all */
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "Handler got: resource [%s]/file [%s] section [%s]"
            " name [%s] value [%s] cf %p\n", 
            cf->resource, cf->fullname, section, name, value, cf);
#endif
    
    if (NULL!=section_start_with)
    {
        needed = EXFALSE;
        if (NULL==section_start_with)
        {
            needed = EXTRUE;
        }
        else while (NULL!=*section_start_with)
        {
            int len = NDRX_MIN(strlen(*section_start_with), strlen(section));
            
            if (0 == strncmp(*section_start_with, section, len))
            {
                needed = EXTRUE;
                break;
            }
            section_start_with++;
        }
    }
    
    /* section not needed. */
    if (!needed)
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "Section not needed - skipping\n");
#endif
        goto out;
    }
    
    /* add/get section */
    mem_section = cfg_section_get(&(cf->sections), (char *)section);
    if (NULL==mem_section)
    {
        ret = 0;
        goto out;
    }
    
    mem_value = NDRX_CALLOC(1, sizeof(ndrx_inicfg_section_keyval_t));
    
    if (NULL==mem_value)
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_section_t: %s", strerror(err));
        ret = 0;
        goto out;
    }
    
    if (NULL==(mem_value->section = strdup(section)))
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to malloc mem_value->section: %s", strerror(err));
        ret = 0;
        goto out;
    }
    
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
    
    if (NULL==(mem_value->val = NDRX_REALLOC(mem_value->val, value_len)))
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
    if (NULL==(mem_value->val = NDRX_REALLOC(mem_value->val, value_len)))
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to truncate mem_value->val to %d: %s", 
                value_len, strerror(err));

        ret = 0;
        goto out;
    }
        
    /* Add stuff to the section */
    EXHASH_ADD_KEYPTR(hh, mem_section->values, mem_value->key, 
            strlen(mem_value->key), mem_value);
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "section/key/value added OK\n");
#endif

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
exprivate int _ndrx_inicfg_load_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    ndrx_inicfg_file_t *cf = NULL;
    int ret = EXSUCCEED;
    char fn[] = "_ndrx_inicfg_load_single_file";
    
    if (NULL==(cf = NDRX_CALLOC(1, sizeof(ndrx_inicfg_file_t))))
    {
        _Nset_error_fmt(NEMALLOC, "%s: Failed to malloc ndrx_inicfg_file_t: %s", 
                fn, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* copy off resource */
    NDRX_STRCPY_SAFE(cf->resource, resource);
    /* copy off fullname of file */
    NDRX_STRCPY_SAFE(cf->fullname, fullname);
    
    cf->refreshed = EXTRUE;
    
    if (EXSUCCEED!=stat(fullname, &cf->attr))
    {
        _Nset_error_fmt(NEUNIX, "%s: stat() failed for [%s]:%s", 
                fn, fullname, strerror(errno));
        EXFAIL_OUT(ret);    
    }
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "Opened config file %p\n", cf);
#endif

    /* start to parse the file by inih */
    if (EXSUCCEED!=(ret=ini_parse(fullname, handler, (void *)cf, 
            (void *)section_start_with, (void *)cfg)))
    {
        _Nset_error_fmt(NEINVALINI, "%s: Invalid ini file: [%s] error on line: %d", 
                fn, fullname, ret);
        EXFAIL_OUT(ret);
    }
    
    EXHASH_ADD_STR( cfg->cfgfile, fullname, cf );
    
out:
    return ret;
}

/**
 * Get the single file
 * @param cfg
 * @param fullname
 * @return 
 */
exprivate ndrx_inicfg_file_t* cfg_single_file_get(ndrx_inicfg_t *cfg, char *fullname)
{
    ndrx_inicfg_file_t * ret = NULL;
   
    EXHASH_FIND_STR( cfg->cfgfile, fullname, ret);
    
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
exprivate int _ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    int ret = EXSUCCEED;
    struct stat attr; 
    char fn[] = "_ndrx_inicfg_update_single_file";
    int ferr = 0;
    /* try to get the file handler (resolve) */
    ndrx_inicfg_file_t *cf = cfg_single_file_get(cfg, fullname);
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: enter resource [%s]/file [%s]\n",
                    fn, resource, fullname);
#endif

    if (EXSUCCEED!=stat(fullname, &attr))
    {
        /* check the error. */
        ferr = errno;
    }
        
#ifdef INICFG_ENABLE_DEBUG
    if (NULL!=cf)
    {
        fprintf(stderr, "%s: tstamp: cur: %ld vs prev: %ld\n",
                        fn, attr.st_mtime, cf->attr.st_mtime);
    }
#endif
    
    if (NULL!=cf && EXSUCCEED==ferr && 
            0!=memcmp(&(attr.st_mtime), &(cf->attr.st_mtime), sizeof(attr.st_mtime)))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: [%s]/file [%s] changed - reload\n",
                        fn, resource, fullname);
#endif
        /* reload the file - kill the old one, and load again */
        ndrx_inicfg_file_free(cfg, cf);
        if (EXSUCCEED!=_ndrx_inicfg_load_single_file(cfg, resource, fullname, 
                section_start_with))
        {
            EXFAIL_OUT(ret);
        }
    }
    else if (NULL!=cf && EXSUCCEED==ferr)
    {
        /* config not changed, mark as refreshed */
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: [%s]/file [%s] not-changed - do nothing\n",
                        fn, resource, fullname);
#endif
        cf->refreshed = EXTRUE;
        goto out;
    }
    else if (NULL==cf && EXSUCCEED==ferr)
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: [%s]/file [%s] config does not exists, "
                "but file exists - load\n",
                        fn, resource, fullname);
#endif
        /* config does not exists, but file exists - load */
        if (EXSUCCEED!=_ndrx_inicfg_load_single_file(cfg, resource, fullname, 
                section_start_with))
        {
            EXFAIL_OUT(ret);
        }
    }
    else if (NULL!=cf && EXSUCCEED!=ferr)
    {
        /* Config exits, file not, kill the config  */
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: [%s]/file [%s] Config exits, "
                "file not, kill the config\n",
                        fn, resource, fullname);
#endif
        ndrx_inicfg_file_free(cfg, cf);
    }
    
out:
    return ret;    
}

/**
 * Load or update resource
 * @param cfg config handler
 * @param resource folder/file to load
 * @param section_start_with list of sections which we are interested in
 * @return 
 */
exprivate int _ndrx_inicfg_add(ndrx_inicfg_t *cfg, char *resource, char **section_start_with)
{
    int ret = EXSUCCEED;
    char fn[] = "_ndrx_inicfg_add";
    cfg_mark_not_loaded(cfg, resource);
 
    /* Check the type (folder or file) 
     * If it is not file, then it is a directory (assumption).
     */
    if (ndrx_file_regular(resource))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "Resource: [%s] is regular file\n", resource);
#endif
        if (EXSUCCEED!=_ndrx_inicfg_update_single_file(cfg, resource, 
                resource, section_start_with))
        {
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        string_list_t* flist = NULL;
        string_list_t* elt = NULL;

        int return_status = EXSUCCEED;
        
#ifdef INICFG_ENABLE_DEBUG  
        fprintf(stderr, "Resource: [%s] seems like directory "
                "(checking for *.ini, *.cfg, *.conf, *.config)\n", resource);
#endif
        
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
                   char tmp[PATH_MAX+1];
                   snprintf(tmp, sizeof(tmp), "%s/%s", resource, elt->qname);
                   
                   if (EXSUCCEED!=_ndrx_inicfg_update_single_file(cfg, resource, 
                           tmp, section_start_with))
                   {
                       EXFAIL_OUT(ret);
                   }
               }         
           }
        }
        
        ndrx_string_list_free(flist);
    }
    
    cfg_remove_not_marked(cfg);
    
    /*
     * Add resource to the hash
     */
    if (NULL==ndrx_string_hash_get(cfg->resource_hash, resource))
    {
#ifdef INICFG_ENABLE_DEBUG  
        fprintf(stderr, "Registering resource [%s]\n", resource);
#endif
        if (EXSUCCEED!=ndrx_string_hash_add(&(cfg->resource_hash), resource))
        {
            _Nset_error_fmt(NEMALLOC, "%s: ndrx_string_hash_add - malloc failed", fn);
            EXFAIL_OUT(ret);
        }
    }
    
out:
    return ret;
}

/**
 * Add item to keyval hash
 * @param h
 * @param str
 * @return SUCCEED/FAIL
 */
exprivate int _ndrx_keyval_hash_add(ndrx_inicfg_section_keyval_t **h, 
            ndrx_inicfg_section_keyval_t *src)
{
    int ret = EXSUCCEED;
    char fn[]="_ndrx_keyval_hash_add";
    ndrx_inicfg_section_keyval_t * tmp = NDRX_CALLOC(1, sizeof(ndrx_inicfg_section_keyval_t));
    
    if (NULL==tmp)
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "alloc of ndrx_inicfg_section_keyval_t (%d) failed\n", 
                (int)sizeof(ndrx_inicfg_section_keyval_t));
#endif
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(tmp->key = strdup(src->key)))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "strdup() failed: %s\n", strerror(errno));
        _Nset_error_fmt(NEMALLOC, "%s: malloc failed", fn);
#endif
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(tmp->val = strdup(src->val)))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "strdup() failed: %s\n", strerror(errno));
        _Nset_error_fmt(NEMALLOC, "%s: malloc failed", fn);
        
#endif
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(tmp->section = strdup(src->section)))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "strdup() failed: %s\n", strerror(errno));
        _Nset_error_fmt(NEMALLOC, "%s: malloc failed", fn);
#endif
        EXFAIL_OUT(ret);
    }
    
    /* Add stuff to hash finaly */
    EXHASH_ADD_KEYPTR( hh, (*h), tmp->key, strlen(tmp->key), tmp );
    
out:
    return ret;
}

/**
 * Search for string existence in hash (not need for API version)
 * @param h hash handler
 * @param str keyval to search for
 * @return NULL not found/not NULL - found
 */
exprivate ndrx_inicfg_section_keyval_t * _ndrx_keyval_hash_get(
        ndrx_inicfg_section_keyval_t *h, char *key)
{
    ndrx_inicfg_section_keyval_t * r = NULL;
    
    EXHASH_FIND_STR( h, key, r);
    
    return r;
}

/**
 * Free up the hash list (no need for API)
 * @param h
 * @return 
 */
exprivate void _ndrx_keyval_hash_free(ndrx_inicfg_section_keyval_t *h)
{
    ndrx_inicfg_section_keyval_t * r=NULL, *rt=NULL;
    /* safe iter over the list */
    EXHASH_ITER(hh, h, r, rt)
    {
        EXHASH_DEL(h, r);
        NDRX_FREE(r->key);
        NDRX_FREE(r->val);
        NDRX_FREE(r->section);
        NDRX_FREE(r);
    }
}

/**
 * Resolve the section
 * @param cfg
 * @param section
 * @param out
 * @return 
 */
exprivate int _ndrx_inicfg_resolve(ndrx_inicfg_t *cfg, char **resources, char *section, 
        ndrx_inicfg_section_keyval_t **out)
{
    int i;
    int found;
    int ret = EXSUCCEED;
    char fn[] = "_ndrx_inicfg_resolve";
    /* Loop over all resources, and check that these are present in  
     * resources var (or resources is NULL) 
     * in that case resolve from all resources found in system.
     */
    
    /* HASH FOR EACH: cfg->cfgfile */
    
    /* check by ndrx_keyval_hash_get() for result 
     * if not found, add...
     * if found ignore.
     */
    ndrx_inicfg_file_t * config_file=NULL, *config_file_temp=NULL;
    ndrx_inicfg_section_t *section_hash;
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: lookup section [%s]\n", fn, section);
#endif
    
    /* Iter over all resources */
    EXHASH_ITER(hh, cfg->cfgfile, config_file, config_file_temp)
    {
        found = EXFALSE;
        i = 0;
        
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: Checking [%s]...\n", fn, config_file->fullname);
#endif
    
        /* If resources is NULL, then look in all config files stored.  */
        if (NULL==resources)
        {
            found = EXTRUE;
#ifdef INICFG_ENABLE_DEBUG
            fprintf(stderr, "%s: Checking [%s] - accept any\n", fn, config_file->fullname);
#endif
        }
        else 
        {
            while(NULL!=resources[i])
            {
                if (0==strcmp(config_file->resource, resources[i]))
                {
                    found = EXTRUE;
                    break;
                }
                i++;
            }
        }
    
        if (found)
        {
#ifdef INICFG_ENABLE_DEBUG
            fprintf(stderr, "%s: Checking [%s] - accepted\n", fn, config_file->fullname);
#endif
            /* find section */
#ifdef INICFG_ENABLE_DEBUG
            fprintf(stderr, "%s: searching for section [%s] in %p\n", 
                fn, section, config_file->sections);
#endif
            EXHASH_FIND_STR(config_file->sections, section, section_hash);
            if (NULL!=section_hash)
            {
                
#ifdef INICFG_ENABLE_DEBUG
                fprintf(stderr, "%s: got section...\n", fn);
#endif
                ndrx_inicfg_section_keyval_t *vals = NULL, *vals_tmp = NULL;
                /* ok we got a section, now get the all values down in section */
                EXHASH_ITER(hh, (section_hash->values), vals, vals_tmp)
                {
                    ndrx_inicfg_section_keyval_t *existing = NULL;
#ifdef INICFG_ENABLE_DEBUG
                    fprintf(stderr, "%s: got section[%s]/key[%s]/val[%s]\n", fn, 
                            vals->section, vals->key, vals->val);
#endif
                    existing = _ndrx_keyval_hash_get((*out), vals->key); 
                    /* Allow deeper sections to override higher sections. */
                    if (NULL==existing || 
                            ndrx_nr_chars(vals->section, NDRX_INICFG_SUBSECT_SPERATOR) > 
                            ndrx_nr_chars(existing->section, NDRX_INICFG_SUBSECT_SPERATOR))
                    {
                        if (EXSUCCEED!=_ndrx_keyval_hash_add(out, vals))
                        {
                            EXFAIL_OUT(ret);
                        }
                    }
                } /* it over the key-vals in section */
            } /* if section found */
        } /* if file found in lookup resources  */
        else
        {
#ifdef INICFG_ENABLE_DEBUG
            fprintf(stderr, "%s: Checking [%s] - NOT accepted\n", fn, config_file->fullname);
#endif
        }
    } /* iter over config files */
    
    
out:
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: returns %p\n", fn, *out);
#endif

    return ret;
}

/**
 * Resolve values including sub-sections
 * [SOME/SECTION/AND/SUBSECT]
 * We need to resolve in this order:
 * 1. SOME/SECTION/AND/SUBSECT
 * 2. SOME/SECTION/AND
 * 3. SOME/SECTION
 * 4. SOME
 * @param cfg
 * @param section
 * @param subsect
 * @return 
 */
expublic int ndrx_inicfg_get_subsect_int(ndrx_inicfg_t *cfg, 
        char **resources, char *section, ndrx_inicfg_section_keyval_t **out)
{
    int ret = EXSUCCEED;
    char *tmp = NULL;
    char *p;
    
    if (NULL==cfg)
    {
        _Nset_error_fmt(NEINVAL, "%s: `cfg' cannot be NULL!", __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==section)
    {
        _Nset_error_fmt(NEINVAL, "%s: `section' cannot be NULL!", __func__);
        EXFAIL_OUT(ret);
    }

    tmp = strdup(section);

    if (NULL==tmp)
    {
        _Nset_error_fmt(NEMALLOC, "%s: malloc failed", __func__);
        EXFAIL_OUT(ret);
    }
    
    while (EXEOS!=tmp[0])
    {
        if (EXSUCCEED!=_ndrx_inicfg_resolve(cfg, resources, tmp, out))
        {
            EXFAIL_OUT(ret);
        }
        p = strrchr(tmp, NDRX_INICFG_SUBSECT_SPERATOR);
        
        if (NULL!=p)
        {
            *p = EXEOS;
        }
        else
        {
            break; /* terminate, we are done */
        }
    }
    
out:
    if (NULL!=tmp)
    {
        NDRX_FREE(tmp);
    }

    return ret;
}

/**
 * Iterate over the sections & return the matched image
 * We might want to return multiple hashes here of the sections found.
 * TODO: Think about iteration... Get the list based on sections
 * @param cfg
 * @param fullsection_starts_with
 * @return List of sections
 */
exprivate int _ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, 
        char **resources,
        char **section_start_with, 
        ndrx_inicfg_section_t **out)
{
    int i;
    int found;
    int ret = EXSUCCEED;
    char fn[] = "_ndrx_inicfg_iterate";
    /* Loop over all resources, and check that these are present in  
     * resources var (or resources is NULL) 
     * in that case resolve from all resources found in system.
     */
    
    /* HASH FOR EACH: cfg->cfgfile */
    
    /* check by ndrx_keyval_hash_get() for result 
     * if not found, add...
     * if found ignore.
     */
    ndrx_inicfg_file_t * config_file=NULL, *config_file_temp=NULL;
    ndrx_inicfg_section_t *section = NULL, *section_temp=NULL;
    ndrx_inicfg_section_t *section_work = NULL;

#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: enter\n", fn);
#endif

    /* Iter over all resources */
    EXHASH_ITER(hh, cfg->cfgfile, config_file, config_file_temp)
    {
        found = EXFALSE;
        i = 0;
        
        /* If resources is NULL, then look in all config files stored.  */
        if (NULL==resources)
        {
            found = EXTRUE;
        }
        else 
        {
            while(NULL!=resources[i])
            {
                if (0==strcmp(config_file->resource, resources[i]))
                {
                    found = EXTRUE;
                    break;
                }
                i++;
            }
        }
        
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: resource [%s] %s for lookup\n", fn, config_file->resource, 
            found?"ok":"not ok");
#endif

        if (found)
        {
            /* find section 
             * - loop over the file sections & fill the results
             * - will do the lookup with population of parent info into
             * childs
            EXHASH_FIND_STR(config_file->sections, section, section_hash);
             * */
            EXHASH_ITER(hh, (config_file->sections), section, section_temp)
            {
                int len;
                
                found = EXFALSE;
                i = 0;
                
                if (NULL==section_start_with)
                {
                    found = EXTRUE;
                }
                else while (NULL!=section_start_with[i])
                {
                    len = NDRX_MIN(strlen(section->section), strlen(section_start_with[i]));
                    if (0==strncmp(section->section, section_start_with[i], len))
                    {
                        found = EXTRUE;
                        break;
                    }
                    i++;
                }

#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: section [%s] %s for lookup\n", fn, section->section, 
            found?"ok":"not ok");
#endif
                if (found)
                {
                    /* build up the result section hash (check is there on or missing)... */
                    if (NULL==(section_work=cfg_section_get(out, section->section)))
                    {
                        EXFAIL_OUT(ret);
                    }

                    ndrx_inicfg_section_keyval_t *vals = NULL, *vals_tmp = NULL;
                    /* ok we got a section, now get the all values down in section */
                    EXHASH_ITER(hh, (section->values), vals, vals_tmp)
                    {
                        if (NULL==_ndrx_keyval_hash_get((section_work->values), vals->key))
                        {
                            if (EXSUCCEED!=_ndrx_keyval_hash_add(&(section_work->values), vals))
                            {
                                EXFAIL_OUT(ret);
                            }
                        }
                    } /* it over the key-vals in section */
                }
            }
        } /* if file found in lookup resources  */
    } /* iter over config files */
    
    
out:
                
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: returns %p", fn, *out);
#endif

    return ret;
}

/**
 * Free all sections from hash
 * @param sections ptr becomes invalid after function call
 */
exprivate void _ndrx_inicfg_sections_free(ndrx_inicfg_section_t *sections)
{
    char fn[] = "_ndrx_inicfg_sections_free";    
    ndrx_inicfg_section_t *section=NULL, *section_temp=NULL;
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: enter %p\n", fn, sections);
#endif
        
    /* kill the sections */
    EXHASH_ITER(hh, sections, section, section_temp)
    {
        EXHASH_DEL(sections, section);
        ndrx_keyval_hash_free(section->values);
        NDRX_FREE(section->section);
        NDRX_FREE(section);
    }
}

/**
 * Free the memory of file
 * @param cfg
 * @param fullfile
 * @return 
 */
exprivate void _ndrx_inicfg_file_free(ndrx_inicfg_t *cfg, ndrx_inicfg_file_t *cfgfile)
{
    char fn[] = "_ndrx_inicfg_file_free";
    ndrx_inicfg_section_t *section=NULL, *section_temp=NULL;
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: enter cfg = %p cfgfile = %p\n", fn, cfg, cfgfile);
#endif
    
    EXHASH_DEL(cfg->cfgfile, cfgfile);
    
    ndrx_inicfg_sections_free(cfgfile->sections);
    
    NDRX_FREE(cfgfile);
}

/**
 * Free the whole config
 * @param cfg config handler will become invalid after this operation
 * @return 
 */
exprivate void _ndrx_inicfg_free(ndrx_inicfg_t *cfg)
{
    char fn[]="_ndrx_inicfg_free";
    ndrx_inicfg_file_t *cf=NULL, *cf_tmp=NULL;
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: enter cfg = %p\n", fn, cfg);
#endif
    
    EXHASH_ITER(hh, cfg->cfgfile, cf, cf_tmp)
    {
        _ndrx_inicfg_file_free(cfg, cf);
    }
    
    ndrx_string_hash_free(cfg->resource_hash);
    
    NDRX_FREE(cfg);
}

/* ===========================================================================*/
/* =========================API FUNCTIONS=====================================*/
/* ===========================================================================*/
/**
 * Reload the config, use the globals to search for value...
 * Already API version.
 * @param cfg
 * @param section_start_with
 * @return 
 */
expublic int ndrx_inicfg_reload(ndrx_inicfg_t *cfg, char **section_start_with)
{
    API_ENTRY;
    return _ndrx_inicfg_reload(cfg, section_start_with);
}

/**
 * Create new config handler
 * @return ptr to config handler or NULL
 */
expublic ndrx_inicfg_t * ndrx_inicfg_new(void)
{
    API_ENTRY;
    return _ndrx_inicfg_new(EXFALSE);
}

/**
 * Parametrised version of config new
 * @return ptr to config handler or NULL
 */
expublic ndrx_inicfg_t * ndrx_inicfg_new2(int load_global_env)
{
    API_ENTRY;
    return _ndrx_inicfg_new(load_global_env);
}

/**
 * API version of _ndrx_inicfg_load_single_file
 */
expublic int ndrx_inicfg_load_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    API_ENTRY;
    
    return _ndrx_inicfg_load_single_file(cfg, resource, fullname, section_start_with);
    
}

/**
 * API version of _ndrx_inicfg_update_single_file
 */
expublic int ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    API_ENTRY;
    return _ndrx_inicfg_update_single_file(cfg, resource, fullname, section_start_with);
}


/**
 * Load or update resource (api version of _ndrx_inicfg_add)
 * @param cfg config handler
 * @param resource folder/file to load
 * @param section_start_with list of sections which we are interested in
 * @return 
 */
expublic int ndrx_inicfg_add(ndrx_inicfg_t *cfg, char *resource, char **section_start_with)
{
    API_ENTRY;
    return _ndrx_inicfg_add(cfg, resource, section_start_with);
}

/**
 * Add item to keyval hash (api version of _ndrx_keyval_hash_add)
 * @param h
 * @param str
 * @return SUCCEED/FAIL
 */
expublic int ndrx_keyval_hash_add(ndrx_inicfg_section_keyval_t **h, 
            ndrx_inicfg_section_keyval_t *src)
{
    API_ENTRY;
    return ndrx_keyval_hash_add(h, src);
}

/**
 * API version of _ndrx_keyval_hash_get
 * @param h
 * @param key
 * @return 
 */
expublic ndrx_inicfg_section_keyval_t * ndrx_keyval_hash_get(
        ndrx_inicfg_section_keyval_t *h, char *key)
{
    API_ENTRY;
    return _ndrx_keyval_hash_get(h, key);
}

/**
 * Free up the hash list (no need for API)
 * @param h
 * @return 
 */
expublic void ndrx_keyval_hash_free(ndrx_inicfg_section_keyval_t *h)
{
    API_ENTRY;
    _ndrx_keyval_hash_free(h);
}

/**
 * Resolve the section (api version of _ndrx_inicfg_resolve)
 * @param cfg
 * @param section
 * @param out
 * @return 
 */
expublic int ndrx_inicfg_resolve(ndrx_inicfg_t *cfg, char **resources, char *section, 
        ndrx_inicfg_section_keyval_t **out)
{
    API_ENTRY;
    return _ndrx_inicfg_resolve(cfg, resources, section, out);
}

/**
 * Resolve values including sub-sections, API version of ndrx_inicfg_get_subsect
 * [SOME/SECTION/AND/SUBSECT]
 * We need to resolve in this order:
 * 1. SOME/SECTION/AND/SUBSECT
 * 2. SOME/SECTION/AND
 * 3. SOME/SECTION
 * 4. SOME
 * @param cfg
 * @param section
 * @param subsect
 * @return 
 */
expublic int ndrx_inicfg_get_subsect(ndrx_inicfg_t *cfg, 
        char **resources, char *section, ndrx_inicfg_section_keyval_t **out)
{
    API_ENTRY;
    
    return ndrx_inicfg_get_subsect_int(cfg, resources, section, out);
}

/**
 * Iterate over the sections & return the matched image
 * We might want to return multiple hashes here of the sections found.
 * API version of _ndrx_inicfg_iterate
 * @param cfg
 * @param fullsection_starts_with
 * @return List of sections
 */
expublic int ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, 
        char **resources,
        char **section_start_with, 
        ndrx_inicfg_section_t **out)
{
    return _ndrx_inicfg_iterate(cfg, resources, section_start_with, out);
}

/**
 * Free all sections from hash
 * API version of _ndrx_inicfg_sections_free
 * @param sections ptr becomes invalid after function call
 */
expublic void ndrx_inicfg_sections_free(ndrx_inicfg_section_t *sections)
{
    API_ENTRY;
    _ndrx_inicfg_sections_free(sections);
}

/**
 * Free the memory of file (API version of _ndrx_inicfg_file_free)
 * API version of 
 * @param cfg
 * @param fullfile
 * @return 
 */
expublic void ndrx_inicfg_file_free(ndrx_inicfg_t *cfg, ndrx_inicfg_file_t *cfgfile)
{
    API_ENTRY;
    _ndrx_inicfg_file_free(cfg, cfgfile);
}


/**
 * Free the whole config
 * API version of _ndrx_inicfg_free
 * @param cfg config handler will become invalid after this operation
 * @return 
 */
expublic void ndrx_inicfg_free(ndrx_inicfg_t *cfg)
{
    API_ENTRY;
    _ndrx_inicfg_free(cfg);
}

/* vim: set ts=4 sw=4 et smartindent: */
