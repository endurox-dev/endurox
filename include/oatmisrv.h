/* 
** ATMI Server level Object API header (auto-generated)
**
** @file oatmisrv.h
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
#ifndef __OATMISRV_H
#define __OATMISRV_H

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
export NDRX_API int Otpadvertise_full(TPCONTEXT_T *p_ctxt, char *svc_nm, void (*p_func)(TPSVCINFO *), char *fn_nm);
export NDRX_API void Otpreturn(TPCONTEXT_T *p_ctxt, int rval, long rcode, char *data, long len, long flags);
export NDRX_API int Otpunadvertise(TPCONTEXT_T *p_ctxt, char *svcname);
export NDRX_API void Otpforward(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags);
export NDRX_API int Otpsrvsetctxdata(TPCONTEXT_T *p_ctxt, char *data, long flags);
export NDRX_API int Otpext_addpollerfd(TPCONTEXT_T *p_ctxt, int fd, uint32_t events, void *ptr1, int (*p_pollevent)(int fd, uint32_t events, void *ptr1));
export NDRX_API int Otpext_delpollerfd(TPCONTEXT_T *p_ctxt, int fd);
export NDRX_API int Otpext_addperiodcb(TPCONTEXT_T *p_ctxt, int secs, int (*p_periodcb)(void));
export NDRX_API int Otpext_addb4pollcb(TPCONTEXT_T *p_ctxt, int (*p_b4pollcb)(void));
#endif  /* __OATMISRV_H */

