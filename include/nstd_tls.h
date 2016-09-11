/* 
** Enduro/X Standard library thread local storage
**
** @file nstd_tls.h
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
#ifndef NSTD_TLS_H
#define	NSTD_TLS_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <pthread.h>
#include <ndrstandard.h>
#include <nerror.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define ERROR_BUFFER_POLL            1024
#define NSTD_CONTEXT_MAGIG          0xa27f0f24
    
    
#define NSTD_TLS_ENTRY  if (NULL==G_nstd_tls) {nstd_tls_new(TRUE, TRUE);};
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * ATMI error handler save/restore
 */
typedef struct
{
    int magic; /* have some magic for context data */
    
    /* ndebug.c */
    long M_threadnr; /* Current thread nr */
    
    /* nerror.c */
    char M_nstd_error_msg_buf[MAX_ERROR_LEN+1];
    int M_nstd_error;
    char errbuf[MAX_ERROR_LEN+1];
    
    /* nstdutil.c */
    char util_buf1[20][20];
    char util_buf2[20][20];
    char util_text[20][128];
    
    /* sys_emqueue.c */
    char emq_x[512];
    
    /* sys_poll.c: */
    int M_last_err;
    char M_last_err_msg[1024];  /* Last error message */
    char poll_strerr[ERROR_BUFFER_POLL];
    
    /* we should have lock inside */
    pthread_mutex_t mutex; /* initialize later with PTHREAD_MUTEX_INITIALIZER */
    
} nstd_tls_t;

/*---------------------------Globals------------------------------------*/
extern NDRX_API __thread nstd_tls_t *G_nstd_tls; /* Enduro/X standard library TLS */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

public NDRX_API nstd_tls_t * nstd_tls_get(void);
public NDRX_API void nstd_tls_set(nstd_tls_t *tls);
public NDRX_API int nstd_tls_free(nstd_tls_t *tls);
public NDRX_API nstd_tls_t * nstd_tls_new(int auto_destroy, int auto_set);
    
#ifdef	__cplusplus
}
#endif

#endif	/* NSTD_CONTEXT_H */

