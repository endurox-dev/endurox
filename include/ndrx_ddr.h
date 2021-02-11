/**
 * @brief Common/shared data structures between server & client.
 *
 * @file ndrxdcmn.h
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
#ifndef NDRX_DDR_H
#define	NDRX_DDR_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <ndrstandard.h>
#include <atmi.h>
#include <exhash.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define NDRX_DDR_FLAG_MIN            0x00000001  /**< This is min value      */
#define NDRX_DDR_FLAG_MAX            0x00000002  /**< This is max value      */
#define NDRX_DDR_FLAG_DEFAULT_VAL    0x00000004  /**< This is default value  */
#define NDRX_DDR_FLAG_DEFAULT_GRP    0x00000008  /**< This is default group  */

/* for sparc we set to 8 */
#define DDR_DEFAULT_ALIGN       EX_ALIGNMENT_BYTES

/**
 * Generic data alignment to default system ALGIN setting
 * @param DSIZE data on which to calc alignment
 */
#define DDR_ALIGNED_GEN(DSIZE) \
    ((DSIZE + DDR_DEFAULT_ALIGN -1 ) / DDR_DEFAULT_ALIGN * DDR_DEFAULT_ALIGN)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Routing criterion sequence contains the range
 * Positions to fields must have aligned access
 */
typedef struct
{
    char grp[NDRX_DDR_GRP_MAX+1];   /**< needs to assign a group code too    */
    double lowerd;                  /**< range is number lower part (double) */
    double upperd;                  /**< range is number upper part (double) */
    long lowerl;                    /**< range is number lower part (long)   */
    long upperl;                    /**< range is number upper part (long)   */
    int len;                        /**< total lenght of the field           */
    int flags;                      /**< routing flags MIN, MAX, DFLT        */
    int strrange_upper;             /**< Offset to upper block               */
    char strrange[1];               /**< string ranges (have space for eof debug)*/
} ndrx_routcritseq_t;

/**
 * Structure to keep the list of extended FDs.
 * NOTE ! size of the block is dynamic as ndrx_routcritseq_t
 * has unknown size at the start...
 */
typedef struct ndrx_routcritseq_dl ndrx_routcritseq_dl_t;
struct ndrx_routcritseq_dl
{
    ndrx_routcritseq_dl_t *prev, *next;
    ndrx_routcritseq_t cseq;    /**< Criterion sequence         */
};

/**
 * Routing criterion
 * These might be several in the row if buffer types does not match.
 * 
 * So SHM would look like:
 * 
 * [ndrx_routcrit_t ndrx_routcritseq_t..N] .. [ndrx_routcrit_t ndrx_routcritseq_t..N]
 * 
 * Positions to fields must have aligned access
 * 
 */
typedef struct
{
    char criterion[NDRX_DDR_CRITMAX+1]; /**< criterion code, for debug      */
    int criterionid;            /**< criterion id                           */
    int len;                    /**< total len of the field including sequences */
    char field[MAXTIDENT+1];    /**< routing field id                       */
    short buffer_type_id;       /**< XATMI buffer type code                 */
    char buftype[256+1];        /**< Buffer type as defined                 */
    char fieldtype[16];         /**< Field type override, mandatory for json*/
    int  fieldtypeid;           /**< Type code of the field                 */
    int  routetype;             /**< Format by which we route the data      */
    BFLDID fldid;               /**< resolved field id for UBF              */
    long rangesnr;              /**< number of ranges follows               */
    char ranges[0];             /**< range offset of the ndrx_routcritseq_t */
} ndrx_routcrit_t;

/**
 * This is hash of the buffer types
 * Only one buffer type + subtype is allowed
 */
typedef struct
{
    char buftype[256+1];        /**< Buffer type, this is key for route crit has*/
    char *ranges;               /**< string dup of ranges                       */
    ndrx_routcrit_t routcrit;   /**< actula criterion data being hashed         */
    ndrx_routcritseq_dl_t *seq; /**< DL of criterion sequences                  */
    EX_hash_handle hh;          /**< Hash handle                                */
} ndrx_routcrit_typehash_t;

/**
 * Hash entry of criterions used
 * during config building
 */
typedef struct
{
    char criterion[NDRX_DDR_CRITMAX+1]; /**< criterion code                     */
    int criterionid;                    /**< store resolved id                  */
    long offset;                        /**< offset in shm where cirtion starts */    
    ndrx_routcrit_typehash_t *btypes;   /**< buffer types hash                  */
    EX_hash_handle hh;                  /**< Hash handle                        */
} ndrx_routcrit_hash_t;


/**
 * Routing information in shared memory
 */
typedef struct
{
    char svcnm[MAXTIDENT+1];    /**< service name in linear hash */
    int prio;                   /**< default call priority       */
    int cirterionid;            /**< criterion id                */
    char criterion[NDRX_DDR_CRITMAX+1]; /**< criterion code, is cirterion used ?*/
    long offset;                /**< memory offset where criterion id starts int cirt mem */
    int autotran;               /**< is autotran used?           */
    unsigned long trantime;     /**< transaction timeout time    */
    long flags;                 /**< this is used by linear hash */
} ndrx_services_t;

/**
 * This is hash handler using during build of configuration
 */
typedef struct
{
    char svcnm[MAXTIDENT+1];    /**< service name in linear hash */
    ndrx_services_t routsvc;     /**< actula data                 */
    EX_hash_handle hh;          /**< Hash handle                 */
} ndrx_services_hash_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_ddr_services_put(ndrx_services_t *svc, char *mem, long memmax);

/** tpcalls shall route to this one... */
extern NDRX_API int ndrx_ddr_grp_get(char *svcnm, size_t svcnmsz, char *data, long len,
        int *prio);
extern NDRX_API int ndrx_ddr_service_get(char *svcnm, int *autotran, unsigned long *trantime);

#ifdef	__cplusplus
}
#endif

#endif	/* NDRXDCMN_H */

/* vim: set ts=4 sw=4 et smartindent: */
