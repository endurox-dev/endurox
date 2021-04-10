/**
 * @brief Aliasing limits, and -N processing
 *
 * @file atmisv34_23.c
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
#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include <unistd.h>
#include <thlock.h>
#include "exassert.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Standard service entry, function
 */
void T3 (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Do initialisation
 */
int tpsvrinit(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");
out:
    return ret;
}

/**
 * Do de-initialisation
 */
void tpsvrdone(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

/* Auto generated system advertise table */
expublic struct tmdsptchtbl_t ndrx_G_tmdsptchtbl[] = {
    { "", "T3", T3, 0, 0 }
    ,{ "A3AAAAA1", "T3", T3, 0, 0 }
    ,{ "A3AAAAA2", "T3", T3, 0, 0 }
    ,{ "A3AAAAA3", "T3", T3, 0, 0 }
    ,{ "A3AAAAA4", "T3", T3, 0, 0 }
    ,{ "A3AAAAA5", "T3", T3, 0, 0 }
    ,{ "A3AAAAA6", "T3", T3, 0, 0 }
    ,{ "A3AAAAA7", "T3", T3, 0, 0 }
    ,{ "A3AAAAA8", "T3", T3, 0, 0 }
    ,{ "A3AAAAA9", "T3", T3, 0, 0 }
    ,{ "A3AAAAA10", "T3", T3, 0, 0 }
    ,{ "A3AAAAA11", "T3", T3, 0, 0 }
    ,{ "A3AAAAA12", "T3", T3, 0, 0 }
    ,{ "A3AAAAA13", "T3", T3, 0, 0 }
    ,{ "A3AAAAA14", "T3", T3, 0, 0 }
    ,{ "A3AAAAA15", "T3", T3, 0, 0 }
    ,{ "A3AAAAA16", "T3", T3, 0, 0 }
    ,{ "A3AAAAA17", "T3", T3, 0, 0 }
    ,{ "A3AAAAA18", "T3", T3, 0, 0 }
    ,{ "A3AAAAA19", "T3", T3, 0, 0 }
    ,{ "A3AAAAA20", "T3", T3, 0, 0 }
    ,{ "A3AAAAA21", "T3", T3, 0, 0 }
    ,{ "A3AAAAA22", "T3", T3, 0, 0 }
    ,{ "A3AAAAA23", "T3", T3, 0, 0 }
    ,{ "A3AAAAA24", "T3", T3, 0, 0 }
    ,{ "A3AAAAA25", "T3", T3, 0, 0 }
    ,{ "A3AAAAA26", "T3", T3, 0, 0 }
    ,{ "A3AAAAA27", "T3", T3, 0, 0 }
    ,{ "A3AAAAA28", "T3", T3, 0, 0 }
    ,{ "A3AAAAA29", "T3", T3, 0, 0 }
    ,{ "A3AAAAA30", "T3", T3, 0, 0 }
    ,{ "A3AAAAA31", "T3", T3, 0, 0 }
    ,{ "A3AAAAA32", "T3", T3, 0, 0 }
    ,{ "A3AAAAA33", "T3", T3, 0, 0 }
    ,{ "A3AAAAA34", "T3", T3, 0, 0 }
    ,{ "A3AAAAA35", "T3", T3, 0, 0 }
    ,{ "A3AAAAA36", "T3", T3, 0, 0 }
    ,{ "A3AAAAA37", "T3", T3, 0, 0 }
    ,{ "A3AAAAA38", "T3", T3, 0, 0 }
    ,{ "A3AAAAA39", "T3", T3, 0, 0 }
    ,{ "A3AAAAA40", "T3", T3, 0, 0 }
    ,{ "A3AAAAA41", "T3", T3, 0, 0 }
    ,{ "A3AAAAA42", "T3", T3, 0, 0 }
    ,{ "A3AAAAA43", "T3", T3, 0, 0 }
    ,{ "A3AAAAA44", "T3", T3, 0, 0 }
    ,{ "A3AAAAA45", "T3", T3, 0, 0 }
    ,{ "A3AAAAA46", "T3", T3, 0, 0 }
    ,{ "A3AAAAA47", "T3", T3, 0, 0 }
    ,{ "A3AAAAA48", "T3", T3, 0, 0 }
    ,{ "A3AAAAA49", "T3", T3, 0, 0 }
    ,{ "A3AAAAA50", "T3", T3, 0, 0 }
    ,{ "A3AAAAA51", "T3", T3, 0, 0 }
    ,{ "A3AAAAA52", "T3", T3, 0, 0 }
    ,{ "A3AAAAA53", "T3", T3, 0, 0 }
    ,{ "A3AAAAA54", "T3", T3, 0, 0 }
    ,{ "A3AAAAA55", "T3", T3, 0, 0 }
    ,{ "A3AAAAA56", "T3", T3, 0, 0 }
    ,{ "A3AAAAA57", "T3", T3, 0, 0 }
    ,{ "A3AAAAA58", "T3", T3, 0, 0 }
    ,{ "A3AAAAA59", "T3", T3, 0, 0 }
    ,{ "A3AAAAA60", "T3", T3, 0, 0 }
    ,{ "A3AAAAA61", "T3", T3, 0, 0 }
    ,{ "A3AAAAA62", "T3", T3, 0, 0 }
    ,{ "A3AAAAA63", "T3", T3, 0, 0 }
    ,{ "A3AAAAA64", "T3", T3, 0, 0 }
    ,{ "A3AAAAA65", "T3", T3, 0, 0 }
    ,{ "A3AAAAA66", "T3", T3, 0, 0 }
    ,{ "A3AAAAA67", "T3", T3, 0, 0 }
    ,{ "A3AAAAA68", "T3", T3, 0, 0 }
    ,{ "A3AAAAA69", "T3", T3, 0, 0 }
    ,{ "A3AAAAA70", "T3", T3, 0, 0 }
    ,{ "A3AAAAA71", "T3", T3, 0, 0 }
    ,{ "A3AAAAA72", "T3", T3, 0, 0 }
    ,{ "A3AAAAA73", "T3", T3, 0, 0 }
    ,{ "A3AAAAA74", "T3", T3, 0, 0 }
    ,{ "A3AAAAA75", "T3", T3, 0, 0 }
    ,{ "A3AAAAA76", "T3", T3, 0, 0 }
    ,{ "A3AAAAA77", "T3", T3, 0, 0 }
    ,{ "A3AAAAA78", "T3", T3, 0, 0 }
    ,{ "A3AAAAA79", "T3", T3, 0, 0 }
    ,{ "A3AAAAA80", "T3", T3, 0, 0 }
    ,{ "A3AAAAA81", "T3", T3, 0, 0 }
    ,{ "A3AAAAA82", "T3", T3, 0, 0 }
    ,{ "A3AAAAA83", "T3", T3, 0, 0 }
    ,{ "A3AAAAA84", "T3", T3, 0, 0 }
    ,{ "A3AAAAA85", "T3", T3, 0, 0 }
    ,{ "A3AAAAA86", "T3", T3, 0, 0 }
    ,{ "A3AAAAA87", "T3", T3, 0, 0 }
    ,{ "A3AAAAA88", "T3", T3, 0, 0 }
    ,{ "A3AAAAA89", "T3", T3, 0, 0 }
    ,{ "A3AAAAA90", "T3", T3, 0, 0 }
    ,{ "A3AAAAA91", "T3", T3, 0, 0 }
    ,{ "A3AAAAA92", "T3", T3, 0, 0 }
    ,{ "A3AAAAA93", "T3", T3, 0, 0 }
    ,{ "A3AAAAA94", "T3", T3, 0, 0 }
    ,{ "A3AAAAA95", "T3", T3, 0, 0 }
    ,{ "A3AAAAA96", "T3", T3, 0, 0 }
    ,{ "A3AAAAA97", "T3", T3, 0, 0 }
    ,{ "A3AAAAA98", "T3", T3, 0, 0 }
    ,{ "A3AAAAA99", "T3", T3, 0, 0 }
    ,{ "A3AAAAA100", "T3", T3, 0, 0 }
    , { NULL, NULL, NULL, 0, 0 }
};
/*---------------------------Prototypes---------------------------------*/

/**
 * Main entry for tmsrv
 */
int main( int argc, char** argv )
{
    _tmbuilt_with_thread_option=EXTRUE;
    struct tmsvrargs_t tmsvrargs =
    {
        &tmnull_switch,
        &ndrx_G_tmdsptchtbl[0],
        0,
        tpsvrinit,
        tpsvrdone,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        tpsvrthrinit,
        tpsvrthrdone
    };
    
    return( _tmstartserver( argc, argv, &tmsvrargs ));
    
}


/* vim: set ts=4 sw=4 et smartindent: */
