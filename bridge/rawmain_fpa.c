/**
 * @brief Integration mode
 *
 * @file rawmain_integra.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
extern void NDRX_INTEGRA(tpsvrdone)(void);
extern int NDRX_INTEGRA(tpsvrinit) (int argc, char **argv);
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Forward the call to NDRX
 */
int main(int argc, char** argv) 
{
    if (NULL==getenv(CONF_NDRX_FPAOPTS))
    {
        /* set the special mode for bridge 
         * from stats we see that process is memory hungry.
         */
        setenv(CONF_NDRX_FPAOPTS, "256:100,S:30", EXTRUE);
    }
    
    return ndrx_main_integra(argc, argv, 
            NDRX_INTEGRA(tpsvrinit), NDRX_INTEGRA(tpsvrdone), 0);
}
/* vim: set ts=4 sw=4 et smartindent: */
