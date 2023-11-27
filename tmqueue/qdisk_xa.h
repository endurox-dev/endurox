/**
 * @brief Disk (Storage) related header, used by XA driver only
 *
 * @file qdisk_xa.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

#ifndef QDISK_XA_H
#define	QDISK_XA_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include "qtran.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * List active records.
 * in buffer transaction id is returned (tmxid_<seq>)
 */
#define NDRX_TMQ_STORAGE_LIST_MODE_ACTIVE       1

/**
 * List prepared records.
 * in buffer transaction id is returned (tmxid_<seq>)
 */
#define NDRX_TMQ_STORAGE_LIST_MODE_PREPARED     2 

/**
 * List committed records
 * In buffer unique message id is returned.
 */
#define NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED    3


/** store interface magic */
#define NDRX_TMQ_STOREIF_MAGIC          "TMQS"
#define NDRX_TMQ_STOREIF_MAGIC_LEN       4

/** Switch version. Must match TMS */
#define NDRX_TMQ_STOREIF_VERSION             1

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Module configuration (for xa disk)
 */
typedef struct
{
    /* Passed from tmqueue main entry point: */
    int setting;
    int (*pf_tmq_setup_cmdheader_dum)(tmq_cmdheader_t *hdr, char *qname,
                                     short nodeid, short srvid, char *qspace, long flags);
    int (*pf_tmq_dum_add)(char *tmxid);
    int (*pf_tmq_unlock_msg)(union tmq_upd_block *b);
    /** returned from tmq_set_tmqueue(): */
    void (*pf_tmq_chkdisk_th)(void *ptr, int *p_finish_off);
    int (*pf_tmq_msgid_exists)(char *msgid_str);
    void (*pf_tpexit)(void);

    /* path to data storage (e.g. directory for files, for SQL ref to connstr): */
    char data_folder[PATH_MAX+1]; /**< Where to store the q data         */

} ndrx_tmq_qdisk_xa_cfg_t;

typedef struct ndrx_tmq_storage ndrx_tmq_storage_t;

/**
 * Data store interface for transactional message queue
 */
struct ndrx_tmq_storage
{
    char magic[4+1];
    char name[16+1];
    int sw_version;

    void *custom_block1;
    void *custom_block2;
    void *custom_block3;
    void *custom_block4;
    ndrx_tmq_storage_t *cfg; /**< ptr to config, may be optionally set by init... */

    /** init interface
     * @param sw storage interface
     * @param p_tmq_cfg Configuration used by queue 
     * @return 0 on success, -1 on error (Nerror is set)
     */
    int (*pf_storage_init)(ndrx_tmq_storage_t *sw, ndrx_tmq_qdisk_xa_cfg_t *p_tmq_cfg);

    /**
     * un-init the interface
     * @param sw switch
     * @return 0 on success, -1 on error (Nerror is set)
     */
    int (*pf_storage_uninit)(ndrx_tmq_storage_t *sw);

    /**
     * List message/transaction store.
     * @param sw storage interface
     * @param mode mode of the list (see NDRX_TMQ_STORAGE_LIST_MODE_* constants)
     * @return pointer to the cursor or NULL on error
     */
    void * (*pf_storage_list_start)(ndrx_tmq_storage_t *sw, int mode); 

    /**
     * List messages in the store
     * @param sw storage interface
     * @param cursor cursor returned by pf_storage_list_start
     * @param ref buffer to store the tmxid or unique message id
     * @param refsz size of the buffer (see NDRX_TMQ_STORAGE_LIST_MODE_* constants)
     * @return EXSUCCEED/EXFAIL
     */
    int (*pf_storage_list_next)(ndrx_tmq_storage_t *sw, void *cursor, char *ref, size_t refsz); 

    /**
     * End the scanning of the store
     * @param sw storage interface
     * @param cursor cursor returned by pf_storage_list_start
     * @return EXSUCCEED/EXFAIL
     */
    int (*pf_storage_list_end)(ndrx_tmq_storage_t *sw, void *cursor); 

    /**
     * Verify that transaction exists in the store in preparad state
     * @param sw storage interface
     * @param tmxid transaction id
     * @return EXTRUE/EXFALSE/EXFAIL
     */
    int (*pf_storage_prep_exists)(ndrx_tmq_storage_t *sw, char *tmxid); 

    /**
     * Read block from the store
     * @param sw storage interface
     * @param ref reference to the block (msgid id or transaction id)
     * @param mode mode of the list (see NDRX_TMQ_STORAGE_LIST_MODE_* constants)
     * @param buf buffer to store the data
     * @param bufsz size of the buffer
     * @return >=0 (number of bytes read) or EXFAIL
     */
    int (*pf_storage_read_block)(ndrx_tmq_storage_t *sw, char *ref, char *buf, size_t bufsz, int mode);

    /**
     * Rollback commands
     * @param sw storage interface
     * @param p_tl transaction log
     * @return EXSUCCEED/EXFAIL
     */
    int (*pf_storage_rollback_cmds)(ndrx_tmq_storage_t *sw, qtran_log_t * p_tl);

    /**
     * Commit commands
     * @param sw storage interface
     * @param p_tl transaction log
     * @return EXSUCCEED/EXFAIL
     */
    int (*pf_storage_commit_cmds)(ndrx_tmq_storage_t *sw, qtran_log_t * p_tl);

    /**
     * Prepare commands
     * @param sw storage interface
     * @param p_tl transaction log
     * @return EXSUCCEED/EXFAIL
     */
    int (*pf_storage_prepare_cmds)(ndrx_tmq_storage_t *sw, qtran_log_t * p_tl);

    /**
     * Write block to the store.
     * This includes: new msg, delete msg, update counters
     * @param sw storage interface
     * @param block block to write
     * @param len length of the block
     * @param cust_tmxid custom transaction id (NULL if not used)
     * @param int_diag internal diagnostic (NULL if not used)
     * @return EXSUCCEED/EXFAIL
     */
    int (*pf_storage_write_block)(char *block, int len, char *cust_tmxid, int *int_diag);

};

/*---------------------------Globals------------------------------------*/

extern ndrx_tmq_storage_t ndrx_G_tmq_store_files;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern int ndrx_xa_qminicall(char *tmxid, char cmd);
extern int ndrx_xa_qminiconnect(char cmd, UBFH **pp_ub, long flags);
extern int xa_recover_entry_tmq(int cd, UBFH *p_ub, long flags);


#ifdef	__cplusplus
}
#endif

#endif	/* QDISK_XA_H */

/* vim: set ts=4 sw=4 et smartindent: */
