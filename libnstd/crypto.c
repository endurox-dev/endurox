/* 
** Cryptography related routines
** We have a poor logging options where, because logger might not be initialized
** when we will require to log some messages. Thus critical things will be logged
** to the userlog();
** The first byte of data will indicate the length of padding used.
** We also need to introduce standard library error handling there. This is needed
** for fact that we might want to give some explanation why decryption failed at
** at the application boot.
** @file crypto.c
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
#include <arpa/inet.h>
#include <errno.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <sys_unix.h>
#include <exsha1.h>
#include <excrypto.h>
#include <userlog.h>
#include <expluginbase.h>
#include <exaes.h>
#include <exbase64.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define IV_INIT {   0xab, 0xcc, 0x1b, 0xc2, \
                    0x3d, 0xe4, 0x44, 0x11, \
                    0x30, 0x54, 0x34, 0x09, \
                    0xef, 0xaf, 0xfc, 0xf5 \
                }
      
/* #define CRYPTODEBUG */

/* #define CRYPTODEBUG_DUMP */

#define CRYPTO_LEN_PFX_BYTES    4

#define API_ENTRY {_Nunset_error();}

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Get encryption key.
 * This is used in expluginbase.c for default crypto function.
 * @param key_out
 * @param klen
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_crypto_getkey_std(char *key_out, int key_out_bufsz)
{
    int ret = EXSUCCEED;
    long len;
    API_ENTRY;

    /* TODO Add some caching over the crypto key! */

    if (EXSUCCEED!=ndrx_sys_get_hostname(key_out, key_out_bufsz))
    {
        _Nset_error_fmt(NEUNIX, "Failed to get hostname!");
        EXFAIL_OUT(ret);
    }
    
    len = strlen(key_out);
    
    
    if (len+1 /*incl EOS*/ < key_out_bufsz)
    {
        snprintf(key_out+len, key_out_bufsz - len, "%s", 
                ndrx_sys_get_cur_username());
    }
    
#ifdef CRYPTODEBUG
    NDRX_LOG_EARLY(log_debug, "Password built: [%s]", key_out);
#endif
    
out:
    return ret;
}

/**
 * Get final key
 * @param sha1key, buffer size NDRX_ENCKEY_BUFSZ
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_get_final_key(char *sha1key)
{
    int ret = EXSUCCEED;
    char password[PATH_MAX+1];
    
    /* first we take password & hash it */
    
    if (ndrx_G_plugins.p_ndrx_crypto_getkey(password, sizeof(password)))
    {
        userlog("Failed to get encryption key by plugin "
                "function, provider: [%s]", 
                ndrx_G_plugins.ndrx_crypto_getkey_provider);
        
        _Nset_error_fmt(NEPLUGIN, "Failed to get encryption key by plugin "
                "function, provider: [%s]", 
                ndrx_G_plugins.ndrx_crypto_getkey_provider);
        
        EXFAIL_OUT(ret);
    }
    
#ifdef CRYPTODEBUG
    NDRX_LOG_EARLY(log_debug, "Clear password: [%s]", password);
#endif
    
    /* make sha1 */
    EXSHA1( sha1key, password, strlen(password) );
    
#ifdef CRYPTODEBUG_DUMP
    NDRX_DUMP(log_debug, "SHA1 key", sha1key, NDRX_ENCKEY_BUFSZ);
#endif
    
out:
    return ret;
}

/**
 * Decrypt data block (internal version, no API entry)
 * @param input input data block
 * @param ibufsz input data block buffer size
 * @param output encrypted data block
 * @param obufsz encrypted data block buffer size
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_crypto_enc_int(char *input, long ilen, char *output, long *olen)
{
    int ret = EXSUCCEED;
    char sha1key[NDRX_ENCKEY_BUFSZ];
    long size_estim;
    uint32_t *len_ind = (uint32_t *)output;
    uint8_t  iv[]  = IV_INIT;
    
    /* encrypt data block */
    
    if (EXSUCCEED!=ndrx_get_final_key(sha1key))
    {
        EXFAIL_OUT(ret);
    }
    
    /* estimate the encrypted data len round to */
    size_estim = 
            ((ilen + NDRX_ENC_BLOCK_SIZE - 1) / NDRX_ENC_BLOCK_SIZE) 
            * NDRX_ENC_BLOCK_SIZE  
            + CRYPTO_LEN_PFX_BYTES;
    
#ifdef CRYPTODEBUG
    NDRX_LOG_EARLY(log_debug, "%s: Data size: %ld, estimated: %ld, output buffer: %ld",
            __func__, ilen, size_estim, *olen);
#endif
#ifdef CRYPTODEBUG_DUMP
    NDRX_DUMP(log_debug, "About to encrypt: ", input, ilen);
#endif
    if (size_estim > *olen)
    {
        userlog("Encryption output buffer to short, estimated: %ld, but on input: %ld",
                size_estim, *olen);
        
        _Nset_error_fmt(NENOSPACE, "Encryption output buffer to short, "
                "estimated: %ld, but on input: %ld",
                size_estim, *olen);
        
        EXFAIL_OUT(ret);
    }
    
    /* so data len will not be encrypted */
    *len_ind = htonl((uint32_t)ilen);
    
    EXAES_CBC_encrypt_buffer((uint8_t*)(output+CRYPTO_LEN_PFX_BYTES), 
            (uint8_t*)input, ilen, (const uint8_t*)sha1key, (const uint8_t*) iv);
    
    
    *olen = size_estim;
    
#ifdef CRYPTODEBUG_DUMP
    
    /* DUMP the data block */
    NDRX_DUMP(log_debug, "Encrypted data block", output, *olen);
    
#endif
    
out:
    return ret;
}

/**
 * Decrypt data block (API entry function)
 * @param input input data block
 * @param ibufsz input data block buffer size
 * @param output encrypted data block
 * @param obufsz encrypted data block buffer size
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_crypto_enc(char *input, long ilen, char *output, long *olen)
{
    API_ENTRY;
    return ndrx_crypto_enc(input, ilen, output, olen);
}

/**
 * Decrypt data block (internal, no API entry)
 * @param input input buffer
 * @param ilen input len
 * @param output output buffer
 * @param olen on input indicates the buffer length on output, output data len
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_crypto_dec_int(char *input, long ilen, char *output, long *olen)
{
    int ret = EXSUCCEED;
    char sha1key[NDRX_ENCKEY_BUFSZ];
    uint32_t *len_ind = (uint32_t *)input;
    uint8_t  iv[]  = IV_INIT;
    long data_size = ntohl(*len_ind);
    
    /* encrypt data block */
    
    if (EXSUCCEED!=ndrx_get_final_key(sha1key))
    {
        EXFAIL_OUT(ret);
    }
    
#ifdef CRYPTODEBUG_DUMP
    NDRX_DUMP(log_debug, "About to decrypt (incl 4 byte len): ", input, ilen);
#endif
    
#ifdef CRYPTODEBUG
    NDRX_LOG_EARLY(log_debug, "Data size: %ld, %ld, output buffer: %ld",
            data_size, olen);
#endif

    if (data_size > *olen)
    {
        userlog("Decryption output buffer too short, data: %ld, output buffer: %ld",
                data_size, *olen);
        
        _Nset_error_fmt(NENOSPACE, "Decryption output buffer too short, "
                "data: %ld, output buffer: %ld",
                data_size, *olen);
        
        EXFAIL_OUT(ret);
    }
    *olen = data_size;
    EXAES_CBC_decrypt_buffer((uint8_t*)(output), 
            (uint8_t*)(input+CRYPTO_LEN_PFX_BYTES), ilen-CRYPTO_LEN_PFX_BYTES, 
            (const uint8_t*)sha1key, (const uint8_t*) iv);
    
    /* DUMP the data block */
    
#ifdef CRYPTODEBUG_DUMP
    NDRX_DUMP(log_debug, "Decrypted data block", output, *olen);
#endif
    
out:
    return ret;
}

/**
 * Decrypt data block, API entry
 * @param input input buffer
 * @param ilen input len
 * @param output output buffer
 * @param olen on input indicates the buffer length on output, output data len
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_crypto_dec(char *input, long ilen, char *output, long *olen)
{
    API_ENTRY;
    return ndrx_crypto_dec_int(input, ilen, output, olen);
}

/**
 * Encrypt string
 * @param input input string, zero terminated
 * @param output base64 string
 * @param olen output buffer length
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_crypto_enc_string(char *input, char *output, long olen)
{
    int ret = EXSUCCEED;
    char buf[NDRX_MSGSIZEMAX];
    long bufsz = sizeof(buf);
    long estim_size;
    long inlen = strlen(input);
    size_t b64len;
    API_ENTRY;
    
    /* encrypt data block */
    if (EXSUCCEED!=ndrx_crypto_enc_int(input, inlen, buf, &bufsz))
    {
        EXFAIL_OUT(ret);
    }
    
    /* translate data block the the base64 (with size estim) */
    estim_size = NDRX_BASE64_SIZE(bufsz) + 1;
    if (NDRX_BASE64_SIZE(bufsz) +1 /* for EOS */ > olen)
    {
        userlog("Output buffer too short. Required for "
                "base64 %ld bytes, but got %ld", estim_size, olen);
#ifdef CRYPTODEBUG
        NDRX_LOG_EARLY(log_error, "Output buffer too short. Required for "
                "base64 %ld bytes, but got %ld", estim_size, olen);
#endif
        
        _Nset_error_fmt(NENOSPACE, "Output buffer too short. Required for "
                "base64 %ld bytes, but got %ld",
              estim_size, olen);
        EXFAIL_OUT(ret);
    }
    
    /* encode to base64... */
    
    ndrx_base64_encode((unsigned char *)buf, bufsz, &b64len, output);
    
    output[b64len] = EXEOS;
    
#ifdef CRYPTODEBUG
    
    NDRX_LOG_EARLY(log_debug, "%s: input: [%s]", __func__, input);
    NDRX_LOG_EARLY(log_debug, "%s: output: [%s]", __func__, output);
    
#endif
    
out:
    return ret;
}

/**
 * Decrypt the base64 string
 * @param input base64 containing encrypted data
 * @param output clear text output storage
 * @param olen output buffer size
 */
expublic int ndrx_crypto_dec_string(char *input, char *output, long olen)
{
    int ret = EXSUCCEED;
    /*char buf[NDRX_MSGSIZEMAX];  have to allocate buffer dynamically -> NDRX_MSGSIZEMAX 
     * might not be known at init stage (reading ini)  */
    long len = strlen(input);
    char *buf = NULL;
    size_t bufsz = len;
    uint32_t *len_ind;
    long data_size;
    
    API_ENTRY;
    
    if (NULL==(buf = NDRX_MALLOC(bufsz)))
    {
        int err = errno;
        
        NDRX_LOG_EARLY(log_error, "%s: Failed to allocate %ld bytes: %s",
                __func__, len, strerror(err));
        userlog("%s: Failed to allocate %ld bytes: %s",
                __func__, len, strerror(err));
        
        _Nset_error_fmt(NESYSTEM, "%s: Failed to allocate %ld bytes: %s",
                __func__, len, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    len_ind = (uint32_t *)buf;
    
#ifdef CRYPTODEBUG
    NDRX_LOG_EARLY(log_debug, "%s, output buf %p, olen=%ld input len: %ld", 
            __func__, output, olen, len);
    NDRX_LOG_EARLY(log_debug, "%s: About to decrypt (b64): [%s]", __func__, input);
#endif

    /* convert base64 to bin */
    if (NULL==ndrx_base64_decode(input, len, &bufsz, buf))
    {
        _Nset_error_fmt(NEFORMAT, "%s, ndrx_base64_decode failed (input len: %ld", 
                __func__, len);
        NDRX_LOG_EARLY(log_error, "%s, ndrx_base64_decode failed (input len: %ld", 
                __func__, len);
        userlog("%s, ndrx_base64_decode failed (input len: %ld", 
                __func__, len);
        EXFAIL_OUT(ret);
    }

#ifdef CRYPTODEBUG_DUMP
    NDRX_DUMP(log_debug, "Binary data to decrypt: ", buf, bufsz);
#endif
    
    /* Check the output buffer len */
    data_size = ntohl(*len_ind);
    
    if (data_size +1 > olen)
    {
        userlog("String decryption output buffer too short, data: %ld, "
                "output buffer: %ld", data_size, olen);
        
        _Nset_error_fmt(NENOSPACE, "String decryption output buffer too short, "
                "data: %ld, output buffer: %ld",
                data_size, olen);
        
        EXFAIL_OUT(ret);
    }
    
    /* decrypt the data to the output buffer */
    
    if (EXSUCCEED!=ndrx_crypto_dec_int(buf, bufsz, output, &olen))
    {
        #ifdef CRYPTODEBUG
        NDRX_LOG_EARLY(log_error, "%s: Failed to decrypt [%s]!", __func__, input);
        #endif
        userlog("%s: Failed to decrypt [%s]!", __func__, input);
    }
    
    output[olen] = EXEOS;
    
    /* check that data does not contain binary zero
     * because we operate with 0x00 strings, thus if zero is found, data
     * is not decrypted.
     */
    if ((len=strlen(output)) != olen)
    {
        userlog("Found EOS at %ld. Output data len %ld", len, olen);
        
        _Nset_error_fmt(NEINVALKEY, "Found EOS at %ld. Output data len %ld", 
                len, olen);
        
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=buf)
    {
        NDRX_FREE(buf);
    }

    return ret;
}
