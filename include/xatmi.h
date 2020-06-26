/*
 * This file as per:
 *   The Open Group, Technical Standard
 *   Distributed Transaction Processing: The XATMI Specification
 * !!! Extended for Enduro/X !!!
 */
#ifndef __XATMI_H__
#define __XATMI_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <ndrstandard.h>
#include <stdint.h>
#include <sys/types.h>
#include <ubf.h>
#include <xa.h>
/*---------------------------Macros-------------------------------------*/
    
/*
 * flag bits for C language xatmi routines
 */
#define TPNOBLOCK	0x00000001
#define TPSIGRSTRT	0x00000002
#define TPNOREPLY	0x00000004
#define TPNOTRAN	0x00000008
#define TPTRAN		0x00000010
#define TPNOTIME	0x00000020
#define TPABSOLUTE	0x00000040
#define TPGETANY	0x00000080
#define TPNOCHANGE	0x00000100
#define TPCONV		0x00000400
#define TPSENDONLY	0x00000800
#define TPRECVONLY	0x00001000
#define TPACK		0x00002000
/** Software raised service error, any   */
#define TPSOFTERR	0x00020000
/** Suspend current transaction          */
#define TPTRANSUSPEND	0x00040000
/** Soft timout condition -> ret TPETIME */
#define TPSOFTTIMEOUT	0x00080000
/** Simulate that service is not found   */
#define TPSOFTENOENT    0x00100000
/** Don't restore autbuf in srv context  */
#define TPNOAUTBUF      0x00200000
/** RFU, tux compatiblity */
#define RESERVED_BIT1   0x00400000
/** Use regular expressoins for match    */
#define TPREGEXMATCH    0x00800000
/** Do not lookup cache                  */
#define TPNOCACHELOOK   0x01000000
/** Do not save data to cache            */
#define TPNOCACHEADD    0x02000000
/** Do not use cached data               */
#define TPNOCACHEDDATA  0x04000000
/** Do not abort global transaction, even if service failed */
#define TPNOABORT       0x08000000

#define TPEVSERVICE	0x00000001
#define TPEVQUEUE       0x00000002 /* RFU */
#define TPEVTRAN        0x00000004 /* RFU */
#define TPEVPERSIST	0x00000008

#define TPEX_NOCHANGE       0x00000004  /**< Reject tpimport with error if 
                                                types does not match*/
#define TPEX_STRING         0x00000008  /**< Export buffer in base64 format */
#define TPIMPEXP_VERSION_MIN    1 /** < import min version */
#define TPIMPEXP_VERSION_MAX    1 /** < import / export max version */


#define NDRX_XID_SERIAL_BUFSIZE     48 /**< Serialized size (base64) xid */
#define NDRX_MAX_RMS                32  /**< Number of resource managers supported */
#define TMTXFLAGS_IS_ABORT_ONLY     0x0001 /**< transaction is marked as abort only */

#define ATMI_XA_TX_INFO_FIELDS      \
    short tmtxflags;                   /* See TMTXFLAGS_* */\
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1]; /* tmxid, serialized */\
    short tmrmid; /* initial resource manager id */\
    short tmnodeid; /* initial node id */\
    short tmsrvid; /* initial TM server id */\
    char tmknownrms[NDRX_MAX_RMS+1]; /* valid values 1..32-1, 0 - reserved + EOS */

/** rval in tpreturn - Service failed */
#define TPFAIL		0x00000001
/** rval in tpreturn - Service Succeed */
#define TPSUCCESS	0x00000002
/** rval in tpreturn - RFU */
#define TPEXIT          0x08000000

/**
 * Posix Queue processing path prefixes
 */
#define NDRX_FMT_SEP      ','                   /**< Seperator in qnames      */
#define NDRX_FMT_SEP_STR  ","                   /**< Seperator in qnames      */
#define NDRX_NDRXD        "%s,sys,bg,ndrxd"
#define NDRX_QTYPE_NDRXD    1                   /**< ndrxd backend q          */
#define NDRX_NDRXCLT      "%s,sys,bg,xadmin,%d"
#define NDRX_NDRXCLT_PFX  "%s,sys,bg,xadmin,"   /**< Prefix for sanity check    */


#define NDRX_SVC_QFMT     "%s,svc,%s"            /**< Q format in epoll mode (one q multiple servers) */
#define NDRX_SVC_QFMT_PFX "%s,svc,"              /**< Service Q prefix */
#define NDRX_QTYPE_SVC      2                    /**< Service Q */
#define NDRX_SVC_QFMT_SRVID "%s,svc,%s,%d"       /**< Q format in poll mode (use server id) */
#define NDRX_ADMIN_FMT    "%s,srv,admin,%s,%d,%d"

#define NDRX_SYS_SVC_PFX          "@"                    /**< Prefix used for system services */
#define NDRX_SYS_SVC_PFXC         '@'                    /**< Prefix used for system services */
#define NDRX_SVC_BRIDGE_STATLEN   9                      /**< Static len of bridge name       */
#define NDRX_SVC_BRIDGE           "@TPBRIDGE%03d"        /**< Bridge service format           */
#define NDRX_SVC_QBRDIGE          "%s,svc,@TPBRIDGE%03d" /**< Bridge service Q format         */
    
#define NDRX_SVC_TPBROAD  "@TPBRDCST%03ld"      /**< notify/broadcast remote dispatcher       */
#define NDRX_SVC_TMIB     ".TMIB"               /**< Tp Management information base           */
#define NDRX_SVC_TMIBNODE ".TMIB-%ld-%d"        /**< Tp Management information base, node, server */

#define NDRX_SVC_RM       "@TM-%d"              /**< resource_id */
#define NDRX_SVC_TM       "@TM-%d-%d"           /**< Node_idresource_id */
#define NDRX_SVC_TM_I     "@TM-%d-%d-%d"        /**< Node_id,resource_id,instance/srvid */
    
#define NDRX_SVC_TMQ      "@TMQ-%ld-%d"         /**< Node_id,srvid */
/* QSPACE service format */
#define NDRX_SVC_QSPACE   "@QSP%s"              /**< Q space format string (for service) */
    
#define NDRX_SVC_CPM      "@CPMSVC"             /**< Client Process Monitor svc */
    
#define NDRX_SVC_CCONF    "@CCONF"              /**< Common-config server */
#define NDRX_SVC_ADMIN    "@ADMINSVC"           /**< Admin service for atmiservices, logical */
#define NDRX_SVC_REPLY    "@REPLYSVC"           /**< Reply service for atmiservices, logical */

#define NDRX_ADMIN_FMT_PFX "%s,srv,admin,"      /**< Prefix for sanity check. */
#define NDRX_QTYPE_SRVADM   3                   /**< Server Admin Q */
    
#define NDRX_SVR_QREPLY   "%s,srv,reply,%s,%d,%d" /**< qpfx, procname, serverid, pid */
#define NDRX_SVR_QREPLY_PFX "%s,srv,reply," /**< Prefix for sanity check. */
#define NDRX_QTYPE_SRVRPLY  4                   /**< Server Reply Q */
    
/* Used for System V interface */
#define NDRX_SVR_SVADDR_FMT "%s,srv,addr,%s,%d" /**< Server address, per proc */
#define NDRX_SVR_RQADDR_FMT "%s,srv,rqaddr,%s" /**< Server request address   */
/** bridge request addr */
#define NDRX_SVR_RQADDR_BRDG "%s,srv,rqaddr,@TPBRIDGE%03d"

/* this may end up in "112233-" if client is not properly initialised */
/* NOTE: Myid contains also node_id, the client Q does not contain it
 * as it is local
 */
#define NDRX_CLT_QREPLY   "%s,clt,reply,%s,%d,%ld" /**< pfx, name, pid, context id*/
/* client rply q parse  */
#define NDRX_CLT_QREPLY_PARSE "%s clt reply %s %d %ld" /**< pfx, name, pid, context_id*/
    
#define NDRX_CLT_QREPLY_PFX   "%s,clt,reply," /**< Prefix for sanity check */
#define NDRX_QTYPE_CLTRPLY  5                   /**< Client Reply Q */
#define NDRX_CLT_QREPLY_CHK   ",clt,reply," /**< (verify that it is reply q) */

#define NDRX_ADMIN_SVC     "%s-%d"

/* This queue basically links two process IDs for conversation */
#define NDRX_CONV_INITATOR_Q  "%s,cnv,c,%s,%d" /**< Conversation initiator */
#define NDRX_CONV_INITATOR_Q_PFX "%s,cnv,c," /**< Prefix for sanity check. */
#define NDRX_QTYPE_CONVINIT 6                   /**< Conv initiator Q */
#define NDRX_CONV_SRV_Q       "%s,cnv,s,%s,%d,%s" /**< Conversation server Q */
#define NDRX_CONV_SRV_Q_PFX "%s,cnv,s," /**< Prefix for sanity check. */
#define NDRX_QTYPE_CONVSRVQ 7                   /**< Conv server Q */

/** binary name, server id, pid, contextid, nodeid */
#define NDRX_MY_ID_SRV        "srv,%s,%d,%d,%ld,%d"
/** binary name, server id, pid, contextid, nodeid for parse */
#define NDRX_MY_ID_SRV_PARSE  "srv %s %d %d %ld %d"
#define NDRX_MY_ID_SRV_NRSEPS  5 /**< Number of separators in myid of server */
    
/** binary name, server id, pid, contextid, nodeid, cd for parse */
#define NDRX_MY_ID_SRV_CNV_PARSE  "srv %s %d %d %ld %d %d"
#define NDRX_MY_ID_SRV_CNV_NRSEPS  6 /**< Number of separators in myid of server */
    
#define NDRX_MY_ID_CLT        "clt,%s,%d,%ld,%d" /**< cltname, pid, contextid, nodeid */
#define NDRX_MY_ID_CLT_PARSE  "clt %s %d %ld %d" /**< cltname, pid, contextid, nodeid */
#define NDRX_MY_ID_CLT_NRSEPS  4 /**< Number of separators in myid of client */
    
#define NDRX_MY_ID_CLT_CNV_PARSE  "clt %s %d %ld %d %d" /**< cltname, pid, contextid, nodeid, cd */
#define NDRX_MY_ID_CLT_CNV_NRSEPS  5 /**< Number of separators in myid of client */

/* Shared memory formats */
#define NDRX_SHM_SRVINFO_SFX    "shm,srvinfo"       /**< Server info SHM               */
#define NDRX_SHM_SRVINFO        "%s," NDRX_SHM_SRVINFO_SFX
#define NDRX_SHM_SRVINFO_KEYOFSZ 0                  /**< IPC Key offset                */

#define NDRX_SHM_SVCINFO_SFX    "shm,svcinfo"       /**< Service info SHM              */
#define NDRX_SHM_SVCINFO        "%s," NDRX_SHM_SVCINFO_SFX
#define NDRX_SHM_SVCINFO_KEYOFSZ 1                  /**< IPC Key offset                */
    
#define NDRX_SHM_BRINFO_SFX     "shm,brinfo"        /**< Bridge info SHM               */
#define NDRX_SHM_BRINFO         "%s," NDRX_SHM_BRINFO_SFX
#define NDRX_SHM_BRINFO_KEYOFSZ 2                   /**< IPC Key offset                */
    
#define NDRX_SHM_P2S_SFX        "shm,p2s"           /**< Posix to System V             */
#define NDRX_SHM_P2S            "%s," NDRX_SHM_P2S_SFX
#define NDRX_SHM_P2S_KEYOFSZ    3                   /**< IPC Key offset                */
    
#define NDRX_SHM_S2P_SFX        "shm,s2p"           /**< System V to Posix             */
#define NDRX_SHM_S2P            "%s," NDRX_SHM_S2P_SFX
#define NDRX_SHM_S2P_KEYOFSZ    4                   /**< IPC Key offset                */

#define NDRX_SHM_CPM_SFX        "shm,cpm"           /**< Client process monitor        */
#define NDRX_SHM_CPM            "%s," NDRX_SHM_CPM_SFX
#define NDRX_SHM_CPM_KEYOFSZ    5                   /**< IPC Key offset                */
    
#define NDRX_SEM_SVCOP          "%s,sem,svcop"      /**< Service operations...         */

#define NDRX_KEY_FMT            "-k %s"             /**< format string for process key */

/* Format @C<P|D>NID/Flags/<Service name> */
#define NDRX_CACHE_EV_PFXLEN    6                   /**< prefix len @CXNNN             */
#define NDRX_CACHE_EV_PUT       "@CP%03d/%s/%s"     /**< Put data in cache, event      */
#define NDRX_CACHE_EV_DEL       "@CD%03d/%s/%s"     /**< Delete data form cache, event */
#define NDRX_CACHE_EV_KILL      "@CK%03d/%s/%s"     /**< Kill the database             */
#define NDRX_CACHE_EV_MSKDEL    "@CM%03d/%s/%s"     /**< Delete by mask                */
#define NDRX_CACHE_EV_KEYDEL    "@CE%03d/%s/%s"     /**< Delete by key                 */
#define NDRX_CACHE_EVSVC        "@CACHEEV%03ld"     /**< Cache event service, delete   */
#define NDRX_CACHE_MGSVC        "@CACHEMG%03ld"     /**< Cache managemtn service       */
    
#define NDRX_CACHE_EV_LEN       3                   /**< Number of chars in command    */
    
#define NDRX_CACHE_EV_PUTCMD    "@CP"               /**< Put command in event          */
#define NDRX_CACHE_EV_DELCMD    "@CD"               /**< Delete command in event       */
#define NDRX_CACHE_EV_KILCMD    "@CK"               /**< Kill/drop ache event          */
#define NDRX_CACHE_EV_MSKDELCMD "@CM"               /**< Delete data by mask, event    */
#define NDRX_CACHE_EV_KEYDELCMD "@CE"               /**< Delete by key event           */

#define tperrno	(*_exget_tperrno_addr())
#define tpurcode (*_exget_tpurcode_addr())

/*
 * error values for tperrno
 */
#define TPMINVAL	0	/* no error, min - fix*/ 
#define TPEABORT	1
#define TPEBADDESC	2
#define TPEBLOCK	3
#define TPEINVAL	4
#define TPELIMIT	5
#define TPENOENT	6
#define TPEOS		7
#define TPEPERM		8
#define TPEPROTO	9
#define TPESVCERR	10
#define TPESVCFAIL	11
#define TPESYSTEM	12
#define TPETIME		13
#define TPETRAN		14
#define TPGOTSIG	15
#define TPERMERR	16
#define TPEITYPE	17
#define TPEOTYPE	18
#define TPERELEASE	19
#define TPEHAZARD	20
#define TPEHEURISTIC	21
#define TPEEVENT	22
#define	TPEMATCH	23
#define TPEDIAGNOSTIC	24
#define TPEMIB		25    
#define TPERFU26        26      /**< reserved for future use */
#define TPERFU27	27      /**< reserved for future use */
#define TPERFU28	28      /**< reserved for future use */
#define TPERFU29	29      /**< reserved for future use */
#define TPINITFAIL	30
#define TPMAXVAL	30	/**< max error */

/*
 * events returned during conversational communication
 */
#define TPEV_DISCONIMM	0x0001
#define TPEV_SVCERR	0x0002
#define TPEV_SVCFAIL	0x0004
#define TPEV_SVCSUCC	0x0008
#define TPEV_SENDONLY	0x0020

    
/* Tpinit flags (RFU) */
#define TPU_MASK	0x00000007
#define TPU_SIG		0x00000001  /* RFU */
#define TPU_DIP		0x00000002
#define TPU_IGN		0x00000004  /* Ignore unsol messages */
    
/* for compatiblity with Tuxedo */
#define TPSA_FASTPATH	0x00000008	
#define TPSA_PROTECTED	0x00000010
    
/* 
 * used by tpconvert()
 */
#define TPCONVMAXSTR    512         /**< Max identifier buffer                */
#define TPTOSTRING      0x00000001  /**< convert to string                    */
#define TPCONVCLTID     0x00000002  /**< Convert client id                    */
#define TPCONVTRANID    0x00000004  /**< Convert transaction id               */
#define TPCONVXID       0x00000008  /**< Convert XID (current not supported   */

/*  Size of TPINIT struct */
#define TPINITNEED(u)	sizeof(TPINIT)

#define CTXT_PRIV_NONE	0x00000         /**< no context data */
#define	CTXT_PRIV_NSTD	0x00001		/**< standard library TLS data */
#define	CTXT_PRIV_UBF   0x00002		/**< UBF TLS data */
#define	CTXT_PRIV_ATMI	0x00004		/**< ATMI level private data */
#define	CTXT_PRIV_TRAN	0x00008         /**< ATMI + Global transaction */
#define	CTXT_PRIV_NOCHK	0x00010		/**< Do not check signatures */
#define	CTXT_PRIV_IGN	0x00020		/**< Ignore existing context */


/* Multi contexting */    
#define TPINVALIDCONTEXT    -1
#define TPSINGLECONTEXT     -2 /**< Not used by Enduro/X */
    
#define TPNULLCONTEXT       0 /**< basically NULL pointer */
#define TPMULTICONTEXTS     0x00000040
    
/*
 * X/Open defined typed buffers
 */
#define X_OCTET		"X_OCTET"
#define X_C_TYPE	"X_C_TYPE"
#define X_COMMON	"X_COMMON"

#define MAXTIDENT                   30      /**< Internal identifed max len   */
#define XATMI_SERVICE_NAME_LENGTH   MAXTIDENT
#define XATMI_TYPE_LEN              8       /**< Max type len                 */
#define XATMI_SUBTYPE_LEN           33      /**< Max sub-type len             */
#define XATMI_EVENT_MAX             42      /**< Max len of event to bcast    */
     
/* Range for cluster ID's
 * Currently we allow 254 nodes.
 */
#define CONF_NDRX_NODEID_MIN        1       /**< Min Node ID                  */
#define CONF_NDRX_NODEID_MAX        33      /**< Max Node ID                  */
/* Total count of cluster nodes */
#define CONF_NDRX_NODEID_COUNT (CONF_NDRX_NODEID_MAX-CONF_NDRX_NODEID_MIN)
#define CONF_NDRX_MAX_SRVIDS_XADMIN 512    /**< max number of server IDs for 
                                               xadmin printing in poll mode */
    
/* Max calls at the same tame
 * This is  used by Call Descritptor check 
 */
#ifdef EX_OS_DARWIN

/* fails to build on libatmi/init.c with 
 * - ld error: section __DATA/__thread_bss extends beyond end of file
 * thus reduce the array.
 */
#define MAX_ASYNC_CALLS         1000

#else

#define MAX_ASYNC_CALLS         16384

#endif

/** Bigger connection count may slowdown whole system. */
#define MAX_CONNECTIONS         10

/** this is upper limit for re-looping the cd
 * basically the max connection is MAX_CONNECTIONS but with each
 * connection we increment the cd until the upper limit.
 * The actual CD position is taken by modulus.
 * The system call descriptor internally tracks the real cd, and rejects
 * the invalid value cd value not with the same exact value
 */
#define NDRX_CONV_UPPER_CNT      (MAX_CONNECTIONS*100)
    
#define NDRX_EVENT_EXPR_MAX     255
#define NDRX_CACHE_KEY_MAX      16384      /**< Max size of cache key            */
#define NDRX_CACHE_FLAGS_MAX    512        /**< Max flags string size            */
    
/* EnduroX system ATMI flags */
#define SYS_FLAG_REPLY_ERROR    0x00000001
#define SYS_CONVERSATION        0x00000002 /**< We have or was open conversation */
#define SYS_SRV_THREAD          0x00000004 /**< This is new server thread        */
/* buffer management flags: */
#define SYS_SRV_CVT_JSON2UBF    0x00000008 /**< Message is converted from JSON to UBF */
#define SYS_SRV_CVT_UBF2JSON    0x00000010 /**< Message is converted from UBF to JSON */

#define SYS_SRV_CVT_JSON2VIEW   0x00000020 /**< Message is converted from JSON to VIEW */
#define SYS_SRV_CVT_VIEW2JSON   0x00000040 /**< Message is converted from UBF to JSON (non NULL)*/
    
/* Test is any flag set */
#define SYS_SRV_CVT_ANY_SET(X) (X & SYS_SRV_CVT_JSON2UBF || X & SYS_SRV_CVT_UBF2JSON ||\
        X & SYS_SRV_CVT_JSON2VIEW || X & SYS_SRV_CVT_VIEW2JSON)
    
#define tpadvertise(_SVCNM, _FNADDR) tpadvertise_full(_SVCNM, _FNADDR, #_FNADDR)

#define NDRX_INTEGRA(X)		__##X##__ /* integration mode*/

#ifndef _
#define _(X)  X
#endif

/********************** Queue support  *****************************************/
#define TMQNAMELEN	15
#define TMMSGIDLEN	32
#define TMMSGIDLEN_STR	45 /* TMMSGIDLEN * 1.4 (base64 overhead) */
#define TMCORRIDLEN	32
#define TMCORRIDLEN_STR	45 /* TMMSGIDLEN * 1.4 (base64 overhead) */
    
/* structure elements that are valid - set in flags */
#define TPNOFLAGS	0x00000		
#define	TPQCORRID	0x00001		/**< set/get correlation id */		
#define	TPQFAILUREQ	0x00002		/**< set/get failure queue */		
#define	TPQBEFOREMSGID	0x00004		/**< RFU, enqueue before message id */		
#define	TPQGETBYMSGIDOLD 0x00008	/**< RFU, deprecated */		
#define	TPQMSGID	0x00010		/**< get msgid of enq/deq message */		
#define	TPQPRIORITY	0x00020		/**< set/get message priority */		
#define	TPQTOP		0x00040		/**< RFU, enqueue at queue top */
#define	TPQWAIT		0x00080		/**< RFU, wait for dequeuing */		
#define	TPQREPLYQ	0x00100		/**< set/get reply queue */		
#define	TPQTIME_ABS	0x00200		/**< RFU, set absolute time */		
#define	TPQTIME_REL	0x00400		/**< RFU, set absolute time */		
#define	TPQGETBYCORRIDOLD 0x00800	/**< deprecated */		
#define	TPQPEEK		0x01000		/**< peek */		
#define TPQDELIVERYQOS  0x02000         /**< RFU, delivery quality of service */		
#define TPQREPLYQOS     0x04000         /**< RFU, reply message quality of service */		
#define TPQEXPTIME_ABS  0x08000         /**< RFU, absolute expiration time */		
#define TPQEXPTIME_REL  0x10000         /**< RFU, relative expiration time */		
#define TPQEXPTIME_NONE 0x20000        	/**< RFU, never expire */		
#define	TPQGETBYMSGID	0x40008		/**< dequeue by msgid */		
#define	TPQGETBYCORRID	0x80800		/**< dequeue by corrid */		
#define TPQASYNC        0x100000        /**< Async complete */
		
/* Valid flags for the quality of service fileds in the TPQCTLstructure */		
#define TPQQOSDEFAULTPERSIST  0x00001   /**< queue's default persistence policy */		
#define TPQQOSPERSISTENT      0x00002   /**< disk message */		
#define TPQQOSNONPERSISTENT   0x00004   /**< memory message */		

#define QMEINVAL	-1
#define QMEBADRMID	-2
#define QMENOTOPEN	-3
#define QMETRAN		-4
#define QMEBADMSGID	-5
#define QMESYSTEM	-6
#define QMEOS		-7
#define QMEABORTED	-8
#define QMENOTA		QMEABORTED		
#define QMEPROTO	-9
#define QMEBADQUEUE	-10
#define QMENOMSG	-11
#define QMEINUSE	-12
#define QMENOSPACE	-13
#define QMERELEASE      -14
#define QMEINVHANDLE    -15
#define QMESHARE        -16

/* Flags for internal TLS processing 
 * for 
 * - _tpgetctxt();
 * - _tpsetctxt();
 */
#define CTXT_PRIV_NONE	0x00000         /**< no context data */
#define	CTXT_PRIV_NSTD	0x00001		/**< standard library TLS data */
#define	CTXT_PRIV_UBF   0x00002		/**< UBF TLS data */
#define	CTXT_PRIV_ATMI	0x00004		/**< ATMI level private data */
#define	CTXT_PRIV_TRAN	0x00008         /**< ATMI + Global transaction */
#define	CTXT_PRIV_NOCHK	0x00010		/**< Do not check signatures */
#define	CTXT_PRIV_IGN	0x00020		/**< Ignore existing context */

/* tpchkauth() return values */
#define TPNOAUTH         0        /**< RFU: no authentication          */
#define TPSYSAUTH        1        /**< RFU: system authentication          */
#define TPAPPAUTH        2        /**< RFU: system and application authentication  */
    

#define TPUNSOLERR	ndrx_ndrx_tmunsolerr_handler
    
/**
 * Internal process identifier
 */
#define TPMYIDTYP_CLIENT       1 /**< Q identifier is client */
#define TPMYIDTYP_SERVER       2 /**< Q identifier is server */
    

/** flag for ndrx_main_integra: Do not use long jump   */
#define ATMI_SRVLIB_NOLONGJUMP     0x00000001
    
/*
 * Flag for transaction processing i.e. 
 * tpbegin/tpcommit/tpclose/tpopen/tpabort/tpsuspend/tpresume 
 */
#define TPTXCOMMITDLOG             0x00000004  /**< Commit decision logged     */

/* Flags to tpscmt() - TP_COMMIT_CONTROL values, for compatibility: */
#define TP_CMT_LOGGED              0x04  /**< return after commit has logged   */
#define TP_CMT_COMPLETE            0x08  /**< return after commit has completed*/

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/* client/caller identifier */
struct clientid_t
{
    /* TODO! see Support #265 for major release scheduled. +1 for EOS */
    char    clientdata[NDRX_MAX_ID_SIZE];
};
typedef struct clientid_t CLIENTID;

/* Transaction ID used by tpsuspend()/tpresume() */
struct tp_tranid_t
{
    /* NOTE ! Same fileds from ATMI_XA_TX_INFO_FIELDS some precompilers have issues with
    parsing macro */
    short tmtxflags;                   /**< See TMTXFLAGS_* */
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1]; /**< tmxid, serialized */
    short tmrmid; /**< initial resource manager id */
    short tmnodeid; /**< initial node id */
    short tmsrvid; /**< initial TM server id */
    char tmknownrms[NDRX_MAX_RMS+1]; /**< valid values 1..32-1, 0 - reserved + EOS */
    /* END OF ATMI_XA_TX_INFO_FIELDS */
    int is_tx_initiator;
};
typedef struct tp_tranid_t TPTRANID;


struct tpevctl_t
{
    long flags;
    char name1[XATMI_SERVICE_NAME_LENGTH];
    char name2[XATMI_SERVICE_NAME_LENGTH];
};
typedef struct tpevctl_t TPEVCTL;

/*
 * service information structure
 */
typedef struct
{
    char	name[XATMI_SERVICE_NAME_LENGTH];
    char	*data;
    long	len;
    long	flags;
    int 	cd;
    long    appkey;
    CLIENTID cltid;
    char	fname[XATMI_SERVICE_NAME_LENGTH+1]; /**< function name */
} TPSVCINFO;

struct	tpinfo_t
{
    char usrname[MAXTIDENT+2];
    char cltname[MAXTIDENT+2];
    char passwd[MAXTIDENT+2];
    char grpname[MAXTIDENT+2];
    long flags;
    long datalen;
    long data;
};
typedef	struct	tpinfo_t TPINIT;

typedef void* TPCONTEXT_T; /* Enduro/X full context switching handler */

/* Queue support structure: */
struct tpqctl_t 
{
    long flags;         /**< indicates which of the values are set */		
    long deq_time;      /**< absolute/relative  time for dequeuing */		
    long priority;      /**< enqueue priority */		
    long diagnostic;    /**< indicates reason for failure */		
    char diagmsg[NDRX_QDIAG_MSG_SIZE]; /* diagnostic message */
    char msgid[TMMSGIDLEN];	/**< id of message before which to queue */		
    char corrid[TMCORRIDLEN];/**< correlation id used to identify message */		
    char replyqueue[TMQNAMELEN+1];/**< queue name for reply message */		
    char failurequeue[TMQNAMELEN+1];/**< queue name for failure message */		
    CLIENTID cltid;     /**< client identifier for originating client */		
    long urcode;        /**< application user-return code */		
    long appkey;        /**< application authentication client key */		
    long delivery_qos;  /**< delivery quality of service  */		
    long reply_qos;     /**< reply message quality of service  */		
    long exp_time;      /**< expiration time  */		
};		
typedef struct tpqctl_t TPQCTL;


struct tpmyid_t
{
    char binary_name[MAXTIDENT+2];
    pid_t pid;
    long contextid;
    int nodeid;
    int srv_id;
    int tpmyidtyp;
    int isconv;
    int cd; /**< if we run in conversational mode */
};

typedef struct tpmyid_t TPMYID;

/* Integration mode API, used by libatmisrvinteg.so: */

/**
 * Advertise table. For tmsvrargs_t must be terminated with NULL/0 entries
 */
struct tmdsptchtbl_t
{
    char *svcnm;                    /**< Service name                         */
    char *funcnm;                   /**< Function name                        */
    void (*p_func)(TPSVCINFO *);    /**< Function to run on service invocation*/
    long rfu1;                      /**< Reserved 1                           */
    long rfu2;                      /**< Reserved 2                           */
};

/**
 * Server startup structure for compatibility mode
 * Used by usable by _tmstartserver()
 */
struct tmsvrargs_t
{
  struct xa_switch_t * xa_switch;   /**< XA Switch                            */
  struct tmdsptchtbl_t *svctab;     /**< Service dispatch table               */
  long flags;                       /**< Reserved for future use              */
  int (*p_tpsvrinit)(int, char **); /**< Server init function                 */
  void (*p_tpsvrdone)(void);        /**< callback to server done              */
  void * reserved0;                 /**< Reserved for future use              */
  void * reserved1;                 /**< Reserved for future use              */
  void * reserved2;                 /**< Reserved for future use              */
  void * reserved3;                 /**< Reserved for future use              */
  void * reserved4;                 /**< Reserved for future use              */
  int (*p_tpsvrthrinit)(int, char **); /**< thread init func                  */
  void (*p_tpsvrthrdone)(void);        /**< thread done func                  */
};

/*---------------------------Globals------------------------------------*/
extern NDRX_API int (*G_tpsvrinit__)(int, char **);
extern NDRX_API void (*G_tpsvrdone__)(void);
/** For compatibility: */
extern NDRX_API int _tmbuilt_with_thread_option;
extern NDRX_API struct xa_switch_t tmnull_switch; /**< null switch            */
/*---------------------------Prototypes---------------------------------*/

/*
 * xatmi communication api
 */
extern NDRX_API int tpacall(char *svc, char *data, long len, long flags);
extern NDRX_API int tpadvertise_full (char *svc_nm, void (*p_func)(TPSVCINFO *), char *fn_nm);
extern NDRX_API char *tpalloc(char *type, char *subtype, long size);
extern NDRX_API int tpcall(char *svc, char *idata, long ilen, char **odata, long *olen, long flags);
extern NDRX_API int tpcancel(int cd);
extern NDRX_API int tpconnect(char *svc, char *data, long len, long flags);
extern NDRX_API int tpdiscon(int cd);
extern NDRX_API void tpfree(char *ptr);
extern NDRX_API int tpisautobuf (char *buf);
extern NDRX_API int tpgetrply(int *cd, char **data, long *len, long flags);
extern NDRX_API char *tprealloc(char *ptr, long size);
extern NDRX_API int tprecv(int cd, char **data, long *len, long flags, long *revent);
extern NDRX_API void tpreturn(int rval, long rcode, char *data, long len, long flags);
extern NDRX_API int tpsend(int cd, char *data, long len, long flags, long *revent);
extern NDRX_API void tpservice(TPSVCINFO *svcinfo);
extern NDRX_API long tptypes(char *ptr, char *type, char *subtype);
extern NDRX_API int tpunadvertise(char *svcname);

/* 
 * Extension functions
 */
extern NDRX_API void tpforward (char *svc, char *data, long len, long flags);
extern NDRX_API int tpabort (long flags);
extern NDRX_API int tpscmt(long flags);
extern NDRX_API int tpbegin (unsigned long timeout, long flags);
extern NDRX_API int tpcommit (long flags);
extern NDRX_API int tpconvert (char *str, char *bin, long flags);
extern NDRX_API int tpsuspend (TPTRANID *tranid, long flags);
extern NDRX_API int tpresume (TPTRANID *tranid, long flags);
extern NDRX_API int tpopen (void);
extern NDRX_API int tpclose (void);
extern NDRX_API int tpgetlev (void);
extern NDRX_API char * tpstrerror (int err);
extern NDRX_API int tperrordetail(long flags);
extern NDRX_API char * tpstrerrordetail(int err, long flags);
extern NDRX_API char * tpecodestr(int err);
extern NDRX_API char * tpsrvgetctxdata (void); 
extern NDRX_API char * tpsrvgetctxdata2 (char *p_buf, long *p_len);
extern NDRX_API void tpsrvfreectxdata(char *p_buf);
extern NDRX_API int tpsrvsetctxdata (char *data, long flags);
extern NDRX_API void tpcontinue (void);
extern NDRX_API long tpgetnodeid(void);
extern NDRX_API long tpsubscribe (char *eventexpr, char *filter, TPEVCTL *ctl, long flags);
extern NDRX_API int tpunsubscribe (long subscription, long flags);
extern NDRX_API int tppost (char *eventname, char *data, long len, long flags);
extern NDRX_API int * _exget_tperrno_addr (void);
extern NDRX_API long * _exget_tpurcode_addr (void);
extern NDRX_API int tpinit(TPINIT *tpinfo);
extern NDRX_API int tpappthrinit(TPINIT *tpinfo);
extern NDRX_API int tpchkauth(void);
extern NDRX_API void (*tpsetunsol (void (*disp) (char *data, long len, long flags))) (char *data, long len, long flags);
extern NDRX_API int tpnotify(CLIENTID *clientid, char *data, long len, long flags);
extern NDRX_API int tpbroadcast(char *lmid, char *usrname, char *cltname, char *data,  long len, long flags);
extern NDRX_API int tpchkunsol(void);

extern NDRX_API int tptoutset(int tout);
extern NDRX_API int tptoutget(void);
extern NDRX_API int tpimport(char *istr, long ilen, char **obuf, long *olen, long flags);
extern NDRX_API int tpexport(char *ibuf, long ilen, char *ostr, long *olen, long flags);
extern NDRX_API void* tpgetconn(void);
extern NDRX_API char *tuxgetenv(char *envname);
extern NDRX_API int tpgetcallinfo(const char *msg, UBFH **obuf, long flags);
extern NDRX_API int tpsetcallinfo(const char *msg, UBFH *obuf, long flags);

/* in external application: */
extern NDRX_API void tpsvrdone(void);
extern NDRX_API int tpsvrinit (int argc, char **argv);

extern NDRX_API int tpsvrthrinit(int argc, char **argv);
extern NDRX_API void tpsvrthrdone (void);

/* Poller & timer extension: */
extern NDRX_API int tpext_addpollerfd(int fd, uint32_t events, 
        void *ptr1, int (*p_pollevent)(int fd, uint32_t events, void *ptr1));
extern NDRX_API int tpext_delpollerfd(int fd);
extern NDRX_API int tpext_addperiodcb(int secs, int (*p_periodcb)(void));
extern NDRX_API int tpext_delperiodcb(void);
extern NDRX_API int tpext_addb4pollcb(int (*p_b4pollcb)(void));
extern NDRX_API int tpext_delb4pollcb(void);
extern NDRX_API int tpterm (void);
extern NDRX_API int tpappthrterm(void);
extern NDRX_API int tpgetsrvid (void);

/* JSON<->UBF buffer support */
extern NDRX_API int tpjsontoubf(UBFH *p_ub, char *buffer);
extern NDRX_API int tpubftojson(UBFH *p_ub, char *buffer, int bufsize);


/* JSON<->VIEW buffer support */
extern NDRX_API int tpviewtojson(char *cstruct, char *view, char *buffer,  
        int bufsize, long flags);
extern NDRX_API char * tpjsontoview(char *view, char *buffer);

/* Queue support: */
extern NDRX_API int tpenqueue (char *qspace, char *qname, TPQCTL *ctl, char *data, long len, long flags);
extern NDRX_API int tpdequeue (char *qspace, char *qname, TPQCTL *ctl, char **data, long *len, long flags);

extern NDRX_API int tpenqueueex (short nodeid, short srvid, char *qname, TPQCTL *ctl, char *data, long len, long flags);
extern NDRX_API int tpdequeueex (short nodeid, short srvid, char *qname, TPQCTL *ctl, char **data, long *len, long flags);

extern NDRX_API int ndrx_main(int argc, char **argv); /* exported by atmisrvnomain */
extern NDRX_API int ndrx_main_integra(int argc, char** argv, int (*in_tpsvrinit)(int, char **), 
            void (*in_tpsvrdone)(void), long flags);

/* for build compatibility: */
extern NDRX_API int _tmstartserver( int argc, char **argv, struct tmsvrargs_t *tmsvrargs);
extern NDRX_API int _tmrunserver(int arg1);
extern NDRX_API struct tmsvrargs_t * _tmgetsvrargs(void);

/* Contexting/switching TLS for all libs */
extern NDRX_API int tpgetctxt(TPCONTEXT_T *context, long flags);
extern NDRX_API int tpsetctxt(TPCONTEXT_T context, long flags);

extern NDRX_API TPCONTEXT_T tpnewctxt(int auto_destroy, int auto_set);
extern NDRX_API void tpfreectxt(TPCONTEXT_T context);

extern int ndrx_tpsetctxt(TPCONTEXT_T context, long flags, long priv_flags);
extern int ndrx_tpgetctxt(TPCONTEXT_T *context, long flags, long priv_flags);
    
extern NDRX_API int tplogsetreqfile(char **data, char *filename, char *filesvc);
extern NDRX_API int tploggetbufreqfile(char *data, char *filename, int bufsize);
extern NDRX_API int tplogdelbufreqfile(char *data);
extern NDRX_API void tplogprintubf(int lev, char *title, UBFH *p_ub);

/* ATMI library TLS: */
extern NDRX_API void * ndrx_atmi_tls_get(long priv_flags);
extern NDRX_API int ndrx_atmi_tls_set(void *data, int flags, long priv_flags);
extern NDRX_API void ndrx_atmi_tls_free(void *data);
extern NDRX_API void * ndrx_atmi_tls_new(void *tls_in, int auto_destroy, int auto_set);

/* Error code - function for unsol: */
extern NDRX_API void ndrx_ndrx_tmunsolerr_handler(char *data, long len, long flags);

extern NDRX_API pid_t ndrx_fork(void);
extern NDRX_API void ndrx_atfork_child(void);
extern NDRX_API void ndrx_atfork_parent(void);
extern NDRX_API void ndrx_atfork_prepare(void);

extern NDRX_API int tpencrypt(char *input, long ilen, char *output, long *olen, long flags);
extern NDRX_API int tpdecrypt(char *input, long ilen, char *output, long *olen, long flags);

#if defined(__cplusplus)
}
#endif

#endif /* __XATMI_H__ */

/* vim: set ts=4 sw=4 et cindent: */
