/* 
** EnduroX cluster procotol.
** TLV will be simple strucutre:
** Tag: 2 bytes
** Length: 2 bytes
** Data...
** All data will be generated as ASCII chars.
**
** @file proto.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/

/*---------------------------Includes-----------------------------------*/
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <atmi.h>

#include <stdio.h>
#include <stdlib.h>

#include <exnet.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>

#include <utlist.h>
#include <ubf_int.h>            /* FLOAT_RESOLUTION, DOUBLE_RESOLUTION */

#include <typed_buf.h>

#include "fdatatype.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define PMSGMAX ATMI_MSG_MAX_SIZE

#define MKSIGN char sign = '0';\
            if (*tmp<0)\
            {\
                sign = '1';\
            }

#define OFSZ(s,e)   OFFSET(s,e), ELEM_SIZE(s,e)
#define OFSZ0       0,0

#define XFLD           0x01               /* Normal field           */
#define XSUB           0x02               /* Dummy field/subfield   */
#define XSBL           0x03               /* Dummy field/subfield len*/
#define XINC           0x04               /* Include table          */
#define XLOOP          0x05               /* Loop construction      */
#define XATMIBUF       0x06               /* ATMI buffer type...    */

#define XTAB1(e1)           1, e1, NULL, NULL, NULL
#define XTAB2(e1,e2)        2, e1, e2, NULL, NULL
#define XTAB3(e1,e2,e3)     3, e1, e2, e3, NULL
#define XTAB4(e1,e2,e3,e4)  4, e1, e2, e3, e4

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/* map between C structure and parm bnlock fields */
typedef struct cproto cproto_t;
typedef struct xmsg xmsg_t;

struct cproto {
    long    tag;                /* TLV tag */
    char*   cname;              /* c field name */
    size_t  offset;             /* offset in structure */
    int     len;                /* len in bytes */
    int     fld_type;           /* field type */
    short   type;               /* field type:  XFLD or XSUB */
    int     min_len;            /* min data len in chars */
    int     max_len;            /* max data len in chars */
    cproto_t *include;          /* include sub_structure */
    size_t  counter_offset;     /* counter offset... */
    size_t  elem_size;          /* size of array element. */
    /* During parsing returns conv struct for XSUB */
    xmsg_t * (*p_classify_fn) (char *ex_buf, long ex_len); 
    
    size_t  buftype_offset;     /* buffer type offset */
};

/* Table used to define network messages. */
struct xmsg{
    char    msg_type;
    int     command;            /* Id of the command... */    
    char    *descr;             /* Descr of the message */
    /* Extra tables to drive sub-elements. */
    int     tabcnt;
    cproto_t    *tab[4];        /* Recursive/linear tables for sub buffers. */
};

/*---------------------------Globals------------------------------------*/

private xmsg_t * classify_netcall (char *ex_buf, long ex_len);

/* Type dscription, for debugging */
static char *M_type[] = {
"EXF_SHORT",    /* 0 */
"EXF_LONG",     /* 1 */
"EXF_CHAR",     /* 2 */
"EXF_FLOAT",    /* 3 */
"EXF_DOUBLE",   /* 4 */
"EXF_STRING",   /* 5 */
"EXF_CARRAY",   /* 6 */
"EXF_NONE",     /* 7 */
"EXF_INT",      /* 8 */
"EXF_ULONG",    /* 9 */
"EXF_UINT",     /* 10 */
"EXF_NTIMER",   /* 11 */
"EXF_TIMET"     /* 12 */
};

/* Converter for cmd_br_net_call_t */
static cproto_t M_cmd_br_net_call_x[] = 
{
    {0x01, "br_magic",  OFSZ(cmd_br_net_call_t,br_magic),   EXF_LONG, XFLD, 6, 6, NULL},
    {0x02, "msg_type",  OFSZ(cmd_br_net_call_t,msg_type),   EXF_CHAR, XFLD, 1, 1},
    {0x23, "command_id",OFSZ(cmd_br_net_call_t,command_id), EXF_INT,  XFLD, 1, 5},
    {0x03, "len",       OFSZ(cmd_br_net_call_t,len),        EXF_LONG, XSBL, 1, 10},
    {0x04, "buf",       OFSZ(cmd_br_net_call_t,buf),        EXF_NONE, XSUB, 0, PMSGMAX, 
                                                NULL, FAIL, FAIL, classify_netcall},
    {FAIL}
};

/* Converter for stnadard ndrxd header. */
static cproto_t M_stdhdr_x[] = 
{
    {0x05, "command_id",    OFSZ(command_call_t,command_id),     EXF_SHORT, XFLD, 1, 4},
    {0x06, "proto_ver",     OFSZ(command_call_t,proto_ver),      EXF_CHAR, XFLD, 1, 1},
    /* it should be 0 AFAIK... */
    {0x07, "proto_magic",   OFSZ(command_call_t,proto_magic),    EXF_INT,  XFLD, 0, 2},
    {FAIL}
};

/* Converter for command_call_t */
static cproto_t M_command_call_x[] = 
{
    {0x20,  "stdhdr",       OFSZ0,                              EXF_NONE,   XINC, 1, PMSGMAX, M_stdhdr_x},
    {0x08,  "magic",        OFSZ(command_call_t,magic),         EXF_ULONG,  XFLD, 6, 6},
    {0x16,  "command",      OFSZ(command_call_t,command),       EXF_INT,    XFLD, 2, 2},
    {0x09,  "msg_type",     OFSZ(command_call_t,msg_type),      EXF_SHORT,  XFLD, 1, 2},
    {0x10,  "msg_src",      OFSZ(command_call_t,msg_src),       EXF_SHORT,  XFLD, 1, 1},
    {0x11,  "reply_queue",  OFSZ(command_call_t,reply_queue),   EXF_STRING, XFLD, 1, 128},
    {0x12,  "flags",        OFSZ(command_call_t,flags),         EXF_INT,    XFLD, 1, 5},
    {0x13,  "caller_nodeid",OFSZ(command_call_t,caller_nodeid), EXF_INT,    XFLD, 1, 3},
    {FAIL}
};

/* Convert for cmd_br_time_sync_t */
static cproto_t M_cmd_br_time_sync_x[] = 
{
    {0x21,  "call",       OFSZ0,                              EXF_NONE,   XINC, 1, PMSGMAX, M_command_call_x},
    {0x15,  "time",       OFSZ(cmd_br_time_sync_t,time),      EXF_NTIMER, XFLD, 40, 40},
    {FAIL}
};

/* Convert for bridge_refresh_svc_t */
static cproto_t Mbridge_refresh_svc_x[] = 
{
    {0x20,  "mode",       OFSZ(bridge_refresh_svc_t,mode),    EXF_CHAR,    XFLD, 1, 6},
    {0x21,  "svc_nm",     OFSZ(bridge_refresh_svc_t,svc_nm),  EXF_STRING,  XFLD, 1, MAXTIDENT},
    {0x22,  "count",      OFSZ(bridge_refresh_svc_t,count),   EXF_INT,     XFLD, 1, 6},
    {FAIL}
};

/* Convert for bridge_refresh_t */
static cproto_t M_bridge_refresh_x[] = 
{
    {0x24,  "call",       OFSZ0,                              EXF_NONE,   XINC, 1, PMSGMAX, M_command_call_x},
    {0x17,  "mode",       OFSZ(bridge_refresh_t,mode),        EXF_INT,    XFLD, 1, 6},
    {0x18,  "count",      OFSZ(bridge_refresh_t,count),       EXF_INT,    XFLD, 1, 6},
    /* We will provide integer as counter for array:  */
    {0x19,  "svcs",       OFSZ(bridge_refresh_t,svcs),        EXF_NONE,   XLOOP, 1, PMSGMAX, Mbridge_refresh_svc_x, 
                            OFFSET(bridge_refresh_t,count), sizeof(bridge_refresh_svc_t)},
    {FAIL}
};
/******************** STUFF FOR UBF *******************************************/
/* Internal structure for driving UBF fields. */
typedef struct proto_ufb_fld proto_ufb_fld_t;
struct proto_ufb_fld
{
    int bfldid;
    int bfldlen;
    char buf[PMSGMAX+1];
};

/**
 * Table bellow at index is UBF field type
 * the value is type data tag in protocol buffer.
 */
static short M_ubf_proto_tag_map[] = 
{
    0xb10, /* BFLD_SHORT- 0   */
    0xb11, /* BFLD_LONG - 1	 */
    0xb12, /* BFLD_CHAR - 2	 */
    0xb13, /* BFLD_FLOAT - 3  */
    0xb14, /* BFLD_DOUBLE - 4 */
    0xb15, /* BFLD_STRING - 5 */
    0xb16, /* BFLD_CARRAY - 6 */  
};

/* Helper driver table for UBF buffer. */
static cproto_t M_ubf_field[] = 
{
    {0xb01,  "bfldid", OFSZ(proto_ufb_fld_t, bfldid),  EXF_INT,   XFLD, 1, 6},
    {0xb02,  "bfldlen",OFSZ(proto_ufb_fld_t, bfldlen), EXF_INT,   XSBL, 1, 6},
    /* Typed fields... */
    {0xb10,  "short", OFSZ(proto_ufb_fld_t, buf), EXF_SHORT,   XFLD, 1, 6},
    {0xb11,  "long",  OFSZ(proto_ufb_fld_t, buf), EXF_LONG,    XFLD, 1, 20},
    {0xb12,  "char",  OFSZ(proto_ufb_fld_t, buf), EXF_CHAR,    XFLD, 1, 1},
    {0xb13,  "float", OFSZ(proto_ufb_fld_t, buf), EXF_FLOAT,   XFLD, 1, 40},
    {0xb14,  "double",OFSZ(proto_ufb_fld_t, buf), EXF_DOUBLE,  XFLD, 1, 40},
    {0xb15,  "string",OFSZ(proto_ufb_fld_t, buf), EXF_STRING,  XFLD, 0, PMSGMAX},
    {0xb16,  "carray",OFSZ(proto_ufb_fld_t, buf), EXF_CARRAY,  XFLD, 0, PMSGMAX,
                            /* Using counter offset as carry len field... */
                            NULL, OFFSET(proto_ufb_fld_t,bfldlen), FAIL, NULL},
    {FAIL}
};

/* TODO: Add XA related fields: */
/* Converter for  tp_command_call_t */
static cproto_t M_tp_command_call_x[] = 
{
    {0xa01,  "stdhdr",    OFSZ0,                            EXF_NONE,    XINC, 1, PMSGMAX, M_stdhdr_x},
    {0xa02,  "buffer_type_id",OFSZ(tp_command_call_t,buffer_type_id),EXF_SHORT,   XFLD, 1, 5},
    {0xa03,  "name",      OFSZ(tp_command_call_t,name),     EXF_STRING, XFLD, 0, XATMI_SERVICE_NAME_LENGTH},
    {0xa04,  "reply_to",  OFSZ(tp_command_call_t,reply_to), EXF_STRING, XFLD, 0, NDRX_MAX_Q_SIZE},
    {0xa05,  "callstack", OFSZ(tp_command_call_t,callstack),EXF_STRING, XFLD, 0, CONF_NDRX_NODEID_COUNT},
    {0xa06,  "my_id",     OFSZ(tp_command_call_t,my_id),    EXF_STRING, XFLD, 0, NDRX_MAX_ID_SIZE},
    {0xa07,  "sysflags",  OFSZ(tp_command_call_t,sysflags), EXF_LONG,   XFLD, 1, 20},
    {0xa08,  "cd",        OFSZ(tp_command_call_t,cd),       EXF_INT,    XFLD, 1, 10},
    {0xa09,  "rval",      OFSZ(tp_command_call_t,rval),     EXF_INT,    XFLD, 1, 10},
    {0xa0a,  "rcode",     OFSZ(tp_command_call_t,rcode),    EXF_LONG,   XFLD, 1, 20},
    {0xa0b,  "extradata", OFSZ(tp_command_call_t,extradata),EXF_STRING, XFLD, 0, 31},
    {0xa0c,  "flags",     OFSZ(tp_command_call_t,flags),    EXF_LONG,   XFLD, 1, 20},
    {0xa0e,  "timestamp", OFSZ(tp_command_call_t,timestamp),EXF_LONG,   XFLD, 1, 20},
    {0xa0f,  "callseq",   OFSZ(tp_command_call_t,callseq),  EXF_UINT,   XFLD, 1, 6},
    {0xa10,  "timer",     OFSZ(tp_command_call_t,timer),    EXF_NTIMER, XFLD, 20, 20},
    {0xa11,  "data_len",  OFSZ(tp_command_call_t,data_len), EXF_LONG,   XSBL, 1, 10},
    {0xa12,  "data",      OFSZ(tp_command_call_t,data),     EXF_NONE,  XATMIBUF, 0, PMSGMAX, NULL, 
                            /* WARNING! Using counter offset here are length FLD offset! */
                           OFFSET(tp_command_call_t,data_len), FAIL, NULL, OFFSET(tp_command_call_t,buffer_type_id)},
    
    
    {0xa13,  "tmxid",  OFSZ(tp_command_call_t,tmxid),    EXF_STRING, XFLD, 1, (NDRX_XID_SERIAL_BUFSIZE+1)},
    {0xa14,  "tmrmid", OFSZ(tp_command_call_t, tmrmid), EXF_SHORT,   XFLD, 1, 6},
    {0xa15,  "tmnodeid", OFSZ(tp_command_call_t, tmnodeid), EXF_SHORT,   XFLD, 1, 6},
    {0xa16,  "tmsrvid", OFSZ(tp_command_call_t, tmsrvid), EXF_SHORT,   XFLD, 1, 6},
    {0xa17,  "tmknownrms",OFSZ(tp_command_call_t,tmknownrms),    EXF_STRING, XFLD, 1, (NDRX_MAX_RMS+1)},
    /* Is transaction marked as abort only? */
    {0xa18,  "tmtxflags", OFSZ(tp_command_call_t, tmtxflags), EXF_SHORT,   XFLD, 1, 1},
    {FAIL}
};

/* Message conversion tables */

/* NDRXD messages: */
static xmsg_t M_ndrxd_x[] = 
{
    {'A', 0, /*any*/             "atmi_any",    XTAB2(M_cmd_br_net_call_x, M_tp_command_call_x)},
    {'X', NDRXD_COM_BRCLOCK_RQ,  "brclockreq",  XTAB2(M_cmd_br_net_call_x, M_cmd_br_time_sync_x)},
    {'X', NDRXD_COM_BRREFERSH_RQ,"brrefreshreq",XTAB2(M_cmd_br_net_call_x, M_bridge_refresh_x)},
    {FAIL, FAIL}
};



/* ATMI messages: */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private int _exproto_proto2ex(cproto_t *cur, char *proto_buf, long proto_len, 
        char *ex_buf, long ex_len, long *max_struct, int level, 
        UBFH *p_x_fb, proto_ufb_fld_t *p_ub_data);


#define FIX_SIGND(x) if ('1'==bdc_sign) *x = -1 * (*x);
#define FIX_SIGNF(x) if ('1'==bdc_sign) *x = -1.0 * (*x);

/**
 * BCD Enconding rules:
 * 
 * signed decimals:
 * Prefix 0 if needs to align to bytes.
 * Always: Postfix 0 - if sign >=0.
 *         Postfix 1 - if sign < 0.
 * For example:
 * Number: 012331 = -1233
 * 
 * Floats encodes with 5 digits (doubles with 6 implied dec pos) after comma:
 * For example -14.55 will be encoded as:
 *          14550001
 * For example 14.55 will be ecoded as:
 *          14550000
 * 
 * 
 * @param fld
 * @param c_buf_in
 * @param c_buf_out
 * @param net_buf_len
 * @param c_buf_in_len - data len, only for carray.
 * @return 
 */
private int x_ctonet(cproto_t *fld, char *c_buf_in,  
                        char *c_buf_out, long *net_buf_len,
                        char *debug_buf, int c_buf_in_len)
{
    int ret=SUCCEED;
    int i;
    int conv_bcd = FALSE;
    
    switch (fld->fld_type)
    {
        case EXF_SHORT:
        {
            short *tmp = (short *)c_buf_in;
            MKSIGN;
                        
            sprintf(c_buf_out, "%hd%c", *tmp, sign);
            *net_buf_len = strlen(c_buf_out);
            conv_bcd = TRUE;
        }
            break;
        case EXF_LONG:
        {
            long *tmp = (long *)c_buf_in;
            MKSIGN;
            
            sprintf(c_buf_out, "%ld%c", *tmp, sign);
            *net_buf_len = strlen(c_buf_out);
            conv_bcd = TRUE;
        }
            break;
        case EXF_CHAR:
        {
            char *tmp = (char *)c_buf_in;
            c_buf_out[0] = *tmp;
            c_buf_out[1] = 0;
            *net_buf_len = 1;
        }
            break;
        case EXF_FLOAT:
        {
            float *tmp = (float *)c_buf_in;
            float tmp_op = *tmp;
            MKSIGN;
            
            for (i=0; i<FLOAT_RESOLUTION; i++)
                tmp_op*=10.0f;
            
            sprintf(c_buf_out, "%.0lf%c", tmp_op, sign);
            *net_buf_len = strlen(c_buf_out);
            
            conv_bcd = TRUE;
        }
            break;
        case EXF_DOUBLE:
        {
            double *tmp = (double *)c_buf_in;
            double tmp_op = *tmp;
            MKSIGN;
            
            for (i=0; i<DOUBLE_RESOLUTION; i++)
                tmp_op*=10.0f;
            
            sprintf(c_buf_out, "%.0lf%c", tmp_op, sign);
            *net_buf_len = strlen(c_buf_out);
            
            conv_bcd = TRUE;
            
        }
            break;
        case EXF_STRING:
        {
            /* EOS must be present in c struct! */
            strcpy(c_buf_out, c_buf_in);
            *net_buf_len = strlen(c_buf_out);
        }
            break;
        case EXF_INT:
        {
            int *tmp = (int *)c_buf_in;
            MKSIGN;
            
            sprintf(c_buf_out, "%d%c", *tmp, sign);
            *net_buf_len = strlen(c_buf_out);
            conv_bcd = TRUE;
            
        }
            break;
        case EXF_ULONG:
        {
            unsigned long *tmp = (unsigned long *)c_buf_in;
            sprintf(c_buf_out, "%lu", *tmp);
            *net_buf_len = strlen(c_buf_out);
            conv_bcd = TRUE;
        }
            break;
        case EXF_UINT:
        {
            unsigned *tmp = (unsigned *)c_buf_in;
            sprintf(c_buf_out, "%u", *tmp);
            *net_buf_len = strlen(c_buf_out);
            conv_bcd = TRUE;
        }    
            break;
        case EXF_NTIMER:
        {
            n_timer_t *tmp = (n_timer_t *)c_buf_in;
            sprintf(c_buf_out, "%020ld%020ld", tmp->t.tv_sec, 
                    tmp->t.tv_nsec);
            NDRX_LOG(6, "time=>[%s]", c_buf_out);
            
            NDRX_LOG(log_debug, "timer = (tv_sec: %ld tv_nsec: %ld)"
                                    " delta: %d", 
                                    tmp->t.tv_sec,  tmp->t.tv_nsec, 
                                    n_timer_get_delta_sec(tmp));
            
            *net_buf_len = strlen(c_buf_out);
            conv_bcd = TRUE;
        }    
            break;
        case EXF_CARRAY:
        {
            /* Support for carray field. */
            memcpy(c_buf_out, c_buf_in, c_buf_in_len);
            *net_buf_len = c_buf_in_len;
            
            /* Built representation for user... for debug purposes... */
            build_printable_string(debug_buf, c_buf_out, c_buf_in_len);
        }    
            break;
                
        case EXF_NONE:
        default:
            NDRX_LOG(log_error, "I do not know how to convert %d "
                    "type to network!", fld->fld_type);
            ret=FAIL;
            goto out;
            break;
    }
    
    if (EXF_CARRAY!=fld->fld_type)
    {
        strcpy(debug_buf, c_buf_out);
    }
    /* else should be set up already by carray func! */
    
    /* Perform length check here... */
    if (conv_bcd)
    {
        char bcd_tmp[128];
        char tmp_char_buf[3];
        char c;
        int hex_dec;
        int j;
        int bcd_tmp_len;
        int bcd_pos = 0;
        
        if (strlen(c_buf_out) % 2)
        {
            strcpy(bcd_tmp, "0");
            strcat(bcd_tmp, c_buf_out);
        }
        else
        {
            strcpy(bcd_tmp, c_buf_out);
        }
        
        /* Now process char by char */
        bcd_tmp_len = strlen(bcd_tmp);
        
        for (j=0; j<bcd_tmp_len; j+=2)
        {
            strncpy(tmp_char_buf, bcd_tmp+j, 2);
            tmp_char_buf[2] = EOS;
            sscanf(tmp_char_buf, "%x", &hex_dec);
            
            /*NDRX_LOG(6, "got hex 0x%x", hex_dec);*/
            
            c_buf_out[bcd_pos] = (char)(hex_dec & 0xff);
            /*NDRX_LOG(6, "put[%d] %x",bcd_pos, c_buf_out[bcd_pos]);*/
                    
            bcd_pos++;
        }
        *net_buf_len = bcd_tmp_len / 2;
    }
    
out:
    return ret;
}

/**
 * Convert net primitive field to c
 * @param fld - field descriptor
 * @param net_buf - net buffer
 * @param net_buf_offset - data offset in netbuffer
 * @param tag_len - length mentioned in tag
 * @param c_buf_out - c structure where data should be written
 * @return SUCCEED/FAIL
 */
private int x_nettoc(cproto_t *fld, 
                    char *net_buf, long net_buf_offset, short tag_len, /* in */
                    char *c_buf_out, BFLDLEN *p_bfldlen, char *debug_buf) /* out */
{
    int ret=SUCCEED;
    int i, j;
    int conv_bcd = FALSE;
    int bcd_sign_used = FALSE;
    char bcd_buf[64] = {EOS};
    char tmp[64];
    char bdc_sign;
    char *datap = (net_buf + net_buf_offset);
    
    /* NDRX_LOG(log_debug, "tag_len = %hd", tag_len);*/
    
    debug_buf[0] = EOS;
    
    /* We should detect is this bcd field or not? */
    switch (fld->fld_type)
    {
        case EXF_SHORT:
        case EXF_LONG:
        case EXF_FLOAT:
        case EXF_DOUBLE:
        case EXF_INT:
        {
            conv_bcd = TRUE;
            bcd_sign_used = TRUE;
        }
        case EXF_ULONG:
        case EXF_UINT:
        case EXF_NTIMER:
            conv_bcd = TRUE;
            break;
    }
    
    /* Convert BCD to string. */
    if (conv_bcd)
    {
        int len;
        /*
        NDRX_LOG(log_debug, "Processing BCD... %p [%ld]", net_buf, net_buf_offset);
         */
        /* Convert all stuff to string  11 => */
        for (i=0; i< tag_len; i++)
        {
            /* so if we have 25, then it is 0x25 =>  */
            int net_byte;
            
            net_byte = net_buf[net_buf_offset + i] & 0xff; 
            
            sprintf(tmp, "%02x", net_byte);
            strcat(bcd_buf, tmp);
        }
        
        /*
        NDRX_LOG(log_debug, "Got BDC: [%s]", bcd_buf);
        */
        /* Unsigned fields goes with out sign...! */
        if (bcd_sign_used)
        {
            len = strlen(bcd_buf);
            bdc_sign = bcd_buf[len-1];
            bcd_buf[len-1] = EOS;

            /*
            NDRX_LOG(log_debug, "Got BDC/sign: [%s]/[%c]", 
                    bcd_buf, bdc_sign);
             */
        }
    }
    
    
    switch (fld->fld_type)
    {
        case EXF_SHORT:
        {
            short *tmp = (short *)c_buf_out;
            sscanf(bcd_buf, "%hd", tmp);
            FIX_SIGND(tmp);
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                sprintf(debug_buf, "%hd", *tmp);
            }
        }
            break;
        case EXF_LONG:
        {
            long *tmp = (long *)c_buf_out;
            
            sscanf(bcd_buf, "%ld", tmp);
            
            FIX_SIGND(tmp);
          
            if (debug_get_ndrx_level() >= log_debug)
            {
                sprintf(debug_buf, "%ld", *tmp);
            }
        }
            break;
        case EXF_CHAR:
        {
            char *tmp = (char *)c_buf_out;
            tmp[0] = datap[0];
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                strcpy(debug_buf, tmp);
            }
        }
            break;
        case EXF_FLOAT:
        {
            float *tmp = (float *)c_buf_out;
            
            sscanf(bcd_buf, "%f", tmp);
            
            for (i=0; i<FLOAT_RESOLUTION; i++)
            {
                *tmp= *tmp / 10.0;
            }
            
            FIX_SIGNF(tmp);
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                sprintf(debug_buf, "%f", *tmp);
            }
            
        }
            break;
        case EXF_DOUBLE:
        {
            double *tmp = (double *)c_buf_out;
            
            sscanf(bcd_buf, "%lf", tmp);
            
            for (i=0; i<DOUBLE_RESOLUTION; i++)
            {
                *tmp= *tmp / 10.0;
            }
            
            FIX_SIGNF(tmp);
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                sprintf(debug_buf, "%lf", *tmp);
            }
        }
            break;
        case EXF_STRING:
        {
            /* EOS must be present in c struct! */
            strncpy(c_buf_out, datap, tag_len);
            c_buf_out[tag_len] = EOS;
            
            /*NDRX_LOG(6, "%s = [%s] (string)", fld->cname, c_buf_out);*/
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                strcpy(debug_buf, c_buf_out);
            }
        }
            break;
        case EXF_INT:
        {
            int *tmp = (int *)c_buf_out;
            
            sscanf(bcd_buf, "%d", tmp);
            
            FIX_SIGND(tmp);
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                sprintf(debug_buf, "%d", *tmp);
            }
        }
            break;
        case EXF_ULONG:
        {
            unsigned long *tmp = (unsigned long *)c_buf_out;
            sscanf(bcd_buf, "%lu", tmp);
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                sprintf(debug_buf, "%lu", *tmp);
            }
        }
            break;
        case EXF_UINT:
        {
            unsigned *tmp = (unsigned *)c_buf_out;
            sscanf(bcd_buf, "%u", tmp);
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                sprintf(debug_buf, "%u", *tmp);
            }
        }    
            break;
        case EXF_NTIMER:
        {
            char timer_buf[21];
            char *p;
            n_timer_t *tmp = (n_timer_t *)c_buf_out;
            
            strncpy(timer_buf, bcd_buf, 20);
            timer_buf[20] = EOS;
            
            p = timer_buf;
            while ('0'==*p) p++;
            
            NDRX_LOG(log_debug, "tv_sec=>[%s]", p);
            sscanf(p, "%ld", &(tmp->t.tv_sec));
            
            
            
            strncpy(timer_buf, bcd_buf+20, 20);
            timer_buf[20] = EOS;
            p = timer_buf;
            while ('0'==*p) p++;
            
            NDRX_LOG(log_debug, "tv_nsec=>[%s]", p);
            sscanf(p, "%ld", &(tmp->t.tv_nsec));
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                sprintf(debug_buf, "%s = [tv_sec = %ld tv_nsec = %ld] (unsigned)", 
                        fld->cname, tmp->t.tv_sec, tmp->t.tv_nsec);
            }
        }    
            break;
        case EXF_CARRAY:
        {
            memcpy(c_buf_out, datap, tag_len);
            *p_bfldlen = tag_len;
            
            if (debug_get_ndrx_level() >= log_debug)
            {
                build_printable_string(debug_buf, c_buf_out, tag_len);
            }
            
        }    
            break;
        case EXF_NONE:
        default:
            NDRX_LOG(log_error, "I do not know how to convert %d "
                    "type to network!", fld->fld_type);
            ret=FAIL;
            goto out;
            break;
    }
    
out:
    return ret;
}


/**
 * Read short (two bytes from network buffer).
 * usable for tag & len
 *s @param buf - network buffer
 * @param proto_buf_offset - offset where data starts
 * @return 
 */
private short read_net_short(char *buf, long *proto_buf_offset)
{
    short net_val;
    short ret;
    
    memcpy((char *)&net_val, buf+*proto_buf_offset, 2);
    
    ret = ntohs(net_val);
    
    
    *proto_buf_offset+=2;
    
    return ret;
}

/**
 * Put tag on network buffer.
 * @param tag - tag
 * @param buf - start of the buffer
 * @param proto_buf_offset - current offset of buffer
 */
private void write_tag(short tag, char *buf, long *proto_buf_offset)
{
    short net_tag;
    net_tag = htons(tag);
    /* Put tag on network */
    memcpy(buf+*proto_buf_offset, (char *)&net_tag, 2);
    *proto_buf_offset+=2;
}

/**
 * Put len on network buffer.
 * @param len - len
 * @param buf - start of the buffer
 * @param proto_buf_offset - current offset of buffer
 */
private void write_len(short len, char *buf, long *proto_buf_offset)
{
    short net_len;
    net_len = htons(len);
    /* Put tag on network */
    memcpy(buf+*proto_buf_offset, (char *)&net_len, 2);
    *proto_buf_offset+=2;
}

/**
 * Build network message by using xmsg record, this table is recursive...
 * @param cv
 * @param ex_buf
 * @param ex_len
 * @param proto_buf
 * @param proto_len
 * @return 
 */
public int exproto_build_ex2proto(xmsg_t *cv, int level, long offset,
        char *ex_buf, long ex_len, char *proto_buf, long *proto_buf_offset,
                        short *accept_tags, proto_ufb_fld_t *p_ub_data)
{
    int ret=SUCCEED;
    cproto_t *p = cv->tab[level];
    char tmp[PMSGMAX];
    char debug[PMSGMAX];
    long len = 0;
    
    /* Length memory: */
    int schedule_length = FALSE;
    cproto_t *len_rec;
    long len_offset;
    short *p_accept;
    short len_written; /* The length we used  */
    
    NDRX_LOG(log_debug, "Building table: %s - enter at %p [%s] "
                        "tag: [0x%x], level: %d", 
                        cv->descr, ex_buf+offset, p->cname, p->tag, level);
    
    while (FAIL!=p->tag)
    {
        len_written = 0;
                
        if (NULL!=accept_tags)
        {
            int accept = FALSE;
            /* Check acceptable tags... */
            p_accept = accept_tags;
            
            while (FAIL!=*p_accept)
            {
                if (*p_accept == p->tag)
                {
                    accept = TRUE;
                    break;
                }
                p_accept++;
            }
            
            if (!accept)
            {
                goto tag_continue;
            }
        }
        /* Check the type and process accordingly... */
        switch (p->type)
        {
            case XFLD:
            {
                /* This is field... */
                if ( 0xb16 == p->tag)
                {
                    ret = x_ctonet(p, ex_buf+offset+p->offset, tmp, &len, debug, 
                                p_ub_data->bfldlen);
                }
                else
                {
                    ret = x_ctonet(p, ex_buf+offset+p->offset, tmp, &len, debug, 0);
                }
                
                if (SUCCEED!=ret)
                {
                    NDRX_LOG(log_error, "Failed to convert tag %x: [%s] %ld"
                            "at offset %ld", p->tag, p->cname, p->offset);
                    ret=FAIL;
                    goto out;
                }
                
                NDRX_LOG(log_debug, "ex2net: tag: [0x%x]\t[%s]\t len:"
                        " %ld (0x%04lx) type:"
                        " [%s]\t data: [%s]"/*netbuf (tag start): %p"*/, 
                        p->tag, p->cname, len, len, M_type[p->fld_type], debug/*, 
                        (proto_buf+(*proto_buf_offset))*/ );
                
                /* Build that stuff */
                
                write_tag((short)p->tag, proto_buf, proto_buf_offset);
                write_len((short)len, proto_buf, proto_buf_offset);
                
                len_written = (short)len;
                
                /* Put data on network */
                memcpy(proto_buf+(*proto_buf_offset), tmp, len);
                *proto_buf_offset=*proto_buf_offset + len;
                
            }
                break;
            case XSUB:
            {
                /* <sub tlv> */
                /* Reserve space for Tag/Length */
                long len_offset;
                long off_start;
                long off_stop;
                
                /* This is sub tlv/ thus put tag... */
                write_tag((short)p->tag, proto_buf, proto_buf_offset);
                
                len_offset = *proto_buf_offset;
                *proto_buf_offset=*proto_buf_offset+2;
                
                off_start = *proto_buf_offset;
                /* </sub tlv> */
                
                /* This is sub field, we should run it from subtable... */
                ret = exproto_build_ex2proto(cv, level+1, offset+p->offset,
                        ex_buf, ex_len, proto_buf, proto_buf_offset, NULL, NULL);
                if (SUCCEED!=ret)
                {
                    NDRX_LOG(log_error, "Failed to convert sub/tag %x: [%s] %ld"
                            "at offset %ld", p->tag, p->cname, p->offset);
                    ret=FAIL;
                    goto out;
                }
                
                /* <sub tlv> */
                off_stop = *proto_buf_offset;
                /* Put back len there.. */
                len_written = (short)(off_stop - off_start);
                write_len(len_written, proto_buf, &len_offset);
                /* </sub tlv> */
            }
                break;
            case XSBL:
            {
                schedule_length = TRUE;
                /* We should save offset in output buffer for TLV */
                len_offset = *proto_buf_offset;
                len_rec = p;
                NDRX_LOG(6, "XSBL at %p", ex_buf+offset+p->offset);
            }
                break;
            case XINC:
            {
                xmsg_t tmp_cv;
                
                /* <sub tlv> */
                long len_offset;
                long off_start;
                long off_stop;
                /* </sub tlv> */
                memcpy(&tmp_cv, cv, sizeof(tmp_cv));
                tmp_cv.tab[0] = p->include;
                
                /* <sub tlv> */
                /* This is sub tlv/ thus put tag... */
                write_tag((short)p->tag, proto_buf, proto_buf_offset);
                
                NDRX_LOG(log_debug, "XINC tag: 0x%x", p->tag);
                
                len_offset = *proto_buf_offset;
                *proto_buf_offset=*proto_buf_offset+2;
                
                off_start = *proto_buf_offset; /* why not +2??? */
                /* </sub tlv> */
                
                /* If we use include the we should go deeper inside, not? */
                ret = exproto_build_ex2proto(&tmp_cv, 0, offset+p->offset,
                        ex_buf, ex_len, proto_buf, proto_buf_offset, NULL, NULL);
                
                if (SUCCEED!=ret)
                {
                    NDRX_LOG(log_error, "Failed to convert sub/tag %x: [%s] %ld"
                            "at offset %ld", p->tag, p->cname, p->offset);
                    ret=FAIL;
                    goto out;
                }
                
                /* <sub tlv> */
                off_stop = *proto_buf_offset;
                /* Put back len there.. */
                len_written = (short)(off_stop - off_start);
                write_len(len_written, proto_buf, &len_offset);
                /* </sub tlv> */
            }
                break;
            case XLOOP:
            {
                xmsg_t tmp_cv;
                
                memcpy(&tmp_cv, cv, sizeof(tmp_cv));
                tmp_cv.tab[0] = p->include;
                
                int *count = (int *)(ex_buf+offset+p->counter_offset);
                int j;
                
                NDRX_LOG(log_info, "Serialising: %d elements, "
                        "current tag: 0x%x", *count, p->tag);
                
                for (j=0; j<*count && SUCCEED==ret; j++)
                {
                    
                    /* Reserve space for Tag/Length */
                    /* <sub tlv> */
                    long len_offset;
                    long off_start;
                    long off_stop;

                    /* This is sub tlv/ thus put tag... */
                    write_tag((short)p->tag, proto_buf, proto_buf_offset);
                    
                    len_offset = *proto_buf_offset;
                    *proto_buf_offset=*proto_buf_offset+2;
                    
                    off_start = *proto_buf_offset;
                    /* </sub tlv> */
                    
                    ret = exproto_build_ex2proto(&tmp_cv, 0, 
                                offset+p->offset + p->elem_size*j,
                                ex_buf, ex_len, proto_buf, proto_buf_offset,
                                NULL, NULL);
                    
                    if (SUCCEED!=ret)
                    {
                        NDRX_LOG(log_error, "Failed to convert "
                                "sub/tag %x: [%s] %ld"
                                "at offset %ld", 
                                p->tag, p->cname, p->offset);
                        ret=FAIL;
                        goto out;
                    }
                    
                    /* <sub tlv> */
                    off_stop = *proto_buf_offset;
                    /* Put back len there.. */
                    len_written = (short)(off_stop - off_start);
                    write_len(len_written, proto_buf, &len_offset);
                    /* </sub tlv> */
                }
                
            }
                break;
                
            case XATMIBUF:
            {
                /* This is special driver for ATMI buffer */
                short *buffer_type = (short *)(ex_buf+offset+p->buftype_offset);
                long *buf_len = (long *)(ex_buf+offset+p->counter_offset);
                char *data = (char *)(ex_buf+offset+p->offset);
                int f_type;
                
                NDRX_LOG(log_debug, "Buffer type is: %hd", 
                        *buffer_type);
                
                if (*buffer_type == BUF_TYPE_UBF)
                {
                    UBFH *p_ub = (UBFH *)data;
                    proto_ufb_fld_t f;
                    BFLDOCC occ;
                    short accept_tags[] = {0xb01, 0xb02, 0, FAIL};
                    
                    /* Reserve space for Tag/Length */
                    /* <sub tlv> */
                    long len_offset;
                    long off_start;
                    long off_stop;
                    
                    xmsg_t tmp_cv;
                

                    /* This is sub tlv/ thus put tag... */
                    write_tag((short)p->tag, proto_buf, proto_buf_offset);
                    
                    len_offset = *proto_buf_offset;
                    *proto_buf_offset=*proto_buf_offset+2;
                    
                    off_start = *proto_buf_offset;
                    /* </sub tlv> */
                    memcpy(&tmp_cv, cv, sizeof(tmp_cv));
                    tmp_cv.tab[0] = M_ubf_field;

                    /* <process field by field> */
                    NDRX_LOG(log_debug, "Processing UBF buffer");
                    
                    /* loop over the buffer & process field by field */
                    memset(f.buf, 0, sizeof(f.buf));
                    f.bfldlen = sizeof(f.buf);
                    f.bfldid = BFIRSTFLDID;
                    while(1==Bnext(p_ub, &f.bfldid, &occ, f.buf, &f.bfldlen))
                    {
                        f_type = Bfldtype(f.bfldid);
                        
                        accept_tags[2] = M_ubf_proto_tag_map[f_type];
                        
                        NDRX_LOG(log_debug, "UBF type %d mapped to "
                                "tag %hd", f_type, accept_tags[2]);
                        
                        /* Hmm lets drive our structure? */
                        
                        ret = exproto_build_ex2proto(&tmp_cv, 0, 0,
                            (char *)&f, sizeof(f), proto_buf, proto_buf_offset, 
                                accept_tags, &f);
                    
                        if (SUCCEED!=ret)
                        {
                            NDRX_LOG(log_error, "Failed to convert "
                                    "sub/tag %x: [%s] %ld"
                                    "at offset %ld", 
                                    p->tag, p->cname, p->offset);
                            ret=FAIL;
                            goto out;
                        }
                        
                        memset(f.buf, 0, sizeof(f.buf));
                        f.bfldlen = sizeof(f.buf);
                    }
                    
                    /* </process field by field> */
                    
                    /* <sub tlv> */
                    off_stop = *proto_buf_offset;
                    /* Put back len there.. */
                    len_written = (short)(off_stop - off_start);
                    write_len(len_written, proto_buf, &len_offset);
                    /* </sub tlv> */
                }
                else
                {
                    /* Should work for string buffers too, if EOS counts in len! */
                    NDRX_LOG(log_debug, "Processing data block buffer");
                    
                    write_tag((short)p->tag, proto_buf, proto_buf_offset);
                    write_len((short)*buf_len, proto_buf, proto_buf_offset);
                    
                    len_written = *buf_len;
                    
                    /* Put data on network */
                    memcpy(proto_buf+(*proto_buf_offset), data, *buf_len);
                    *proto_buf_offset=*proto_buf_offset + *buf_len;
                }
            }
                break;
        }
        
        /* Verify data length (currently at warning level!) - it should be
         * in range!
         */
        if ((len_written < p->min_len  || len_written > p->max_len) && p->type != XSBL)
        {
            NDRX_LOG(log_error, "Experimental verification: WARNING! INVALID LEN!"
                    " tag: 0x%x (%s)"
                    " min_len=%ld max_len=%ld but got: %hd",
                    p->tag, p->cname, p->min_len, p->max_len, len_written);
            
            NDRX_DUMP(log_debug, "Invalid chunk:", 
                    /* Get to the start of the buffer: */
                    (char *) (proto_buf+(*proto_buf_offset) - len_written - 4), 
                    len_written + 4 /* two byte tag, two byte len */);
            
            /* TODO: Might consider to ret=FAIL; goto out; - 
             * When all will be debugged!
             */
        }
        
tag_continue:
        p++;
    }
out:
    NDRX_LOG(log_debug, "Return %d level %d", ret, level);

    return ret;
}

/**
 * Convert EnduroX internal format to Network Format.
 * @param 
 */
public int exproto_ex2proto(char *ex_buf, long ex_len, char *proto_buf, long *proto_len)
{
	int ret=SUCCEED;
    /* Identify the message */
    cmd_br_net_call_t *msg = (cmd_br_net_call_t *)ex_buf;
    char *fn = "exproto_ex2proto";
    xmsg_t *cv;
    
    /* Field used in search: */
    char msg_type; /* Identify our message type... */
    int     command;
    /* /Field used in search: */
    
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    switch (msg->msg_type)
    {
        case BR_NET_CALL_MSG_TYPE_ATMI:
            /* This is NDRXD message */
        {
            tp_command_generic_t *call = (tp_command_generic_t *)msg->buf;
            msg_type = 'A';
            command = call->command_id;
        }
            break;
        case BR_NET_CALL_MSG_TYPE_NDRXD:
            /* This is ATMI message */
        {
            command_call_t *call = (command_call_t *)msg->buf;
            
            msg_type = 'X';
            command = call->command;
            
        }
            break;
    }
    
    cv = M_ndrxd_x;
    /* OK, we should pick up the table and start to conv. */
    while (FAIL!=cv->command)
    {

        if (msg_type == cv->msg_type && command == cv->command
                || 'A' == msg_type /* Accept any ATMI - common structure! */)
        {
            NDRX_LOG(log_debug, "Found conv table for: %c/%d/%s", 
                    cv->msg_type, cv->command, cv->descr);

            ret = exproto_build_ex2proto(cv, 0, 0, ex_buf, ex_len, 
                    proto_buf, proto_len, NULL, NULL);

            break;
        }
        cv++;
    }

    if (FAIL==cv->command)
    {
        NDRX_LOG(log_error, "No conv table for ndrxd command: %d"
                " - FAIL", cv->command);
        ret=FAIL;
        goto out;
    }
    
    
out:
   
    NDRX_LOG(log_debug, "%s - returns %d", fn, ret);
	return ret;
}


/**
 * Search field from tag...
 * @param cur
 * @param tag
 * @return 
 */
private cproto_t * get_descr_from_tag(cproto_t *cur, short tag)
{
    while (FAIL!=cur->tag && cur->tag!=tag)
    {
        cur++;
    }
    
    if (FAIL==cur->tag)
    {
        return NULL;
    }
    
    return cur;
}

/**
 * Entry point from deblock of network message.
 * @param proto_buf
 * @param proto_len
 * @param ex_buf
 * @param ex_len
 * @param max_struct
 * @return 
 */
public int exproto_proto2ex(char *proto_buf, long proto_len, 
        char *ex_buf, long *max_struct)
{
    *max_struct = 0;
    return _exproto_proto2ex(M_cmd_br_net_call_x, proto_buf, proto_len, 
        ex_buf, 0, max_struct, 0, NULL, NULL);
}

/**
 * Deblock the network message...
 * Hm Also we need to get total len of message, in c structure?
 * @param ex_buf
 * @param ex_len
 * @param proto_buf
 * @param proto_len
 * @return 
 */
private int _exproto_proto2ex(cproto_t *cur, char *proto_buf, long proto_len, 
        char *ex_buf, long ex_len, long *max_struct, int level, 
        UBFH *p_x_fb, proto_ufb_fld_t *p_ub_data)
{
    int ret=SUCCEED;
    char *fn = "exproto_proto2ex";
    xmsg_t *cv = NULL;
    /* Current conv table: */
    cproto_t *fld = NULL;
    
    short net_tag;
    short net_len;
    int loop_keeper = 0;
    long int_pos = 0;
    int  *p_fld_len;
    int  xatmi_fld_len;
    char debug[PMSGMAX];
    
    NDRX_LOG(log_debug, "Enter field: [%s] max_struct: %ld", 
                        cur->cname, *max_struct);
    
    NDRX_DUMP(log_debug, "_exproto_proto2ex enter", 
                    proto_buf, proto_len);
    while (int_pos < proto_len)
    {
        /* Read tag */
        net_tag = read_net_short(proto_buf, &int_pos);
        
        /* Read len */
        net_len = read_net_short(proto_buf, &int_pos);
        /*
        NDRX_LOG(log_debug, "Got tag: %x, got len: %x (%hd)", 
                net_tag, net_len, net_len);
        */
        /* We should understand now what this type of field is...? */
        fld = get_descr_from_tag(cur, net_tag);
        
        if (NULL==fld)
        {
            NDRX_LOG(log_warn, "No descriptor for tag: %x - SKIP!", 
                    net_tag);
        }
        else
        {
            p_fld_len = &fld->len;
         /*   NDRX_LOG(log_debug, "Processing field.... [%s]", fld->cname);*/
            
            /* Verify data length (currently at warning level!) - it should be
             * in range!
             */
            if (net_len<fld->min_len || net_len>fld->max_len)
            {
                NDRX_LOG(log_error, "Experimental verification: WARNING! "
                        "INVALID LEN! tag: 0x%x (%s) "
                        "min_len=%ld max_len=%ld but got: %hd",
                        fld->tag, fld->cname, fld->min_len, fld->max_len, net_len);
                
                NDRX_DUMP(log_debug, "Invalid chunk:", 
                                (char *)(proto_buf + int_pos - 4), 
                                net_len + 4 /* two byte tag, two byte len */);
                /* TODO: Might consider to ret=FAIL; goto out; - 
                 * When all will be debugged!
                 */
            }

            switch (fld->type)
            {
                case XFLD:
                case XSBL:
                {
                    BFLDLEN bfldlen = 0; /* Default zero, used for carray! */
                    loop_keeper = 0;
                    
                    if (SUCCEED!=x_nettoc(fld, proto_buf, int_pos, net_len, 
                            (char *)(ex_buf+ex_len+fld->offset), &bfldlen, debug))
                    {
                        NDRX_LOG(log_error, "Failed to convert from net"
                                " tag: %x!", net_tag);
                        ret=FAIL;
                        goto out;
                    }
                    
                    if (NULL!=p_ub_data && ( 
                            0xb10 == net_tag ||
                            0xb11 == net_tag ||
                            0xb12 == net_tag ||
                            0xb13 == net_tag ||
                            0xb14 == net_tag ||
                            0xb15 == net_tag ||
                            0xb16 == net_tag))
                    {
                        
                        NDRX_LOG(log_debug, "Installing FB field: "
                                "id=%d, len=%d", p_ub_data->bfldid, bfldlen);
                        
                        if (SUCCEED!=Badd(p_x_fb, p_ub_data->bfldid, 
                                p_ub_data->buf, bfldlen))
                        {
                            NDRX_LOG(log_error, "Failed to setup field %s:%s",
                                    Bfname(p_ub_data->bfldid), Bstrerror(Berror));
                            
                            ret=FAIL;
                            goto out;
                        }
                    }
                    
                    NDRX_LOG(6, "net2ex: tag: [0x%x]\t[%s]\t len: %ld (0x%04lx) type:"
                        " [%s]\t data: [%s]"/*" netbuf (data start): %p"*/, 
                        net_tag, fld->cname, net_len, net_len, M_type[fld->fld_type], 
                            debug/*, (proto_buf+int_pos)*/ );
                }
                    break;
                case XSUB:
                {
                    loop_keeper = 0;
                    NDRX_LOG(log_debug, "XSUB");
                    /* This uses sub-table, lets request function to 
                     * find out the table! 
                     */
                    if (NULL==(cv = fld->p_classify_fn(ex_buf, ex_len)))
                    {
                        /* We should have something! */
                        ret=FAIL;
                        goto out;
                    }
                    else
                    {
                        ret = _exproto_proto2ex(cv->tab[level+1], 
                                    (char *)(proto_buf+int_pos), net_len, 
                                    ex_buf, ex_len+fld->offset,
                                    max_struct, level+1, NULL, NULL);
                        
                        if (SUCCEED!=ret)
                        {
                            goto out;
                        }
                        
                    }
                }
                    break;
                    
                case XINC:
                {
                    loop_keeper = 0;
                    NDRX_LOG(log_debug, "XINC");
                    /* Go level deeper & use included table */
                    
                    ret = _exproto_proto2ex(fld->include, 
                                    (char *)(proto_buf+int_pos), net_len, 
                                    ex_buf, ex_len+fld->offset,
                                    max_struct, level+1, NULL, NULL);

                    if (SUCCEED!=ret)
                    {
                        goto out;
                    }
                    NDRX_LOG(log_debug, "return from XINC...");
                }
                    break;
                    
                case XLOOP:
                {
                    NDRX_LOG(log_debug, "XLOOP, array elem: %d", 
                            loop_keeper);
                    ret = _exproto_proto2ex(fld->include, 
                                    (char *)(proto_buf+int_pos), net_len, 
                                    ex_buf, (ex_len+fld->offset + fld->elem_size*loop_keeper),
                                    max_struct, level+1, NULL, NULL);
                    
                    if (SUCCEED!=ret)
                    {
                        goto out;
                    }

                    loop_keeper++;
                }
                    break;
                    
                case XATMIBUF:
                {
                    short *buffer_type = (short *)(ex_buf+ex_len+fld->buftype_offset);
                    long *buf_len = (long *)(ex_buf+ex_len+fld->counter_offset);
                    char *data = (char *)(ex_buf+ex_len+fld->offset);
                    
                    
                    NDRX_LOG(log_debug, "Processing XATMIBUF");
                    
                    if (*buffer_type == BUF_TYPE_UBF)
                    {    
                        UBFH *p_ub = (UBFH *)(ex_buf + ex_len+fld->offset);
                        UBF_header_t *hdr  = (UBF_header_t *)p_ub;
                        int tmp_buf_size = PMSGMAX - ex_len - fld->offset;
                        
                        proto_ufb_fld_t f;
                        
                        NDRX_DUMP(log_debug, "Got UBF buffer", 
                                (char *)(proto_buf+int_pos), net_len);
                                                /* 
                         * Init the FB to max possible size, then we will reduce OK!?
                         */
                        NDRX_LOG(log_debug, "Initial FB size: %d", 
                                tmp_buf_size);
                        
                        if (SUCCEED!=Binit(p_ub, tmp_buf_size))
                        {
                            NDRX_LOG(log_error, "Failed to init FB: %s", 
                                    Bstrerror(Berror) );
                            ret=FAIL;
                            goto out;
                        }
                        
                        /* OK, we have a FB now process field by field...? */
                        ret = _exproto_proto2ex(M_ubf_field,  
                                    (char *)(proto_buf+int_pos), net_len, 
                                    /* Drive over internal variable + we should 
                                     * have callback when data completed, so that
                                     * we can install them in FB! */
                                    (char *)&f, 0,
                                    max_struct, level,
                                    p_ub, &f);
                        
                        if (SUCCEED!=ret)
                        {
                            goto out;
                        }
                        
                        /* Resize FB? */
                        hdr->buf_len = hdr->bytes_used;
                        
                        xatmi_fld_len = hdr->buf_len;
                        p_fld_len = &xatmi_fld_len;
                        
                        *buf_len = hdr->buf_len;
                                
                        Bprint(p_ub);
                        
                        /*  have some debug */
                        {
                            tp_command_call_t *more_debug = 
                                    (tp_command_call_t *)(ex_buf + sizeof(cmd_br_net_call_t));
                            
                            NDRX_LOG(log_debug, "timer = (%ld %ld) %d", 
                                    more_debug->timer.t.tv_sec, 
                                    more_debug->timer.t.tv_nsec,
                                    n_timer_get_delta_sec(&more_debug->timer));
                            NDRX_LOG(log_debug, "callseq  %hd", 
                                    more_debug->callseq);
                            NDRX_LOG(log_debug, "cd  %d", 
                                    more_debug->cd);
                            NDRX_LOG(log_debug, "my_id  [%s]", 
                                    more_debug->my_id);
                            NDRX_LOG(log_debug, "reply_to  [%s]", 
                                    more_debug->reply_to);
                            NDRX_LOG(log_debug, "name  [%s]", 
                                    more_debug->name);
                        }
                        
                    }
                    else
                    {
                        
                        *buf_len = net_len;
                        NDRX_LOG(log_debug, "XATMIBUF - other type buffer, "
                                                "just copy memory... (%d bytes)!", 
                                                *buf_len);
                        /* Just copy off the memory & setup sizes (max offset) */
                        memcpy(data, (char *)(proto_buf+int_pos), *buf_len);
                        
                        xatmi_fld_len = *buf_len;
                        p_fld_len = &xatmi_fld_len;
                    }
                    
                }
                    break;
                
                default:
                    NDRX_LOG(log_error, "Unknown subfield type!");
                    ret=FAIL;
                    break;
            }
            
            /* Increase the max struct size... 
             * TODO: Here maybe tricks with array?
             */
            if (NULL==p_ub_data) /* Do not process in case if we have FB processing.! */
            {
                /*
                NDRX_LOG(log_debug, "I am here! %ld vs %ld", 
                                    fld->offset + ex_len + *p_fld_len, *max_struct);
                */
                if ((fld->offset + ex_len + *p_fld_len) > *max_struct)
                {
                    *max_struct = fld->offset +ex_len+ *p_fld_len;
                    /*
                    NDRX_LOG(log_debug, "max len=>%ld", *max_struct);
                     */
                }
            }
            
        }
        /*
        NDRX_LOG(log_debug, "jump over: %hd", net_len);
        */
        int_pos+=net_len;
    }
    
out:
    return ret;
}


/**
 * Classsify the netcall message (return driver record).
 * @param ex_buf
 * @param ex_len
 * @return xmsg_t ptr or NULL
 */
private xmsg_t * classify_netcall (char *ex_buf, long ex_len)
{
    xmsg_t *cv = M_ndrxd_x;
    cmd_br_net_call_t *msg = (cmd_br_net_call_t *)ex_buf;
    
    /* OK, we should pick up the table and start to conv. */
    while (FAIL!=cv->command)
    {
        if (msg->msg_type == cv->msg_type && msg->command_id == cv->command
                || 'A' == msg->msg_type /* Accept any ATMI - common structure! */)
        {
            NDRX_LOG(log_debug, "Found conv table for: %c/%d/%s", 
                    cv->msg_type, cv->command, cv->descr);
            
            return cv;
        }
        cv++;
    }
    
    NDRX_LOG(log_error, "No conv table for ndrxd command: %d"
            " - FAIL", cv->command);
    
    return NULL;
}
