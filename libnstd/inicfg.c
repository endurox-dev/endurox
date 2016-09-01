/* 
** Enduro/X advanced configuration driver
** We have a problem with logging here - chicken-egg issue.
** We cannot start to log unless debug is initialised. But debug init depends
** on ini config. Thus we print here debugs to stderr, if needed.
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

/* #define INICFG_ENABLE_DEBUG */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private ndrx_inicfg_t * _ndrx_inicfg_new(void);
private int _ndrx_inicfg_reload(ndrx_inicfg_t *cfg, char **section_start_with);
private void cfg_mark_not_loaded(ndrx_inicfg_t *cfg, char *resource);
private void cfg_remove_not_marked(ndrx_inicfg_t *cfg);
private ndrx_inicfg_section_t * cfg_section_new(ndrx_inicfg_section_t **sections_h, char *section);
private ndrx_inicfg_section_t * cfg_section_get(ndrx_inicfg_section_t **sections_h, char *section);
private int handler(void* cf_ptr, void *vsection_start_with, const char* section, const char* name,
                   const char* value);
private int _ndrx_inicfg_load_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with);
private ndrx_inicfg_file_t* cfg_single_file_get(ndrx_inicfg_t *cfg, char *fullname);
private int _ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with);
private int _ndrx_inicfg_add(ndrx_inicfg_t *cfg, char *resource, char **section_start_with);
private int _ndrx_keyval_hash_add(ndrx_inicfg_section_keyval_t **h, 
            ndrx_inicfg_section_keyval_t *src);
private ndrx_inicfg_section_keyval_t * _ndrx_keyval_hash_get(
        ndrx_inicfg_section_keyval_t *h, char *key);
private void _ndrx_keyval_hash_free(ndrx_inicfg_section_keyval_t *h);
private int _ndrx_inicfg_resolve(ndrx_inicfg_t *cfg, char **resources, char *section, 
        ndrx_inicfg_section_keyval_t **out);
private int _ndrx_inicfg_get_subsect(ndrx_inicfg_t *cfg, 
        char **resources, char *section, ndrx_inicfg_section_keyval_t **out);
private int _ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, 
        char **resources,
        char **section_start_with, 
        ndrx_inicfg_section_t **out);
private void _ndrx_inicfg_sections_free(ndrx_inicfg_section_t *sections);
private void _ndrx_inicfg_file_free(ndrx_inicfg_t *cfg, ndrx_inicfg_file_t *cfgfile);
private void _ndrx_inicfg_free(ndrx_inicfg_t *cfg);

/**
 * Create new config handler
 * @return ptr to config handler or NULL
 */
private ndrx_inicfg_t * _ndrx_inicfg_new(void)
{
    ndrx_inicfg_t *ret = NULL;
    
    if (NULL==(ret = calloc(1, sizeof(ndrx_inicfg_t))))
    {
        _Nset_error_fmt(NEMALLOC, "Failed to malloc ndrx_inicfg_t: %s", 
                strerror(errno));
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "Failed to alloc: ndrx_inicfg_t!\n");
#endif
        goto out;
    }
    
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
private int _ndrx_inicfg_reload(ndrx_inicfg_t *cfg, char **section_start_with)
{
    int i;
    int ret = SUCCEED;
    char fn[] = "_ndrx_inicfg_reload";
    string_hash_t * r, *rt;
    
    /* safe iter over the list */
    EXHASH_ITER(hh, cfg->resource_hash, r, rt)
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: Reloading [%s]\n", fn, r->str);
#endif
        if (SUCCEED!=_ndrx_inicfg_add(cfg, r->str, section_start_with))
        {
            FAIL_OUT(ret);
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
private void cfg_mark_not_loaded(ndrx_inicfg_t *cfg, char *resource)
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
        
            f->refreshed = FALSE;
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
private ndrx_inicfg_section_t * cfg_section_new(ndrx_inicfg_section_t **sections_h, char *section)
{
    ndrx_inicfg_section_t * ret = calloc(1, sizeof(ndrx_inicfg_section_t));
            
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
    
    ret = NULL;
    
    EXHASH_FIND_STR( (*sections_h), section, ret);
    
out:
    return ret;
}

/**
 * Get QHASH record for particular q
 * @param qname
 * @return 
 */
private ndrx_inicfg_section_t * cfg_section_get(ndrx_inicfg_section_t **sections_h, char *section)
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
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "Handler got: resource [%s]/file [%s] section [%s]"
            " name [%s] value [%s] cf %p\n", 
            cf->resource, cf->fullname, section, name, value, cf);
#endif
    
    if (NULL!=section_start_with)
    {
        needed = FALSE;
        if (NULL==section_start_with)
        {
            needed = TRUE;
        }
        else while (NULL!=*section_start_with)
        {
            int len = NDRX_MIN(strlen(*section_start_with), strlen(section));
            
            if (0 == strncmp(*section_start_with, section, len))
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
    
    mem_value = calloc(1, sizeof(ndrx_inicfg_section_keyval_t));
    
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
private int _ndrx_inicfg_load_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    ndrx_inicfg_file_t *cf = NULL;
    int ret = SUCCEED;
    char fn[] = "_ndrx_inicfg_load_single_file";
    
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
    
    cf->refreshed = TRUE;
    
    if (SUCCEED!=stat(fullname, &cf->attr))
    {
        _Nset_error_fmt(NEUNIX, "%s: stat() failed for [%s]:%s", 
                fn, fullname, strerror(errno));
        FAIL_OUT(ret);    
    }
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "Opened config file %p\n", cf);
#endif

    /* start to parse the file by inih */
    if (SUCCEED!=(ret=ini_parse(fullname, handler, (void *)cf, (void *)section_start_with)))
    {
        _Nset_error_fmt(NEINVALINI, "%s: Invalid ini file: [%s] error on line: %d", 
                fn, fullname, ret);
        FAIL_OUT(ret);
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
private ndrx_inicfg_file_t* cfg_single_file_get(ndrx_inicfg_t *cfg, char *fullname)
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
private int _ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    int ret = SUCCEED;
    struct stat attr; 
    char fn[] = "_ndrx_inicfg_update_single_file";
    int ferr = 0;
    /* try to get the file handler (resolve) */
    ndrx_inicfg_file_t *cf = cfg_single_file_get(cfg, fullname);
    
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: enter resource [%s]/file [%s]\n",
                        fn, resource, fullname);
#endif

    if (SUCCEED!=stat(fullname, &attr))
    {
        /* check the error. */
        ferr = errno;
    }
    
    if (NULL!=cf && SUCCEED==ferr && 
            0!=memcmp(&attr.st_mtime, &cf->attr.st_mtime, sizeof(attr.st_mtime)))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: [%s]/file [%s] changed - reload\n",
                        fn, resource, fullname);
#endif
        /* reload the file - kill the old one, and load again */
        ndrx_inicfg_file_free(cfg, cf);
        if (SUCCEED!=_ndrx_inicfg_load_single_file(cfg, resource, fullname, 
                section_start_with))
        {
            FAIL_OUT(ret);
        }
    }
    else if (NULL!=cf && SUCCEED==ferr)
    {
        /* config not changed, mark as refreshed */
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: [%s]/file [%s] not-changed - do nothing\n",
                        fn, resource, fullname);
#endif
        cf->refreshed = TRUE;
        goto out;
    }
    else if (NULL==cf && SUCCEED==ferr)
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: [%s]/file [%s] config does not exists, "
                "but file exists - load\n",
                        fn, resource, fullname);
#endif
        /* config does not exists, but file exists - load */
        if (SUCCEED!=_ndrx_inicfg_load_single_file(cfg, resource, fullname, 
                section_start_with))
        {
            FAIL_OUT(ret);
        }
    }
    else if (NULL!=cf && SUCCEED!=ferr)
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
private int _ndrx_inicfg_add(ndrx_inicfg_t *cfg, char *resource, char **section_start_with)
{
    int ret = SUCCEED;
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
        if (SUCCEED!=_ndrx_inicfg_update_single_file(cfg, resource, 
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
                   
                   if (SUCCEED!=_ndrx_inicfg_update_single_file(cfg, resource, 
                           tmp, section_start_with))
                   {
                       FAIL_OUT(ret);
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
        if (SUCCEED!=ndrx_string_hash_add(&(cfg->resource_hash), resource))
        {
            _Nset_error_fmt(NEMALLOC, "%s: ndrx_string_hash_add - malloc failed", fn);
            FAIL_OUT(ret);
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
private int _ndrx_keyval_hash_add(ndrx_inicfg_section_keyval_t **h, 
            ndrx_inicfg_section_keyval_t *src)
{
    int ret = SUCCEED;
    char fn[]="_ndrx_keyval_hash_add";
    ndrx_inicfg_section_keyval_t * tmp = calloc(1, sizeof(ndrx_inicfg_section_keyval_t));
    
    if (NULL==tmp)
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "alloc of ndrx_inicfg_section_keyval_t (%d) failed\n", 
                (int)sizeof(ndrx_inicfg_section_keyval_t));
#endif
        FAIL_OUT(ret);
    }
    
    if (NULL==(tmp->key = strdup(src->key)))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "strdup() failed: %s\n", strerror(errno));
        _Nset_error_fmt(NEMALLOC, "%s: malloc failed", fn);
#endif
        FAIL_OUT(ret);
    }
    
    if (NULL==(tmp->val = strdup(src->val)))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "strdup() failed: %s\n", strerror(errno));
        _Nset_error_fmt(NEMALLOC, "%s: malloc failed", fn);
        
#endif
        FAIL_OUT(ret);
    }
    
    if (NULL==(tmp->section = strdup(src->section)))
    {
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "strdup() failed: %s\n", strerror(errno));
        _Nset_error_fmt(NEMALLOC, "%s: malloc failed", fn);
#endif
        FAIL_OUT(ret);
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
private ndrx_inicfg_section_keyval_t * _ndrx_keyval_hash_get(
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
private void _ndrx_keyval_hash_free(ndrx_inicfg_section_keyval_t *h)
{
    ndrx_inicfg_section_keyval_t * r=NULL, *rt=NULL;
    /* safe iter over the list */
    EXHASH_ITER(hh, h, r, rt)
    {
        EXHASH_DEL(h, r);
        free(r->key);
        free(r->val);
        free(r->section);
        free(r);
    }
}

/**
 * Resolve the section
 * @param cfg
 * @param section
 * @param out
 * @return 
 */
private int _ndrx_inicfg_resolve(ndrx_inicfg_t *cfg, char **resources, char *section, 
        ndrx_inicfg_section_keyval_t **out)
{
    int i;
    int found;
    int ret = SUCCEED;
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
        found = FALSE;
        i = 0;
        
#ifdef INICFG_ENABLE_DEBUG
        fprintf(stderr, "%s: Checking [%s]...\n", fn, config_file->fullname);
#endif
    
        /* If resources is NULL, then look in all config files stored.  */
        if (NULL==resources)
        {
            found = TRUE;
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
                    found = TRUE;
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
                        if (SUCCEED!=_ndrx_keyval_hash_add(out, vals))
                        {
                            FAIL_OUT(ret);
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
private int _ndrx_inicfg_get_subsect(ndrx_inicfg_t *cfg, 
        char **resources, char *section, ndrx_inicfg_section_keyval_t **out)
{
    int ret = SUCCEED;
    char fn[] = "_ndrx_inicfg_section_keyval_t";
    char *tmp = strdup(section);
    char *p;
    
    if (NULL==tmp)
    {
        _Nset_error_fmt(NEMALLOC, "%s: malloc failed", fn);
        FAIL_OUT(ret);
    }
    
    while (EOS!=tmp[0])
    {
        if (SUCCEED!=_ndrx_inicfg_resolve(cfg, resources, tmp, out))
        {
            FAIL_OUT(ret);
        }
        p = strchr(tmp, NDRX_INICFG_SUBSECT_SPERATOR);
        
        if (NULL!=p)
        {
            *p = EOS;
        }
        else
        {
            break; /* terminate, we are done */
        }
    }
    
out:
    if (NULL!=tmp)
    {
        free(tmp);
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
private int _ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, 
        char **resources,
        char **section_start_with, 
        ndrx_inicfg_section_t **out)
{
    int i;
    int found;
    int ret = SUCCEED;
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
        found = FALSE;
        i = 0;
        
        /* If resources is NULL, then look in all config files stored.  */
        if (NULL==resources)
        {
            found = TRUE;
        }
        else 
        {
            while(NULL!=resources[i])
            {
                if (0==strcmp(config_file->resource, resources[i]))
                {
                    found = TRUE;
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
                
                found = FALSE;
                i = 0;
                
                if (NULL==section_start_with)
                {
                    found = TRUE;
                }
                while (EOS!=section_start_with[i])
                {
                    len = NDRX_MIN(strlen(section->section), strlen(section_start_with[i]));
                    if (0==strncmp(section->section, section_start_with[i], len))
                    {
                        found = TRUE;
                        break;
                    }
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
                        FAIL_OUT(ret);
                    }

                    ndrx_inicfg_section_keyval_t *vals = NULL, *vals_tmp = NULL;
                    /* ok we got a section, now get the all values down in section */
                    EXHASH_ITER(hh, (section->values), vals, vals_tmp)
                    {
                        if (NULL==_ndrx_keyval_hash_get((section_work->values), vals->key))
                        {
                            if (SUCCEED!=_ndrx_keyval_hash_add(&(section_work->values), vals))
                            {
                                FAIL_OUT(ret);
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
private void _ndrx_inicfg_sections_free(ndrx_inicfg_section_t *sections)
{
    char fn[] = "_ndrx_inicfg_sections_free";    
    ndrx_inicfg_section_t *section=NULL, *section_temp=NULL;
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: enter %p\n", fn, sections);
#endif
        
    /* kill the sections */
    EXHASH_ITER(hh, sections, section, section_temp)
    {
        ndrx_keyval_hash_free(section->values);
        free(section->section);
        free(section);
    }
}

/**
 * Free the memory of file
 * @param cfg
 * @param fullfile
 * @return 
 */
private void _ndrx_inicfg_file_free(ndrx_inicfg_t *cfg, ndrx_inicfg_file_t *cfgfile)
{
    char fn[] = "_ndrx_inicfg_file_free";
    ndrx_inicfg_section_t *section=NULL, *section_temp=NULL;
    
#ifdef INICFG_ENABLE_DEBUG
    fprintf(stderr, "%s: enter cfg = %p cfgfile = %p\n", fn, cfg, cfgfile);
#endif
    
    EXHASH_DEL(cfg->cfgfile, cfgfile);
    
    ndrx_inicfg_sections_free(cfgfile->sections);
    
    free(cfgfile);
}

/**
 * Free the whole config
 * @param cfg config handler will become invalid after this operation
 * @return 
 */
private void _ndrx_inicfg_free(ndrx_inicfg_t *cfg)
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
    
    free(cfg);
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
public int ndrx_inicfg_reload(ndrx_inicfg_t *cfg, char **section_start_with)
{
    API_ENTRY;
    return _ndrx_inicfg_reload(cfg, section_start_with);
}


/**
 * Create new config handler
 * @return ptr to config handler or NULL
 */
public ndrx_inicfg_t * ndrx_inicfg_new(void)
{
    API_ENTRY;
    return _ndrx_inicfg_new();
}



/**
 * API version of _ndrx_inicfg_load_single_file
 */
public int ndrx_inicfg_load_single_file(ndrx_inicfg_t *cfg, 
        char *resource, char *fullname, char **section_start_with)
{
    API_ENTRY;
    
    return _ndrx_inicfg_load_single_file(cfg, resource, fullname, section_start_with);
    
}



/**
 * API version of _ndrx_inicfg_update_single_file
 */
public int ndrx_inicfg_update_single_file(ndrx_inicfg_t *cfg, 
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
public int ndrx_inicfg_add(ndrx_inicfg_t *cfg, char *resource, char **section_start_with)
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
public int ndrx_keyval_hash_add(ndrx_inicfg_section_keyval_t **h, 
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
public ndrx_inicfg_section_keyval_t * ndrx_keyval_hash_get(
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
public void ndrx_keyval_hash_free(ndrx_inicfg_section_keyval_t *h)
{
    API_ENTRY;
    return _ndrx_keyval_hash_free(h);
}

/**
 * Resolve the section (api version of _ndrx_inicfg_resolve)
 * @param cfg
 * @param section
 * @param out
 * @return 
 */
public int ndrx_inicfg_resolve(ndrx_inicfg_t *cfg, char **resources, char *section, 
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
public int ndrx_inicfg_get_subsect(ndrx_inicfg_t *cfg, 
        char **resources, char *section, ndrx_inicfg_section_keyval_t **out)
{
    API_ENTRY;
    
    return _ndrx_inicfg_get_subsect(cfg, resources, section, out);
}

/**
 * Iterate over the sections & return the matched image
 * We might want to return multiple hashes here of the sections found.
 * API version of _ndrx_inicfg_iterate
 * @param cfg
 * @param fullsection_starts_with
 * @return List of sections
 */
public int ndrx_inicfg_iterate(ndrx_inicfg_t *cfg, 
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
public void ndrx_inicfg_sections_free(ndrx_inicfg_section_t *sections)
{
    API_ENTRY;
    ndrx_inicfg_sections_free(sections);
}

/**
 * Free the memory of file (API version of _ndrx_inicfg_file_free)
 * API version of 
 * @param cfg
 * @param fullfile
 * @return 
 */
public void ndrx_inicfg_file_free(ndrx_inicfg_t *cfg, ndrx_inicfg_file_t *cfgfile)
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
public void ndrx_inicfg_free(ndrx_inicfg_t *cfg)
{
    API_ENTRY;
    _ndrx_inicfg_free(cfg);
}

