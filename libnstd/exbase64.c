/* 
** Base64 handling
**
** @file exbase64.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <sys_unix.h>
#include <exbase64.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
static char encoding_table_xa[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '_'};

static char encoding_table_normal[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

static char *decoding_table_xa = NULL;
static char *decoding_table_normal = NULL;

static int mod_table[] = {0, 2, 1};

/*---------------------------Prototypes---------------------------------*/
exprivate char * build_decoding_table(char *encoding_table);

/* private void base64_cleanup(void); */

exprivate char * b64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data, 
                    char *encoding_table);


exprivate unsigned char *b64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data,
                             char *encoding_table);

/**
 * XA Version of Base64 encode
 * @param data
 * @param input_length
 * @param output_length
 * @param encoded_data
 * @return 
 */
expublic char * ndrx_xa_base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data) 
{
    return b64_encode(data, input_length, output_length, 
            encoded_data, encoding_table_xa);
}

/**
 * XA Version of base64 decode
 * @param data
 * @param input_length
 * @param output_length
 * @param decoded_data
 * @return 
 */
expublic unsigned char *ndrx_xa_base64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data)
{
    if (decoding_table_xa == NULL)
    {
        decoding_table_xa =  build_decoding_table(encoding_table_xa);
    }
    
    return b64_decode((unsigned char *)data, input_length, output_length, 
            decoded_data, decoding_table_xa);
}


/**
 * Standard Version of Base64 encode
 * @param data
 * @param input_length
 * @param output_length
 * @param encoded_data
 * @return 
 */
char * ndrx_base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data) 
{
    return b64_encode(data, input_length, output_length, 
            encoded_data, encoding_table_normal);
}

/**
 * Standard Version of base64 decode
 * @param data
 * @param input_length
 * @param output_length
 * @param decoded_data
 * @return 
 */
unsigned char *ndrx_base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data)
{
    if (decoding_table_normal == NULL)
    {
        decoding_table_normal =  build_decoding_table(encoding_table_normal);
    }
    
    return b64_decode((unsigned char *)data, input_length, output_length, 
            decoded_data, decoding_table_normal);
}

/**
 * Encode the data
 * @param data
 * @param input_length
 * @param output_length
 * @return 
 */
exprivate char * b64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data, 
                    char *encoding_table) 
{
    int i;
    int j;
    *output_length = 4 * ((input_length + 2) / 3);

    /*
    char *encoded_data = NDRX_MALLOC(*output_length);
    if (encoded_data == NULL) return NULL;*/

    for (i = 0, j = 0; i < input_length;) 
    {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


/**
 * Decode the data
 * @param data
 * @param input_length
 * @param output_length
 * @return 
 */
exprivate unsigned char *b64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data,
                             char *decoding_table)
{

    int i;
    int j;

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    /*
    unsigned char *decoded_data = NDRX_MALLOC(*output_length);
    if (decoded_data == NULL) return NULL;
*/
    for (i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return (unsigned char *)decoded_data;
}

/**
 * Build encoding table
 */
exprivate char * build_decoding_table(char *encoding_table)
{
    int i;
    char *ptr = NDRX_MALLOC(256);

    for (i = 0; i < 64; i++)
        ptr[(unsigned char) encoding_table[i]] = i;
    
    return ptr;
}

#if 0
/**
 * Cleanup table
 */
exprivate void base64_cleanup(void)
{
    NDRX_FREE(decoding_table);
}
#endif

