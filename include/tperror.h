/* 
** Enduro/X ATMI Error handling header
**
** @file tperror.h
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
#ifndef TPERROR_H
#define	TPERROR_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <regex.h>
#include <ndrstandard.h>
#include <ubf_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_TP_ERROR_LEN   1024
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * ATMI error handler save/restore
 */
typedef struct
{
    char atmi_error_msg_buf[MAX_TP_ERROR_LEN+1];
    int atmi_error;
    short atmi_reason;
} atmi_error_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API void ndrx_TPset_error(int error_code);
extern NDRX_API void ndrx_TPset_error_msg(int error_code, char *msg);
extern NDRX_API void ndrx_TPset_error_fmt(int error_code, const char *fmt, ...);
extern NDRX_API void ndrx_TPset_error_fmt_rsn(int error_code, short reason, const char *fmt, ...);
extern NDRX_API void ndrx_TPunset_error(void);
extern NDRX_API int ndrx_TPis_error(void);
extern NDRX_API void ndrx_TPappend_error_msg(char *msg);
extern NDRX_API void ndrx_TPoverride_code(int error_code);

extern NDRX_API void ndrx_TPsave_error(atmi_error_t *p_err);
extern NDRX_API void ndrx_TPrestore_error(atmi_error_t *p_err);

/* xa error handling */
extern NDRX_API void atmi_xa_set_error(UBFH *p_ub, short error_code, short reason);
extern NDRX_API void atmi_xa_set_error_msg(UBFH *p_ub, short error_code, short reason, char *msg);
extern NDRX_API void atmi_xa_set_error_fmt(UBFH *p_ub, short error_code, short reason, const char *fmt, ...);
extern NDRX_API void atmi_xa_override_error(UBFH *p_ub, short error_code);
extern NDRX_API void atmi_xa_unset_error(UBFH *p_ub);
extern NDRX_API int atmi_xa_is_error(UBFH *p_ub);
extern NDRX_API void atmi_xa_error_msg(UBFH *p_ub, char *msg);
extern NDRX_API void atmi_xa2tperr(UBFH *p_ub);
extern NDRX_API char *atmi_xa_geterrstr(int code);
extern NDRX_API void atmi_xa_approve(UBFH *p_ub);
extern NDRX_API short atmi_xa_get_reason(void);
#ifdef	__cplusplus
}
#endif

#endif	/* TPERROR_H */

