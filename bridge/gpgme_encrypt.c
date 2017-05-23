/* 
** GPG-ME based message encryption routines
**
** @file gpgme_encrypt.c
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
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <gpgme.h>
#include <stdarg.h>
#include "exhash.h"
#include <errno.h>
#include "gpgme_encrypt.h"
#include <ndebug.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* API ENTRY script */
#define GPGA_API_ENTRY	gpga_reset_err();\
			if (NULL==p_enc) {\
				gpga_set_err(GPGA_ERR_INVALID_PARAM, 0, "control structure `p_enc' is NULL!");\
				ret=FAIL; goto out;}

#define ERR_BUF	2048+1

#ifndef FAIL
#define	SUCCEED 0
#define FAIL -1
#endif

#define MAX_RCPTS			500

/* we might want to think about subkeys!
 * See reference:
 * http://balsa.sourcearchive.com/documentation/2.3.13-3/gmime-gpgme-context_8c-source.html
 */
#define KEY_IS_OK(k)   (!((k)->expired || (k)->revoked || \
                           (k)->disabled || (k)->invalid))
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
int M_aerrno;
int M_aerrno_gpg;
char M_aerrmsg[ERR_BUF];
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Loggin fucntion
 * @param p_enc - p_encryption context
 * @param fmt - format stirng
 * @param ... - format details
 */
static void gpga_log(pgpgme_enc_t *p_enc, int lev, const char *fmt, ...)
{
	char msg[1024] = {'\0'};
	va_list ap;

	if (p_enc->log_callback)
	{
		va_start(ap, fmt);
		(void) vsnprintf(msg, sizeof(msg), fmt, ap);
		va_end(ap);
		p_enc->log_callback(lev, msg);
	}
}

/**
 * Return API errno
 */
int gpga_aerrno(void)
{
	return M_aerrno;
}

/**
 * Return GPG Errno
 */
int gpga_gerrno(void)
{
	return M_aerrno_gpg;
}

/**
 * Return common error messa
 */
char * gpga_strerr(int aerrno, int gerrno)
{
    return M_aerrmsg;
}

/**
 * Reset the error msg
 */
static int gpga_reset_err(void)
{
    M_aerrno = 0;
    M_aerrno_gpg = 0;
    M_aerrmsg[0] = '\0';
    
    return SUCCEED;
}

/**
 * Internal api set error
 */
static void gpga_set_err(int aerrno, int gerrno, char *msg)
{
	M_aerrno = aerrno;
	
	if (GPGA_ERR_GPG_ME_ERR == aerrno)
	{
		M_aerrno_gpg = gerrno;
		strcpy(M_aerrmsg, (char *)gpgme_strerror(gerrno));
	}
	else
	{
		strcpy(M_aerrmsg, msg);
	}
}

/**
 * Init the p_encryption library
 */
int pgpa_init(pgpgme_enc_t *p_enc,
		void (*log_callback)(int lev, char *msg), int use_signing)
{
	int ret=SUCCEED;
	char error_buf[1024];
	char *fn = "pgpa_init";
	gpgme_key_t key = NULL;
	
	GPGA_API_ENTRY;
	
	memset(p_enc, 0, sizeof(pgpgme_enc_t));
	
	/* Initializes gpgme */
	gpgme_check_version (NULL);
	
	p_enc->log_callback = log_callback;
	
	/* Initialize the locale environment.  */
	setlocale (LC_ALL, "");
	gpgme_set_locale (NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
	#ifdef LC_MESSAGES
	gpgme_set_locale (NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
	#endif

	if (SUCCEED!= (ret = gpgme_new(&p_enc->context)))
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
		goto out;
	}
	
	/* Setting the output type must be done at the beginning */
	gpgme_set_armor(p_enc->context, 0);

	/* Check OpenPGP */
	if (SUCCEED!= (ret=gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP)))
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
		goto out;
	}
	
	if (SUCCEED!=(ret=gpgme_get_engine_info (&p_enc->info)))
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
		goto out;
	}
	
	while (p_enc->info && 
		p_enc->info->protocol != gpgme_get_protocol (p_enc->context)) 
	{
		p_enc->info = p_enc->info->next;
	}
	
	/* TODO: we should test there *is* a suitable protocol */
	/* some debug stuff: */
	
	gpga_log(p_enc, 5, "Engine OpenPGP %s is installed at %s", p_enc->info->version,
		p_enc->info->file_name); /* And not "path" as the documentation says */
	
	/* Initializes the context */
	if (SUCCEED!=(ret = gpgme_ctx_set_engine_info (p_enc->context, GPGME_PROTOCOL_OpenPGP, NULL,
					NULL)))
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
		goto out;
	}
	
	p_enc->use_signing = use_signing;
	gpga_log(p_enc, 4, "Using message signing = %s", 
			p_enc->use_signing?"TRUE":"FALSE");

out:
	gpga_log(p_enc, 5, "%s returns %s", fn, SUCCEED==ret?"SUCCEED":"FAIL");
	return ret;
}

/**
 * Set data sender or receiver party
 * NOTE: Possibly we will have to release the recipients?
 */
int pgpa_set_signer(pgpgme_enc_t *p_enc, char *signer)
{
	int ret=SUCCEED;
	char *fn = "pgpa_set_signer";
	GPGA_API_ENTRY;
	signer_hash_t *r = NULL;
	signer_hash_t *tmp = NULL;
	
	if (NULL==signer)
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, 0, "signer is NULL!");
		goto out;
	}
	
	EXHASH_FIND_STR( p_enc->signer_tab, signer, r);
	
	if (NULL==r || 0!=strcmp(r->signer, signer))
	{
		gpga_log(p_enc, 3, "Failed to lookup [%s] in hash", signer);
		
		if (NULL==(tmp = NDRX_CALLOC(1, sizeof(signer_hash_t))))
		{
			gpga_log(p_enc, 1, "Malloc failed!");
			
			gpga_set_err(GPGA_ERR_NOMEM, 0, strerror(errno));
			ret=FAIL;
			goto out;
		}
		
		if (SUCCEED!=(ret=gpgme_op_keylist_start(p_enc->context, signer, 0)))
		{
			gpga_set_err(GPGA_ERR_GPG_ME_ERR, 0, (char *)gpgme_strerror(ret));
			goto out;
		}
		
		/* search for a valid key */
		do
		{
			if (SUCCEED!=(ret=gpgme_op_keylist_next(p_enc->context, 
				&tmp->signers[0])))
			{
				gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, 
					     (char *)gpgme_strerror(ret));
				goto out;
			}
		} while (!KEY_IS_OK(tmp->signers[0]));
		
		if (SUCCEED!=(ret=gpgme_op_keylist_end(p_enc->context)))
		{
			gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
			goto out;
		}
		p_enc->p_signer = &tmp->signers[0];
		
		/* Add stuff from to hash */
		strcpy(tmp->signer, signer);
		EXHASH_ADD_STR( p_enc->signer_tab, signer, tmp );
        }
        else
	{
		gpga_log(p_enc, 3, "Signer [%s] in hash", signer);
		p_enc->p_signer = &r->signers[0];
	}
	
	p_enc->siguser = p_enc->p_signer[0]->uids;
	
	/* TODO: remove... */
	gpga_log(p_enc, 4, "Signing by %s <%s>", p_enc->siguser->name, p_enc->siguser->email);
	
	/* set the signing key in context? */
	
	/* clean context */
	gpgme_signers_clear(p_enc->context);
	
	/* set signer */
	gpgme_signers_add(p_enc->context, p_enc->p_signer[0]);
	
out:
	gpga_log(p_enc, 5, "%s returns %s", fn, SUCCEED==ret?"SUCCEED":"FAIL");
	return ret;
}


/**
 * Set data sender or receiver party
 * NOTE: Possibly we will have to release the recipients?
 */
int pgpa_set_recipient(pgpgme_enc_t *p_enc, char *rcpt)
{
	int ret=SUCCEED;
	char *fn = "pgpa_set_recipient";
	GPGA_API_ENTRY;
	rcpt_hash_t *r = NULL;
	rcpt_hash_t *tmp = NULL;
	
	if (NULL==rcpt)
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, 0, "rcpt is NULL!");
		goto out;
	}
	
	EXHASH_FIND_STR( p_enc->rcpt_tab, rcpt, r);
	
	if (NULL==r || 0!=strcmp(r->rcpt, rcpt))
	{
		gpga_log(p_enc, 3, "Failed to lookup [%s] in hash", rcpt);
		
		if (NULL==(tmp = NDRX_CALLOC(1, sizeof(rcpt_hash_t))))
		{
			gpga_log(p_enc, 1, "Malloc failed!");
			
			gpga_set_err(GPGA_ERR_NOMEM, ret, strerror(errno));
			ret=FAIL;
			goto out;
		}
		
		if (SUCCEED!=(ret=gpgme_op_keylist_start(p_enc->context, rcpt, 0)))
		{
			gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
			goto out;
		}
		
		do
		{
			if (SUCCEED!=(ret=gpgme_op_keylist_next(p_enc->context, 
				&tmp->recipients[0])))
			{
				gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, 
					     (char *)gpgme_strerror(ret));
				goto out;
			}
		/* Might no need for sign option? */
		} while (!KEY_IS_OK(tmp->recipients[0]));
		
		if (SUCCEED!=(ret=gpgme_op_keylist_end(p_enc->context)))
		{
			gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
			goto out;
		}
		p_enc->p_recipient = &tmp->recipients[0];
		
		/* Add stuff from to hash */
		strcpy(tmp->rcpt, rcpt);
		EXHASH_ADD_STR( p_enc->rcpt_tab, rcpt, tmp );
        }
        else
	{
		gpga_log(p_enc, 3, "Recipient [%s] in hash", rcpt);
		p_enc->p_recipient = &r->recipients[0];
	}
	
	p_enc->user = p_enc->p_recipient[0]->uids;
	
	/* TODO: remove... */
	gpga_log(p_enc, 4, "Encrypting for %s <%s>", p_enc->user->name, p_enc->user->email);
	
out:
	gpga_log(p_enc, 5, "%s returns %s", fn, SUCCEED==ret?"SUCCEED":"FAIL");
	return ret;
}

/**
 * Remove stuff after p_encryption/decryption session.
 */
int pgpa_release(pgpgme_enc_t *p_enc)
{
	int ret=SUCCEED;
	char *fn = "pgpa_release";
	GPGA_API_ENTRY;
	
	/*
	gpgme_recipients_release (p_enc->recipients);
	*/
out:
	return ret;
}


/**
 *  Encryp the data block
 * @out_data_p_enc -> zero terminated string.
 * 
 */
int pgpa_encrypt(pgpgme_enc_t *p_enc, char *in_data_clr, int in_data_len_clr, 
			char *out_data_p_enc, int *out_data_p_enc_len)
{
	int ret=SUCCEED;
	ssize_t nbytes;
	gpgme_encrypt_result_t  result;
	gpgme_data_t clear_text, p_encrypted_text;
	char error_buf[512];
	char *fn = "pgpa_encrypt";
	GPGA_API_ENTRY;
	
	/* Prepare the data buffers */
	if (SUCCEED!=(ret = gpgme_data_new_from_mem(&clear_text, 
				in_data_clr, in_data_len_clr, 0)))
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
		goto out;
	}
	
	if (SUCCEED!=(ret = gpgme_data_new(&p_encrypted_text)))
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
		goto out;
	}
	
	if (p_enc->use_signing)
	{
		if (SUCCEED!=(ret = gpgme_op_encrypt_sign(p_enc->context, 
					p_enc->p_recipient, 
					GPGME_ENCRYPT_ALWAYS_TRUST, 
					clear_text, p_encrypted_text)))
		{
			gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
			goto out;
		}
	}
	else
	{
		if (SUCCEED!=(ret = gpgme_op_encrypt(p_enc->context, 
					p_enc->p_recipient, 
					GPGME_ENCRYPT_ALWAYS_TRUST, 
					clear_text, p_encrypted_text)))
		{
			gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
			goto out;
		}
	}
	
	result = gpgme_op_encrypt_result(p_enc->context);
	if (result->invalid_recipients)
	{
		sprintf (error_buf, "Invalid recipient found: %s",
			result->invalid_recipients->fpr);
		
		gpga_set_err(GPGA_ERR_INVALID_RCPT, 0, error_buf);
		ret=FAIL;
		goto out;
	}

	nbytes = gpgme_data_seek (p_encrypted_text, 0, SEEK_SET);
	if (nbytes == -1) {
		sprintf (error_buf,  "%s:%d: Error in data seek: ",			
			__FILE__, __LINE__);
		gpga_set_err(GPGA_ERR_DATA_SEEK, 0, error_buf);
		ret=FAIL;
		goto out;
	}  
	
	nbytes = gpgme_data_read(p_encrypted_text, out_data_p_enc, *out_data_p_enc_len);
	if (nbytes == -1) {
		sprintf (error_buf,  "%s:%d: %s\n",			
			__FILE__, __LINE__, "Error in data read");
		gpga_set_err(GPGA_ERR_DATA_READ, 0, error_buf);
		ret=FAIL;
		goto out;
	}
	*out_data_p_enc_len = nbytes;
	
	/* bonus zero out there: 
	out_data_p_enc[nbytes] = '\0'; */
	
	gpga_log(p_enc, 5, "Encrypted text (%i bytes)", (int)nbytes);
	
out:
	gpgme_data_release (clear_text);
	gpgme_data_release (p_encrypted_text);
	
	gpga_log(p_enc, 5, "%s returns %s", fn, SUCCEED==ret?"SUCCEED":"FAIL");
	return ret;
}
 
/**
 *  Encryp the data block
 * @p_enc -> full context
 * @in_data_p_enc -> zero terminated string.
 * @out_data_clr -> output data
 * @out_data_len_clr -> bytes written to output. At input this indicates max buf size.
 */
int pgpa_decrypt(pgpgme_enc_t *p_enc, char *in_data_p_enc, int in_data_len_p_enc, 
		char *out_data_clr, int *out_data_len_clr)
{
	int ret=SUCCEED;
	ssize_t nbytes;
	gpgme_encrypt_result_t  result;
	gpgme_data_t clear_text, p_encrypted_text;
	char error_buf[512];
	char *fn = "pgpa_decrypt";
	/* Try to decrypt now...! */
	
	if (SUCCEED!=(ret = gpgme_data_new (&clear_text)))
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
		goto out;
	}
	
	/*
	error = gpgme_data_new (&p_encrypted_text);
	fail_if_err(error);
	*/
	
	/* create a buffer */
	if (SUCCEED!=(ret = gpgme_data_new_from_mem (&p_encrypted_text, in_data_p_enc, 
		in_data_len_p_enc, 1)))
	{
		gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
		goto out;
	}
	/* prepare memory for cleartext */
	
	/* Decrypt the stuff...! */
	if (p_enc->use_signing)
	{
		gpgme_verify_result_t result; 

		if (SUCCEED!=(ret = gpgme_op_decrypt_verify(p_enc->context, 
			p_encrypted_text, clear_text)))
		{
			gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
			goto out;
		}

		if (NULL==(result = gpgme_op_verify_result(p_enc->context)) ||
			NULL==result->signatures)
		{
			gpga_log(p_enc, 2, "Missing signature in message, "
					" does other side do signing?");
			gpga_set_err(GPGA_ERR_VERIFICATION, 0, 
					"No signature");
			ret=FAIL;
			goto out;
		}
		
		gpga_log(p_enc, 5, "Verification result %d status %d validity_reason %d", 
			result->signatures->summary, result->signatures->status,
			result->signatures->validity_reason);

		if (result->signatures->summary & GPGME_SIGSUM_VALID ||
			result->signatures->summary & GPGME_SIGSUM_GREEN ||
			0==result->signatures->summary /* mathematically ok */)
		{
			gpga_log(p_enc, 5, "Sender signature verified OK" );
		}
		else
		{
			gpga_log(p_enc, 2, "Sender signature verification failed!");
			gpga_set_err(GPGA_ERR_VERIFICATION, 0, 
					"Failed to verify sender signature");
			ret=FAIL;
			goto out;
		}
	}
	else
	{
		if (SUCCEED!=(ret = gpgme_op_decrypt(p_enc->context, 
			p_encrypted_text, clear_text)))
		{
			gpga_set_err(GPGA_ERR_GPG_ME_ERR, ret, (char *)gpgme_strerror(ret));
			goto out;
		}
	}
	
	
	nbytes = gpgme_data_seek (clear_text, 0, SEEK_SET);
	if (nbytes == -1)
	{
		sprintf (error_buf,  "%s:%d: Error in data seek: ",			
				__FILE__, __LINE__);
		gpga_set_err(GPGA_ERR_DATA_SEEK, 0, error_buf);
		ret=FAIL;
		goto out;					
	}  
	
	nbytes = gpgme_data_read( clear_text, out_data_clr, *out_data_len_clr );
	if (nbytes == -1)
	{
		sprintf (error_buf,  "%s:%d: %s\n",			
			__FILE__, __LINE__, "Error in data read");
		gpga_set_err(GPGA_ERR_DATA_READ, 0, error_buf);
		ret=FAIL;
		goto out;					
	}
	*out_data_len_clr = nbytes;

	
	/* Read the output buffer...! */
	gpga_log(p_enc, 5, "Decrypted text (%i bytes)", *out_data_len_clr);
out:
	gpgme_data_release (clear_text);
	gpgme_data_release (p_encrypted_text);
	
	gpga_log(p_enc, 4, "%s returns %s", fn, SUCCEED==ret?"SUCCEED":"FAIL");
	return ret;
	
}

/* #define TESTEST  */


#ifdef TESTEST


#define BUF_LEN			2048
/**
 * Simple logger
 */
void log_t(int lev, char *msg)
{
	fprintf(stderr, "%d:%s\n", lev, msg);
}

/**
 * Run some p_encryption test..!
 */
int main(int argc, char **argv)
{
	int ret=SUCCEED;
	pgpgme_enc_t enc;
	char enc_buffer[BUF_LEN]; /* where to store encrypted data */
	char clr_buffer[BUF_LEN]; /* where to store clr data */
	int enc_data_len;
	int crl_data_len;
	char test_data[100] = "HELLO WORLD!";
	int test_data_len;
	int i;
	
	memset(enc_buffer, 0, sizeof(enc_buffer));
	
	if (SUCCEED!=pgpa_init(&enc, log_t, 1))
	{
		fprintf(stderr, "fail: apierr=%d gpg_meerr=%d: %s ", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		log_t(1, "Failed on pgpa_init()!");
		goto out;
	}
	
	if (SUCCEED!=pgpa_set_signer(&enc, "testuser"))
	{
		fprintf(stderr, "fail: apierr=%d gpg_meerr=%d: %s ", 
			gpga_aerrno(), gpga_gerrno(), 
			gpga_strerr(gpga_aerrno(), gpga_gerrno()));
		log_t(1, "Failed on pgpa_set_signer()!");
		goto out;
	}
	
	for (i = 0; i< 1000; i++)
	{
		sprintf(test_data, "TEST NO %d HELLO WORLD!", i);
	
		enc_data_len = BUF_LEN;
		
		test_data_len = strlen(test_data) + 1;
		
		/* Hm we need kind of receipient cache! */
		if (SUCCEED!=pgpa_set_recipient(&enc, "testuser"))
		{
			fprintf(stderr, "fail: apierr=%d gpg_meerr=%d: %s\n", 
				gpga_aerrno(), gpga_gerrno(), 
				gpga_strerr(gpga_aerrno(), gpga_gerrno()));
			
			log_t(1, "Failed on pgpa_set_recipient()!");
			goto out;
		}
			
		if (SUCCEED!=pgpa_encrypt(&enc, test_data, test_data_len, 
				enc_buffer, &enc_data_len))
		{
			fprintf(stderr, "fail: apierr=%d gpg_meerr=%d: %s\n", 
				gpga_aerrno(), gpga_gerrno(), 
				gpga_strerr(gpga_aerrno(), gpga_gerrno()));
			log_t(1, "Failed on pgpa_encrypt()!");
			goto out;
		}
		
		if (SUCCEED!=pgpa_set_recipient(&enc, "testuser"))
		{
			fprintf(stderr, "fail: apierr=%d gpg_meerr=%d: %s\n", 
				gpga_aerrno(), gpga_gerrno(), 
				gpga_strerr(gpga_aerrno(), gpga_gerrno()));
			log_t(1, "Failed on pgpa_set_recipient()!");
			goto out;
		}
		
		crl_data_len = BUF_LEN;
		
		if (SUCCEED!=pgpa_decrypt(&enc, enc_buffer, enc_data_len,
			clr_buffer, &crl_data_len))
		{
			fprintf(stderr, "fail: apierr=%d gpg_meerr=%d: %s\n", 
				gpga_aerrno(), gpga_gerrno(), 
				gpga_strerr(gpga_aerrno(), gpga_gerrno()));
			log_t(1, "Failed on pgpa_decrypt()!");
			goto out;
		}
		
		fprintf(stderr, "Got decrypted data: [%s]\n", clr_buffer);
		
		if (crl_data_len!=test_data_len)
		{
			fprintf(stderr, "Error! Dec len %d org len %d\n", 
				crl_data_len, test_data_len);
			ret=FAIL;
			goto out;
		}
		
		if (0!=strcmp(clr_buffer, test_data))
		{
			fprintf(stderr, "Error! Dec data [%s] org data [%s]\n", 
				clr_buffer, test_data);
			ret=FAIL;
			goto out;
		}
	}
out:
	return ret;
}


#endif
