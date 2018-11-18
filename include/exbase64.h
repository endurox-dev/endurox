/**
 * @brief Base64 handling
 *
 * @file exbase64.h
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
#ifndef EXBASE64_H
#define EXBASE64_H


#if defined(__cplusplus)
extern "C" {
#endif

    
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
/*---------------------------Externs------------------------------------*/
    
#define NDRX_BASE64_SIZE(X) (((X+2)/3)*4)

/*---------------------------Macros-------------------------------------*/    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API char * ndrx_base64_encode(unsigned char *data, size_t input_length, 
        size_t *output_length, char *encoded_data);

extern NDRX_API unsigned char *ndrx_base64_decode(const char *data, size_t input_length, 
        size_t *output_length, char *decoded_data);
        
/* Base64 encode/decode with file system valid output */
extern NDRX_API char * ndrx_xa_base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data);
    
extern NDRX_API unsigned char *ndrx_xa_base64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data);

#if defined(__cplusplus)
}
#endif


#endif
/* vim: set ts=4 sw=4 et smartindent: */
