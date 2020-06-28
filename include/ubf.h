/**
 * @brief UBF API library
 *
 * @file ubf.h
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
#ifndef UBF_H
#define UBF_H

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <ndrx_config.h>
#include <exdb.h>
#include <expluginbase.h>   /**<for log printing hooking  */
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define UBF_VERSION     1
#define UBF_EXTENDED
#define MAXUBFLEN	0xffffffff		/* Maximum UBFH length */
#define BF_LENGTH        32

/* UFB field types, suggest the c data types */
    
/** Minimum data type code */
#define BFLD_MIN        0
/** 'short' C data type for UBF buffer */
#define BFLD_SHORT	0
/** 'long' C data type for UBF buffer */
#define BFLD_LONG	1
#define BFLD_CHAR	2
#define BFLD_FLOAT	3
#define BFLD_DOUBLE	4
#define BFLD_STRING	5
#define BFLD_CARRAY	6
    
#define BFLD_INT        7   /**< Used for view access                       */
#define BFLD_RFU0       8   /**< Reserved for future use                    */
#define BFLD_PTR        9   /**< pointer to a buffer                        */
#define BFLD_UBF        10  /**< embedded FML32 buffer                      */
#define BFLD_VIEW       11  /**< embedded VIEW32 buffer                     */

#define BFLD_MAX	11

/* invalid field id - returned from functions where field id not found */
#define BBADFLDID (BFLDID)0
/* define an invalid field id used for first call to Bnext */
#define BFIRSTFLDID (BFLDID)0

#define BMINVAL             0  /**< min error */
#define BERFU0              1  /**< Reserved for future use */
#define BALIGNERR           2  /**< Buffer not aligned to platform address or corrupted */
#define BNOTFLD             3  /**< Buffer not fielded / TLV formatted          */
#define BNOSPACE            4  /**< No space in buffer */
#define BNOTPRES            5  /**< Field not present */
#define BBADFLD             6  /**< Bad field ID */
#define BTYPERR             7  /**< Bad field type */
#define BEUNIX              8  /**< System error occurred */
#define BBADNAME            9  /**< Bad field name */
#define BMALLOC             10 /**< Malloc failed, out of mem? */
#define BSYNTAX             11 /**< UBF Boolean expression error or UBF bad text format */
#define BFTOPEN             12 /**< Failed to open field tables */
#define BFTSYNTAX           13 /**< Field table syntax error */
#define BEINVAL             14 /**< Invalid value */
#define BERFU1              15 /**< Reserved for future use */
#define BBADTBL             16 /**< Reserved for future use */
#define BBADVIEW            17 /**< Invalid compiled VIEW file */
#define BVFSYNTAX           18 /**< Source VIEW file syntax error */
#define BVFOPEN             19 /**< Failed to open VIEW file */
#define BBADACM             20 /**< Reserved for future use */
#define BNOCNAME            21 /**< Structure field not found for VIEW */
#define BEBADOP             22 /**< Buffer operation not supported (complex type) */
#define BMAXVAL             22 /**< max error */
    
/* Bvopt options: */
#define B_FTOS              1 /**< flag S, one way UBF->Struct */
#define B_STOF              2 /**< flag F, one way Struct->UBF */
#define B_OFF               3 /**< Zero way mapping, N */
#define B_BOTH              4 /**< both F & S */

#define BUPDATE             1   /**< Update buffer */
#define BOJOIN              2   /**< outer joing buffers, RFU */
#define BJOIN               3   /**< join buffers, RFU */
#define BCONCAT             4   /**< contact buffers */
    
/* Configuration: */
#define CONF_NDRX_UBFMAXFLDS     "NDRX_UBFMAXFLDS"

#define Berror	(*ndrx_Bget_Ferror_addr())
#define BFLDID32 BFLDID
	
#define BVACCESS_NOTNULL	0x00000001	/* return value if not NULL */

/* View settings: */    
#define NDRX_VIEW_CNAME_LEN             256  /* Max c field len in struct   */
#define NDRX_VIEW_FLAGS_LEN             16   /* Max flags                   */
#define NDRX_VIEW_NULL_LEN              2660 /* Max len of the null value   */
/*XATMI_SUBTYPE_LEN  from atmi*/
/* Same as XATMI_SUBTYPE_LEN */
#define NDRX_VIEW_NAME_LEN              33   /* Max len of view name        */
#define NDRX_VIEW_COMPFLAGS_LEN         128  /* Compiled flags len          */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
typedef int BFLDID;
typedef int BFLDLEN;
typedef int BFLDOCC;
typedef struct Ubfh UBFH;


/**
 * Handler for adding view to UBF buffer
 */
typedef struct
{ 
    unsigned int vflags;                /**< RFU flags                  */
    char vname[NDRX_VIEW_NAME_LEN+1];   /**< View field name            */ 
    char *data;                         /**< pointer to view structure  */ 
} BVIEWFLD;

/**
 * Temporary store for returned UBF value
 * thread related...
 */
typedef struct
{
    BVIEWFLD viewfld;
} ndrx_ubf_tls_bufval_t;

/**
 *  Bnext state struct 
 */
struct Bnext_state
{
    BFLDID *p_cur_bfldid;
    BFLDOCC cur_occ;
    UBFH *p_ub;
    size_t size;
    /** view data returned by Bnext */
    ndrx_ubf_tls_bufval_t vstorage;
};
typedef struct Bnext_state Bnext_state_t;

/**
 * Next state for view iteration...
 */
struct Bvnext_state
{
    void *v;
    void *vel;
};
typedef struct Bvnext_state Bvnext_state_t;

/* get_loc state info */
typedef struct
{
    BFLDID *last_checked;
} Bfld_loc_info_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int * ndrx_Bget_Ferror_addr (void);
extern NDRX_API int * _Bget_Ferror_addr (void); /* for backwards compatibility */
extern NDRX_API int Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int CBadd (UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len, int usrtype);
extern NDRX_API int CBchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len, int usrtype);
extern NDRX_API int CBget (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype);
extern NDRX_API int Bdel (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int Bproj (UBFH * p_ub, BFLDID * fldlist);
extern NDRX_API int Bprojcpy (UBFH * p_ub_dst, UBFH * p_ub_src, BFLDID * fldlist);
extern NDRX_API BFLDID Bfldid (char *fldnm);
extern NDRX_API char * Bfname (BFLDID bfldid);
extern NDRX_API int Bcpy (UBFH * p_ub_dst, UBFH * p_ub_src);
extern NDRX_API int Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len);
extern NDRX_API int Binit (UBFH * p_ub, BFLDLEN len);
extern NDRX_API int Bnext (UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len);
extern NDRX_API int Bget (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN * buflen);
extern NDRX_API char * Bboolco (char * expr);
extern NDRX_API char * Bfind (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * p_len);
extern NDRX_API int Bboolev (UBFH * p_ub, char *tree);
extern NDRX_API double Bfloatev (UBFH * p_ub, char *tree);
extern NDRX_API int Badd (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len);
extern NDRX_API int Baddfast (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len, Bfld_loc_info_t *next_fld);
extern NDRX_API char * Becodestr(int err);
extern NDRX_API void B_error (char *str);
extern NDRX_API char * Bstrerror (int err);
extern NDRX_API BFLDID Bmkfldid (int fldtype, BFLDID bfldid);
extern NDRX_API BFLDOCC Boccur (UBFH * p_ub, BFLDID bfldid);
extern NDRX_API long Bused (UBFH *p_ub);
extern NDRX_API int Bfldtype (BFLDID bfldid);
extern NDRX_API int Bdelall (UBFH *p_ub, BFLDID bfldid);
extern NDRX_API int Bdelete (UBFH *p_ub, BFLDID *fldlist);
extern NDRX_API BFLDOCC Bfldno (BFLDID bfldid);
extern NDRX_API long Bunused (UBFH *p_ub);
extern NDRX_API long Bsizeof (UBFH *p_ub);
extern NDRX_API char * Btype (BFLDID bfldid);
extern NDRX_API int Bfree (UBFH *p_ub);
extern NDRX_API UBFH * Balloc (BFLDOCC f, BFLDLEN v);
extern NDRX_API UBFH * Brealloc (UBFH *p_ub, BFLDOCC f, BFLDLEN v);
extern NDRX_API int Bupdate (UBFH *p_ub_dst, UBFH *p_ub_src);
extern NDRX_API int Bconcat (UBFH *p_ub_dst, UBFH *p_ub_src);
extern NDRX_API char * CBfind (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * len, int usrtype);
extern NDRX_API char * CBgetalloc (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, int usrtype, BFLDLEN *extralen);
extern NDRX_API BFLDOCC CBfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len, int usrtype);
extern NDRX_API BFLDOCC Bfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len);
extern NDRX_API char * Bgetalloc (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern NDRX_API char * Bfindlast (UBFH * p_ub, BFLDID bfldid,
                                    BFLDOCC *occ, BFLDLEN *len);
extern NDRX_API int Bgetlast (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC *occ, char *buf, BFLDLEN *len);
extern NDRX_API int Bprint (UBFH *p_ub);
extern NDRX_API int Bfprint (UBFH *p_ub, FILE * outf);
extern NDRX_API int Bfprintcb (UBFH *p_ub, ndrx_plugin_tplogprintubf_hook_t p_writef, void *dataptr1);
extern NDRX_API char * Btypcvt (BFLDLEN * to_len, int to_type,
                    char *from_buf, int from_type, BFLDLEN from_len);
extern NDRX_API int Bextread (UBFH * p_ub, FILE *inf);
extern NDRX_API int Bextreadcb (UBFH * p_ub, 
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern NDRX_API void Bboolpr (char * tree, FILE *outf);
extern NDRX_API void Bboolprcb (char * tree, 
        int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1);
extern NDRX_API int Bread (UBFH * p_ub, FILE * inf);
extern int Breadcb (UBFH * p_ub, 
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern NDRX_API int Bwrite (UBFH *p_ub, FILE * outf);
extern NDRX_API int Bwritecb (UBFH *p_ub, 
        long (*p_writef)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern NDRX_API void Btreefree (char *tree);
/* Callback function that can be used from expressions */
extern NDRX_API int Bboolsetcbf (char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname));
extern NDRX_API int Badds (UBFH *p_ub, BFLDID bfldid, char *buf);
extern NDRX_API int Bchgs (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf);
extern NDRX_API int Bgets (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf);
extern NDRX_API char * Bgetsa (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern NDRX_API char * Bfinds (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int Bisubf (UBFH *p_ub);
extern NDRX_API int Bindex (UBFH * p_ub, BFLDOCC occ);
extern NDRX_API BFLDOCC Bunindex (UBFH * p_ub);
extern NDRX_API long Bidxused (UBFH * p_ub);
extern NDRX_API int Brstrindex (UBFH * p_ub, BFLDOCC occ);

extern NDRX_API int Bjoin(UBFH *dest, UBFH *src);
extern NDRX_API int Bojoin(UBFH *dest, UBFH *src);

extern NDRX_API int Bcmp(UBFH *p_ubf1, UBFH *p_ubf2);
extern NDRX_API int Bsubset(UBFH *p_ubf1, UBFH *p_ubf2);
extern NDRX_API BFLDOCC Bnum (UBFH * p_ub);
extern NDRX_API long Bneeded(BFLDOCC nrfields, BFLDLEN totsize);

/* VIEW related */
extern NDRX_API int Bvnull(char *cstruct, char *cname, BFLDOCC occ, char *view);
extern NDRX_API int Bvselinit(char *cstruct, char *cname, char *view);
extern NDRX_API int Bvsinit(char *cstruct, char *view);
extern NDRX_API void Bvrefresh(void);
extern NDRX_API int Bvopt(char *cname, int option, char *view);
extern NDRX_API int Bvftos(UBFH *p_ub, char *cstruct, char *view);
extern NDRX_API int Bvstof(UBFH *p_ub, char *cstruct, int mode, char *view);

extern NDRX_API int CBvget(char *cstruct, char *view, char *cname, BFLDOCC occ, 
             char *buf, BFLDLEN *len, int usrtype, long flags);
extern NDRX_API int CBvchg(char *cstruct, char *view, char *cname, BFLDOCC occ, 
             char *buf, BFLDLEN len, int usrtype);
extern NDRX_API long Bvsizeof(char *view);
extern NDRX_API long Bvcpy(char *cstruct_dst, char *cstruct_src, char *view);
extern NDRX_API BFLDOCC Bvoccur(char *cstruct, char *view, char *cname, 
        BFLDOCC *maxocc, BFLDOCC *realocc, long *dim_size, int* fldtype);
extern NDRX_API int Bvsetoccur(char *cstruct, char *view, char *cname, BFLDOCC occ);
extern NDRX_API int Bvnext (Bvnext_state_t *state, char *view, char *cname, 
			    int *fldtype, BFLDOCC *maxocc, long *dim_size);
extern NDRX_API int Bvcmp(char *cstruct1, char *view1, char *cstruct2, char *view2);
/* VIEW related, END */

/* ATMI library TLS: */
extern NDRX_API void * ndrx_ubf_tls_get(void);
extern NDRX_API int ndrx_ubf_tls_set(void *data);
extern NDRX_API void ndrx_ubf_tls_free(void *data);
extern NDRX_API void * ndrx_ubf_tls_new(int auto_destroy, int auto_set);


/* Custom field database API: */

extern NDRX_API EDB_env * Bfldddbgetenv (EDB_dbi **dbi_id, EDB_dbi **dbi_nm);
extern NDRX_API int Bflddbload(void);
extern NDRX_API BFLDID Bflddbid (char *fldname);
extern NDRX_API char * Bflddbname (BFLDID bfldid);
extern NDRX_API int Bflddbget(EDB_val *data, short *p_fldtype,
        BFLDID *p_bfldno, BFLDID *p_bfldid, char *fldname, int fldname_bufsz);
extern NDRX_API int Bflddbunlink(void);
extern NDRX_API void Bflddbunload(void);
extern NDRX_API int Bflddbdrop(EDB_txn *txn);
extern NDRX_API int Bflddbdel(EDB_txn *txn, BFLDID bfldid);
extern NDRX_API int Bflddbadd(EDB_txn *txn, short fldtype, BFLDID bfldno, 
        char *fldname);

extern NDRX_API int B32to16(UBFH *dest, UBFH *src);
extern NDRX_API int B16to32(UBFH *dest, UBFH *src);

#if defined(__cplusplus)
}
#endif

#endif
/* vim: set ts=4 sw=4 et smartindent: */
