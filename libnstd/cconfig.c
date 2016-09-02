/* 
** Enduro/X Common-Config
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
#include <inicfg.h>
#include <cconfig.h>
#include <nerror.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* #define CCONFIG_ENABLE_DEBUG */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/* These should be semaphore mutex globals... */
ndrx_inicfg_t *G_cconfig = NULL; /* Common-config handler */
char *G_cctag = NULL;
int G_tried_to_load = FALSE; /* did we try to load the config? */
/*---------------------------Statics------------------------------------*/

private char *M_sections[] = {NDRX_CONF_SECTION_GLOBAL, 
                        NDRX_CONF_SECTION_DEBUG, 
                        NDRX_CONF_SECTION_QUEUE, 
                        NULL};
MUTEX_LOCKDECL(M_load_lock);
/*---------------------------Prototypes---------------------------------*/

/**
 * Split up the CCTAG (by tokens)
 * and lookup data by any of the tags 
 * @param section
 * @return 
 */
public int ndrx_cconfig_get_cf(ndrx_inicfg_t *cfg, char *section, ndrx_inicfg_section_keyval_t **out)
{
    int ret = SUCCEED;
    int len;
    char *tmp1 = NULL; /* lookup section  */
    char *tmp2 = NULL; /* token cctag */
    char fn[] = "ndrx_cconfig_get";
    char *saveptr1;
    char *token_cctag;
    
    /*  if we have CC tag, with sample value "RM1/DBG2"
     * Then lookup will take in this order:
     * [@debug/RM1]
     * [@debug/DBG2]
     */
    if (NULL!=G_cctag)
    {
        len = strlen(section);
        if (NULL!=G_cctag)
        {
            len+=strlen(G_cctag);
        }

        if (NULL==(tmp1 = malloc(len+1)))
        {
            fprintf(stderr, "%s: tmp1 malloc failed: %s\n", fn, strerror(errno));
            FAIL_OUT(ret);
        }


        if (NULL==(tmp2 = malloc(strlen(G_cctag)+1)))
        {
            fprintf(stderr, "%s: tmp2 malloc failed: %s\n", fn, strerror(errno));
            FAIL_OUT(ret);
        }

        strcpy(tmp2, G_cctag);


        token_cctag = strtok_r(tmp2, NDRX_INICFG_SUBSECT_SPERATOR_STR, &saveptr1);

        while( token_cctag != NULL )
        {
            strcpy(tmp1, section);
            strcat(tmp1, NDRX_INICFG_SUBSECT_SPERATOR_STR);
            strcat(tmp1, token_cctag);

            if (SUCCEED!=ndrx_inicfg_get_subsect(cfg, 
                                NULL,  /* all config files */
                                tmp1,  /* global section */
                                out))
            {
                fprintf(stderr, "%s: %s\n", fn, Nstrerror(Nerror));
                FAIL_OUT(ret);
            }
            
            token_cctag = strtok_r(NULL, NDRX_INICFG_SUBSECT_SPERATOR_STR, &saveptr1);
        }
    }/* Direct lookup if no cctag */
    else if (SUCCEED!=ndrx_inicfg_get_subsect(cfg, 
                                NULL,  /* all config files */
                                section,  /* global section */
                                out))
    {
        fprintf(stderr, "%s: %s\n", fn, Nstrerror(Nerror));
        FAIL_OUT(ret);
    }
    
out:

    if (NULL!=tmp1)
    {
        free(tmp1);
    }

    if (NULL!=tmp2)
    {
        free(tmp2);
    }


    return ret;
}

/**
 * Get the Enduro/X config
 * @param section
 * @param out
 * @return 
 */
public int ndrx_cconfig_get(char *section, ndrx_inicfg_section_keyval_t **out)
{
    return ndrx_cconfig_get_cf(G_cconfig, section,  out);
}

/**
 * Load config (for Enduro/X only)
 * @return 
 */
private int _ndrx_cconfig_load(ndrx_inicfg_t **cfg, int is_internal)
{
    int ret = SUCCEED;
    int slot = 0;
    int have_config = FALSE;
    char fn[] = "ndrx_cconfig_load";
    char *config_resources[]= { NULL, 
                          NULL, 
                          NULL, 
                          NULL, 
                          NULL, 
                          NULL, 
                          NULL};
    
    if (NULL!=*cfg)
    {
        /* config already loaded... */
        return SUCCEED;
    }
    
    ndrx_inicfg_section_keyval_t *keyvals = NULL, *keyvals_iter = NULL, 
                *keyvals_iter_tmp = NULL;
    
    if (NULL!=(config_resources[slot] = getenv(NDRX_CCONFIG5)))
    {
        slot++;
    }
    
    if (NULL!=(config_resources[slot] = getenv(NDRX_CCONFIG4)))
    {
        slot++;
    }
    
    if (NULL!=(config_resources[slot] = getenv(NDRX_CCONFIG3)))
    {
        slot++;
    }
    
    if (NULL!=(config_resources[slot] = getenv(NDRX_CCONFIG2)))
    {
        slot++;
    }
    if (NULL!=(config_resources[slot] = getenv(NDRX_CCONFIG1)))
    {
        slot++;
    }
    if (NULL!=(config_resources[slot] = getenv(NDRX_CCONFIG)))
    {
        slot++;
    }
        
    /* Check if envs are set before try to load */
    if (NULL==(*cfg = ndrx_inicfg_new()))
    {
        fprintf(stderr, "%s: %s\n", fn, Nstrerror(Nerror));
        FAIL_OUT(ret);
    }
    
    /* Load the stuff */
    slot = 0;
    
    while (NULL!=config_resources[slot])
    {
#ifdef CCONFIG_ENABLE_DEBUG
        fprintf(stderr, "have config at slot [%d] [%s]\n", slot, config_resources[slot]);
#endif
        have_config = TRUE;
        if (SUCCEED!=ndrx_inicfg_add(*cfg, config_resources[slot], 
                (is_internal?(char **)M_sections:NULL)))
        {
            fprintf(stderr, "%s: %s\n", fn, Nstrerror(Nerror));
            FAIL_OUT(ret);
        }
        slot++;
    }

   if (NULL==config_resources[0])
   {
        have_config = FALSE;
        goto out;
   }
    
    if (is_internal)
    {
        /* Get globals & transfer to setenv() */
        if (SUCCEED!=ndrx_cconfig_get_cf(*cfg, NDRX_CONF_SECTION_GLOBAL, &keyvals))
        {
            fprintf(stderr, "%s: %s lookup failed: %s\n", fn, 
                    NDRX_CONF_SECTION_GLOBAL, Nstrerror(Nerror));
            FAIL_OUT(ret);
        }

        /* Loop over and load the stuff... */
        EXHASH_ITER(hh, keyvals, keyvals_iter, keyvals_iter_tmp)
        {
#ifdef CCONFIG_ENABLE_DEBUG
            fprintf(stderr, "settings %s=%s\n", keyvals_iter->key, keyvals_iter->val);
#endif

            if (SUCCEED!=setenv(keyvals_iter->key, keyvals_iter->val, TRUE))
            {
                fprintf(stderr, "%s: failed to set %s=%s: %s\n", fn, 
                    keyvals_iter->key, keyvals_iter->val, strerror(errno));
                FAIL_OUT(ret);
            }
#ifdef CCONFIG_ENABLE_DEBUG
            fprintf(stderr, "test value %s\n", getenv(keyvals_iter->key));
#endif
        }
    }
    
out:

    if (NULL!=keyvals)
    {
        ndrx_keyval_hash_free(keyvals);
        keyvals = NULL;
    }
    if (SUCCEED!=ret || !have_config)
    {
        if (NULL!=*cfg)
        {
#ifdef CCONFIG_ENABLE_DEBUG
        fprintf(stderr, "Removing config %p\n", *cfg);
#endif
            ndrx_inicfg_free(*cfg);
            *cfg = NULL;
        }
    }

    if (SUCCEED==ret && is_internal)
    {
        G_tried_to_load = TRUE;
    }

    return ret;
}
/**
 * Internal for Enduro/X - load the config
 * @return 
 */
public int ndrx_cconfig_load(void)
{
    int ret;
    /* protect against multi-threading */
    MUTEX_LOCK_V(M_load_lock);
    
    /* todo: might need first. */
    if (NULL!=G_cctag)
    {
        G_cctag = getenv(NDRX_CCTAG);
    }
    
    ret = _ndrx_cconfig_load(&G_cconfig, TRUE);
    
    MUTEX_UNLOCK_V(M_load_lock);
    
    return ret;
}

/**
 * General for user (make config out of the Enduro/X cfg files)
 * Use shared config
 * @param cfg double ptr to config object
 * @return 
 */
public int ndrx_cconfig_load_general(ndrx_inicfg_t **cfg)
{
    /* todo: might need first. */
    if (NULL!=G_cctag)
    {
        G_cctag = getenv(NDRX_CCTAG);
    }
    
    return _ndrx_cconfig_load(cfg, FALSE);
}

/**
 * Reload Enduro/X CConfig
 * @return 
 */
public int ndrx_cconfig_reload(void)
{
    char fn[]="ndrx_cconfig_reload";
    if (SUCCEED!=ndrx_inicfg_reload(G_cconfig, M_sections))
    {
        fprintf(stderr, "%s: %s lookup to reload: %s\n", fn, 
                NDRX_CONF_SECTION_GLOBAL, Nstrerror(Nerror));
    }
}

/**
 * Access to to atmi lib env globals
 * @return 
 */
public ndrx_inicfg_t *ndrx_get_G_cconfig(void)
{
#ifdef CCONFIG_ENABLE_DEBUG
    fprintf(stderr, "returning G_cconfig %p\n", G_cconfig);
#endif
    if (!G_tried_to_load)
    {
        /* try to load */
        ndrx_cconfig_load();
    }
    return G_cconfig;
}
