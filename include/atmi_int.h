/* 
** ATMI internals
**
** @file atmi_int.h
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

#ifndef ATMI_INT_H
#define	ATMI_INT_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <sys_mqueue.h>
#include <xa.h>
#include <atmi.h>
#include <ndrxdcmn.h>
#include <stdint.h>
#include <ntimer.h>
#include <sys/sem.h>
#include <exhash.h>
#include <ndrstandard.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/*
 * List of internal queue errors
 */
#define GEN_QUEUE_ERR_NO_DATA   -2

/*
 * List of ATMI internal protocol commands
 */
#define ATMI_COMMAND_UNKNOWN    0
#define ATMI_COMMAND_TPCALL     1
#define ATMI_COMMAND_TPREPLY    2
#define ATMI_COMMAND_TPFORWARD  3
#define ATMI_COMMAND_CONNECT    4
#define ATMI_COMMAND_CONVDATA   5
#define ATMI_COMMAND_CONNRPLY   6
#define ATMI_COMMAND_DISCONN    7
#define ATMI_COMMAND_CONNUNSOL  8
#define ATMI_COMMAND_CONVACK    9
#define ATMI_COMMAND_SHUTDOWN   10
#define ATMI_COMMAND_EVPOST     11
#define ATMI_COMMAND_SELF_SD    12      /* Self shutdown                */
#define ATMI_COMMAND_TPNOTIFY   13      /* Notification message         */
#define ATMI_COMMAND_BROADCAST  14      /* Broadcast notification       */

/* Call states */
#define CALL_NOT_ISSUED         0
#define CALL_WAITING_FOR_ANS    1
#define CALL_CANCELED           2


/* Global conversation status */
#define CONV_NO_INITATED          0
#define CONV_IN_CONVERSATION      1

/* Translate error code */
#define CONV_ERROR_CODE(_ret, _err) \
            if (EINTR==_ret)\
            {\
                _err=TPSIGRSTRT;\
            }\
            else if (ETIMEDOUT==_ret)\
            {\
                _err=TPETIME;\
            }\
            else\
            {\
                _err=TPEOS;\
            }

#define TPEXIT_ENOENT           150             /* No such file or directory */
    
#define Q_SEND_GRACE            10              /* Number of messages for q to wait to process */
    
/* Even processing */
#define EV_TPEVSUBS         "TPEVSUBS"
#define EV_TPEVUNSUBS       "TPEVUNSUBS"
#define EV_TPEVPOST         "TPEVPOST"
#define EV_TPEVDOPOST       "TPEVDOPOST"
    
/* RECOVERY processing */
#define TPRECOVERSVC        "TPRECOVER"     /* Recovery administrative service */
    
/* Special flags for tpcallex */
#define TPCALL_EVPOST           0x0001          /* Event posting    */
#define TPCALL_BRCALL           0x0002          /* Bridge call      */
#define TPCALL_BROADCAST        0x0004          /* Broadcast call   */

/* XA TM reason codes */
/* Lower reason includes XA error code. */    
#define NDRX_XA_ERSN_BASE            2000
#define NDRX_XA_ERSN_NONE            0      /* No reason specified for error */
#define NDRX_XA_ERSN_LOGFAIL         2001   /* Log failed                    */
#define NDRX_XA_ERSN_INVPARAM        2002   /* Invalid parameters sent to TMS*/
#define NDRX_XA_ERSN_NOTX            2003   /* Transaction not logged        */
#define NDRX_XA_ERSN_PREPFAIL        2004   /* One of the nodes failed to prepare */
#define NDRX_XA_ERSN_RMLOGFAIL       2005   /* New RM logging failed         */
#define NDRX_XA_ERSN_RMCOMMITFAIL    2006   /* Some resource manager failed to commit */
#define NDRX_XA_ERSN_UBFERR          2007   /* UBF Error                     */
#define NDRX_XA_ERSN_RMERR           2008   /* Resource Manager Failed       */

    
#define NDRX_XID_FORMAT_ID  0x6194f7a1L         /* Enduro/X XID format id*/

/* Helpers: */    
#define XA_IS_DYNAMIC_REG       (G_atmi_env.xa_sw->flags & TMREGISTER)

#define NDRX_CONF_MSGSEQ_START       65530   /* Have a high number for wrap test */    
    
/* Memory allocation helpers */
    
/**
 * Allocate buffer, if fail set ATMI error, and goto out
 */
#define NDRX_SYSBUF_ALLOC_WERR_OUT(__buf, __p_bufsz, __ret)  \
{\
    __buf = NDRX_CALLOC(1, (G_atmi_env.msgsize_max>ATMI_MSG_MAX_SIZE?G_atmi_env.msgsize_max:ATMI_MSG_MAX_SIZE));\
    if (NULL==__buf)\
    {\
        int err = errno;\
        _TPset_error_fmt(TPEOS, "%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        NDRX_LOG(log_error, "%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        userlog("%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        errno = err;\
        FAIL_OUT(__ret);\
    }\
    if (NULL!=__p_bufsz)\
    {\
        *((int *)__p_bufsz) = (G_atmi_env.msgsize_max>ATMI_MSG_MAX_SIZE?G_atmi_env.msgsize_max:ATMI_MSG_MAX_SIZE);\
    }\
}
    
 /**
 * Allocate buffer, if fail set ATMI error, and goto out
 */
#define NDRX_SYSBUF_MALLOC_WERR_OUT(__buf, __p_bufsz, __ret)  \
{\
    __buf = NDRX_MALLOC(G_atmi_env.msgsize_max>ATMI_MSG_MAX_SIZE?G_atmi_env.msgsize_max:ATMI_MSG_MAX_SIZE);\
    if (NULL==__buf)\
    {\
        int err = errno;\
        _TPset_error_fmt(TPEOS, "%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        NDRX_LOG(log_error, "%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        userlog("%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        errno = err;\
        FAIL_OUT(__ret);\
    }\
    if (NULL!=__p_bufsz)\
    {\
        *((int *)__p_bufsz) = (G_atmi_env.msgsize_max>ATMI_MSG_MAX_SIZE?G_atmi_env.msgsize_max:ATMI_MSG_MAX_SIZE);\
    }\
}
    
/**
 * Allocate the ATMI system buffer (MALLOC mode, just a hint)
 */
#define NDRX_SYSBUF_MALLOC_OUT(__buf, __p_bufsz, __ret)\
{\
    __buf = NDRX_MALLOC((G_atmi_env.msgsize_max>ATMI_MSG_MAX_SIZE?G_atmi_env.msgsize_max:ATMI_MSG_MAX_SIZE));\
    if (NULL==__buf)\
    {\
        int err = errno;\
        NDRX_LOG(log_error, "%s: failed to allocate sysbuf: %s", __func__,  strerror(errno));\
        userlog("%s: failed to allocate sysbuf: %s", __func__,  strerror(errno));\
        errno = err;\
        FAIL_OUT(__ret);\
    }\
    if (NULL!=__p_bufsz)\
    {\
        *((int *)__p_bufsz) = (G_atmi_env.msgsize_max>ATMI_MSG_MAX_SIZE?G_atmi_env.msgsize_max:ATMI_MSG_MAX_SIZE);\
    }\
}
    

#define NDRX_MSGPRIO_DEFAULT            0 /* Default prioity used by tpcall, tpreturn etc. */
#define NDRX_MSGPRIO_NOTIFY             1 /* Notification is higher prio     */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/**
 * Structure represents typed buffer instance
 */
typedef struct buffer_obj buffer_obj_t;
struct buffer_obj
{
    int type_id;
    int sub_type_id;
    short autoalloc;  /* Is buffer automatically allocated by tpcall? */
    char *buf;
    long size;        /* Allocated size.... */
    /* Move to hash by buf */
    /* buffer_obj_t *prev, *next; */
    EX_hash_handle hh;         /* makes this structure hashable */
};

/*
 * Typed buffer descriptor table
 */
typedef struct typed_buffer_descr typed_buffer_descr_t;
struct typed_buffer_descr
{
    int type_id;
    char *type;
    char *alias;
    char *subtype;
    /*
     * Prepare buffer for outgoing call/reply
     * idata - buffer data
     * ilen - data len (if needed)
     * obuf - place where to copy prepared buffer
     * olen - the actual lenght of data that should sent
     *
     * Error detail should be set by this funcion
     */
    int (*pf_prepare_outgoing) (typed_buffer_descr_t *descr, char *idata, long ilen, char *obuf, long *olen, long flags);

    /*
     * Prepare received buffer for internal processing.
     * May re-allocate the buffer.
     * rcv_data - received data buffer
     * odata - ptr to handler. Existing buffer may be reused or re-allocated
     * olen - output data length
     *
     * Error detail should be set by this function.
     */
    int (*pf_prepare_incoming) (typed_buffer_descr_t *descr, char *rcv_data, long rcv_len, char **odata, long *olen, long flags);

    /*
     * This will allocate buffer memory. Error details should be provided by
     * this function by it self.
     * On error NULL shall be returned.
     */
    char *(*pf_alloc) (typed_buffer_descr_t *descr, long len);

    /*
     * Reallocate memory
     * cur_ptr - pointer to current buffer
     * len - new len that should be set
     *
     * returns NULL on error.
     * Error detail should be set by this function
     */
    char *(*pf_realloc) (typed_buffer_descr_t *descr, char *cur_ptr, long len);

    /*
     * Free up allocate buffer with proper routines!
     */
    void (*pf_free)(typed_buffer_descr_t *descr, char *buf);

    int (*pf_test)(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr);
};

/**
 * Structure for keeping calldescriptor states
 */
struct call_descriptor_state
{
    int cd;
    short status;
    time_t timestamp;
    unsigned callseq;
    long flags; /* call flags associated.. */
};
typedef struct call_descriptor_state call_descriptor_state_t;

/*
 * Structure for holding atmi library configuration and other global
 * variables.
 * 
 * This structure also will hold all environmental configuration for
 * EnduroX!
 */
struct atmi_lib_conf
{
    int is_client; /* Is this client? */
    char reply_q_str[NDRX_MAX_Q_SIZE+1]; /* Provide reply q better debug */
    mqd_t reply_q; /* Reply queue */
    /* queue attributes */
    struct mq_attr q_attr; /* Queue attributes. */
    /*
     * ID string. For example:
     * srv.testsrv-1
     * or
     * clt.testclt-1265
     */
    char my_id[NDRX_MAX_ID_SIZE+1]; /* Id of the resource */

    /* Queue prefix (from environment) */
    char q_prefix[NDRX_MAX_Q_SIZE+1];

    char ndrxd_q_str[NDRX_MAX_Q_SIZE+1]; /* Queue name for ndrxd connection */
    
    long contextid;         /* In which context lib is initialized? */
    
};
typedef struct atmi_lib_conf atmi_lib_conf_t;

/**
 * Common atmi lib environmental configuration
 */
struct atmi_lib_env
{   
    /* Other global settings */
    int     max_servers; /* Max server instance count - CONF_NDRX_SRVMAX*/
    int     max_svcs;   /* Max services per server - CONF_NDRX_SVCMAX*/
    char    rnd_key[NDRX_MAX_KEY_SIZE];   /* random key to be passed to all EnduroX servers in session */
    int     msg_max;    /* maximum number of messages in a posix queue */
    int     msgsize_max;    /* maximum message size for a posix queue */
    key_t   ipckey;            /* IPC Key */
    int     time_out; /* Timeout in seconds to be applied for calls */
    /* Cluster node id */
    int     our_nodeid;
    int     ldbal;  /* Load balance settings */
    int     is_clustered; /* Will we run in cluster mode or not? */
    
    /* <XA Protocol configuration> */
    short xa_rmid;    /* XA Resource ID 1..255 range */
    char xa_open_str[PATH_MAX]; /* XA Open string           */
    char xa_close_str[PATH_MAX];/* XA Close string          */
    char xa_driverlib[PATH_MAX];/* Enduro RM Library/driver */
    char xa_rmlib[PATH_MAX];    /* RM library               */
    int  xa_lazy_init;/* Should we                */
    struct xa_switch_t * xa_sw; /* handler to XA switch */
    /* </XA Protocol configuration> */
    
    int     nrsems; /* number of sempahores for poll() mode of service mem */
    
    int     maxsvcsrvs; /* Max servers per service (only for poll() mode) */
    
    char    qprefix[NDRX_MAX_Q_SIZE+1]; /* Queue prefix (common, finally!) */
    char    qprefix_match[NDRX_MAX_Q_SIZE+1]; /* Includes separator at the end */
    int     qprefix_match_len;              /* Includes number bytes to match */
    char    qpath[PATH_MAX+1]; /* Queue path (common, finally!) */
    
};
typedef struct  atmi_lib_env atmi_lib_env_t;

/*
 * Generic command handler, tp commands.
 */
struct tp_command_generic
{
    /* <standard comms header:> */
    short command_id;
    char proto_ver[4];
    int proto_magic;
    /* </standard comms header> */
};
typedef struct tp_command_generic tp_command_generic_t;

/*
 * Call handler.
 */
struct tp_command_call
{
    /* <standard comms header:> */
    short command_id;
    char proto_ver[4];
    int proto_magic;
    /* </standard comms header> */
    
    short buffer_type_id;
    char name[XATMI_SERVICE_NAME_LENGTH+1];
    char reply_to[NDRX_MAX_Q_SIZE+1];
    /* Zero terminated string... (might contain special symbols)*/
    char callstack[CONF_NDRX_NODEID_COUNT+1];
    char my_id[NDRX_MAX_ID_SIZE]; /* ID of caller */
    long sysflags; /* internal flags of the call */
    int cd;
    int rval;
    long rcode; /* should be preset on reply only */
    char extradata[31+1]; /* Extra char data to be passed over the call */
    long flags; /* should be preset on reply only */
    time_t timestamp; /* provide time stamp of the call */
    unsigned short callseq;
    /* message sequence for conversational over multithreaded bridges*/
    unsigned short msgseq;
    /* call timer so that we do not operate with timed-out calls. */
    ndrx_timer_t timer;    
    
    /* <XA section begin> */
    ATMI_XA_TX_INFO_FIELDS;
    /* <XA section end> */
    
    /* Have a ptr to auto-buffer: */
    buffer_obj_t * autobuf;
    
    long data_len;
    char data[0];
};
typedef struct tp_command_call tp_command_call_t;

/*
 * Notification message
 */
struct tp_notif_call
{
    /* <standard comms header:> */
    short command_id;
    char proto_ver[4];
    int proto_magic;
    /* </standard comms header> */
    
    /* See clientid_t, same fields */
    char destclient[NDRX_MAX_ID_SIZE];      /* Destination client ID */
    
    /* fields from boradcast */
    char nodeid[MAXTIDENT*2]; /* In case of using regex */
    int nodeid_isnull;        /* Is NULL */
    char usrname[MAXTIDENT*2]; /* In case of using regex */
    int usrname_isnull;        /* Is NULL */
    char cltname[MAXTIDENT*2]; /* In case of using regex */
    int cltname_isnull;        /* Is NULL */
    
    short buffer_type_id;
    /* See clientid_t, same fields, end */
    char reply_to[NDRX_MAX_Q_SIZE+1];
    /* Zero terminated string... (might contain special symbols)*/
    char callstack[CONF_NDRX_NODEID_COUNT+1];
    char my_id[NDRX_MAX_ID_SIZE]; /* ID of caller */
    long sysflags; /* internal flags of the call */
    int cd;
    int rval;
    long rcode; /* should be preset on reply only */
    long flags; /* should be preset on reply only */
    time_t timestamp; /* provide time stamp of the call */
    unsigned short callseq;
    /* message sequence for conversational over multithreaded bridges*/
    unsigned short msgseq;
    /* call timer so that we do not operate with timed-out calls. */
    ndrx_timer_t timer;    
    
    /* Have a ptr to auto-buffer: */
    buffer_obj_t * autobuf;
    
    long destnodeid; /* Dest node to which we are sending the msg */
    
    long data_len;
    char data[0];
    
};
typedef struct tp_notif_call tp_notif_call_t;

/**
 * Conversational buffer by message sequence number
 * Needed to keep the messages in memory to have them in order
 * as the muli-threaded bridge can send messages out of the order
 */
typedef struct tpconv_buffer tpconv_buffer_t;
struct tpconv_buffer
{
    int msgseq;
    char *buf;
    size_t size;        /* Allocated size.... */
    
    EX_hash_handle hh;         /* makes this structure hashable */
};


/**
 * Structure for holding conversation details
 */
struct tp_conversation_control
{
    int status;
    int cd;
    long flags;

    char reply_q_str[NDRX_MAX_Q_SIZE+1]; /* Reply back queue string */
    mqd_t reply_q; /* Reply back queue (file descriptor) - should be already open. */
    struct mq_attr reply_q_attr; /* Reply queue attributes. */

    char my_listen_q_str[NDRX_MAX_Q_SIZE+1]; /* Reply back queue string */
    mqd_t my_listen_q; /* Queue on which we are going to wait for msg */
    struct mq_attr my_q_attr; /* My listening queue attributes. */
    time_t timestamp;
    unsigned short callseq; /* Call/conv sequence number */
    
    /* message sequence number (from our side to their) 
     * Basically this is message number we are sending them
     * The other side will follow the incremental order of the messages.
     */
    unsigned short msgseqout;  /* Next number to send */
    unsigned short msgseqin;  /* incoming message sequence number, expecting num */
    int rval; /* when tpreturn took a place */
    long rcode; /* when tpreturn took a place */
    long revent; /* Last event occurred at channel */
    short handshaked;
    
    tpconv_buffer_t *out_of_order_msgs; /* hash for out of the order messages */
    
};
typedef struct tp_conversation_control tp_conversation_control_t;


/* Simple command to get ACK back when we have sent something */
struct tp_conv_ack
{
    short command_id;
    int cd;
};
typedef struct tp_conv_ack tp_conv_ack_t;


/**
 * Structure to keep the list of extended FDs.
 */
typedef struct pollextension_rec pollextension_rec_t;
struct pollextension_rec
{
    int fd; /* FD that is polled. */
    void *ptr1; /* Used defined pointer to be holded in extension  */
    int (*p_pollevent)(int fd, uint32_t events, void *ptr1); /* Event callback */
    uint32_t events; /*  Events subscribed to. */
    pollextension_rec_t *prev, *next;
};

/**
 * Type used for service listing LL return.
 */
typedef struct atmi_svc_list atmi_svc_list_t;
struct atmi_svc_list
{
    /* Service name */
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    
    /* Linked list */
    atmi_svc_list_t *next;
};

/**
 * Enduro/X Queue details
 * parsed all attributes of the queue in single struct
 * Used only those attribs which match the q type.
 * Currently holds only those fields which are needed for certain system
 * functions, for example client reply Q details for tpbroadcast..
 */
struct ndrx_qdet
{
    int qtype; /* queue type, see  NDRX_QTYPE_* */
    char qprefix[NDRX_MAX_Q_SIZE+1]; 
    char binary_name[MAXTIDENT+2];
    pid_t pid;
    long contextid;
};

typedef struct ndrx_qdet ndrx_qdet_t;

/* This may have some issues with alignment and this make
 * actual size smaller than 1 char */
#define MAX_CALL_DATA_SIZE (ATMI_MSG_MAX_SIZE-sizeof(tp_command_call_t))

/*---------------------------Globals------------------------------------*/
extern NDRX_API atmi_lib_env_t G_atmi_env; /* global access to env configuration */
extern NDRX_API int G_srv_id;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* Utilities */
extern NDRX_API int ndrx_load_common_env(void);
extern NDRX_API int ndrx_load_new_env(char *file);
extern NDRX_API int generic_q_send(char *queue, char *data, long len, long flags, unsigned int msg_prio);
extern NDRX_API int generic_q_send_2(char *queue, char *data, long len, long flags, long tout, unsigned int msg_prio);
extern NDRX_API int generic_qfd_send(mqd_t q_descr, char *data, long len, long flags);
extern NDRX_API long generic_q_receive(mqd_t q_descr, char *buf, long buf_max, unsigned *prio, long flags);
extern NDRX_API int ndrx_get_q_attr(char *q, struct mq_attr *p_att);

extern NDRX_API mqd_t ndrx_mq_open_at(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
extern NDRX_API mqd_t ndrx_mq_open_at_wrp(const char *name, int oflag);
extern NDRX_API void ndrx_mq_fix_mass_send(int *cntr);
extern NDRX_API int ndrx_q_setblock(mqd_t q_descr, int blocked);

extern NDRX_API void br_dump_nodestack(char *stack, char *msg);
extern NDRX_API int fill_reply_queue(char *nodestack, 
            char *org_reply_to, char *reply_to);

extern NDRX_API int ndrx_cvnq_parse_client(char *qname, TPMYID *p_myid);
extern NDRX_API int ndrx_cvnq_parse_server(char *qname, TPMYID *p_myid_first,  
        TPMYID *p_myid_second);
extern NDRX_API int ndrx_myid_parse(char *my_id, TPMYID *out, int iscnv_initator);
extern NDRX_API int ndrx_myid_parse_clt(char *my_id, TPMYID *out, int iscnv_initator);
extern NDRX_API int ndrx_myid_parse_srv(char *my_id, TPMYID *out, int iscnv_initator);
extern NDRX_API int ndrx_myid_is_alive(TPMYID *p_myid);
extern NDRX_API void ndrx_myid_dump(int lev, TPMYID *p_myid, char *msg);
extern NDRX_API int ndrx_myid_convert_to_q(TPMYID *p_myid, char *rply_q, int rply_q_buflen);

extern NDRX_API int ndrx_myid_convert_from_qdet(TPMYID *p_myid, ndrx_qdet_t *qdet, long nodeid);
extern NDRX_API void ndrx_myid_to_my_id_str(TPMYID *p_myid, char *my_id);
extern NDRX_API int ndrx_qdet_parse_cltqstr(ndrx_qdet_t *qdet, char *qstr);
extern NDRX_API void ndrx_qdet_dump(int lev, ndrx_qdet_t *qdet, char *msg);

extern NDRX_API int ndrx_atmiutil_init(void);

/* Base64 encode/decode with file system valid output */
extern NDRX_API char * atmi_xa_base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data);
    
extern NDRX_API unsigned char *atmi_xa_base64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data);

/* ATMI calls */
extern NDRX_API int _tpacall (char *svc, char *data,
               long len, long flags, char *extradata, int dest_node, int ex_flags,
                TPTRANID *p_tran);
extern NDRX_API int _tpnotify(CLIENTID *clientid, TPMYID *p_clientid_myid, 
        char *cltq, /* client q already built by broadcast */
        char *data, long len, long flags, 
        int dest_node, char *usrname,  char *cltname, char *nodeid, /* RFU */
        int ex_flags);
extern int _tpchkunsol(void);
extern NDRX_API void ndrx_process_notif(char *buf, long len);
extern NDRX_API char * _tprealloc (char *buf, long len);
extern NDRX_API long	_tptypes (char *ptr, char *type, char *subtype);
extern NDRX_API char * _tpalloc (typed_buffer_descr_t *known_type,
                    char *type, char *subtype, long len);
extern NDRX_API void free_auto_buffers(void);
extern NDRX_API int tp_internal_init(atmi_lib_conf_t *init_data);
extern NDRX_API int tp_internal_init_upd_replyq(mqd_t reply_q, char *reply_q_str);
extern NDRX_API void tp_thread_shutdown(void *ptr, int *p_finish_off);
extern NDRX_API void ndrx_dump_call_struct(int lev, tp_command_call_t *call);
extern NDRX_API unsigned short ndrx_get_next_callseq_shared(void);

extern NDRX_API int _tpsend (int cd, char *data, long len, long flags, long *revent,
                            short command_id);
extern NDRX_API void _tpfree (char *buf, buffer_obj_t *known_buffer);
extern NDRX_API int _tpisautobuf (char *buf);
extern NDRX_API void cancel_if_expected(tp_command_call_t *call);
/* Functions for conversation */
extern NDRX_API int accept_connection(void);
extern NDRX_API int svc_fail_to_start(void);
extern NDRX_API int normal_connection_shutdown(tp_conversation_control_t *conv, int killq);
extern NDRX_API int close_open_client_connections(void);
extern NDRX_API int have_open_connection(void);
extern NDRX_API int get_ack(tp_conversation_control_t *conv, long flags);

/* Extended version of tpcall, accepts extradata (31+1) symbols */
extern NDRX_API int tpcallex (char *svc, char *idata, long ilen,
                char * *odata, long *olen, long flags,
                char *extradata, int dest_node, int ex_flags);

extern NDRX_API int tpacallex (char *svc, char *data, 
        long len, long flags, char *extradata, int dest_node, int is_evpost);
/* event API implementation */
extern NDRX_API long _tpunsubscribe(long subscription, long flags);
extern NDRX_API long _tpsubscribe(char *eventexpr, char *filter, TPEVCTL *ctl, long flags);
extern NDRX_API int _tppost(char *eventname, char *data, long len, long flags);
extern NDRX_API void	tpext_configbrige 
    (int nodeid, int flags, int (*p_qmsg)(char *buf, int len, char msg_type));
extern NDRX_API int _get_evpost_sendq(char *send_q, size_t send_q_bufsz, char *extradata);

extern NDRX_API char * atmi_base64_encode(unsigned char *data, size_t input_length, 
        size_t *output_length, char *encoded_data);
extern NDRX_API unsigned char *atmi_base64_decode(const char *data, size_t input_length, 
        size_t *output_length, char *decoded_data);

extern NDRX_API int _tpjsontoubf(UBFH *p_ub, char *buffer);
extern NDRX_API int _tpubftojson(UBFH *p_ub, char *buffer, int bufsize);
extern NDRX_API int _tpcall (char *svc, char *idata, long ilen,
                char * *odata, long *olen, long flags,
                char *extradata, int dest_node, int ex_flags);
extern NDRX_API int _tpgetrply (int *cd,
                       int cd_exp,
                       char * *data ,
                       long *len, long flags,
                       TPTRANID *p_tranid);
extern NDRX_API int _tpcancel (int cd);
extern NDRX_API int _tpterm (void);
extern NDRX_API int _tpconnect (char *svc, char *data, long len, long flags);
extern NDRX_API int _tprecv (int cd, char * *data, 
                        long *len, long flags, long *revent,
                        short *command_id);
extern NDRX_API int _tpdiscon (int cd);
extern NDRX_API int _tpenqueue (char *qspace, short nodeid, short srvid, char *qname, TPQCTL *ctl, 
        char *data, long len, long flags);
extern NDRX_API int _tpdequeue (char *qspace, short nodeid, short srvid, char *qname, TPQCTL *ctl, 
        char **data, long *len, long flags);

extern NDRX_API void _tpfreectxt(TPCONTEXT_T context);

/* debug logging */
extern NDRX_API int _tplogsetreqfile(char **data, char *filename, char *filesvc);
extern NDRX_API int _tploggetbufreqfile(char *data, char *filename, int bufsize);
extern NDRX_API int _tplogdelbufreqfile(char *data);
extern NDRX_API void _tplogprintubf(int lev, char *title, UBFH *p_ub);

/* ATMI level process management: */
extern NDRX_API int ndrx_chk_server(char *procname, short srvid);
extern NDRX_API int ndrx_chk_ndrxd(void);
extern NDRX_API int ndrx_down_sys(char *qprefix, char *qpath, int is_force);
extern NDRX_API int ndrx_killall(char *mask);
extern NDRX_API int ndrx_q_exists(char *qpath);
extern NDRX_API int ndrx_get_cached_svc_q(char *q);

/* Access to symbols: */

extern NDRX_API tp_command_call_t *ndrx_get_G_last_call(void);
extern NDRX_API atmi_lib_conf_t *ndrx_get_G_atmi_conf(void);
extern NDRX_API atmi_lib_env_t *ndrx_get_G_atmi_env(void);
extern NDRX_API tp_conversation_control_t *ndrx_get_G_accepted_connection(void);

#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_INT_H */

