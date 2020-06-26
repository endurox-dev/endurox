/**
 * @brief Base64 handling
 *
 * @file exbase64.c
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
static char M_encoding_table_xa[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '_'};

static char M_encoding_table_normal[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

/* built by build_decoding_table(M_encoding_table_xa); */
static char M_decoding_table_xa[] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x3f,
0x00, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* built by build_decoding_table(M_encoding_table_normal); */
static char M_decoding_table_normal[] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x3f,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int mod_table[] = {0, 2, 1};

/*---------------------------Prototypes---------------------------------*/
exprivate char * build_decoding_table(char *encoding_table);

/* private void base64_cleanup(void); */

exprivate char * ndrx_b64encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data, 
                    char *encoding_table);


exprivate unsigned char *ndrx_b64decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data,
                             char *encoding_table);

/**
 * XA Version of Base64 encode
 * EOS is installed in output data
 * @param data
 * @param input_length
 * @param output_length includes EOS byte
 * @param encoded_data
 * @return 
 */
expublic char * ndrx_xa_base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data)
{
    return ndrx_b64encode(data, input_length, output_length, 
            encoded_data, M_encoding_table_xa);
}

/**
 * XA Version of base64 decode
 * @param data input data
 * @param input_length input len
 * @param output_length output data len (ptr to). Only output len, no len checking.
 * @param decoded_data decoded output data
 * @return 
 */
expublic unsigned char *ndrx_xa_base64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data)
{
    return ndrx_b64decode((unsigned char *)data, input_length, output_length, 
            decoded_data, (char *)M_decoding_table_xa);
}


/**
 * Standard Version of Base64 encode
 * EOS is installed in output data
 * @param data
 * @param input_length
 * @param output_length includes EOS byte
 * @param encoded_data
 * @return 
 */
char * ndrx_base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data) 
{
    return ndrx_b64encode(data, input_length, output_length, 
            encoded_data, M_encoding_table_normal);
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
    return ndrx_b64decode((unsigned char *)data, input_length, output_length, 
            decoded_data, (char *)M_decoding_table_normal);
}

/**
 * Encode the data
 * Output string contains EOS
 * @param data
 * @param input_length
 * @param output_length includes the EOS
 * @return 
 */
exprivate char * ndrx_b64encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data, 
                    char *encoding_table) 
{
    int i;
    int j;
    size_t tmp_len;
    
    /* Ensure position for EOS */
    tmp_len = 4 * ((input_length + 2) / 3);
    
    if (*output_length > 0 && *output_length < (tmp_len+1) /*  +1 for EOS */ )
    {
        NDRX_LOG(log_error, "Failed to encode data len incl EOS %d but buffer sz %d",
                (int)(tmp_len+1), (int)*output_length);
        return NULL;
    }
    
    *output_length = tmp_len;

    /*
    char *encoded_data = NDRX_MALLOC(*output_length);
    if (encoded_data == NULL) return NULL;*/
    
    /* TODO: Check output buffer size!!!! */

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

    encoded_data[*output_length] = EXEOS;
    
    /* we wrote to output including EOS.. */
    (*output_length)++;
    
    return encoded_data;
}


/**
 * Decode the data
 * @param data
 * @param input_length
 * @param output_length
 * @return 
 */
exprivate unsigned char *ndrx_b64decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data,
                             char *decoding_table)
{

    int i;
    int j;
    size_t tmp_len;

    if (input_length % 4 != 0) 
    {
        NDRX_LOG(log_error, "Invalid input_length: %d!", input_length);
        return NULL;
    }
    
    if (input_length <= 0)
    {
        NDRX_LOG(log_error, "Invalid input length %d <= 0!", input_length);
        return NULL;
    }

    tmp_len = input_length / 4 * 3;
    
    /* strip off padding if any.. */
    if (data[input_length - 1] == '=') tmp_len--;
    if (data[input_length - 2] == '=') tmp_len--;
    
    if (*output_length > 0 && *output_length < tmp_len)
    {
        NDRX_LOG(log_error, "Output buffer too short: Output buffer size: %d, "
                "but data output size: %d", (int)*output_length, (int)tmp_len);
        return NULL;
    }

    *output_length = tmp_len;
    
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

#if 0
/**
 * Build encoding table
 */
exprivate char * build_decoding_table(char *encoding_table)
{
    int i;
    char *ptr = NDRX_MALLOC(256);

    memset(ptr, 0, 256);

    for (i = 0; i < 64; i++)
        ptr[(unsigned char) encoding_table[i]] = i;
    
    fprintf(stderr, "START\n");
    for (i=0; i< 256; i++)
    {
        fprintf(stderr, "0x%02x, ", ptr[i]);

        if (0==(i+1)%16)
        {
            fprintf(stderr, "\n");
        }

    }
    fprintf(stderr, "END\n");
    return ptr;
}
#endif

#if 0
/**
 * Cleanup table
 */
exprivate void base64_cleanup(void)
{
    NDRX_FREE(decoding_table);
}
#endif

/* vim: set ts=4 sw=4 et smartindent: */
