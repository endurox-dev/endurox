/**
 * @brief Enduro/X System testing entry points
 *
 * @file sys_test.h
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
#ifndef SYS_TEST_H
#define	SYS_TEST_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
/*---------------------------Externs------------------------------------*/
extern NDRX_API int ndrx_G_systest_enabled;
/*---------------------------Macros-------------------------------------*/
#define NDRX_SYSTEST_ENBLD      (NDRX_UNLIKELY((0!=ndrx_G_systest_enabled)))


/** 
 * Do not commit
 * Thus leave transactions as prepared.
 * For TMSRV error will be returned and commit will fail.
 */
#define NDRX_SYSTEST_TMSCOMMIT   ",TMSNOCOMMIT,"
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int ndrx_systest_init(void);
extern NDRX_API int ndrx_systest_case(char *mode);

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_TEST_H */

/* vim: set ts=4 sw=4 et smartindent: */
