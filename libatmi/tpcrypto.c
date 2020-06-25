/**
 * @brief TP level crypto functions
 *
 * @file tpcrypto.c
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
#include <errno.h>
#include <sys/sem.h>
#include <signal.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <excrypto.h>

#include <nstd_shm.h>
#include <cpm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Encrypt data block.
 * Currently AES-128 is used, CBC mode
 * In string mode (TPEX_STRING) - input is 0x0 terminated string, on output base64
 * encoded data (output buffer size is checked, but no len provided)
 * @param input input input data
 * @param ilen input string len (not used for TPEX_STRING)
 * @param output output buffer
 * @param olen output buffer len
 * @param flags TPEX_STRING - input & output data is string
 * @return EXSUCCEED/EXFAIL tperror loaded
 */
expublic int tpencrypt_int(char *input, long ilen, char *output, long *olen, long flags)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_debug, "%s - flags=%lx", __func__, flags);
    
    if (flags & TPEX_STRING)
    {
        if (EXSUCCEED!=ndrx_crypto_enc_string(input, output, olen))
        {
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        if (EXSUCCEED!=ndrx_crypto_enc(input, ilen, output, olen))
        {
            EXFAIL_OUT(ret);
        }
    }
    
out:
    if (EXSUCCEED!=ret)   
    {
        /* map the errors... */
        ndrx_TPset_error_nstd();
    }
    
    NDRX_LOG(log_debug, "%s return %d", __func__, ret);

    return ret;
}

/**
 * Decrypt data block.
 * Currently AES-128 is used, CBC mode
 * In string mode (TPEX_STRING) - input is 0x0 terminated string with base64 data,
 * on output 0x0 terminate string is provided. No len is provided on output
 * but output buffer size is tested.
 * @param input input data buffer, string or binary data
 * @param ilen input len (only for binary)
 * @param output output buffer
 * @param olen output buffer size, on output len provided (only for binary mode)
 * @param flags TPEX_STRING - input & output data is string
 * @return EXSUCCEED/EXFAIL
 */
expublic int tpdecrypt_int(char *input, long ilen, char *output, long *olen, long flags)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_debug, "%s - flags=%lx", __func__, flags);
    
    if (flags & TPEX_STRING)
    {
        if (EXSUCCEED!=ndrx_crypto_dec_string(input, output, olen))
        {
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        if (EXSUCCEED!=ndrx_crypto_dec(input, ilen, output, olen))
        {
            EXFAIL_OUT(ret);
        }
    }
    
out:
    if (EXSUCCEED!=ret)   
    {
        /* map the errors... */
        ndrx_TPset_error_nstd();
    }

    NDRX_LOG(log_debug, "%s return %d", __func__, ret);
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
