/* 
** Enduro/X common-config
**
** @file cconfig.h
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
#ifndef _CCONFIG_H
#define	_CCONFIG_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <sys/stat.h>
#include <ndrxdcmn.h>
#include <stdint.h>
#include <nstopwatch.h>
#include <exhash.h>
#include <sys_unix.h>
#include <inicfg.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_CCONFIG5 "NDRX_CCONFIG5"
#define NDRX_CCONFIG4 "NDRX_CCONFIG4"
#define NDRX_CCONFIG3 "NDRX_CCONFIG3"
#define NDRX_CCONFIG2 "NDRX_CCONFIG2"
#define NDRX_CCONFIG1 "NDRX_CCONFIG1"
#define NDRX_CCONFIG  "NDRX_CCONFIG"
    
#define NDRX_CCTAG "NDRX_CCTAG" /* common-config tag */
    
#define NDRX_CONF_SECTION_GLOBAL "@global"
#define NDRX_CONF_SECTION_DEBUG  "@debug"
#define NDRX_CONF_SECTION_QUEUE  "@queue"
#define NDRX_CONF_SECTION_CACHE  "@cache"
    
    
#define NDRX_CCTAG_MAX      32          /* max len of cctag */
    
/*
 * Command for cconfig
 */
#define NDRX_CCONFIG_CMD_GET        'g' /* get config (default) */
#define NDRX_CCONFIG_CMD_LIST       'l' /* list config */
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
extern NDRX_API int ndrx_cconfig_get(char *section, ndrx_inicfg_section_keyval_t **out);
extern NDRX_API int ndrx_cconfig_load(void);
extern NDRX_API ndrx_inicfg_t *ndrx_get_G_cconfig(void);
extern NDRX_API int ndrx_cconfig_reload(void);

/* for user: */
extern NDRX_API int ndrx_cconfig_load_general(ndrx_inicfg_t **cfg);
extern NDRX_API int ndrx_cconfig_get_cf(ndrx_inicfg_t *cfg, char *section, 
        ndrx_inicfg_section_keyval_t **out);

#ifdef	__cplusplus
}
#endif

#endif	/* _CCONFIG_H */

