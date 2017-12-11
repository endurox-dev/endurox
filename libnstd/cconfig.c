/* 
** Enduro/X Common-Config
**
** @file inicfg.c
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
#include <userlog.h>
#include <expluginbase.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* #define CCONFIG_ENABLE_DEBUG */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/* These should be semaphore mutex globals... */
ndrx_inicfg_t *G_cconfig = NULL; /* Common-config handler */
char *G_cctag = NULL;
int G_tried_to_load = EXFALSE; /* did we try to load the config? */
/*---------------------------Statics------------------------------------*/

exprivate char *M_sections[] = {NDRX_CONF_SECTION_GLOBAL, 
                        NDRX_CONF_SECTION_DEBUG, 
                        NDRX_CONF_SECTION_QUEUE, 
                        NULL};

/* for first pass load only global! */
exprivate char *M_sections_first_pass[] = {NDRX_CONF_SECTION_GLOBAL,
                        NULL};

MUTEX_LOCKDECL(M_load_lock);
/*---------------------------Prototypes---------------------------------*/

exprivate int _ndrx_cconfig_load_pass(ndrx_inicfg_t **cfg, int is_internal, 
        char **section_start_with);

/**
 * Split up the CCTAG (by tokens)
 * and lookup data by any of the tags 
 * @param section
 * @return 
 */
expublic int ndrx_cconfig_get_cf(ndrx_inicfg_t *cfg, char *section, ndrx_inicfg_section_keyval_t **out)
{
    int ret = EXSUCCEED;
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

        if (NULL==(tmp1 = NDRX_MALLOC(len+2))) /* 1 for eos, another for seperator */
        {
            userlog("%s: tmp1 malloc failed: %s", fn, strerror(errno));
            EXFAIL_OUT(ret);
        }


        if (NULL==(tmp2 = NDRX_MALLOC(strlen(G_cctag)+1)))
        {
            userlog("%s: tmp2 malloc failed: %s", fn, strerror(errno));
            EXFAIL_OUT(ret);
        }

        strcpy(tmp2, G_cctag);


        token_cctag = strtok_r(tmp2, NDRX_INICFG_SUBSECT_SPERATOR_STR, &saveptr1);

        while( token_cctag != NULL )
        {
            strcpy(tmp1, section);
            strcat(tmp1, NDRX_INICFG_SUBSECT_SPERATOR_STR);
            strcat(tmp1, token_cctag);

            if (EXSUCCEED!=ndrx_inicfg_get_subsect(cfg, 
                                NULL,  /* all config files */
                                tmp1,  /* global section */
                                out))
            {
                userlog("%s: %s", fn, Nstrerror(Nerror));
                EXFAIL_OUT(ret);
            }
            
            token_cctag = strtok_r(NULL, NDRX_INICFG_SUBSECT_SPERATOR_STR, &saveptr1);
        }
    }/* Direct lookup if no cctag */
    else if (EXSUCCEED!=ndrx_inicfg_get_subsect(cfg, 
                                NULL,  /* all config files */
                                section,  /* global section */
                                out))
    {
        userlog("%s: %s", fn, Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=tmp1)
    {
        NDRX_FREE(tmp1);
    }

    if (NULL!=tmp2)
    {
        NDRX_FREE(tmp2);
    }


    return ret;
}

/**
 * Get the Enduro/X config
 * @param section
 * @param out
 * @return 
 */
expublic int ndrx_cconfig_get(char *section, ndrx_inicfg_section_keyval_t **out)
{
    return ndrx_cconfig_get_cf(G_cconfig, section,  out);
}

/**
 * Two pass config load orchestrator
 * First pass will load global variables (environment)
 * Second pass will allow to reference for example other variable from global
 * section.
 * Firstly we will load dummy value (${XXX} not substituted as empty), but second
 * pass will once again load the actual resolved values.
 * @param cfg
 * @param is_internal
 * @return 
 */
exprivate int _ndrx_cconfig_load(ndrx_inicfg_t **cfg, int is_internal)
{
    int ret = EXSUCCEED;
    
    if (is_internal)
    {
        ndrx_inicfg_t *cfg_first_pass = NULL;
        /* two pass loading */
        
        /* first pass will go with dummy config, then we make it free... 
         * and we are interested only in global section
         */
        if (EXSUCCEED!=_ndrx_cconfig_load_pass(&cfg_first_pass, EXTRUE, 
                M_sections_first_pass))
        {
            userlog("Failed to load first pass config!");
            EXFAIL_OUT(ret);
        }
        
        /* release the config 
         * Load second pass only when first was ok i.e. present.
         */
        if (NULL!=cfg_first_pass)
        {
            ndrx_inicfg_free(cfg_first_pass);
            ret = _ndrx_cconfig_load_pass(cfg, EXTRUE, M_sections);
        }
        
    }
    else
    {
        /* single pass */
        ret = _ndrx_cconfig_load_pass(cfg, EXFALSE, NULL);
    }
    
out:
    return ret;
}

/**
 * Load config (for Enduro/X only)
 * @return 
 */
exprivate int _ndrx_cconfig_load_pass(ndrx_inicfg_t **cfg, int is_internal, 
        char **section_start_with)
{
    int ret = EXSUCCEED;
    int slot = 0;
    int have_config = EXFALSE;
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
        return EXSUCCEED;
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
    if (NULL==(*cfg = ndrx_inicfg_new2(EXTRUE)))
    {
        userlog("%s: %s", fn, Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
    /* Load the stuff */
    slot = 0;

    while (NULL!=config_resources[slot])
    {
#ifdef CCONFIG_ENABLE_DEBUG
        userlog("have config at slot [%d] [%s]", slot, config_resources[slot]);
#endif
        NDRX_LOG_EARLY(log_debug, "have config at slot [%d] [%s]", 
                slot, config_resources[slot]);
        
        have_config = EXTRUE;
        
        
        if (EXSUCCEED!=ndrx_inicfg_add(*cfg, config_resources[slot], section_start_with))
        {
            userlog("%s: %s", fn, Nstrerror(Nerror));
            EXFAIL_OUT(ret);
        }
        
        slot++;
    }

    if (NULL==config_resources[0])
    {
        have_config = EXFALSE;
        goto out;
    }
    
    if (is_internal)
    {
        /* Get globals & transfer to setenv() */
        if (EXSUCCEED!=ndrx_cconfig_get_cf(*cfg, NDRX_CONF_SECTION_GLOBAL, &keyvals))
        {
            userlog("%s: %s lookup failed: %s", fn, 
                    NDRX_CONF_SECTION_GLOBAL, Nstrerror(Nerror));
            EXFAIL_OUT(ret);
        }

        /* Loop over and load the stuff... */
        EXHASH_ITER(hh, keyvals, keyvals_iter, keyvals_iter_tmp)
        {
#ifdef CCONFIG_ENABLE_DEBUG
            userlog("settings %s=%s", keyvals_iter->key, keyvals_iter->val);
#endif

            if (EXSUCCEED!=setenv(keyvals_iter->key, keyvals_iter->val, EXTRUE))
            {
                userlog("%s: failed to set %s=%s: %s", fn, 
                    keyvals_iter->key, keyvals_iter->val, strerror(errno));
                EXFAIL_OUT(ret);
            }
#ifdef CCONFIG_ENABLE_DEBUG
            userlog("test value %s\n", getenv(keyvals_iter->key));
#endif
        }
    }
    
out:

    if (NULL!=keyvals)
    {
        ndrx_keyval_hash_free(keyvals);
        keyvals = NULL;
    }
    if (EXSUCCEED!=ret || !have_config)
    {
        if (NULL!=*cfg)
        {
#ifdef CCONFIG_ENABLE_DEBUG
        userlog("Removing config %p", *cfg);
#endif
            ndrx_inicfg_free(*cfg);
            *cfg = NULL;
        }
    }
    
    if (EXSUCCEED==ret && is_internal)
    {
        G_tried_to_load = EXTRUE;
    }

    /* Add some debug on config load */
    NDRX_LOG_EARLY(log_debug, "%s: ret: %d is_internal: %d G_tried_to_load: %d",
            __func__, ret, is_internal, G_tried_to_load);

    return ret;
}
/**
 * Internal for Enduro/X - load the config
 * @return 
 */
expublic int ndrx_cconfig_load(void)
{
    /* this might be called from debug... */
    static int first = EXTRUE;
    static int first_ret = EXSUCCEED;
    
    if (first)
    {
        /* protect against multi-threading */
        MUTEX_LOCK_V(M_load_lock);
        
        /* Lock the debug... */
        ndrx_dbg_intlock_set();
        
        /* if still is not set... */
        if (first)
        {
            /* basically at this point we must load the plugins.. */
            ndrx_plugins_load();
            
            if (NULL==G_cctag)
            {
                G_cctag = getenv(NDRX_CCTAG);
            }

            first_ret = _ndrx_cconfig_load(&G_cconfig, EXTRUE);

            first = EXFALSE;
        }
        
        /* Do this outside the lock... */
        ndrx_dbg_intlock_unset();
        
        MUTEX_UNLOCK_V(M_load_lock);
        
    }
    
    return first_ret;
}

/**
 * General for user (make config out of the Enduro/X cfg files)
 * Use shared config
 * @param cfg double ptr to config object
 * @return 
 */
expublic int ndrx_cconfig_load_general(ndrx_inicfg_t **cfg)
{
    if (NULL==G_cctag)
    {
        G_cctag = getenv(NDRX_CCTAG);
    }
    
    return _ndrx_cconfig_load(cfg, EXFALSE);
}

/**
 * Reload Enduro/X CConfig
 * @return 
 */
expublic int ndrx_cconfig_reload(void)
{
    char fn[]="ndrx_cconfig_reload";
    if (EXSUCCEED!=ndrx_inicfg_reload(G_cconfig, M_sections))
    {
        userlog("%s: %s lookup to reload: %s", fn, 
                NDRX_CONF_SECTION_GLOBAL, Nstrerror(Nerror));
        return EXFAIL;
    }

    return EXSUCCEED;
}

/**
 * Access to to atmi lib env globals
 * @return 
 */
expublic ndrx_inicfg_t *ndrx_get_G_cconfig(void)
{
#ifdef CCONFIG_ENABLE_DEBUG
    userlog("returning G_cconfig %p", G_cconfig);
#endif
    if (!G_tried_to_load)
    {
        /* try to load */
        ndrx_cconfig_load();
    }
    return G_cconfig;
}
