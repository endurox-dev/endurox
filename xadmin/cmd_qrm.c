/**
 * @brief Remove all matched queues
 *
 * @file cmd_qrm.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * AGPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <nclopt.h>
#include <sys_unix.h>
#include <utlist.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Remove specific q
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_qrm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    int i;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    
    if (argc>=2)
    {
        if (NULL!=(qlist = ndrx_sys_mqueue_list_make(ndrx_get_G_atmi_env()->qpath, &ret)))
        {
            LL_FOREACH(qlist,elt)
            {
                for (i=1; i<argc; i++)
                {
                    if (0==strcmp(elt->qname, argv[i]))
                    {
                        printf("Removing [%s] ...", elt->qname);

                        if (EXSUCCEED==ndrx_mq_unlink(elt->qname))
                        {
                            printf("SUCCEED\n");
                        }
                        else
                        {
                            printf("FAIL\n");
                        }
                    }
                }
            }
        }
    }
    else
    {
        EXFAIL_OUT(ret);
    }
    
out:

    ndrx_string_list_free(qlist);
    return ret;
}


/**
 * Remove all queues
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_qrmall(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    int i;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    
    if (argc>=2)
    {
        if (NULL!=(qlist = ndrx_sys_mqueue_list_make(ndrx_get_G_atmi_env()->qpath, &ret)))
        {
            LL_FOREACH(qlist,elt)
            {
                for (i=1; i<argc; i++)
                {
                    if (NULL!=strstr(elt->qname, argv[i]))
                    {
                        printf("Removing [%s] ...", elt->qname);


                        if (EXSUCCEED==ndrx_mq_unlink(elt->qname))
                        {
                            printf("SUCCEED\n");
                        }
                        else
                        {
                            printf("FAIL\n");
                        }
                    }
                }
            }
        }
    }
    else
    {
        EXFAIL_OUT(ret);
    }
    
out:

    ndrx_string_list_free(qlist);
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
