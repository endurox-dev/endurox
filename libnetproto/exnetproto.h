/**
 * @brief Multi-buffer conversion to / from machine independent network
 *  format.
 *
 * @file exnetproto.h
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

#ifndef EXNETPROTO_H
#define	EXNETPROTO_H

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define PMSGMAX -2 /* special case for max buffer size */

#define TAG_BYTES   2   /* Number of bytes used in tag */
#define LEN_BYTES   4   /* Number of bytes used in len */

#define MKSIGN char sign = '0';\
            if (*tmp<0)\
            {\
                sign = '1';\
            }

#define OFSZ(s,e)   EXOFFSET(s,e), EXELEM_SIZE(s,e)
#define OFSZ0       0,0

#define XFLD           0x01     /**< Normal field                           */
#define XSUB           0x02     /**< Dummy field/subfield                   */
#define XSBL           0x03     /**< Dummy field/subfield len               */
#define XINC           0x04     /**< Include table                          */
#define XLOOP          0x05     /**< Loop construction                      */
#define XATMIBUF       0x06     /**< ATMI buffer type...                    */
#define XMASTERBUF     0x07     /**< internal C tlv list of XATMIBUFS       */
#define XFLDPTR        0x08     /**< Field is pointer to data block         */
#define XATMIBUFPTR    0x09     /**< This is pointer to XATMI buf           */

#define XTAB1(e1)           1, e1, NULL, NULL, NULL
#define XTAB2(e1,e2)        2, e1, e2, NULL, NULL
#define XTAB3(e1,e2,e3)     3, e1, e2, e3, NULL
#define XTAB4(e1,e2,e3,e4)  4, e1, e2, e3, e4


/**
 * Table bellow at index is UBF field type
 * the value is type data tag in protocol buffer.
 */
#define UBF_TAG_BFLD_SHORT          0x1113 
#define UBF_TAG_BFLD_LONG           0x111D
#define UBF_TAG_BFLD_CHAR           0x1127
#define UBF_TAG_BFLD_FLOAT          0x1131
#define UBF_TAG_BFLD_DOUBLE         0x113B
#define UBF_TAG_BFLD_STRING         0x1145
#define UBF_TAG_BFLD_CARRAY         0x114F

#define UBF_TAG_BFLDID     0x10FF
#define UBF_TAG_BFLDLEN    0x1109

/*
 * Standard check for output buffer space
 */
#define CHECK_PROTO_BUFSZ(RET, CURSIZE, MAXSIZE, TO_WRITE) \
    if ((CURSIZE) + TO_WRITE > MAXSIZE) \
    {\
        NDRX_LOG(log_error, "ERROR ! EX2NET: Message size in bytes max: %ld, at "\
                    "current state: %ld, about "\
                    "to write: %ld (new total: %d) - EXCEEDS message size. "\
                    "Please increase NDRX_MSGSIZEMAX!", \
                    (long)MAXSIZE, (long)(CURSIZE), (long)TO_WRITE, (long)(CURSIZE) + (long)TO_WRITE);\
        userlog("ERROR ! EX2NET: Message size in bytes max: %ld, at current state: %ld, about "\
                    "to write: %ld (new total: %d) - EXCEEDS message size. "\
                    "Please increase NDRX_MSGSIZEMAX!", \
                    (long)MAXSIZE, (long)(CURSIZE), (long)TO_WRITE, (long)(CURSIZE) + (long)TO_WRITE);\
        EXFAIL_OUT(RET);\
    }
/*
 *  Check the internal incoming buffers, for data overrun
 */
#define CHECK_EX_BUFSZ(RET, CUROFFSZ, FLD_OFFSZ, MAXSIZE, TO_WRITE) \
if ((CUROFFSZ) + (FLD_OFFSZ) + (TO_WRITE) > MAXSIZE) \
    {\
        NDRX_LOG(log_error, "ERROR ! NET2EX: Incomming buffer size in bytes: %ld, incoming "\
                            "message is larger %ld (offset: %ld, fld_offs: %ld, "\
                            "datasize: %ld) - dropping, please increase NDRX_MSGSIZEMAX!",\
                    (long)MAXSIZE, (long)((CUROFFSZ) + (FLD_OFFSZ) + (TO_WRITE)),\
                    (long)(CUROFFSZ), (long)(FLD_OFFSZ), (long)(TO_WRITE)\
                    );\
        userlog("ERROR ! NET2EX: Incomming buffer size in bytes: %ld, incoming "\
                            "message is larger %ld (offset: %ld, fld_offs: %ld, "\
                            "datasize: %ld) - dropping, please increase NDRX_MSGSIZEMAX!",\
                    (long)MAXSIZE, (long)((CUROFFSZ) + (FLD_OFFSZ) + (TO_WRITE)),\
                    (long)(CUROFFSZ), (long)(FLD_OFFSZ), (long)(TO_WRITE));\
        EXFAIL_OUT(RET);\
    }

#define CHECK_EX_BUFSZ_SIMPLE(RET, MAXSIZE, TO_WRITE) \
if ((TO_WRITE) > MAXSIZE) \
    {\
        NDRX_LOG(log_error, "ERROR ! NET2EX: Incomming buffer size in bytes: %ld"\
                                "data size: %ld - dropping, please increase NDRX_MSGSIZEMAX!",\
                MAXSIZE, (TO_WRITE));\
        userlog( "ERROR ! NET2EX: Incomming buffer size in bytes: %ld"\
                                "data size: %ld - dropping, please increase NDRX_MSGSIZEMAX!",\
                MAXSIZE, (TO_WRITE));\
        EXFAIL_OUT(RET);\
    }


/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/* map between C structure and parm bnlock fields */
typedef struct cproto cproto_t;
typedef struct xmsg xmsg_t;

struct cproto
{
    int     tableid;            /* table id. */
    long    tag;                /* TLV tag */
    char*   cname;              /* c field name */
    long    offset;             /* offset in structure */
    int     len;                /* len in bytes */
    int     fld_type;           /* field type */
    short   type;               /* field type:  XFLD or XSUB */
    int     min_len;            /* min data len in chars */
    int     max_len;            /* max data len in chars */
    cproto_t *include;          /* include sub_structure */
    long    counter_offset;     /* counter offset... */
    long    elem_size;          /* size of array element. */
    /* During parsing returns conv struct for XSUB */
    xmsg_t * (*p_classify_fn) (char *ex_buf, long ex_len); 
    
    size_t  buftype_offset;     /* buffer type offset */
};

/* Table used to define network messages. */
struct xmsg
{
    char    msg_type;
    int     command;            /* Id of the command... */    
    char    *descr;             /* Descr of the message */
    /* Extra tables to drive sub-elements. */
    int     tabcnt;
    cproto_t    *tab[4];        /* Recursive/linear tables for sub buffers. */
};

/* protocol table info */
struct ptinfo
{
    int idx;
    int dim;
};
typedef struct ptinfo ptinfo_t;


/* Internal structure for driving UBF fields. */
typedef struct proto_ufb_fld proto_ufb_fld_t;
struct proto_ufb_fld
{
    int bfldid;     /**< used also as offset indicator in exbuf for mbuf     */
    int bfldlen;    /**< used also as indicator for total buffer len in mbuf */
    unsigned typetag;/**< type in mbuf bits                                  */
    unsigned typelen;/**< used for sub-buffers                               */
    char cname [NDRX_VIEW_CNAME_LEN+1]; /**< used by views                   */
#if EX_ALIGNMENT_BYTES == 8
    long         padding1;
#endif
    char buf[0];
};


/*---------------------------Globals------------------------------------*/

extern cproto_t ndrx_G_ndrx_mbuf_tlv_x[];

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern int ndrx_write_tag(short tag, char *buf, long *proto_buf_offset, 
        long proto_bufsz);

extern int ndrx_write_len(int len, char *buf, long *proto_buf_offset, 
        long proto_bufsz);

extern int _exproto_proto2ex_mbuf(cproto_t *fld, char *proto_buf, long proto_len, 
        char *ex_buf, long ex_offset, long *max_struct, int level, 
        UBFH *p_x_fb, proto_ufb_fld_t *p_ub_data, long ex_bufsz);

extern int exproto_build_ex2proto_mbuf(cproto_t *fld, int level, long offset,
        char *ex_buf, long ex_len, char *proto_buf, long *proto_buf_offset,
        short *accept_tags, proto_ufb_fld_t *p_ub_data, 
        long proto_bufsz);

extern int exproto_build_ex2proto(xmsg_t *cv, int level, long offset,
        char *ex_buf, long ex_len, char *proto_buf, long *proto_buf_offset,
        short *accept_tags, proto_ufb_fld_t *p_ub_data, 
        long proto_bufsz);

extern long _exproto_proto2ex(cproto_t *cur, char *proto_buf, long proto_len, 
        char *ex_buf, long ex_offset, long *max_struct, int level, 
        UBFH *p_x_fb, proto_ufb_fld_t *p_ub_data, long ex_bufsz);

#endif	/* EXNETPROTO_H */

/* vim: set ts=4 sw=4 et smartindent: */
