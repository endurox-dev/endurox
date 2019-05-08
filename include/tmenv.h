/**
 * @brief Added for compatiblity
 *
 * @file tmenv.h
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

#ifndef TMENV_H
#define	TMENV_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define _(X)  X
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * Integration mode Context based storage
 */
struct ndrx_ctx_priv
{
    void *integptr1; /**< integration pointer 1, private */
    void *integptr2; /**< integration pointer 2, private */
    void *integptr3; /**< integration pointer 3, private */
    long integlng4;  /**< Integration storage 4          */
};
typedef struct ndrx_ctx_priv ndrx_ctx_priv_t;

/**
 * Integration mode Environment based storage
 */
struct ndrx_env_priv
{
    long integlng0;  /**< Integration storage 0          */
    void *integptr1; /**< integration pointer 1, private */
    void *integptr2; /**< integration pointer 2, private */
    void *integptr3; /**< integration pointer 3, private */
};
typedef struct ndrx_env_priv ndrx_env_priv_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* Additional contexting - integration related */

/* Data from TLS Context */
extern NDRX_API ndrx_ctx_priv_t* ndrx_ctx_priv_get(void);

/* Data from environment */
extern NDRX_API ndrx_env_priv_t* ndrx_env_priv_get(void);

/* XA Driver settings... */
extern NDRX_API void ndrx_xa_noapisusp(int val);
extern NDRX_API void ndrx_xa_nojoin(int val);
extern NDRX_API void ndrx_xa_nostartxid(int val);

#ifdef	__cplusplus
}
#endif

#endif	/* TMENV_H */

/* vim: set ts=4 sw=4 et smartindent: */
