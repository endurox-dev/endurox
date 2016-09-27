/* 
** Basic INI tests...
**
** @file atmiclt29.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include "test029.h"
#include <inicfg.h>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <exhash.h>
#include <nerror.h>
#include <cconfig.h>
#include <errno.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Append text file
 * @param file
 * @param line
 * @return 
 */
int append_text_file(char *file, char *line)
{
    int ret = SUCCEED;
    FILE *f = fopen(file, "a");
    
    if (NULL==f)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to open [%s]: %s", file, strerror(errno));
        FAIL_OUT(ret);
    }
    
    fprintf(f, "%s", line);
    
out:
    if (NULL!=f)
    {
        fclose(f);
    }
    return ret;
}

/*
 * Main test case entry
 */
int main(int argc, char** argv)
{
    int ret = SUCCEED;
    ndrx_inicfg_t *cfg;
    ndrx_inicfg_section_keyval_t *out = NULL;
    ndrx_inicfg_section_keyval_t *val;
    ndrx_inicfg_section_t *sections, *iter, *iter_tmp;
    char *iterfilter[] = {"my", NULL};
    int i;
    
    for (i=0; i<5; i++)
    {
        out = NULL;
        
        system("/bin/cp C_test_template.ini C_test.ini");
        
        if (NULL==(cfg=ndrx_inicfg_new()))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to make inicfg: %s", Nstrerror(Nerror));
            FAIL_OUT(ret);
        }

        /* any sections */
        if (SUCCEED!=ndrx_inicfg_add(cfg, "./cfg_folder1", NULL))
        {
             NDRX_LOG(log_error, "TESTERROR: failed to add resource: %s", Nstrerror(Nerror));
            FAIL_OUT(ret);       
        }

        if (SUCCEED!=ndrx_inicfg_add(cfg, "A_test.ini", NULL))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to make resource: %s", Nstrerror(Nerror));
            FAIL_OUT(ret);        
        }

        if (SUCCEED!=ndrx_inicfg_add(cfg, "B_test.ini", NULL))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to make resource: %s", Nstrerror(Nerror));
            FAIL_OUT(ret);    
        }
        
        if (SUCCEED!=ndrx_inicfg_add(cfg, "C_test.ini", NULL))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to make resource: %s", Nstrerror(Nerror));
            FAIL_OUT(ret);    
        }


        /* resource == NULL, any resource. */
        if (SUCCEED!=ndrx_inicfg_get_subsect(cfg, NULL, "mysection/subsect1/f/f", &out))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to resolve [mysection/subsect1/f/f]: %s",
                    Nstrerror(Nerror));
            FAIL_OUT(ret);    
        }


        /* get the value from  'out' - should be 4 
         * As firstly it will get the stuff 
         * [mysection/subsect1/f]
         * MYVALUE1=3
         * and then it will get the exact value from B_test.ini
         */

        if (NULL==(val=ndrx_keyval_hash_get(out, "MYVALUE1")))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get MYVALUE1!");
            FAIL_OUT(ret);
        }

        if (0!=strcmp(val->val, "4"))
        {
            NDRX_LOG(log_error, "TESTERROR: [mysection/subsect1/f/f] not 4!");
            FAIL_OUT(ret);
        }

        /* free the list */
        ndrx_keyval_hash_free(out);

        out=NULL;
        /* try some iteration over the config */
        sections = NULL;
        iter = NULL;
        iter_tmp = NULL;

        if (SUCCEED!=ndrx_inicfg_iterate(cfg, NULL, iterfilter, &sections))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to iterate config!");
            FAIL_OUT(ret);
        }

        /* print the stuff we have in config */

        EXHASH_ITER(hh, sections, iter, iter_tmp)
        {
            NDRX_LOG(log_info, "iter: section [%s]", iter->section);
            
            if (0==strcmp(iter->section, "mytest"))
            {
                /* we should have "THIS" key there */
                if (NULL==(val=ndrx_keyval_hash_get(iter->values, "THIS")))
                {
                    NDRX_LOG(log_error, "TESTERROR: Failed to get THIS of [mytest]!");
                    FAIL_OUT(ret);
                }

                if (0!=strcmp(val->val, "IS TEST"))
                {
                    NDRX_LOG(log_error, "TESTERROR: [mytest]/THIS not "
                            "equal to 'IS TEST' but [%s]!", 
                            val->val);
                    FAIL_OUT(ret);
                }
            }
        }
        
        /* kill the section listing */
        ndrx_inicfg_sections_free(sections);
        
        sleep(1); /* have some time to change file-system timestamp... */
        
        /* Update config file */
        append_text_file("C_test.ini", "\n[MOTORCYCLE]\n");
        append_text_file("C_test.ini", "SPEED=120\n");
        append_text_file("C_test.ini", "COLOR=red\n");
        
        /* update/reload */
        if (SUCCEED!=ndrx_inicfg_reload(cfg, NULL))
        {
            NDRX_LOG(log_error, "Failed to reload: %s", Nstrerror(Nerror));
            FAIL_OUT(ret);
        }
        
        /* out=NULL; - should be already */
        /* resource == NULL, any resource. */
        if (SUCCEED!=ndrx_inicfg_get_subsect(cfg, NULL, "MOTORCYCLE", &out))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to resolve [MOTORCYCLE]: %s",
                    Nstrerror(Nerror));
            FAIL_OUT(ret);    
        }

        if (NULL==(val=ndrx_keyval_hash_get(out, "COLOR")))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get COLOR!");
            FAIL_OUT(ret);
        }

        if (0!=strcmp(val->val, "red"))
        {
            NDRX_LOG(log_error, "TESTERROR: [COLOR] is not red, but [%s]!", val->val);
            FAIL_OUT(ret);
        }
        
        /* free-up the lookup */
        ndrx_keyval_hash_free(out);
        out=NULL;
        
        /* free up the config */
        ndrx_inicfg_free(cfg);
    }
    
    
    /* get the Enduro/X handler */
    cfg = NULL;
    out = NULL;
    
    if (SUCCEED!=ndrx_cconfig_load_general(&cfg))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get Enduro/X config handler: %s", Nstrerror(Nerror));
        FAIL_OUT(ret);  
    }
    
    if (SUCCEED!=ndrx_cconfig_get_cf(cfg, "@debug", &out))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to resolve [debug]: %s",
                Nstrerror(Nerror));
        FAIL_OUT(ret);    
    }
    
    /* there should be * */
    if (NULL==(val=ndrx_keyval_hash_get(out, "*")))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get *!");
        FAIL_OUT(ret);
    }

    if (0!=strcmp(val->val, "ndrx=5"))
    {
        NDRX_LOG(log_error, "TESTERROR: invalid value for debug: [%s] vs ", 
                val->val, "ndrx=5");
        FAIL_OUT(ret);
    }
    
    /* we can append the result "virtual secton" with new data: */
    if (SUCCEED!=ndrx_cconfig_get_cf(cfg, "HELLO2", &out))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to resolve [debug]: %s",
                Nstrerror(Nerror));
        FAIL_OUT(ret);    
    }
    
    if (NULL==(val=ndrx_keyval_hash_get(out, "MY")))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get MY!");
        FAIL_OUT(ret);
    }

    if (0!=strcmp(val->val, "PHONE"))
    {
        NDRX_LOG(log_error, "TESTERROR: invalid value for debug: [%s]", val->val);
        FAIL_OUT(ret);
    }
    
    ndrx_keyval_hash_free(out);
    ndrx_inicfg_free(cfg);
    
    /* Now test only Enduro/X cconfig (must not have HELLO2) */
    
    cfg = NULL;
    out = NULL;
    
    if (SUCCEED!=ndrx_cconfig_load())
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get Enduro/X config handler: %s", Nstrerror(Nerror));
        FAIL_OUT(ret);  
    }
    
    if (SUCCEED!=ndrx_cconfig_get("HELLO2", &out))
    {
        NDRX_LOG(log_error, "TESTERROR: Enduro/X internal config must not have [HELLO2]!");
        FAIL_OUT(ret);    
    }
    
    ndrx_keyval_hash_free(out);
    ndrx_inicfg_free(ndrx_get_G_cconfig());
    
out:
    NDRX_LOG(log_info, "Test returns %d", ret);
    return ret;
}

