/**
 * @brief GPG-ME based message encryption routines
 *
 * @file gpgme_encrypt.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */

#ifndef __GPGME_ENCRYPT_H
#define __GPGME_ENCRYPT_H

/*---------------------------Includes-----------------------------------*/
#include "exhash.h"
#include <gpgme.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define GPGA_ERR_MIN			0	/* minimal error 		*/
#define GPGA_ERR_NO_ERR			0	/* no error			*/
#define GPGA_ERR_INVALID_PARAM		1	/* invalid paramter top func	*/
#define GPGA_ERR_GPG_ME_ERR		2	/* gpg-me generated error	*/
#define GPGA_ERR_INVALID_RCPT		3	/* invalid recipient		*/
#define GPGA_ERR_DATA_SEEK		4	/* error in data seek		*/
#define GPGA_ERR_DATA_READ		5	/* error in data read		*/
#define GPGA_ERR_NOMEM			6	/* Malloc failed		*/
#define GPGA_ERR_VERIFICATION		7	/* Signature verification failed*/
#define GPGA_ERR_MAX			7
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
struct rcpt_hash {
    char rcpt[33];             /* key (string is WITHIN the structure) 	*/
    gpgme_key_t recipients[2];
    EX_hash_handle hh;         /* makes this structure hashable 	*/
};
typedef struct rcpt_hash rcpt_hash_t;


struct signer_hash {
    char signer[33];             /* key (string is WITHIN the structure) 	*/
    gpgme_key_t signers[2];
    EX_hash_handle hh;         /* makes this structure hashable 	*/
};
typedef struct signer_hash signer_hash_t;


typedef struct
{
	gpgme_error_t error;
	gpgme_engine_info_t info;
	gpgme_ctx_t context;
	/* we should create lots of receipients and store them in cache!
	 * could use ut hash for storing rcpt name -> index id hash
	 */
	rcpt_hash_t *rcpt_tab;
	gpgme_user_id_t user;
	gpgme_key_t *p_recipient;
	
	signer_hash_t *signer_tab;
	gpgme_key_t *p_signer; /* we will sign messages with this key */
	gpgme_user_id_t siguser;
	
	void (*log_callback)(int lev, char *msg);
	int use_signing;
} pgpgme_enc_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int gpga_aerrno(void);
extern int gpga_gerrno(void);
extern char * gpga_strerr(int aerrno, int gerrno);
extern int pgpa_init(pgpgme_enc_t *p_enc,
		void (*log_callback)(int lev, char *msg), int use_signing);
extern int pgpa_set_recipient(pgpgme_enc_t *p_enc, char *rcpt);
extern int pgpa_set_signer(pgpgme_enc_t *p_enc, char *signer);
extern int pgpa_encrypt(pgpgme_enc_t *p_enc, char *in_data_clr, int in_data_len_clr, 
			char *out_data_p_enc, int *out_data_p_enc_len);
extern int pgpa_decrypt(pgpgme_enc_t *p_enc, char *in_data_p_enc, int in_data_len_p_enc, 
		char *out_data_clr, int *out_data_len_clr);
/* __GPGME_ENCRYPT_H*/
#endif 
/* vim: set ts=4 sw=4 et smartindent: */
