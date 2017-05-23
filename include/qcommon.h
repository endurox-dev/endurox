/* 
** Persistent queue commons
**
** @file nstdutil.h
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
#ifndef QCOMMON_H
#define	QCOMMON_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <stdint.h>
#include <ubf.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define TMQ_DEFAULT_BUFSZ               1024 /* default buffer size     */
    
/* Commands loaded into EX_QCMD: */
#define TMQ_CMD_ENQUEUE         'E'      /* Enqueue                     */
#define TMQ_CMD_DEQUEUE         'D'      /* Dequeue                     */
#define TMQ_CMD_NOTIFY          'N'      /* Notify tmq for XA completion*/
#define TMQ_CMD_MQLQ            'P'       /* Print queue                */
#define TMQ_CMD_MQLC            'C'       /* List configuration of q    */
#define TMQ_CMD_MQLM            'M'       /* List messages              */
#define TMQ_CMD_MQRC            'R'       /* Reload config              */
#define TMQ_CMD_MQCH            'H'       /* Change Q  config (runtime) */
    
#define TMQ_QDEF_MAX            512      /* max buffer size for Q def   */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
extern NDRX_API int tmq_tpqctl_to_ubf_enqreq(UBFH *p_ub, TPQCTL *ctl);
extern NDRX_API int tmq_tpqctl_from_ubf_enqreq(UBFH *p_ub, TPQCTL *ctl);

extern NDRX_API int tmq_tpqctl_to_ubf_enqrsp(UBFH *p_ub, TPQCTL *ctl);
extern NDRX_API int tmq_tpqctl_from_ubf_enqrsp(UBFH *p_ub, TPQCTL *ctl);

extern NDRX_API int tmq_tpqctl_to_ubf_deqreq(UBFH *p_ub, TPQCTL *ctl);
extern NDRX_API int tmq_tpqctl_from_ubf_deqreq(UBFH *p_ub, TPQCTL *ctl);

extern NDRX_API int tmq_tpqctl_to_ubf_deqrsp(UBFH *p_ub, TPQCTL *ctl);
extern NDRX_API int tmq_tpqctl_from_ubf_deqrsp(UBFH *p_ub, TPQCTL *ctl);

extern NDRX_API char * tmq_msgid_serialize(char *msgid_in, char *msgid_str_out);
extern NDRX_API char * tmq_msgid_deserialize(char *msgid_str_in, char *msgid_out);

/* API: */
extern NDRX_API int _tpenqueue (char *qspace, short nodeid, short srvid, char *qname, TPQCTL *ctl, 
        char *data, long len, long flags);

extern NDRX_API int _tpdequeue (char *qspace, short nodeid, short srvid, char *qname, TPQCTL *ctl, 
        char **data, long *len, long flags);
#ifdef	__cplusplus
}
#endif

#endif	/* QCOMMON_H */

