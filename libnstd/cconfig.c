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
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

ndrx_inicfg_t *G_cconfig = NULL; /* Common-config handler */
char *G_cctag = NULL;
/*---------------------------Prototypes---------------------------------*/


/**
 * TODO: Split up the CCTAG (by tokens)
 * and lookup data by any of the tags 
 * @param section
 * @return 
 */
public int ndrx_cconfig_get(char *section, ndrx_inicfg_section_keyval_t **out)
{
    int ret = SUCCEED;
    
    
out:
    return ret;
}
/**
 * Load cconfig
 */
public int ndrx_cconfig_load(void)
{
    int ret = SUCCEED;
    int slot = 0;
    int have_config = TRUE;
    char fn[] = "ndrx_cconfig_load";
    char *config_resources[]= { NULL, 
                          NULL, 
                          NULL, 
                          NULL, 
                          NULL, 
                          NULL, 
                          NULL};
    G_cctag = getenv(NDRX_CCTAG);
    
    char *sections[] = {"@globals", "@debug", "@queues", NULL};
    
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
        if (SUCCEED!=ndrx_inicfg_add(G_cconfig, config_resources[slot], 
                (char **)sections))
        {
            fprintf(stderr, "%s: %s\n", fn, Nstrerror(Nerror));
            FAIL_OUT(ret);
        }
        slot++;
    }
    
    /* TODO: get globals & transfer to setenv() */
    
out:

    if (SUCCEED!=NULL)
    {
        ndrx_inicfg_free(G_cconfig);
        G_cconfig = NULL;
    }

    return ret;
}

/**
 * Access to to atmi lib env globals
 * @return 
 */
public ndrx_inicfg_t *ndrx_get_G_cconfig(void)
{
    return G_cconfig;
}
