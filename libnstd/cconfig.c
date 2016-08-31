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
#define CCONFIG_ENABLE_DEBUG
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

private char *M_sections[] = {NDRX_CONF_SECTION_GLOBAL, 
                        NDRX_CONF_SECTION_DEBUG, 
                        NDRX_CONF_SECTION_QUEUE, 
                        NULL};
/* These should be semaphore mutex globals... */
ndrx_inicfg_t *G_cconfig = NULL; /* Common-config handler */
char *G_cctag = NULL;
/*---------------------------Prototypes---------------------------------*/

/**
 * Split up the CCTAG (by tokens)
 * and lookup data by any of the tags 
 * @param section
 * @return 
 */
public int ndrx_cconfig_get(char *section, ndrx_inicfg_section_keyval_t **out)
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

            if (SUCCEED!=ndrx_inicfg_get_subsect(G_cconfig, 
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
    else if (SUCCEED!=ndrx_inicfg_get_subsect(G_cconfig, 
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
 * Load cconfig
 */
public int ndrx_cconfig_load(void)
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
    G_cctag = getenv(NDRX_CCTAG);
    
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
    if (NULL==(G_cconfig = ndrx_inicfg_new()))
    {
        fprintf(stderr, "%s: %s\n", fn, Nstrerror(Nerror));
        FAIL_OUT(ret);
    }
    
    /* Load the stuff */
    slot = 0;
    
    while (NULL!=config_resources[slot])
    {
        have_config = TRUE;
        if (SUCCEED!=ndrx_inicfg_add(G_cconfig, config_resources[slot], 
                (char **)M_sections))
        {
            fprintf(stderr, "%s: %s\n", fn, Nstrerror(Nerror));
            FAIL_OUT(ret);
        }
        slot++;
    }
    
    /* Get globals & transfer to setenv() */
    if (SUCCEED!=ndrx_cconfig_get(NDRX_CONF_SECTION_GLOBAL, &keyvals))
    {
        fprintf(stderr, "%s: %s lookup failed: %s\n", fn, 
                NDRX_CONF_SECTION_GLOBAL, Nstrerror(Nerror));
        FAIL_OUT(ret);
    }
    
    /* Loop over and load the stuff... */
    HASH_ITER(hh, keyvals, keyvals_iter, keyvals_iter_tmp)
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
    
out:

    if (NULL!=keyvals)
    {
        ndrx_keyval_hash_free(keyvals);
        keyvals = NULL;
    }
    if (SUCCEED!=ret)
    {
        if (NULL!=G_cconfig)
        {
            ndrx_inicfg_free(G_cconfig);
            G_cconfig = NULL;
        }
    }
    else if (!have_config)
    {
        ndrx_inicfg_free(G_cconfig);
        G_cconfig = NULL;
    }

    return ret;
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
    return G_cconfig;
}
