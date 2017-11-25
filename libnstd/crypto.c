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

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <sys_unix.h>
#include <exsha1.h>
#include <excrypto.h>
#include <userlog.h>
#include <expluginbase.h>
#include <exaes.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define IV_INIT { 0xab, 0xcc, 0x1b, 0xc2, 0x3d, 0xe4, 0x44, 0x11, 0x30, 0x54, 0x34, 0x09, 0xef, 0xaf, 0xfc, 0xf5 }
      
#define CRYPTODEBUG
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/* TODO: Have ptr to crypto func... (to have a hook) */

/*---------------------------Prototypes---------------------------------*/


/**
 * Get encryption key.
 * This is used in expluginbase.c for default crypto function.
 * @param key_out
 * @param klen
 * @return 
 */
expublic int ndrx_crypto_getkey_std(char *key_out, long key_out_bufsz)
{
    int ret = EXSUCCEED;
    long len;

    if (EXSUCCEED!=ndrx_sys_get_hostname(key_out, key_out_bufsz))
    {
        EXFAIL_OUT(ret);
    }
    
    len = strlen(key_out);
    
    
    if (len+1 /*incl EOS*/ < key_out_bufsz)
    {
        snprintf(key_out+len, key_out_bufsz - len, "%s", 
                ndrx_sys_get_cur_username());
    }
    
    userlog("YOPT! password: [%s]", key_out);
    
out:
    return ret;
}

/**
 * Get final key
 * @param sha1key, buffer size NDRX_ENCKEY_LEN
 * @return 
 */
exprivate int ndrx_get_final_key(char *sha1key)
{
    int ret = EXSUCCEED;
    
    char password[PATH_MAX+1];
    
    /* first we take password & hash it */
    
    if (ndrx_G_plugins.p_ndrx_crypto_getkey(password, sizeof(password)))
    {
        userlog("Failed to get encryption key!");
        EXFAIL_OUT(ret);
    }
    
#ifdef CRYPTODEBUG
    NDRX_LOG(log_debug, "Clear password: [%s]", password);
#endif
    
    /* make sha1 */
    EXSHA1( sha1key, password, strlen(password) );
    
#ifdef CRYPTODEBUG
    NDRX_DUMP(log_debug, "SHA1 key", sha1key, sizeof(sha1key));
#endif
    
out:
    return ret;
}

/**
 * Decrypt data block
 * @param input input data block
 * @param ibufsz input data block buffer size
 * @param output encrypted data block
 * @param obufsz encrypted data block buffer size
 * @return EXSUCCED/EXFAIL
 */
expublic int ndrx_crypto_enc(char *input, long ilen, char *output, long *olen)
{
    int ret = EXSUCCEED;
    char sha1key[NDRX_ENCKEY_LEN];
    long size_estim;
    uint32_t *len_ind = (uint32_t *)output;
    uint8_t  iv[]  = IV_INIT;
    /* encrypt data block */
    
    if (EXSUCCEED!=ndrx_get_final_key(sha1key))
    {
        EXFAIL_OUT(ret);
    }
    
    /* estimate the encrypted data len */
    /* 4x bytes are for data len indicator */
    size_estim = ilen + ilen % NDRX_ENC_BLOCK_SIZE + 4;
    
#ifdef CRYPTODEBUG
    NDRX_LOG(log_debug, "Data size: %ld, estimated: %ld, output buffer: %ld",
            ilen, size_estim, *olen);
    NDRX_DUMP(log_debug, "About to encrypt: ", input, ilen);
#endif
    if (size_estim > *olen)
    {
        userlog("Encryption output buffer to short, estimated: %ld, but on input: %ld",
                size_estim, *olen);
        EXFAIL_OUT(ret);
    }
    
    /* so data len will not be encrypted */
    *len_ind = htonl((uint32_t)ilen);
    
    EXAES_CBC_encrypt_buffer((uint8_t*)(output+4), (uint8_t*)input, ilen, 
            (const uint8_t*)sha1key, (const uint8_t*) iv);
    
    /* DUMP the data block */
    
#ifdef CRYPTODEBUG
    
    NDRX_DUMP(log_debug, "Encrypt data block", output, size_estim);
    
#endif
    
out:
    return ret;
}

/**
 * Decrypt data block
 * @param input
 * @param ilen
 * @param output
 * @param olen
 * @return 
 */
expublic int ndrx_crypto_dec(char *input, long ilen, char *output, long olen)
{
    return EXFAIL;
}
