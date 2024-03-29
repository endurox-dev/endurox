/**
 * @brief Simple static main library
 *
 * @file rawmain.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
extern int ndrx_main(int argc, char** argv);
extern int tpsvrinit(int argc, char **argv);
extern void tpsvrdone(void);
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/** server init call        */
expublic int (*G_tpsvrinit__)(int, char **) = tpsvrinit;

/** system call for server init */
expublic int (*ndrx_G_tpsvrinit_sys)(int, char **) = NULL;

/** call for server done    */
expublic void (*G_tpsvrdone__)(void) = tpsvrdone;

/** Server init func        */
expublic int (*ndrx_G_tpsvrthrinit)(int, char **) = NULL;

/** thread server done func */
expublic void (*ndrx_G_tpsvrthrdone)(void) = NULL;
  
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Forward the call to NDRX
 */
int main(int argc, char** argv)
{

    return ndrx_main(argc, argv);
}

/* vim: set ts=4 sw=4 et smartindent: */
