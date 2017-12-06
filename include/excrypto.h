/* 
** Cryptography related
**
** @file excrypto.h
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
#ifndef EXCRYPTO_H
#define	EXCRYPTO_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_ENCKEY_BUFSZ       21 /* string with EOS */
#define NDRX_ENC_BLOCK_SIZE     16
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_crypto_getkey_std(char *key_out, int key_out_bufsz);
extern NDRX_API int ndrx_crypto_enc(char *input, long ilen, char *output, long *olen);
extern NDRX_API int ndrx_crypto_dec_int(char *input, long ilen, char *output, long *olen);
extern NDRX_API int ndrx_crypto_dec(char *input, long ilen, char *output, long *olen);
extern NDRX_API int ndrx_crypto_enc_string(char *input, char *output, long olen);
extern NDRX_API int ndrx_crypto_dec_string(char *input, char *output, long olen);

    
#ifdef	__cplusplus
}
#endif

#endif	/* EXCRYPTO_H */

