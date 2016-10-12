/* 
** ATMI Object API header (auto-generated)
**
** @file oatmi.h
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
#ifndef __OATMI_H
#define __OATMI_H

#ifdef  __cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <stdint.h>
#include <ubf.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
export NDRX_API int Otpacall(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags);
export NDRX_API char * Otpalloc(TPCONTEXT_T *p_ctxt, char *type, char *subtype, long size);
export NDRX_API int Otpcall(TPCONTEXT_T *p_ctxt, char *svc, char *idata, long ilen, char **odata, long *olen, long flags);
export NDRX_API int Otpcancel(TPCONTEXT_T *p_ctxt, int cd);
export NDRX_API int Otpconnect(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags);
export NDRX_API int Otpdiscon(TPCONTEXT_T *p_ctxt, int cd);
export NDRX_API void Otpfree(TPCONTEXT_T *p_ctxt, char *ptr);
export NDRX_API int Otpgetrply(TPCONTEXT_T *p_ctxt, int *cd, char **data, long *len, long flags);
export NDRX_API char * Otprealloc(TPCONTEXT_T *p_ctxt, char *ptr, long size);
export NDRX_API int Otprecv(TPCONTEXT_T *p_ctxt, int cd, char **data, long *len, long flags, long *revent);
export NDRX_API int Otpsend(TPCONTEXT_T *p_ctxt, int cd, char *data, long len, long flags, long *revent);
export NDRX_API long Otptypes(TPCONTEXT_T *p_ctxt, char *ptr, char *type, char *subtype);
export NDRX_API int Otpabort(TPCONTEXT_T *p_ctxt, long flags);
export NDRX_API int Otpbegin(TPCONTEXT_T *p_ctxt, unsigned long timeout, long flags);
export NDRX_API int Otpcommit(TPCONTEXT_T *p_ctxt, long flags);
export NDRX_API int Otpconvert(TPCONTEXT_T *p_ctxt, char *strrep, char *binrep, long flags);
export NDRX_API int Otpsuspend(TPCONTEXT_T *p_ctxt, TPTRANID *tranid, long flags);
export NDRX_API int Otpresume(TPCONTEXT_T *p_ctxt, TPTRANID *tranid, long flags);
export NDRX_API char * Otpstrerror(TPCONTEXT_T *p_ctxt, int err);
export NDRX_API long Otpsubscribe(TPCONTEXT_T *p_ctxt, char *eventexpr, char *filter, TPEVCTL *ctl, long flags);
export NDRX_API int Otpunsubscribe(TPCONTEXT_T *p_ctxt, long subscription, long flags);
export NDRX_API int Otppost(TPCONTEXT_T *p_ctxt, char *eventname, char *data, long len, long flags);
export NDRX_API int Otpinit(TPCONTEXT_T *p_ctxt, TPINIT *tpinfo);
export NDRX_API int Otpjsontoubf(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *buffer);
export NDRX_API int Otpubftojson(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *buffer, int bufsize);
export NDRX_API int Otpenqueue(TPCONTEXT_T *p_ctxt, char *qspace, char *qname, TPQCTL *ctl, char *data, long len, long flags);
export NDRX_API int Otpdequeue(TPCONTEXT_T *p_ctxt, char *qspace, char *qname, TPQCTL *ctl, char **data, long *len, long flags);
export NDRX_API int Otpenqueueex(TPCONTEXT_T *p_ctxt, short nodeid, short srvid, char *qname, TPQCTL *ctl, char *data, long len, long flags);
export NDRX_API int Otpdequeueex(TPCONTEXT_T *p_ctxt, short nodeid, short srvid, char *qname, TPQCTL *ctl, char **data, long *len, long flags);
export NDRX_API int Otpgetctxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T *context, long flags);
export NDRX_API int Otpsetctxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T context, long flags);
export NDRX_API void Otpfreectxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T context);
export NDRX_API int Otplogsetreqfile(TPCONTEXT_T *p_ctxt, char **data, char *filename, char *filesvc);
export NDRX_API int Otploggetbufreqfile(TPCONTEXT_T *p_ctxt, char *data, char *filename, int bufsize);
export NDRX_API int Otplogdelbufreqfile(TPCONTEXT_T *p_ctxt, char *data);
export NDRX_API void Otplogprintubf(TPCONTEXT_T *p_ctxt, int lev, char *title, UBFH *p_ub);
#endif  /* __OATMI_H */

