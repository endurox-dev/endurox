/**
 * @brief Enduro/X Standard library
 *
 * @file ndrstandard.h
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
#ifndef NDRSTANDARD_H
#define	NDRSTANDARD_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <ndrx_config.h>
#include <stdint.h>
#include <string.h>
    
#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
#define SYS32BIT
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYS64BIT
#else
/* wtf */
#endif

#ifndef EXFAIL
#define EXFAIL		-1
#endif
    
#ifndef EXSUCCEED
#define EXSUCCEED		0
#endif

#ifndef expublic
#define expublic
#endif
    
#ifndef exprivate
#define exprivate     	static
#endif

#ifndef EXEOS
#define EXEOS             '\0'
#endif
    
#ifndef EXBYTE
#define EXBYTE(x) ((x) & 0xff)
#endif

#ifndef EXFALSE
#define EXFALSE        0
#endif

#ifndef EXTRUE
#define EXTRUE         1
#endif

/** Directory seperator symbol */
#define EXDIRSEP      '/'

#define N_DIM(a)        (sizeof(a)/sizeof(*(a)))

#ifndef EXFAIL_OUT
#define EXFAIL_OUT(X)    {X=EXFAIL; goto out;}
#endif


#ifndef EXOFFSET
#ifdef SYS64BIT
#define EXOFFSET(STRUCT,ELM)   ((long) &(((STRUCT *)0)->ELM) )
#else
#define EXOFFSET(STRUCT,ELM)   ((const int) &(((STRUCT *)0)->ELM) )
#endif
#endif
    
#define NDRX_WORD_SIZE  (int)sizeof(void *)*8

#ifndef EXELEM_SIZE
#define EXELEM_SIZE(STRUCT,ELM)        (sizeof(((STRUCT *)0)->ELM))
#endif

#define NDRX_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
    
#define NDRX_ATMI_MSG_MAX_SIZE   65536 /* internal */
    
#define NDRX_STACK_MSG_FACTOR        30 /** max number of messages in stack, not used */

/** Default stack size if main is unlimited*/
#define NDRX_STACK_MAX (8192*1024)
/* Feature #127 
 * Allow dynamic buffer sizing with Variable Length Arrays (VLS) in C99
 */
extern NDRX_API long ndrx_msgsizemax (void);
#define NDRX_MSGSIZEMAX          ndrx_msgsizemax()

/**
 * Overhead applied to max buffer size used during the trasnport / 
 * encapuslation structures.
 */
#define NDRX_MSGSIZEMAX_OVERHD   200

#define NDRX_PADDING_MAX         16 /* Max compiled padding in struct (assumed) */

#ifdef HAVE_STRNLEN

#define NDRX_STRNLEN strnlen

#else

extern NDRX_API size_t ndrx_strnlen(char *str, size_t max);

#define NDRX_STRNLEN ndrx_strnlen

#endif
    
#ifdef HAVE_STRLCPY
#define NDRX_STRCPY_SAFE(X, Y) strlcpy(X, Y, sizeof(X));
	
#define NDRX_STRCPY_SAFE_DST(X, Y, N) strlcpy(X, Y, N);
    
#else
    
/**
 * Copies string to maximum of target buffer size
 * @param X destination buffer (must be static char array with size sizeof available)
 * @param Y string to copy from
 */
#define NDRX_STRCPY_SAFE(X, Y) do {\
	int ndrx_I5SmWDM_len = strlen(Y);\
	int ndrx_XgCmDEk_bufzs = sizeof(X)-1;\
	if (ndrx_I5SmWDM_len > ndrx_XgCmDEk_bufzs)\
	{\
            ndrx_I5SmWDM_len = ndrx_XgCmDEk_bufzs;\
	}\
	memcpy(X, Y, ndrx_I5SmWDM_len);\
	X[ndrx_I5SmWDM_len]=0;\
	} while(0)
    
/**
 * Safe copy to target buffer (not overflowing it...)
 * N - buffer size of X 
 * This always copies EOS
 * @param X dest buffer
 * @param Y source buffer
 * @param N dest buffer size
 */	
#define NDRX_STRCPY_SAFE_DST(X, Y, N) do {\
	int ndrx_I5SmWDM_len = strlen(Y);\
	int ndrx_XgCmDEk_bufzs = (N)-1;\
	if (ndrx_I5SmWDM_len > ndrx_XgCmDEk_bufzs)\
	{\
		ndrx_I5SmWDM_len = ndrx_XgCmDEk_bufzs;\
	}\
	memcpy((X), (Y), ndrx_I5SmWDM_len);\
	(X)[ndrx_I5SmWDM_len]=0;\
	} while(0)
#endif

/**
 * This is compatible with strncpy()
 * Ensure that dest buffer does not go over. Does not ensure for EOS. If
 * possible it copies the EOS from source buffer.
 * @param X dest buffer
 * @param Y source buffer
 * @param N number of bytes in dest buffer.
 */
#define NDRX_STRNCPY(X, Y, N) do {\
	int ndrx_I5SmWDM_len = strlen(Y)+1;\
	if (ndrx_I5SmWDM_len > (N))\
	{\
            ndrx_I5SmWDM_len = (N);\
	}\
	memcpy((X), (Y), ndrx_I5SmWDM_len);\
	} while(0)

/**
 * Copy number of chars, ensure that string is terminated with EOS
 * Ensure that dest buffer does not go over. Ensure for EOS
 * @param X dest buffer
 * @param Y source buffer
 * @param N number chars to copy
 * @param S dest buffer size
 */
#define NDRX_STRNCPY_EOS(X, Y, N, S) do {\
	int ndrx_I5SmWDM_len = strlen(Y);\
	if (ndrx_I5SmWDM_len > (N))\
	{\
            ndrx_I5SmWDM_len = (N);\
	}\
        if (ndrx_I5SmWDM_len>=(S)) ndrx_I5SmWDM_len=(S)-1;\
	memcpy((X), (Y), ndrx_I5SmWDM_len);\
        (X)[ndrx_I5SmWDM_len]=EXEOS;\
	} while(0)

/**
 * Copy the maxing at source buffer, not checking the dest
 * N - number of symbols to test in source buffer.
 * The dest buffer is assumed to be large enough.
 */
#define NDRX_STRNCPY_SRC(X, Y, N) do {\
        int ndrx_I5SmWDM_len = NDRX_STRNLEN((Y), (N));\
        memcpy((X), (Y), ndrx_I5SmWDM_len);\
	} while(0)

/**
 * Copy last NRLAST_ chars from SRC_ to DEST_
 * safe mode with target buffer size checking.
 * @param DEST_ destination buffer where to copy the string (should be static def)
 * @param SRC_ source string to copy last bytes off
 * @param NRLAST_ number of bytes to copy from string to dest
 */
#define NDRX_STRCPY_LAST_SAFE(DEST_, SRC_, NRLAST_) do {\
        int ndrx_KFWnP6Q_len = strlen(SRC_);\
        if (ndrx_KFWnP6Q_len > (NRLAST_)) {\
            NDRX_STRCPY_SAFE((DEST_), ((SRC_)+ (ndrx_KFWnP6Q_len - (NRLAST_))) );\
        } else {\
            NDRX_STRCPY_SAFE((DEST_), (SRC_));\
        }\
	} while(0)

#ifdef EX_HAVE_STRCAT_S

#define NDRX_STRCAT_S(DEST_, DEST_SIZE_, SRC_) strcat_s(DEST_, DEST_SIZE_, SRC_)

#else

/**
 * Cat string at the end. Dest size of the string is given
 * @param DEST_ dest buffer
 * @param DEST_SIZE_ dest buffer size
 * @param SRC_ source buffer to copy
 */
#define NDRX_STRCAT_S(DEST_, DEST_SIZE_, SRC_) do {\
        int ndrx_VeIlgbK9tx_len = strlen(DEST_);\
        NDRX_STRCPY_SAFE_DST( (DEST_+ndrx_VeIlgbK9tx_len), SRC_, (DEST_SIZE_ - ndrx_VeIlgbK9tx_len));\
} while(0)
#endif

/*
 * So we use these two macros where we need know that more times they will be
 * true, than false. This makes some boost for CPU code branching.
 * 
 * So for example if expect that some variable (c) must not be NULL
 * then we could run NDRX_UNLIKELY(NDRX_UNLIKELY).
 * This is useful for error handling, because mostly we do not have errors like
 * malloc fail etc..
 */
#if HAVE_EXPECT

#define NDRX_LIKELY(x)      __builtin_expect(!!(x), 1)
#define NDRX_UNLIKELY(x)    __builtin_expect(!!(x), 0)

#else

/*
 * And this is fallback if we do not have expect compiler option.
 */
#define NDRX_LIKELY(x)      (x)
#define NDRX_UNLIKELY(x)    (x)

#endif

#define NDRX_DIR_PERM 0775  /**< user and group have write perm, use umask to limit */
#define NDRX_COM_SVQ_PRIV   999 /**< This is private command id use by system v */

/**
 * globals
 */
#define ATMI_MSG_MAX_SIZE   NDRX_MSGSIZEMAX
#define NDRX_MAX_Q_SIZE     128
#define NDRX_MAX_ID_SIZE    96      /**< pfx + binary name + server id + pid + nodeid */
#define NDRX_MAX_KEY_SIZE   128     /**< Key size for random key                      */
#define NDRX_QDIAG_MSG_SIZE 256     /**< Q diagnostic message size                    */
/* List of configuration environment variables */
#define CONF_NDRX_TOUT           "NDRX_TOUT"
#define CONF_NDRX_ULOG           "NDRX_ULOG"
#define CONF_NDRX_QPREFIX        "NDRX_QPREFIX"
#define CONF_NDRX_SVCMAX         "NDRX_SVCMAX"
#define CONF_NDRX_SRVMAX         "NDRX_SRVMAX"
#define CONF_NDRX_CLTMAX         "NDRX_CLTMAX"     /**< Max number of client, cpm */
#define CONF_NDRX_CONFIG         "NDRX_CONFIG"
#define CONF_NDRX_QPATH          "NDRX_QPATH"
#define CONF_NDRX_SHMPATH        "NDRX_SHMPATH"
#define CONF_NDRX_CMDWAIT        "NDRX_CMDWAIT"    /**< Command wait time */
#define CONF_NDRX_DPID           "NDRX_DPID"       /**< PID file for backend-process */
#define CONF_NDRX_DMNLOG         "NDRX_DMNLOG"     /**< Log file for backend process */
#define CONF_NDRX_LOG            "NDRX_LOG"        /**< Log file for command line utilty */
#define CONF_NDRX_RNDK           "NDRX_RNDK"       /**< Random key */
#define CONF_NDRX_MSGMAX         "NDRX_MSGMAX"     /**< Posix queues, max msgs */
#define CONF_NDRX_MSGSIZEMAX     "NDRX_MSGSIZEMAX" /**< Maximum size of message for posixq */
#define CONF_NDRX_MSGQUEUESMAX   "NDRX_MSGQUEUESMAX"/**< Max number of Queues (for sysv)  */
#define CONF_NDRX_SVQREADERSMAX  "NDRX_SVQREADERSMAX"/**< SysV Shared mem max readers (rwlck)  */
#define CONF_NDRX_SANITY         "NDRX_SANITY"     /**< Time in seconds after which do sanity check for dead processes */
#define CONF_NDRX_QPATH          "NDRX_QPATH"      /**< Path to place on fs where queues lives */
#define CONF_NDRX_IPCKEY         "NDRX_IPCKEY"     /**< IPC Key for shared memory */
#define CONF_NDRX_DQMAX          "NDRX_DQMAX"      /**< Internal NDRXD Q len (max msgs) */
#define CONF_NDRX_NODEID         "NDRX_NODEID"     /**< Cluster Node Id */
#define CONF_NDRX_LDBAL          "NDRX_LDBAL"      /**< Load balance, 0 = run locally, 100 = run on cluster */
#define CONF_NDRX_CLUSTERISED    "NDRX_CLUSTERISED"/**< Do we run in clusterized environment? */
/* thise we can override with env_over: */
#define CONF_NDRX_XA_RES_ID      "NDRX_XA_RES_ID"   /**< XA Resource ID           */
#define CONF_NDRX_XA_OPEN_STR    "NDRX_XA_OPEN_STR" /**< XA Open string           */
#define CONF_NDRX_XA_CLOSE_STR   "NDRX_XA_CLOSE_STR"/**< XA Close string          */
#define CONF_NDRX_XA_DRIVERLIB   "NDRX_XA_DRIVERLIB"/**< Enduro RM Library        */
#define CONF_NDRX_XA_RMLIB       "NDRX_XA_RMLIB"    /**< RM library               */
#define CONF_NDRX_XA_FLAGS       "NDRX_XA_FLAGS"    /**< Enduro/X Specific flags  */
#define CONF_NDRX_XA_LAZY_INIT   "NDRX_XA_LAZY_INIT"/**< 0 - load libs on 
                                                      init, 1 - load at use     */
#define CONF_NDRX_NRSEMS         "NDRX_NRSEMS"      /**< Number of semaphores used for
                                                       service shared memory (for poll() mode)*/
#define CONF_NDRX_NRSEMS_DFLT    30                 /**< default number of semphoares  */
#define CONF_NDRX_MAXSVCSRVS     "NDRX_MAXSVCSRVS"  /**< Max servers per service (only for poll() mode) */
#define CONF_NDRX_MAXSVCSRVS_DFLT 30                /**< default for NDRX_MAXSVCSRVS param */
#define CONF_NDRX_XADMIN_CONFIG  "NDRX_XADMIN_CONFIG" /**< Xadmin config file          */
#define CONF_NDRX_DEBUG_CONF     "NDRX_DEBUG_CONF"  /**< debug config file              */
#define CONF_NDRX_PLUGINS        "NDRX_PLUGINS"     /**< list of plugins, ';' seperated */
#define CONF_NDRX_SYSFLAGS       "NDRX_SYSFLAGS"    /**< Additional flags to process    */
#define CONF_NDRX_SILENT         "NDRX_SILENT"      /**< Make xadmin silent             */
#define CONF_NDRX_TESTMODE       "NDRX_TESTMODE"    /**< Is Enduro/X test mode enabled  */
/** call/receive timeout for xadmin - override of NDRX_TOUT */
#define CONF_NDRX_XADMINTOUT     "NDRX_XADMINTOUT" 
/** Service process name */
#define CONF_NDRX_SVPROCNAME     "NDRX_SVPROCNAME" 
/** Service command line */
#define CONF_NDRX_SVCLOPT        "NDRX_SVCLOPT" 
/** Parent process PID of server process */
#define CONF_NDRX_SVPPID         "NDRX_SVPPID" 
/** Server ID */
#define CONF_NDRX_SVSRVID        "NDRX_SVSRVID" 
/** Number of attempts (with 1 sec sleep in between) to wait for ndrxd normal
 * state required by command
 */
#define CONF_NDRX_NORMWAITMAX    "NDRX_NORMWAITMAX"
/** Default for  NDRX_NORMWAITMAX */    
#define CONF_NDRX_NORMWAITMAX_DLFT    60
    
/** resource manager override file*/
#define CONF_NDRX_RMFILE        "NDRX_RMFILE"
    
/** Enduro/X MW home               */
#define CONF_NDRX_HOME          "NDRX_HOME"
    
/** Feedback pool allocator options */
#define CONF_NDRX_FPAOPTS       "NDRX_FPAOPTS"
    
/** Stack size for new threads produced by Enduro/X in kilobytes */
#define CONF_NDRX_THREADSTACKSIZE   "NDRX_THREADSTACKSIZE"

/** Minimum number of dispatch threads for ATMI Server */
#define CONF_NDRX_MINDISPATCHTHREADS   "NDRX_MINDISPATCHTHREADS"

/** Maximum number of dispatch threads for ATMI Server (not used currently) */
#define CONF_NDRX_MAXDISPATCHTHREADS   "NDRX_MAXDISPATCHTHREADS"

/** Used by System-V tout thread -> sleep period between timeout-scans    
 * in milli-seconds. Default is 1000.
 */
#define CONF_NDRX_SCANUNIT   "NDRX_SCANUNIT"
#define CONF_NDRX_SCANUNIT_DFLT   1000
#define CONF_NDRX_SCANUNIT_MIN    1


#define CONF_VIEWFILES  "VIEWFILES"         /* List of view files to load      */
#define CONF_VIEWDIR    "VIEWDIR"           /* Folders with view files stored, ':' - sep   */

#define CONF_FLDTBLDIR	"FLDTBLDIR"
#define CONF_FIELDTBLS	"FIELDTBLS"

#ifdef	__cplusplus
}
#endif

#endif	/* NDRSTANDARD_H */

/* vim: set ts=4 sw=4 et smartindent: */
