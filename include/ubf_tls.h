/**
 * @brief UBF library TLS storage
 *
 * @file ubf_tls.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#ifndef UBF_TLS_H
#define	UBF_TLS_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <pthread.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <ferror.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define UBF_TLS_MAGIG          0x150519be
#define UBF_TLS_ENTRY  if (NDRX_UNLIKELY(NULL==G_ubf_tls)) \
        {G_ubf_tls=(ubf_tls_t *)ndrx_ubf_tls_new(EXTRUE, EXTRUE);};
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * UBF Library TLS
 */
typedef struct
{
    int magic; /* have some magic for context data */
    
    /* bprint_impl.c */
    Bnext_state_t bprint_state;
    
    /* fdatatype.c */
    short tbuf_s; /* =0; */
    long tbuf_l;/*=0; */
    int tbuf_i;/*=0; */
    char tbuf_c;/*=0; */
    float tbuf_f; /*=0.0; */
    double tbuf_d; /*=0.0; */
    
    char *str_buf_ptr;/* = NULL; */
    int str_dat_len;/* = 0;*/
     
    char *carray_buf_ptr;/*  = NULL; */
    int carray_dat_len;/* = 0; */
    
    char M_ubf_error_msg_buf[MAX_ERROR_LEN+1];/* = {EOS}; */
    int M_ubf_error;/* = BMINVAL; */
    
    char errbuf[MAX_ERROR_LEN+1];
    
    /* fieldtable.c */
    char bfname_buf[64];
    
    /* ubf.c */
    Bnext_state_t bnext_state;
    
    int is_auto; /* is this auto-allocated (thus do the auto-free) */
    /* we should have lock inside */
    pthread_mutex_t mutex; /* initialize later with PTHREAD_MUTEX_INITIALIZER */
    
} ubf_tls_t;

/*---------------------------Globals------------------------------------*/
extern NDRX_API __thread ubf_tls_t *G_ubf_tls; /* Enduro/X standard library TLS */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
#ifdef	__cplusplus
}
#endif

#endif	/* UBF_CONTEXT_H */

/* vim: set ts=4 sw=4 et smartindent: */
