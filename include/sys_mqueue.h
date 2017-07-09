/* 
** Message queue abstractions
**
** @file sys_mqueue.h
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
#ifndef _SYS_MQUEUE_H
#define	_SYS_MQUEUE_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>

#ifdef EX_USE_EMQ

/* use queue emulation: */
#include <sys_emqueue.h>

#else

#include <mqueue.h>

#endif

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#if 1==EX_USE_EMQ

#define  ndrx_mq_timedreceive emq_timedreceive
#define  ndrx_mq_timedsend    emq_timedsend
#define  ndrx_mq_close        emq_close
#define  ndrx_mq_getattr      emq_getattr
#define  ndrx_mq_notify       emq_notify
#define  ndrx_mq_receive      emq_receive
#define  ndrx_mq_send         emq_send
#define  ndrx_mq_setattr      emq_setattr
	
#elif 1==EX_OS_SUNOS
	
#define  ndrx_mq_timedreceive sol_timedreceive
#define  ndrx_mq_timedsend    sol_timedsend
#define  ndrx_mq_close        sol_close
#define  ndrx_mq_getattr      sol_getattr
#define  ndrx_mq_notify       sol_notify
#define  ndrx_mq_receive      sol_receive
#define  ndrx_mq_send         sol_send
#define  ndrx_mq_setattr      sol_setattr	

#else

#define  ndrx_mq_timedreceive mq_timedreceive
#define  ndrx_mq_timedsend    mq_timedsend
#define  ndrx_mq_close        mq_close
#define  ndrx_mq_getattr      mq_getattr
#define  ndrx_mq_notify       mq_notify
#define  ndrx_mq_receive      mq_receive
#define  ndrx_mq_send         mq_send
#define  ndrx_mq_setattr      mq_setattr

#endif
    
#if USE_FS_REGISTRY

extern mqd_t ndrx_mq_open_with_registry(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
extern int ndrx_mq_unlink_with_registry (const char *name);

#define  ndrx_mq_open         ndrx_mq_open_with_registry
#define  ndrx_mq_unlink       ndrx_mq_unlink_with_registry

#else
    
#ifdef EX_USE_EMQ

#define  ndrx_mq_open         emq_open
#define  ndrx_mq_unlink       emq_unlink

#else

#define  ndrx_mq_open         mq_open
#define  ndrx_mq_unlink       mq_unlink

#endif
    
    
#endif
    

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_MQUEUE_H */

