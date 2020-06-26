/**
 * @brief ATMI internals
 *
 * @file atmi_int.h
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
#include <nstopwatch.h>
#include <sys/sem.h>
#include <exhash.h>
#include <ndrstandard.h>
#include <userlog.h>
#include <tperror.h>
#include <exparson.h>
#include <tmenv.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#define NDRX_API_EXPORT __declspec(dllexport)
#else
#define NDRX_API_EXPORT
#endif

/*
 * List of internal queue errors
 */
#define GEN_QUEUE_ERR_NO_DATA   -2

    
/* Special queue logical numbers used by atmi server */
#define ATMI_SRV_ADMIN_Q            0           /**< This is admin queue */
#define ATMI_SRV_REPLY_Q            1           /**< This is reply queue */
#define ATMI_SRV_Q_ADJUST           2           /**< Adjustment for Q nr */
#define ATMI_SRV_SVC                3           /**< Normal service Q    */

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
/* #define ATMI_COMMAND_EVPOST     11 */
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
            else if (EAGAIN==_ret)\
            {\
                _err=TPEBLOCK;\
            }\
            else\
            {\
                _err=TPEOS;\
            }

#define TPEXIT_ENOENT           150             /* No such file or directory */
    
#define Q_SEND_GRACE            10              /* Number of messages for q to wait to process */
    
/* Even processing */
#define EV_TPEVSUBS         "TPEVSUBS%03hd"
#define EV_TPEVUNSUBS       "TPEVUNSUBS%03hd"
#define EV_TPEVPOST         "TPEVPOST%03hd"
#define EV_TPEVDOPOST       "TPEVDOPOST%03hd"
    
/* RECOVERY processing */
#define TPRECOVERSVC        "TPRECOVER"     /* Recovery administrative service */
    
/* Special flags for tpcallex */
#define TPCALL_EVPOST               0x0001 /**< Event posting                 */
#define TPCALL_BRCALL               0x0002 /**< Bridge call                   */
#define TPCALL_BROADCAST            0x0004 /**< Broadcast call                */

/* XA TM reason codes */
/* Lower reason includes XA error code. */    
#define NDRX_XA_ERSN_BASE           2000
#define NDRX_XA_ERSN_NONE           0      /**< No reason specified for error */
#define NDRX_XA_ERSN_LOGFAIL        2001   /**< Log failed                    */
#define NDRX_XA_ERSN_INVPARAM       2002   /**< Invalid parameters sent to TMS*/
#define NDRX_XA_ERSN_NOTX           2003   /**< Transaction not logged        */
#define NDRX_XA_ERSN_PREPFAIL       2004   /**< One of the nodes failed to prepare */
#define NDRX_XA_ERSN_RMLOGFAIL      2005   /**< New RM logging failed         */
#define NDRX_XA_ERSN_RMCOMMITFAIL   2006   /**< Some resource manager failed to commit */
#define NDRX_XA_ERSN_UBFERR         2007   /**< UBF Error                     */
#define NDRX_XA_ERSN_RMERR          2008   /**< Resource Manager Failed       */

    
#define NDRX_XID_FORMAT_ID  0x6194f7a1L    /**< Enduro/X XID format id        */
#define NDRX_XID_TRID_LEN   sizeof(exuuid_t)
#define NDRX_XID_BQUAL_LEN  sizeof(unsigned char) + sizeof(short) + sizeof(short)

/* Helpers: */    
#define XA_IS_DYNAMIC_REG       (G_atmi_env.xa_sw->flags & TMREGISTER)
#define NDRX_CONF_MSGSEQ_START       65530   /* Have a high number for wrap test */    
    
/* Memory allocation helpers */

/**
 * Allocate buffer, if fail set UBF error, and goto out
 */
#define NDRX_SYSBUF_MALLOC_WERR_OUT(__buf, __p_bufsz, __ret)  \
{\
    int __buf_size__ = NDRX_MSGSIZEMAX;\
    __buf = NDRX_FPMALLOC(__buf_size__, NDRX_FPSYSBUF);\
    if (NULL==__buf)\
    {\
        int err = errno;\
        ndrx_TPset_error_fmt(TPEOS, "%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        NDRX_LOG(log_error, "%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        userlog("%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        errno = err;\
        EXFAIL_OUT(__ret);\
    }\
    __p_bufsz = __buf_size__;\
}
    
#define NDRX_MSGPRIO_DEFAULT            0 /* Default prioity used by tpcall, tpreturn etc. */
#define NDRX_MSGPRIO_NOTIFY             1 /* Notification is higher prio            */

#define NDRX_XA_FLAG_NOJOIN  "NOJOIN"  /**< XA Switch does not support TMJOIN mode  */
#define NDRX_XA_FLAG_NOSTARTXID  "NOSTARTXID"  /**< No XID in start call to RM  */
#define NDRX_XA_FLAG_RECON  "RECON"    /**< Reconnect on tpbegin(), xa_start() if fails */
#define NDRX_XA_FLAG_RECON_TEST  "RECON:"  /**< Test the line                       */
#define NDRX_XA_FLAGS_RECON_RETCODES_BUFSZ  32 /**< List of error codes for retry   */
    
/**
 * Internal system flags
 * @defgroup xa_flags_sys
 * @{
 */
#define NDRX_XA_FLAG_SYS_NOAPISUSP      0x00000001  /**< No tran susp in contexting */
#define NDRX_XA_FLAG_SYS_NOJOIN         0x00000002  /**< No join supported          */
#define NDRX_XA_FLAG_SYS_NOSTARTXID     0x00000004  /**< No XID given in start      */

/** @} */ /* xa_flags_sys */
    
#define NDRX_BANNER(X) \
    if (NULL==getenv(CONF_NDRX_SILENT))\
    {\
        if (X[0])\
        {\
            fprintf(stderr, "%s\n\n", X);\
        }\
        fprintf(stderr, "%s, build %s %s, using %s for %s (%ld bits)\n\n", NDRX_VERSION, \
                        __DATE__, __TIME__, ndrx_epoll_mode(), NDRX_BUILD_OS_NAME, sizeof(void *)*8);\
        fprintf(stderr, "Enduro/X Middleware Platform for Distributed Transaction Processing\n");\
        fprintf(stderr, "Copyright (C) 2009-2016 ATR Baltic Ltd.\n");\
        fprintf(stderr, "Copyright (C) 2017-2020 Mavimax Ltd. All Rights Reserved.\n\n");\
        fprintf(stderr, "This software is released under one of the following licenses:\n");\
        fprintf(stderr, "AGPLv3 (with Java and Go exceptions) or Mavimax license for commercial use.\n\n");\
    }

/** Used by NDRX_SYSFLAGS env variable, Full application start */
#define NDRX_PRC_SYSFLAGS_FULLSTART     0x00000001

/** xadmin error format */
#define NDRX_XADMIN_ERR_FMT_PFX         "ERROR ! "

/** EINVAL on queues: */
#define NDRX_QERR_MSG_EINVAL NDRX_XADMIN_ERR_FMT_PFX \
    "Invalid queue config, see \"Enduro/X Administration Manual\" (ex_adminman), chapter 2."
    
/** ENOSPC on queues: */
#define NDRX_QERR_MSG_ENOSPC NDRX_XADMIN_ERR_FMT_PFX \
    "Insufficient space for the creation of a new message queue"
    
/** Generic queue error */
#define NDRX_QERR_MSG_SYSERR NDRX_XADMIN_ERR_FMT_PFX \
    "Queue error, check logs"
    
/** The 31 bit on, indicates that connection is servers accept one */
#define NDRX_CONV_SRVMASK       0x40000000
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/**
 * Structure represents typed buffer instance
 */
typedef struct buffer_obj buffer_obj_t;
struct buffer_obj
{
    int type_id;
    /** int sub_type_id; */
    char subtype[XATMI_SUBTYPE_LEN+1];
    short autoalloc;  /**< Is buffer automatically allocated by tpcall? */
    char *buf;        /**< allocated datat block                        */
    long size;        /**< Allocated size....                           */
    char *callinfobuf;/**< Call info allocated buffer                   */
    long callinfobuf_len; /**< buffer len                               */
    /* Move to hash by buf */
    /* buffer_obj_t *prev, *next; */
    EX_hash_handle hh;         /**< makes this structure hashable */
};

/**
 * Typed buffer descriptor table. Type of \ref typed_buffer_descr
 */
typedef struct typed_buffer_descr typed_buffer_descr_t;

/**
 * Buffer type descriptor structure
 */
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
    int (*pf_prepare_outgoing) (typed_buffer_descr_t *descr, char *idata, 
        long ilen, char *obuf, long *olen, long flags);

    /*
     * Prepare received buffer for internal processing.
     * May re-allocate the buffer.
     * rcv_data - received data buffer
     * odata - ptr to handler. Existing buffer may be reused or re-allocated
     * olen - output data length
     *
     * Error detail should be set by this function.
     */
    int (*pf_prepare_incoming) (typed_buffer_descr_t *descr, char *rcv_data, 
        long rcv_len, char **odata, long *olen, long flags);

    /*
     * This will allocate buffer memory. Error details should be provided by
     * this function by it self.
     * On error NULL shall be returned.
     */
    char *(*pf_alloc) (typed_buffer_descr_t *descr, char *subtype, long *len);

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

/**
 * Structure for holding atmi library configuration and other global
 * variables.
 * 
 * This structure also will hold all environmental configuration for
 * EnduroX!
 */
struct atmi_lib_conf
{
    /** Is this client? */
    int is_client;
    /** Provide reply q better debug */
    char reply_q_str[NDRX_MAX_Q_SIZE+1];
    /** Reply queue */
    mqd_t reply_q;
    /** queue attributes - of replyq*/
    struct mq_attr reply_q_attr;
    /**
     * ID string. For example:
     * srv.testsrv-1
     * or
     * clt.testclt-1265
     */
    char my_id[NDRX_MAX_ID_SIZE+1];

    /** Queue prefix (from environment) */
    char q_prefix[NDRX_MAX_Q_SIZE+1];

    /** Queue name for ndrxd connection */
    char ndrxd_q_str[NDRX_MAX_Q_SIZE+1]; /* Queue name for ndrxd connection */
    
    /** In which context lib is initialized? */
    long contextid;
    
};
/** typedef for atmi_lib_conf structure */
typedef struct atmi_lib_conf atmi_lib_conf_t;

/**
 * Common atmi lib environmental configuration
 */
struct atmi_lib_env
{   
    /* Other global settings */
    int     max_servers; /**< Max server instance count - CONF_NDRX_SRVMAX  */
    int     max_svcs;    /**< Max services per server - CONF_NDRX_SVCMAX    */
    int     max_clts;    /**< Max number of CPMSRV clients                  */
    char    rnd_key[NDRX_MAX_KEY_SIZE];   /**< random key to be passed to all EnduroX servers in session */
    int     msg_max;     /**< maximum number of messages in a posix queue   */
    int     msgsize_max; /**< maximum message size for a posix queue        */
    key_t   ipckey;      /**< IPC Key                                       */
    int     time_out;    /**< Timeout in seconds to be applied for calls    */
    int     our_nodeid;  /**< Cluster node id                               */
    int     ldbal;       /**< Load balance settings                         */
    int     is_clustered;/**< Will we run in cluster mode or not?           */
    
    /**
     * @defgroup xa_params XA configuration parameters
     * @{
     */
    short xa_rmid;    /**< XA Resource ID 1..255 range */
    char xa_open_str[PATH_MAX]; /**< XA Open string           */
    char xa_close_str[PATH_MAX];/**< XA Close string          */
    char xa_driverlib[PATH_MAX];/**< Enduro RM Library/driver */
    char xa_rmlib[PATH_MAX];    /**< RM library               */
    int  xa_lazy_init;          /**< Should we  init lately?  */
    char xa_flags[PATH_MAX];    /**< Specific flags for XA    */
    long xa_flags_sys;          /**< Internal driver specfic flags */
    struct xa_switch_t * xa_sw; /**< handler to XA switch     */
    
    char xa_recon_retcodes[NDRX_XA_FLAGS_RECON_RETCODES_BUFSZ];
    int xa_recon_times;         /**< Number of times to retry the recon    */
    long xa_recon_usleep;       /**< Microseconds to sleep between retries */
    
    int (*pf_xa_loctxabort)(XID *xid, long flags); /**< Local abort function  */
    
    void* (*pf_getconn)(void);  /**< Return connection object  */
    
    /**@}*/
    
    int     nrsems; /**< number of sempahores for poll() mode of service mem    */
    int     maxsvcsrvs; /**< Max servers per service (only for poll() mode)     */
    int     max_normwait; /**< Max number of attempts for busy context of ndrxd */
    char    qprefix[NDRX_MAX_Q_SIZE+1]; /**< Queue prefix (common, finally!)    */
    char    qprefix_match[NDRX_MAX_Q_SIZE+1]; /**< Includes separator at the end*/
    int     qprefix_match_len;              /**< Includes number bytes to match */
    char    qpath[PATH_MAX+1]; /**< Queue path (common, finally!)               */
    char    ndrxd_pidfile[PATH_MAX];    /**< ndrxd pid file                     */
    ndrx_env_priv_t integpriv;    /**< integration  private data                */
    
};
typedef struct  atmi_lib_env atmi_lib_env_t;

/**
 * Generic command handler, tp commands.
 */
struct tp_command_generic
{
    /* <standard comms header:> */
#if defined(EX_USE_SYSVQ) || defined(EX_USE_SVAPOLL)
    long mtype; /* mandatory for System V queues */
#endif
    short command_id;
    char proto_ver[4];
    int proto_magic;
    /* </standard comms header> */
};
typedef struct tp_command_generic tp_command_generic_t;

/**
 * Call handler.
 * For storing the tppost associated timestamp, we could allow data to be installed
 * in the rval/rcode for requests...
 */
struct tp_command_call
{
    /* <standard comms header:> */
#if defined(EX_USE_SYSVQ) || defined(EX_USE_SVAPOLL)
    long mtype; /* mandatory for System V queues */
#endif
    short command_id;
    char proto_ver[4];
    int proto_magic;
    /* </standard comms header> */
    
    short buffer_type_id;
    char name[XATMI_SERVICE_NAME_LENGTH+1];
    char reply_to[NDRX_MAX_Q_SIZE+1];
    /** Zero terminated string... (might contain special symbols)*/
    char callstack[CONF_NDRX_NODEID_COUNT+1];
    char my_id[NDRX_MAX_ID_SIZE+1]; /* ID of caller */
    long sysflags; /* internal flags of the call */
    int cd;
    /* User1 field in request: */
    int rval;   /* This also should be present only on reply... */
    /* User2 field in request: */
    long rcode; /* should be preset on reply only */
    int user3;  /* user field 3, request */
    long user4; /* user field 4, request */
    int clttout; /* client process timeout setting */
    /** Extended size for storing cache updates in format
     * @CD002/Flgs/SERVICENAMEXXXXXXXXXXXXXXXXXXX
     * @CA002//SERVICENAMEXXXXXXXXXXXXXXXXXXX
     * where @CA -> Cache Add, @CD -> Cache delete, 002 -> source node id
     * Flgs -> max 4 flags ascii letters. And service name
     */
    char extradata[XATMI_EVENT_MAX+1]; /* Extra char data to be passed over the call */
    long flags; /* should be preset on reply only */
    time_t timestamp; /* provide time stamp of the call */
    unsigned short callseq;
    /** message sequence for conversational over multithreaded bridges*/
    unsigned short msgseq;
    /** call timer so that we do not operate with timed-out calls. */
    ndrx_stopwatch_t timer;    
    
    /* <XA section begin> */
    ATMI_XA_TX_INFO_FIELDS;
    /* <XA section end> */
    
    /* Have a ptr to auto-buffer: */
    buffer_obj_t * autobuf;
    
#if EX_SIZEOF_LONG == 4
    /* we need data to aligned to 8 */
    long padding1;
#endif

    /* Payload: */
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
#if defined(EX_USE_SYSVQ) || defined(EX_USE_SVAPOLL)
    long mtype; /* mandatory for System V queues */
#endif
    short command_id;
    char proto_ver[4];
    int proto_magic;
    /* </standard comms header> */
    
    /* See clientid_t, same fields */
    char destclient[NDRX_MAX_ID_SIZE+1];      /**< Destination client ID */
    
    /* fields from boradcast */
    char nodeid[MAXTIDENT*2];  /**< In case of using regex */
    int nodeid_isnull;         /**< Is NULL */
    char usrname[MAXTIDENT*2]; /**< In case of using regex */
    int usrname_isnull;        /**< Is NULL */
    char cltname[MAXTIDENT*2]; /**< In case of using regex */
    int cltname_isnull;        /**< Is NULL */
    
    short buffer_type_id;
    /** See clientid_t, same fields, end */
    char reply_to[NDRX_MAX_Q_SIZE+1];
    /** Zero terminated string... (might contain special symbols)*/
    char callstack[CONF_NDRX_NODEID_COUNT+1];
    char my_id[NDRX_MAX_ID_SIZE+1]; /**< ID of caller */
    long sysflags; /**< internal flags of the call */
    int cd;
    int rval; /**< on request -> userfield1 */
    long rcode; /**< should be preset on reply only, on request -> userfield2 */
    long flags; 
    time_t timestamp; /**< provide time stamp of the call */
    unsigned short callseq;
    /** message sequence for conversational over multithreaded bridges*/
    unsigned short msgseq;
    /** call timer so that we do not operate with timed-out calls. */
    ndrx_stopwatch_t timer;    
    
    buffer_obj_t * autobuf; /**< Have a ptr to auto-buffer: */
    
    long destnodeid; /**< Dest node to which we are sending the msg */
    
#if EX_SIZEOF_LONG == 4
    /* we need data to aligned to 8 */
    long padding1;
#endif

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
    size_t size;        /**< Allocated size....                 */
    
    EX_hash_handle hh;  /**< makes this structure hashable      */
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
    
    /** 
     * message sequence number (from our side to their) 
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

/**
 * tpcall() cache control structure - additional data to tpacall for providing
 * results back if cache lookup is done (i.e. service is present and 
 * result is found. Needed for cases to detect actual service existence
 * if service does not exists, then cache will not provide any results back.
 * because cache is transparent and shall not interfere with logic if service
 * does not exists.
 */
struct ndrx_tpcall_cache_ctl
{
    int should_cache;           /**< should we cache response?              */
    int cached_rsp;             /**< data is in cache, respond with them    */
    int saved_tperrno;
    long saved_tpurcode;
    long *olen;
    char **odata;
};
typedef struct ndrx_tpcall_cache_ctl ndrx_tpcall_cache_ctl_t;


/* This may have some issues with alignment and this make
 * actual size smaller than 1 char */
#define MAX_CALL_DATA_SIZE (NDRX_MSGSIZEMAX-sizeof(tp_command_call_t))


/* Indicators for  tpexportex() and tpimportex() */
struct ndrx_expbufctl
{
    char buftype[XATMI_TYPE_LEN+1];
    short buftype_ind;

    char subtype[XATMI_SUBTYPE_LEN+1];
    short subtype_ind;

    double version;
    short version_ind;

    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    short svcnm_ind;

    int rval;
    short rval_ind;

    long rcode;
    short rcode_ind;

    int tperror;
    short tperror_ind;

    char tpstrerror[MAX_TP_ERROR_LEN+1];
    short tpstrerror_ind;
};

typedef struct ndrx_expbufctl ndrx_expbufctl_t;

/*---------------------------Globals------------------------------------*/
extern NDRX_API atmi_lib_env_t G_atmi_env; /* global access to env configuration */
extern NDRX_API int G_srv_id;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* Utilities */
extern NDRX_API int ndrx_load_common_env(void);
extern NDRX_API long ndrx_ctxid_op(int make_free, long ctxid);
extern NDRX_API int ndrx_load_new_env(char *file);
extern NDRX_API int ndrx_generic_q_send(char *queue, char *data, long len, long flags, unsigned int msg_prio);
extern NDRX_API int ndrx_generic_q_send_2(char *queue, char *data, long len, long flags, long tout, unsigned int msg_prio);
extern NDRX_API int ndrx_generic_qfd_send(mqd_t q_descr, char *data, long len, long flags);
extern NDRX_API ssize_t ndrx_generic_q_receive(mqd_t q_descr, char *q_str, 
        struct mq_attr *reply_q_attr,
        char *buf, long buf_max, 
        unsigned *prio, long flags);
    
extern NDRX_API int ndrx_get_q_attr(char *q, struct mq_attr *p_att);
extern NDRX_API int ndrx_setup_queue_attrs(struct mq_attr *p_q_attr,
                                mqd_t listen_q,
                                char *listen_q_str, 
                                long flags);
extern NDRX_API mqd_t ndrx_mq_open_at(char *name, int oflag, mode_t mode, struct mq_attr *attr);
extern NDRX_API mqd_t ndrx_mq_open_at_wrp(char *name, int oflag);
extern NDRX_API void ndrx_tptoutset(int tout);
extern NDRX_API int ndrx_tptoutget();
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
extern NDRX_API int ndrx_q_type_get(char *q);

extern NDRX_API int ndrx_atmiutil_init(void);

/* ATMI calls */
extern NDRX_API int ndrx_tpacall (char *svc, char *data,
               long len, long flags, char *extradata, int dest_node, int ex_flags,
               TPTRANID *p_tran, int user1, long user2, int user3, long user4,
               ndrx_tpcall_cache_ctl_t *p_cachectl);
extern NDRX_API int ndrx_tpnotify(CLIENTID *clientid, TPMYID *p_clientid_myid, 
        char *cltq, /* client q already built by broadcast */
        char *data, long len, long flags, 
        int dest_node, char *nodeid, char *usrname,  char *cltname,
        int ex_flags);
extern NDRX_API int ndrx_tpchkunsol(void);
extern NDRX_API int ndrx_add_to_memq(char **pbuf, size_t pbuf_len, ssize_t rply_len);
extern NDRX_API int ndrx_tpbroadcast_local(char *nodeid, char *usrname, char *cltname,
        char *data,  long len, long flags, int dispatch_local);
extern NDRX_API void ndrx_process_notif(char *buf, ssize_t len);
extern NDRX_API char * ndrx_tprealloc (char *buf, long len);
extern NDRX_API long	ndrx_tptypes (char *ptr, char *type, char *subtype);
extern NDRX_API char * ndrx_tpalloc (typed_buffer_descr_t *known_type,
                    char *type, char *subtype, long len);
extern NDRX_API void free_auto_buffers(void);
extern NDRX_API int tp_internal_init(atmi_lib_conf_t *init_data);
extern NDRX_API int tp_internal_init_upd_replyq(mqd_t reply_q, char *reply_q_str);
extern NDRX_API void tp_thread_shutdown(void *ptr, int *p_finish_off);
extern NDRX_API void ndrx_dump_call_struct(int lev, tp_command_call_t *call);
extern NDRX_API int ndrx_tpcall_init_once(void);
extern NDRX_API unsigned short ndrx_get_next_callseq_shared(void);

extern NDRX_API int ndrx_tpsend (int cd, char *data, long len, long flags, long *revent,
                            short command_id);
extern NDRX_API void ndrx_tpfree (char *buf, buffer_obj_t *known_buffer);
extern NDRX_API int ndrx_tpisautobuf (char *buf);
extern NDRX_API void cancel_if_expected(tp_command_call_t *call);
/* Functions for conversation */
extern NDRX_API int accept_connection(void);
extern NDRX_API int svc_fail_to_start(void);
extern NDRX_API int normal_connection_shutdown(tp_conversation_control_t *conv, 
        int killq, char *dbgmsg);
extern NDRX_API int close_open_client_connections(void);
extern NDRX_API int have_open_connection(void);
extern NDRX_API int ndrx_get_ack(tp_conversation_control_t *conv, long flags);

/* Extended version of tpcall, accepts extradata (31+1) symbols */
extern NDRX_API int tpcallex (char *svc, char *idata, long ilen,
                char * *odata, long *olen, long flags,
                char *extradata, int dest_node, int ex_flags,
                int user1, long user2, int user3, long user4);

extern NDRX_API int tpacallex (char *svc, char *data, 
        long len, long flags, char *extradata, int dest_node, int is_evpost,
        int user1, long user2, int user3, long user4);
/* event API implementation */
extern NDRX_API long ndrx_tpunsubscribe(long subscription, long flags);
extern NDRX_API long ndrx_tpsubscribe(char *eventexpr, char *filter, TPEVCTL *ctl, long flags);
extern NDRX_API int ndrx_tppost(char *eventname, char *data, long len, long flags,
        int user1, long user2, int user3, long user4);

extern NDRX_API void	tpext_configbrige 
    (int nodeid, int flags, int (*p_qmsg)(char **buf, int len, char msg_type));

extern NDRX_API int ndrx_tpjsontoubf(UBFH *p_ub, char *buffer, EXJSON_Object *data_object);
extern NDRX_API int ndrx_tpubftojson(UBFH *p_ub, char *buffer, int bufsize, EXJSON_Object *data_object);
extern NDRX_API int ndrx_tpcall (char *svc, char *idata, long ilen,
                char * *odata, long *olen, long flags,
                char *extradata, int dest_node, int ex_flags,
                int user1, long user2, int user3, long user4);
extern NDRX_API int ndrx_tpgetrply (int *cd,
                       int cd_exp,
                       char * *data ,
                       long *len, long flags,
                       TPTRANID *p_tranid);
extern NDRX_API int ndrx_tpcancel (int cd);
extern NDRX_API int ndrx_tpterm (void);
extern NDRX_API int ndrx_tpconnect (char *svc, char *data, long len, long flags);
extern NDRX_API int ndrx_tprecv (int cd, char * *data, 
                        long *len, long flags, long *revent,
                        short *command_id);
extern NDRX_API int ndrx_tpdiscon (int cd);
extern NDRX_API int ndrx_tpenqueue (char *qspace, short nodeid, short srvid, char *qname, TPQCTL *ctl, 
        char *data, long len, long flags);
extern NDRX_API int ndrx_tpdequeue (char *qspace, short nodeid, short srvid, char *qname, TPQCTL *ctl, 
        char **data, long *len, long flags);

extern NDRX_API void ndrx_tpfreectxt(TPCONTEXT_T context);
extern NDRX_API int ndrx_tpconvert(char *str, char *bin, long flags);

/* debug logging */
extern NDRX_API int ndrx_tplogsetreqfile(char **data, char *filename, char *filesvc);
extern NDRX_API int ndrx_tploggetbufreqfile(char *data, char *filename, int bufsize);
extern NDRX_API int ndrx_tplogdelbufreqfile(char *data);
extern NDRX_API void ndrx_tplogprintubf(int lev, char *title, UBFH *p_ub);

/* ATMI level process management: */
extern NDRX_API int ndrx_chk_server(char *procname, short srvid);
extern NDRX_API int ndrx_chk_ndrxd(void);
extern NDRX_API pid_t ndrx_ndrxd_pid_get(void);
extern NDRX_API int ndrx_down_sys(char *qprefix, char *qpath, int is_force, int user_res);
extern NDRX_API void ndrx_down_userres(void);
extern NDRX_API int ndrx_killall(char *mask);
extern NDRX_API int ndrx_q_exists(char *qpath);
extern NDRX_API int ndrx_get_cached_svc_q(char *q);
extern NDRX_API int ndrx_ndrxd_ping(int *p_seq, long *p_time_msec,
                    mqd_t listen_q, char * listen_q_str);

/* Access to symbols: */

extern NDRX_API tp_command_call_t *ndrx_get_G_last_call(void);
extern NDRX_API atmi_lib_conf_t *ndrx_get_G_atmi_conf(void);
extern NDRX_API atmi_lib_env_t *ndrx_get_G_atmi_env(void);
extern NDRX_API tp_conversation_control_t *ndrx_get_G_accepted_connection(void);

/* tpimport() functions */
extern NDRX_API int tpimportex(ndrx_expbufctl_t *bufctl, char *istr, long ilen, char **obuf, long *olen, long flags);
extern NDRX_API int ndrx_tpimportex(ndrx_expbufctl_t *bufctl, char *istr, long ilen, char **obuf, long *olen, long flags);

/* tpexport() functions */
extern NDRX_API int tpexportex(ndrx_expbufctl_t *bufctl, char *ibuf, long ilen, char *ostr, long *olen, long flags);
extern NDRX_API int ndrx_tpexportex(ndrx_expbufctl_t *bufctl, char *ibuf, long ilen, char *ostr, long *olen, long flags);

/* export the symbol */
extern NDRX_API struct xa_switch_t * ndrx_xa_builtin_get(void);

extern NDRX_API int ndrx_tpgetcallinfo(const char *msg, UBFH **obuf, long flags);
extern NDRX_API int ndrx_tpsetcallinfo(const char *msg, UBFH *obuf, long flags);

/* tp encryption functions */
extern NDRX_API int tpencrypt_int(char *input, long ilen, char *output, long *olen, long flags);
extern NDRX_API int tpdecrypt_int(char *input, long ilen, char *output, long *olen, long flags);

#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_INT_H */

/* vim: set ts=4 sw=4 et smartindent: */
