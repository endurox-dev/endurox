/* 
 * @brief ATMI Object API header (auto-generated)
 *
 * @file oatmi.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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
#define Otperrno(P_CTXT) (*O_exget_tperrno_addr(P_CTXT))
#define Otpurcode(P_CTXT) (*O_exget_tpurcode_addr(P_CTXT))
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int Otpacall(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags);
extern NDRX_API char * Otpalloc(TPCONTEXT_T *p_ctxt, char *type, char *subtype, long size);
extern NDRX_API int Otpcall(TPCONTEXT_T *p_ctxt, char *svc, char *idata, long ilen, char **odata, long *olen, long flags);
extern NDRX_API int Otpcancel(TPCONTEXT_T *p_ctxt, int cd);
extern NDRX_API int Otpconnect(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags);
extern NDRX_API int Otpdiscon(TPCONTEXT_T *p_ctxt, int cd);
extern NDRX_API void Otpfree(TPCONTEXT_T *p_ctxt, char *ptr);
extern NDRX_API int Otpisautobuf(TPCONTEXT_T *p_ctxt, char *buf);
extern NDRX_API int Otpgetrply(TPCONTEXT_T *p_ctxt, int *cd, char **data, long *len, long flags);
extern NDRX_API char * Otprealloc(TPCONTEXT_T *p_ctxt, char *ptr, long size);
extern NDRX_API int Otprecv(TPCONTEXT_T *p_ctxt, int cd, char **data, long *len, long flags, long *revent);
extern NDRX_API int Otpsend(TPCONTEXT_T *p_ctxt, int cd, char *data, long len, long flags, long *revent);
extern NDRX_API long Otptypes(TPCONTEXT_T *p_ctxt, char *ptr, char *type, char *subtype);
extern NDRX_API int Otpabort(TPCONTEXT_T *p_ctxt, long flags);
extern NDRX_API int Otpbegin(TPCONTEXT_T *p_ctxt, unsigned long timeout, long flags);
extern NDRX_API int Otpcommit(TPCONTEXT_T *p_ctxt, long flags);
extern NDRX_API int Otpconvert(TPCONTEXT_T *p_ctxt, char *str, char *bin, long flags);
extern NDRX_API int Otpsuspend(TPCONTEXT_T *p_ctxt, TPTRANID *tranid, long flags);
extern NDRX_API int Otpresume(TPCONTEXT_T *p_ctxt, TPTRANID *tranid, long flags);
extern NDRX_API int Otpopen(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otpclose(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otpgetlev(TPCONTEXT_T *p_ctxt);
extern NDRX_API char * Otpstrerror(TPCONTEXT_T *p_ctxt, int err);
extern NDRX_API char * Otpecodestr(TPCONTEXT_T *p_ctxt, int err);
extern NDRX_API long Otpgetnodeid(TPCONTEXT_T *p_ctxt);
extern NDRX_API long Otpsubscribe(TPCONTEXT_T *p_ctxt, char *eventexpr, char *filter, TPEVCTL *ctl, long flags);
extern NDRX_API int Otpunsubscribe(TPCONTEXT_T *p_ctxt, long subscription, long flags);
extern NDRX_API int Otppost(TPCONTEXT_T *p_ctxt, char *eventname, char *data, long len, long flags);
extern NDRX_API int * O_exget_tperrno_addr(TPCONTEXT_T *p_ctxt);
extern NDRX_API long * O_exget_tpurcode_addr(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otpinit(TPCONTEXT_T *p_ctxt, TPINIT *tpinfo);
extern NDRX_API int Otpchkauth(TPCONTEXT_T *p_ctxt);
extern NDRX_API void (*Otpsetunsol (TPCONTEXT_T *p_ctxt, void (*disp) (char *data, long len, long flags))) (char *data, long len, long flags);
extern NDRX_API int Otpnotify(TPCONTEXT_T *p_ctxt, CLIENTID *clientid, char *data, long len, long flags);
extern NDRX_API int Otpbroadcast(TPCONTEXT_T *p_ctxt, char *lmid, char *usrname, char *cltname, char *data,  long len, long flags);
extern NDRX_API int Otpchkunsol(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otptoutset(TPCONTEXT_T *p_ctxt, int tout);
extern NDRX_API int Otptoutget(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otpterm(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Otpjsontoubf(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *buffer);
extern NDRX_API int Otpubftojson(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *buffer, int bufsize);
extern NDRX_API int Otpviewtojson(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *buffer,  int bufsize, long flags);
extern NDRX_API char * Otpjsontoview(TPCONTEXT_T *p_ctxt, char *view, char *buffer);
extern NDRX_API int Otpenqueue(TPCONTEXT_T *p_ctxt, char *qspace, char *qname, TPQCTL *ctl, char *data, long len, long flags);
extern NDRX_API int Otpdequeue(TPCONTEXT_T *p_ctxt, char *qspace, char *qname, TPQCTL *ctl, char **data, long *len, long flags);
extern NDRX_API int Otpenqueueex(TPCONTEXT_T *p_ctxt, short nodeid, short srvid, char *qname, TPQCTL *ctl, char *data, long len, long flags);
extern NDRX_API int Otpdequeueex(TPCONTEXT_T *p_ctxt, short nodeid, short srvid, char *qname, TPQCTL *ctl, char **data, long *len, long flags);
extern NDRX_API int Otpgetctxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T *context, long flags);
extern NDRX_API int Otpsetctxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T context, long flags);
extern NDRX_API TPCONTEXT_T Otpnewctxt(TPCONTEXT_T *p_ctxt, int auto_destroy, int auto_set);
extern NDRX_API void Otpfreectxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T context);
extern NDRX_API int Otplogsetreqfile(TPCONTEXT_T *p_ctxt, char **data, char *filename, char *filesvc);
extern NDRX_API int Otploggetbufreqfile(TPCONTEXT_T *p_ctxt, char *data, char *filename, int bufsize);
extern NDRX_API int Otplogdelbufreqfile(TPCONTEXT_T *p_ctxt, char *data);
extern NDRX_API void Otplogprintubf(TPCONTEXT_T *p_ctxt, int lev, char *title, UBFH *p_ub);
extern NDRX_API void Ondrx_ndrx_tmunsolerr_handler(TPCONTEXT_T *p_ctxt, char *data, long len, long flags);
#endif  /* __OATMI_H */

