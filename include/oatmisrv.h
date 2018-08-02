/* 
 * @brief ATMI Server level Object API header (auto-generated)
 *
 * @file oatmisrv.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
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
extern NDRX_API int Otpadvertise_full(TPCONTEXT_T *p_ctxt, char *svc_nm, void (*p_func)(TPSVCINFO *), char *fn_nm);
extern NDRX_API void Otpreturn(TPCONTEXT_T *p_ctxt, int rval, long rcode, char *data, long len, long flags);
extern NDRX_API int Otpunadvertise(TPCONTEXT_T *p_ctxt, char *svcname);
extern NDRX_API void Otpforward(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags);
extern NDRX_API char * Otpsrvgetctxdata(TPCONTEXT_T *p_ctxt);
extern NDRX_API char * Otpsrvgetctxdata2(TPCONTEXT_T *p_ctxt, char *p_buf, long *p_len);
extern NDRX_API void Otpsrvfreectxdata(TPCONTEXT_T *p_ctxt, char *p_buf);
extern NDRX_API int Otpsrvsetctxdata(TPCONTEXT_T *p_ctxt, char *data, long flags);
extern NDRX_API void Otpcontinue(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otpext_addpollerfd(TPCONTEXT_T *p_ctxt, int fd, uint32_t events, void *ptr1, int (*p_pollevent)(int fd, uint32_t events, void *ptr1));
extern NDRX_API int Otpext_delpollerfd(TPCONTEXT_T *p_ctxt, int fd);
extern NDRX_API int Otpext_addperiodcb(TPCONTEXT_T *p_ctxt, int secs, int (*p_periodcb)(void));
extern NDRX_API int Otpext_delperiodcb(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otpext_addb4pollcb(TPCONTEXT_T *p_ctxt, int (*p_b4pollcb)(void));
extern NDRX_API int Otpext_delb4pollcb(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otpgetsrvid(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Ondrx_main(TPCONTEXT_T *p_ctxt, int argc, char **argv);
#endif  /* __OATMISRV_H */

