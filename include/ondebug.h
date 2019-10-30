/**
 * @brief Standard library debug routines
 *
 * @file ondebug.h
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
#ifndef __ONDEBUG_H
#define __ONDEBUG_H

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
extern NDRX_API void Otplogdumpdiff(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr1, void *ptr2, int len);
extern NDRX_API void Otplogdump(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr, int len);
extern NDRX_API void Otplog(TPCONTEXT_T *p_ctxt, int lev, char *message);
extern NDRX_API long Otplogqinfo(TPCONTEXT_T *p_ctxt, int lev, long flags);
extern NDRX_API void Otplogex(TPCONTEXT_T *p_ctxt, int lev, char *file, long line, char *message);
extern NDRX_API void Ondrxlogex(TPCONTEXT_T *p_ctxt, int lev, char *file, long line, char *message);
extern NDRX_API void Oubflogex(TPCONTEXT_T *p_ctxt, int lev, char *file, long line, char *message);
extern NDRX_API char * Otploggetiflags(TPCONTEXT_T *p_ctxt);
extern NDRX_API void Ondrxlogdumpdiff(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr1, void *ptr2, int len);
extern NDRX_API void Ondrxlogdump(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr, int len);
extern NDRX_API void Ondrxlog(TPCONTEXT_T *p_ctxt, int lev, char *message);
extern NDRX_API void Oubflogdumpdiff(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr1, void *ptr2, int len);
extern NDRX_API void Oubflogdump(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr, int len);
extern NDRX_API void Oubflog(TPCONTEXT_T *p_ctxt, int lev, char *message);
extern NDRX_API int Otploggetreqfile(TPCONTEXT_T *p_ctxt, char *filename, int bufsize);
extern NDRX_API int Otplogconfig(TPCONTEXT_T *p_ctxt, int logger, int lev, char *debug_string, char *module, char *new_file);
extern NDRX_API void Otplogclosereqfile(TPCONTEXT_T *p_ctxt);
extern NDRX_API void Otplogclosethread(TPCONTEXT_T *p_ctxt);
extern NDRX_API void Otplogsetreqfile_direct(TPCONTEXT_T *p_ctxt, char *filename);
#endif  /* __ONDEBUG_H */

/* vim: set ts=4 sw=4 et smartindent: */
