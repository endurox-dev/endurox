/**
 * @brief ATMI cache structures
 *
 * @file atmi_cache.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

#ifndef ATMI_CACHE_H
#define	ATMI_CACHE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <cconfig.h>
#include <atmi.h>
#include <atmi_int.h>
#include <exhash.h>
#include <exdb.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define NDRX_TPCACHE_DEBUG
    
/**
 * Keywords for cache definition (KWC) - flags
 */
#define NDRX_TPCACHE_KWC_INVLKEYGRP             "invalkeygrp"
#define NDRX_TPCACHE_KWC_INVAL                  "inval"
#define NDRX_TPCACHE_KWC_SAVEREG                "putrex"
#define NDRX_TPCACHE_KWC_REPL                   "getreplace"
#define NDRX_TPCACHE_KWC_MERGE                  "getmerge"
#define NDRX_TPCACHE_KWC_SAVEFULL               "putfull"
#define NDRX_TPCACHE_KWC_SAVESETOF              ""
#define NDRX_TPCACHE_KWC_NEXT                   "next"
#define NDRX_TPCACHE_KWC_DELREG                 "delrex"
#define NDRX_TPCACHE_KWC_DELFULL                "delfull"
#define NDRX_TPCACHE_KWC_DELSETOF               ""
#define NDRX_TPCACHE_KWC_KEYITEMS               ""
    
/* Cache settings keywords: */
#define NDRX_TPCACHE_KWC_KEYGRPMAXTPERRNO       "keygrpmaxtperrno"
#define NDRX_TPCACHE_KWC_KEYGRPMAXTPURCODE      "keygrpmaxtpurcode"
#define NDRX_TPCACHE_KWC_KEYGROUPMREJ           "keygroupmrej"
#define NDRX_TPCACHE_KWC_KEYGROUPMAX            "keygroupmax"
#define NDRX_TPCACHE_KWC_FLAGS                  "flags"
#define NDRX_TPCACHE_KWC_TYPE                   "type"
#define NDRX_TPCACHE_KWC_SUBTYPE                "subtype"
#define NDRX_TPCACHE_KWC_CACHEDB                "cachedb"
#define NDRX_TPCACHE_KWC_INVAL_SVC              "inval_svc"
#define NDRX_TPCACHE_KWC_INVAL_IDX              "inval_idx"
#define NDRX_TPCACHE_KWC_KEYGRPDB               "keygrpdb"
#define NDRX_TPCACHE_KWC_KEYFMT                 "keyfmt"
#define NDRX_TPCACHE_KWC_KEYGRPFMT              "keygrpfmt"
#define NDRX_TPCACHE_KWC_RULE                   "rule"
#define NDRX_TPCACHE_KWC_REFRESHRULE            "refreshrule"
#define NDRX_TPCACHE_KWC_SAVE                   "save"
/* RFU, currently only full delete broadcast is supported */
#define NDRX_TPCACHE_KWC_DELETE                 "delete"
#define NDRX_TPCACHE_KWC_RSPRULE                "rsprule"
    
/* KWD -> keywords for database */
#define NDRX_TPCACHE_KWD_MAX_READERS            "max_readers"
#define NDRX_TPCACHE_KWD_MAX_DBS                "max_dbs"
#define NDRX_TPCACHE_KWD_MAP_SIZE               "map_size"
#define NDRX_TPCACHE_KWD_SUBSCR                 "subscr"
#define NDRX_TPCACHE_KWD_CACHEDB                "cachedb"
#define NDRX_TPCACHE_KWD_RESOURCE               "resource"
#define NDRX_TPCACHE_KWD_PERMS                  "perms"
#define NDRX_TPCACHE_KWD_LIMIT                  "limit"
#define NDRX_TPCACHE_KWD_EXPIRY                 "expiry"
#define NDRX_TPCACHE_KWD_FLAGS                  "flags"
    
/* Database flags, keywords */

#define NDRX_TPCACHE_KWD_KEYITEMS               "keyitems"
#define NDRX_TPCACHE_KWD_BOOTRST                "bootreset"
#define NDRX_TPCACHE_KWD_LRU                    "lru"
#define NDRX_TPCACHE_KWD_HITS                   "hits"
#define NDRX_TPCACHE_KWD_FIFO                   "fifo"
#define NDRX_TPCACHE_KWD_KEYGRP                 "keygroup"
#define NDRX_TPCACHE_KWD_BCASTPUT               "bcastput"
#define NDRX_TPCACHE_KWD_BCASTDEL               "bcastdel"
#define NDRX_TPCACHE_KWD_TIMESYNC               "timesync"
#define NDRX_TPCACHE_KWD_SCANDUP                "scandup"
#define NDRX_TPCACHE_KWD_CLRNOSVC               "clrnosvc"
#define NDRX_TPCACHE_KWD_NOSYNC                 "nosync"
#define NDRX_TPCACHE_KWD_NOMETASYNC             "nometasync"

/* Database flags: */
    
#define NDRX_TPCACHE_FLAGS_EXPIRY    0x00000001   /* Cache recoreds expires after add */
#define NDRX_TPCACHE_FLAGS_LRU       0x00000002   /* limited, last recently used stays*/
#define NDRX_TPCACHE_FLAGS_HITS      0x00000004   /* limited, more hits, longer stay  */
#define NDRX_TPCACHE_FLAGS_FIFO      0x00000008   /* First in, first out cache        */
#define NDRX_TPCACHE_FLAGS_BOOTRST   0x00000010   /* reset cache on boot              */
#define NDRX_TPCACHE_FLAGS_BCASTPUT  0x00000020   /* Shall we broadcast the events?   */
#define NDRX_TPCACHE_FLAGS_BCASTDEL  0x00000040   /* Broadcast delete events?         */
#define NDRX_TPCACHE_FLAGS_TIMESYNC  0x00000080   /* Perfrom timsync                  */
#define NDRX_TPCACHE_FLAGS_SCANDUP   0x00000100   /* Scan for duplicates by tpcached  */
#define NDRX_TPCACHE_FLAGS_CLRNOSVC  0x00000200   /* Clean unadvertised svc records   */
#define NDRX_TPCACHE_FLAGS_NOSYNC    0x00000400   /* Do not flush to disk at commit   */
#define NDRX_TPCACHE_FLAGS_NOMETASYNC 0x00000800  /* Do not flush to disk metadata    */
    
/* so in case if this is key item, then add record to keygroup
 * if removing key group, then remove all linked key items.
 */
    
#define NDRX_TPCACHE_FLAGS_KEYGRP    0x00000800   /* Is this key group?               */
#define NDRX_TPCACHE_FLAGS_KEYITEMS  0x00001000   /* Is this key item?                */
    
#define NDRX_TPCACHE_TPCF_SAVEREG    0x00000001   /* Save record can be regexp        */
#define NDRX_TPCACHE_TPCF_REPL       0x00000002   /* Replace buf                      */
#define NDRX_TPCACHE_TPCF_MERGE      0x00000004   /* Merge buffers                    */
#define NDRX_TPCACHE_TPCF_SAVEFULL   0x00000008   /* Save full buffer                 */
#define NDRX_TPCACHE_TPCF_SAVESETOF  0x00000010   /* Save set of fields               */
#define NDRX_TPCACHE_TPCF_INVAL      0x00000020   /* Invalidate other cache           */
#define NDRX_TPCACHE_TPCF_NEXT       0x00000040   /* Process next rule (only for inval)*/
#define NDRX_TPCACHE_TPCF_DELREG     0x00000080   /* Delete record can be regexp      */
#define NDRX_TPCACHE_TPCF_DELFULL    0x00000100   /* Delete full buffer               */
#define NDRX_TPCACHE_TPCF_DELSETOF   0x00000200   /* Delete set of fields             */
#define NDRX_TPCACHE_TPCF_KEYITEMS   0x00000400   /* Cache is items for group         */
#define NDRX_TPCACHE_TPCF_INVLKEYGRP 0x00000800   /* invalidate whole group during op */

#define NDRX_TPCACH_INIT_NORMAL      0   /* Normal init (client & server)    */
#define NDRX_TPCACH_INIT_BOOT        1   /* Boot mode init (ndrxd startst)   */

/* -1 = EXFAIL standard error */
#define NDRX_TPCACHE_ENOTFOUND      -2   /* Record not found                 */
#define NDRX_TPCACHE_ENOCACHEDATA   -3   /* Data not cached                  */
#define NDRX_TPCACHE_ENOCACHE       -4   /* Service not in cache config      */
#define NDRX_TPCACHE_ENOKEYDATA     -5   /* No key data found                */
#define NDRX_TPCACHE_ENOTYPESUPP    -6   /* Type not supported               */
#define NDRX_TPCACHE_ENOTFOUNDLIM   -7   /* not found but lookups limited    */


#define NDRX_TPCACHE_BCAST_DFLT     ""   /* default event                    */
#define NDRX_TPCACHE_BCAST_DELFULL  "F"  /* delete full                      */
#define NDRX_TPCACHE_BCAST_DELFULLC 'F'  /* delete full                      */
    
#define NDRX_TPCACHE_BCAST_GROUP    "G"  /* Group operatoin                  */
#define NDRX_TPCACHE_BCAST_GROUPC   'G'  /* Group operatoin                  */
    
#define NDRX_CACHES_BLOCK           "caches"
#define NDRX_CACHE_MAX_READERS_DFLT 1000
#define NDRX_CACHE_MAP_SIZE_DFLT    10485760 /* 10M */
#define NDRX_CACHE_PERMS_DFLT       0664
#define NDRX_CACHE_MAX_DBS_DFLT     2
    
#define NDRX_CACHE_BCAST_MODE_PUT   1
#define NDRX_CACHE_BCAST_MODE_DEL   2
#define NDRX_CACHE_BCAST_MODE_KIL   3       /* drop the database              */
#define NDRX_CACHE_BCAST_MODE_MSK   4       /* Delete by mask                 */
#define NDRX_CACHE_BCAST_MODE_DKY   5       /* Delete by key                  */

/*
 * Command code sent to tpcachesv
 */
#define NDRX_CACHE_SVCMD_DELBYEXPR  'E'     /* Delete by expression           */
#define NDRX_CACHE_SVCMD_DELBYKEY   'K'     /* Delete by key (direct lookup)  */

/* Command line commands: */
#define NDRX_CACHE_SVCMD_CLSHOW     's'     /* Show cache cli                 */
#define NDRX_CACHE_SVCMD_CLCDUMP    'd'     /* Dump cli                       */
#define NDRX_CACHE_SVCMD_CLDEL      'D'     /* Delete, cli                    */

/* Command flags */
#define NDRX_CACHE_SVCMDF_DELREG    0x00000001    /* Delete key by regexp     */
    
    
#define NDRX_CACHE_OPEXPRMAX        PATH_MAX /* max len of operation expression*/
#define NDRX_CACHE_NAMEDBSEP        '@'     /* named db seperatror             */

/**
 * Dump the cache database configuration
 */
#define NDRX_TPCACHEDB_DUMPCFG(LEV, CACHEDB)\
    NDRX_LOG(LEV, "------------ CACHE DB CONFIG DUMP ---------------");\
    NDRX_LOG(LEV, "%s full name=[%s]", NDRX_TPCACHE_KWD_CACHEDB, CACHEDB->cachedb);\
    NDRX_LOG(LEV, "cachedbnam logical name=[%s]", CACHEDB->cachedbnam);\
    NDRX_LOG(LEV, "cachedbphy physical name=[%s]", CACHEDB->cachedbphy);\
    NDRX_LOG(LEV, "%s ptr=[%p]", NDRX_TPCACHE_KWD_CACHEDB, CACHEDB);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWD_RESOURCE, CACHEDB->resource);\
    NDRX_LOG(LEV, "%s=[%ld]", NDRX_TPCACHE_KWD_LIMIT, CACHEDB->limit);\
    NDRX_LOG(LEV, "%s=[%ld] sec", NDRX_TPCACHE_KWD_EXPIRY, CACHEDB->expiry);\
    NDRX_LOG(LEV, "%s=[%ld]", NDRX_TPCACHE_KWD_FLAGS, CACHEDB->flags);\
    NDRX_LOG(LEV, "flags, 'expiry' = [%d]", \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_EXPIRY));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_LRU, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_LRU));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_HITS, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_HITS));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_FIFO, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_FIFO));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_BOOTRST, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_BOOTRST));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_BCASTPUT, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_BCASTPUT));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_BCASTDEL, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_BCASTDEL));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_TIMESYNC, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_TIMESYNC));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_SCANDUP, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_SCANDUP));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_CLRNOSVC, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_CLRNOSVC));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_KEYITEMS, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_KEYITEMS));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_KEYGRP, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_KEYGRP));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_NOSYNC, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_NOSYNC));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWD_NOMETASYNC, \
                    !!(CACHEDB->flags &  NDRX_TPCACHE_FLAGS_NOMETASYNC));\
    NDRX_LOG(LEV, "%s=[%ld]", NDRX_TPCACHE_KWD_MAX_READERS, CACHEDB->max_readers);\
    NDRX_LOG(LEV, "%s=[%ld]", NDRX_TPCACHE_KWD_MAP_SIZE, CACHEDB->map_size);\
    NDRX_LOG(LEV, "%s=[%o]", NDRX_TPCACHE_KWD_PERMS, CACHEDB->perms);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWD_SUBSCR, CACHEDB->subscr);\
    NDRX_LOG(LEV, "-------------------------------------------------");

    
/**
 * Dump tpcall configuration
 */
#define NDRX_TPCACHETPCALL_DUMPCFG(LEV, TPCALLCACHE)\
    NDRX_LOG(LEV, "------------ TPCALL CACHE CONFIG DUMP ---------------");\
    NDRX_LOG(LEV, "cache ptr=[%p]", TPCALLCACHE);\
    NDRX_LOG(LEV, "%s name full =[%s]", NDRX_TPCACHE_KWC_CACHEDB,\
                TPCALLCACHE->cachedb);\
    NDRX_LOG(LEV, "cachedb_ptr=[%p]", TPCALLCACHE->cachedb);\
    NDRX_LOG(LEV, "idx=[%d]", TPCALLCACHE->idx);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_KEYFMT, TPCALLCACHE->keyfmt);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_KEYGRPFMT, TPCALLCACHE->keygrpfmt);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_SAVE, TPCALLCACHE->saveproj.expression);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_DELETE, TPCALLCACHE->delproj.expression);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_RULE, TPCALLCACHE->rule);\
    NDRX_LOG(LEV, "rule_tree=[%p]", TPCALLCACHE->rule_tree);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_REFRESHRULE, TPCALLCACHE->refreshrule);\
    NDRX_LOG(LEV, "refreshrule_tree=[%p]", TPCALLCACHE->refreshrule_tree);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_RSPRULE, TPCALLCACHE->rsprule);\
    NDRX_LOG(LEV, "rsprule_tree=[%p]", TPCALLCACHE->rsprule_tree);\
    NDRX_LOG(LEV, "str_buf_type=[%s]", TPCALLCACHE->str_buf_type);\
    NDRX_LOG(LEV, "str_buf_subtype=[%s]", TPCALLCACHE->str_buf_subtype);\
    NDRX_LOG(LEV, "buf_type=[%s]", TPCALLCACHE->buf_type->type);\
    NDRX_LOG(LEV, "flags=[%s]", TPCALLCACHE->flagsstr);\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWC_SAVEREG,\
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_SAVEREG));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWC_REPL,\
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_REPL));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWC_MERGE,\
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_MERGE));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWC_SAVEFULL,\
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_SAVEFULL));\
    NDRX_LOG(LEV, "flags 'inval' = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_INVAL));\
    NDRX_LOG(LEV, "flags (computed) save list = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_SAVESETOF));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWC_DELREG,\
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_DELREG));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", NDRX_TPCACHE_KWC_DELFULL,\
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_DELFULL));\
    NDRX_LOG(LEV, "flags (computed) delete list = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_DELSETOF));\
    NDRX_LOG(LEV, "flags (computed) key items = [%d]", \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_KEYITEMS));\
    NDRX_LOG(LEV, "flags, '%s' = [%d]", \
                    NDRX_TPCACHE_KWC_INVLKEYGRP, \
                    !!(TPCALLCACHE->flags &  NDRX_TPCACHE_TPCF_INVLKEYGRP));\
    NDRX_LOG(LEV, "inval_cache=[%p]", TPCALLCACHE->inval_cache);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_INVAL_SVC, TPCALLCACHE->inval_svc);\
    NDRX_LOG(LEV, "%s=[%d]", NDRX_TPCACHE_KWC_INVAL_IDX, TPCALLCACHE->inval_idx);\
    NDRX_LOG(LEV, "%s=[%ld]", NDRX_TPCACHE_KWC_KEYGROUPMAX,\
                    TPCALLCACHE->keygroupmax);\
    NDRX_LOG(LEV, "%s=[%s]", NDRX_TPCACHE_KWC_KEYGROUPMREJ,\
                    TPCALLCACHE->keygroupmrej?TPCALLCACHE->keygroupmrej:"");\
    NDRX_LOG(LEV, "keygroupmrej_abuf=[%p]", TPCALLCACHE->keygroupmrej_abuf);\
    NDRX_LOG(LEV, "%s=[%d]", NDRX_TPCACHE_KWC_KEYGRPMAXTPERRNO,\
                    TPCALLCACHE->keygroupmtperrno);\
    NDRX_LOG(LEV, "%s=[%ld]", NDRX_TPCACHE_KWC_KEYGRPMAXTPURCODE,\
                    TPCALLCACHE->keygroupmtpurcode);\
    NDRX_LOG(LEV, "-------------------------------------------------");


#define NDRX_TPCACHETPCALL_DBDATA(LEV, DBDATA)\
    NDRX_LOG(LEV, "------------------ DB DATA DUMP -----------------");\
    NDRX_LOG(LEV, "saved_tperrno = [%d]", DBDATA->saved_tperrno);\
    NDRX_LOG(LEV, "saved_tpurcode = [%ld]", DBDATA->saved_tpurcode);\
    NDRX_LOG(LEV, "atmi_buf_len = [%ld]", DBDATA->saved_tpurcode);\
    NDRX_DUMP(LEV, "BLOB data", DBDATA->atmi_buf, DBDATA->atmi_buf_len);\
    NDRX_LOG(LEV, "-------------------------------------------------");
    

#define NDRX_CACHE_TPERROR(atmierr, fmt, ...)\
        NDRX_LOG(log_error, fmt, ##__VA_ARGS__);\
        userlog(fmt, ##__VA_ARGS__);\
        ndrx_TPset_error_fmt(atmierr, fmt, ##__VA_ARGS__);

#define NDRX_CACHE_ERROR(fmt, ...)\
        NDRX_LOG(log_error, fmt, ##__VA_ARGS__);\
        userlog(fmt, ##__VA_ARGS__);
    
    
#define NDRX_CACHE_TPERRORNOU(atmierr, fmt, ...)\
        NDRX_LOG(log_error, fmt, ##__VA_ARGS__);\
        ndrx_TPset_error_fmt(atmierr, fmt, ##__VA_ARGS__);
    
#define NDRX_CACHE_MAGIC        0xab4388ef
    
    
/* macro is used to verify cache record. */
#define NDRX_CACHE_CHECK_DBDATA(cachedata_, exdata_, key_, atmierr_)\
if (cachedata_->mv_size < sizeof(ndrx_tpcache_data_t))\
    {\
        if (atmierr_ > TPMINVAL)\
        {\
            NDRX_CACHE_TPERROR(atmierr_, "Corrupted cache data - invalid minimums size, "\
                "expected: %ld, got %ld for key: [%s]", \
                (long)sizeof(ndrx_tpcache_data_t), (long)cachedata_->mv_size, key_?key_:"(nil)");\
        }\
        else\
        {\
            NDRX_CACHE_ERROR("Corrupted cache data - invalid minimums size, "\
                "expected: %ld, got %ld for key: [%s]", \
                (long)sizeof(ndrx_tpcache_data_t), (long)cachedata_->mv_size, key_?key_:"(nil)");\
        }\
        EXFAIL_OUT(ret);\
    }\
    if (NDRX_CACHE_MAGIC!=exdata_->magic)\
    {\
        if (atmierr_ > TPMINVAL)\
        {\
            NDRX_CACHE_TPERROR(atmierr_, "Corrupted cache data - invalid "\
                    "magic expected: %x got %x", exdata_->magic, NDRX_CACHE_MAGIC);\
        }\
        else\
        {\
            NDRX_CACHE_ERROR("Corrupted cache data - invalid "\
                    "magic expected: %x got %x", exdata_->magic, NDRX_CACHE_MAGIC);\
        }\
        EXFAIL_OUT(ret);\
    }

/* verify key db record */
#define NDRX_CACHE_CHECK_DBKEY(keydb_, atmierr_)\
    if (EXEOS!=((char *)keydb_->mv_data)[keydb_->mv_size-1])\
        {\
            NDRX_DUMP(log_error, "Invalid cache key", \
                        keydb_->mv_data, keydb_->mv_size);\
            if (atmierr_ > TPMINVAL)\
            {\
                 NDRX_CACHE_TPERROR(atmierr_, "%s: Invalid cache key, len: %ld not "\
                        "terminated with EOS!", __func__, keydb_->mv_size);\
            }\
            else\
            {\
                NDRX_CACHE_ERROR("%s: Invalid cache key, len: %ld not "\
                        "terminated with EOS!", __func__, keydb_->mv_size);\
            }\
            EXFAIL_OUT(ret);\
        }
    
/**
 * Number of bytes to move around in c struct
 */
#define NDRX_TPCACHE_ALISZ (EXOFFSET(ndrx_tpcache_data_t,atmi_buf) - \
        EXOFFSET(ndrx_tpcache_data_t,magic))
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * Physical db
 */
typedef struct ndrx_tpcache_phydb ndrx_tpcache_phydb_t;
struct ndrx_tpcache_phydb
{
    char cachedb[NDRX_CCTAG_MAX+1];/*common logical name (after @)           */
    char resource[PATH_MAX+1];     /* physical path of the cache folder       */
    EDB_env *env; /* env handler */
    int num_usages;                 /* number of logical dbs using this        */
    /* Make structure hashable: */
    EX_hash_handle hh;
};


/**
 * Cache database, logical
 */
typedef struct ndrx_tpcache_db ndrx_tpcache_db_t;
struct ndrx_tpcache_db
{
    char cachedb[NDRX_CCTAG_MAX+1];/* full logical name with @ inside               */
    char cachedbnam[NDRX_CCTAG_MAX+1];/* logicla name of db                         */
    char cachedbphy[NDRX_CCTAG_MAX+1];/* physical name of db                        */
    char resource[PATH_MAX+1];     /* physical path of the cache folder             */
    ndrx_tpcache_phydb_t *phy;  /* link to physical db                              */
    long limit;                 /* number of records limited for cache used by 2,3,4*/
    long expiry;                /* Number of seconds for record to live             */
    long flags;                 /* configuration flags for this cache               */
    long max_readers;           /* db settings                                      */
    long map_size;              /* db settings                                      */
    long max_dbs;               /* max number of databases                          */
    int broadcast;              /* Shall we broadcast the events                    */
    int perms;                  /* permissions of the database resource             */
    char subscr[NDRX_EVENT_EXPR_MAX]; /* expression for consuming PUT events        */
    
    /* LMDB Related */
    
    EDB_dbi dbi;  /* named (unnamed) db */
    
    /* Make structure hashable: */
    EX_hash_handle hh;
};

/**
 * This structure describes how to project a slice of the buffer
 */
struct ndrx_tpcache_projbuf
{
    char expression[PATH_MAX+1]; /* Projection expression               */
    
    /* Save can be regexp, so we need to compile it...!                 */
    int regex_compiled;
    regex_t regex;
    void *typpriv; /* private list of save data, could be projcpy list? */
    long typpriv2;
};
typedef struct ndrx_tpcache_projbuf ndrx_tpcache_projbuf_t;

/**
 * cache entry, this is linked list as 
 */
typedef struct ndrx_tpcallcache ndrx_tpcallcache_t;
struct ndrx_tpcallcache
{
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    char cachedbnm[NDRX_CCTAG_MAX+1]; /* cache db logical name (subsect of @cachedb)  */
    ndrx_tpcache_db_t *cachedb;
    ndrx_tpcache_db_t *keygrpdb;          /* key group indicator              */
    char keyfmt[PATH_MAX+1];
    /*
     * To use key group,
     * the database shall be marked with flag as "keyitems"
     * and the master database shall be marked as "keygroup"
     */
    char keygrpfmt[PATH_MAX+1];         /* Key group format                 */
    int idx;                            /* index of this cache for service  */
    
    ndrx_tpcache_projbuf_t saveproj;    /* Save buffer projection           */
    ndrx_tpcache_projbuf_t delproj;     /* Delete buffer projection         */
    
    /* We need a flags here to allow regex, for example. But the regex is */
    char flagsstr[NDRX_CACHE_FLAGS_MAX+1];
    long flags;
    
    /* Rule for refreshing the data (this is higher priority) */
    char refreshrule[PATH_MAX+1];
    char *refreshrule_tree;
    
    /* Rule for saving the data: */
    char rule[PATH_MAX+1];
    char *rule_tree;
    
    char rsprule[PATH_MAX+1];
    char *rsprule_tree;
    char str_buf_type[XATMI_TYPE_LEN+1];
    char str_buf_subtype[XATMI_SUBTYPE_LEN+1];
    
    typed_buffer_descr_t *buf_type;
    
    /* For invalidating their cache, in case if rule matched */
    ndrx_tpcallcache_t *inval_cache;    /* their cache to invalidate        */
    char inval_svc[MAXTIDENT+1];        /* Service name of their cache      */
    int inval_idx;                      /* Index of their cache, 0 based    */
    
    long keygroupmax;   /* maximum number of keys in keygroup               */
    char *keygroupmrej; /* Reject expression of keygroup, if max reached    */
    char *keygroupmrej_abuf; /* Atmi allocated worker buffer for keygroy*/
    
    /* might want to reject with specific ATMI code? */
    int keygroupmtperrno;
    long keygroupmtpurcode;
    
    /* this is linked list of caches */
    ndrx_tpcallcache_t *next, *prev;
};

/**
 * This is hash of services which are cached.
 */
struct ndrx_tpcache_svc
{
    char svcnm[MAXTIDENT+1];    /* cache db logical name (subsect of @cachedb)*/

    int in_hash;                /* Are we added to hash list?                 */
    ndrx_tpcallcache_t *caches; /* This list list of caches */
        
    /* Make structure hashable: */
    EX_hash_handle hh;
};
typedef struct ndrx_tpcache_svc ndrx_tpcache_svc_t;


/**
 * Structure for holding data up, payload
 */
struct ndrx_tpcache_data
{
    /* ...align from magic (including) */
    int magic;          /**< Magic bytes                      */
    char svcnm[MAXTIDENT+1]; /* Service name of data        */
    int cache_idx;      /**< this is cache index of adder     */
    int saved_tperrno;
    long saved_tpurcode;
    long t;             /**< UTC timestamp of message         */
    long tusec;         /**< UTC microseconds                 */
    int nrshift;        /**< number of byte shifted for alignment */
    /** time when we picked up the record */
    long hit_t;         /**< UTC timestamp of message         */
    long hit_tusec;     /**< UTC microseconds                 */
    long hits;          /**< Number of cache hits             */
    long flags;         /**< cache flags                      */
    
    short nodeid;       /**< Node id who put the msg          */
    short atmi_type_id; /**< ATMI type id                     */
    
    /* ...till Payload data (including) */
    long atmi_buf_len;  /**< saved buffer len                 */
    char atmi_buf[0];   /**< the data follows                 */
};
typedef struct ndrx_tpcache_data ndrx_tpcache_data_t;

struct ndrx_tpcache_datasort
{
    /* we need a ptr to key too... */
    
    EDB_val key; /* allocated key */
    
    ndrx_tpcache_data_t data; /* just copy header of data block */
};
typedef struct ndrx_tpcache_datasort ndrx_tpcache_datasort_t;
/*
 * NOTE: Key is used directly as binary data and length 
 */

/*
 * Need a structure for holding the buffer rules according to data types
 */
typedef struct ndrx_tpcache_typesupp ndrx_tpcache_typesupp_t;
struct ndrx_tpcache_typesupp
{
    int type_id;
    /* This shall compile the refresh rule too */
    int (*pf_rule_compile) (ndrx_tpcallcache_t *cache, char *errdet, int errdetbufsz);
    
    int (*pf_rule_eval) (ndrx_tpcallcache_t *cache, char *idata, long ilen, 
                char *errdet, int errdetbufsz);
    
    /* Refresh rule evaluate */
    int (*pf_refreshrule_eval) (ndrx_tpcallcache_t *cache, char *idata, long ilen, 
                char *errdet, int errdetbufsz);
    
    int (*pf_get_key) (ndrx_tpcallcache_t *cache, char *idata, long ilen, char
                *okey, int okey_bufsz, char *errdet, int errdetbufsz);
    
    /* Receive message from cache */
    int (*pf_cache_get) (ndrx_tpcallcache_t *cache, ndrx_tpcache_data_t *exdata, 
            typed_buffer_descr_t *buf_type,
            char *idata, long ilen, char **odata, long *olen, long flags);
    
    int (*pf_cache_put) (ndrx_tpcallcache_t *cache, ndrx_tpcache_data_t *exdata, 
        typed_buffer_descr_t *descr, char *idata, long ilen, long flags);
    
    int (*pf_cache_del) (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char **odata, long *olen);
    
    /* check flags for given type and process the save rule if any */
    int (*pf_process_flags)(ndrx_tpcallcache_t *cache, char *errdet, int errdetbufsz);
    
    /* cache delete callback, to free up memory of any */
    int (*pf_cache_delete)(ndrx_tpcallcache_t *cache);
    
    /* Reject when max reached in group */
    int (*pf_cache_maxreject)(ndrx_tpcallcache_t *cache, char *idata, long ilen, 
        char **odata, long *olen, long flags);
};

/*---------------------------Globals------------------------------------*/

extern NDRX_API ndrx_tpcache_db_t *ndrx_G_tpcache_db; /* ptr to cache database */
extern NDRX_API ndrx_tpcache_svc_t *ndrx_G_tpcache_svc; /* service cache       */
extern NDRX_API ndrx_tpcache_typesupp_t ndrx_G_tpcache_types[];

/*---------------------------Prototypes---------------------------------*/


/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int ndrx_cache_init(int mode);
extern NDRX_API void ndrx_cache_uninit(void);
extern NDRX_API ndrx_tpcache_db_t *ndrx_cache_dbgethash(void);
extern NDRX_API int ndrx_cache_used(void);
extern NDRX_API char* ndrx_cache_mgt_getsvc(void);

extern NDRX_API int ndrx_cache_save (char *svc, char *idata, 
        long ilen, int save_tperrno, long save_tpurcode, int nodeid, long flags,
        int tusec, long t, int is_event);

extern NDRX_API int ndrx_cache_lookup(char *svc, char *idata, long ilen, 
        char **odata, long *olen, long flags, int *should_cache,
        int *saved_tperrno, long *saved_tpurcode, int seterror_not_found);
extern NDRX_API int ndrx_cache_inval_their(char *svc, ndrx_tpcallcache_t *cache, 
        char *key, char *idata, long ilen);

extern NDRX_API int ndrx_cache_inval_by_data(char *svc, char *idata, long ilen,
        char *flags);
extern NDRX_API int ndrx_cache_drop(char *cachedbnm, short nodeid);
extern NDRX_API long ndrx_cache_inval_by_expr(char *cachedbnm, 
        char *keyexpr, short nodeid);
extern NDRX_API int ndrx_cache_inval_by_key(char *cachedbnm, ndrx_tpcache_db_t* db_resolved, 
        char *key, short nodeid, EDB_txn *txn, int ext_tran);
extern NDRX_API int ndrx_cache_maperr(int unixerr);
extern NDRX_API ndrx_tpcallcache_t* ndrx_cache_findtpcall(ndrx_tpcache_svc_t *svcc, 
        typed_buffer_descr_t *buf_type, char *idata, long ilen, int idx);

extern NDRX_API ndrx_tpcallcache_t* ndrx_cache_findtpcall_byidx(char *svcnm, int idx);

extern NDRX_API int ndrx_cache_cmp_fun(const EDB_val *a, const EDB_val *b);

extern NDRX_API int ndrx_cache_edb_get(ndrx_tpcache_db_t *db, EDB_txn *txn, 
        char *key, EDB_val *data_out, int seterror_not_found, int *align);
extern NDRX_API int ndrx_cache_edb_abort(ndrx_tpcache_db_t *db, EDB_txn *txn);
extern NDRX_API int ndrx_cache_edb_commit(ndrx_tpcache_db_t *db, EDB_txn *txn);
extern NDRX_API int ndrx_cache_edb_begin(ndrx_tpcache_db_t *db, EDB_txn **txn,
        	unsigned int flags);

extern NDRX_API int ndrx_cache_edb_set_dupsort(ndrx_tpcache_db_t *db, EDB_txn *txn, 
            EDB_cmp_func *cmp);

extern NDRX_API int ndrx_cache_edb_del (ndrx_tpcache_db_t *db, EDB_txn *txn, 
        char *key, EDB_val *data);

extern NDRX_API int ndrx_cache_edb_put (ndrx_tpcache_db_t *db, EDB_txn *txn, 
        char *key, EDB_val *data, unsigned int flags, int ignore_err);

extern NDRX_API int ndrx_cache_edb_stat (ndrx_tpcache_db_t *db, EDB_txn *txn, 
        EDB_stat * stat);

extern NDRX_API  int ndrx_cache_edb_cursor_open(ndrx_tpcache_db_t *db, EDB_txn *txn, 
            EDB_cursor ** cursor);
extern NDRX_API int ndrx_cache_edb_cursor_get(ndrx_tpcache_db_t *db, EDB_cursor * cursor,
        char *key, EDB_val *data_out, EDB_cursor_op op, int *align);

extern NDRX_API int ndrx_cache_edb_cursor_getfullkey(ndrx_tpcache_db_t *db, 
        EDB_cursor * cursor, EDB_val *keydb, EDB_val *data_out, EDB_cursor_op op,
        int *align);

extern NDRX_API int ndrx_cache_edb_delfullkey (ndrx_tpcache_db_t *db, EDB_txn *txn, 
        EDB_val *keydb, EDB_val *data);

extern NDRX_API ndrx_tpcache_db_t* ndrx_cache_dbresolve(char *cachedb, int mode);

/* management */

extern NDRX_API int ndrx_cache_mgt_ubf2data(UBFH *p_ub, ndrx_tpcache_data_t *cdata, 
        char **data, char **keydata, char **odata, long *olen);

extern NDRX_API int ndrx_cache_mgt_data2ubf(ndrx_tpcache_data_t *cdata, char *keydata,
        UBFH **pp_ub, int incl_blob);

/* UBF support: */
extern NDRX_API int ndrx_cache_delete_ubf(ndrx_tpcallcache_t *cache);
extern NDRX_API int ndrx_cache_proc_flags_ubf(ndrx_tpcallcache_t *cache, 
        char *errdet, int errdetbufsz);

extern NDRX_API int ndrx_cache_maxreject_ubf(ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char **odata, long *olen, long flags);

extern NDRX_API int ndrx_cache_put_ubf (ndrx_tpcallcache_t *cache,
        ndrx_tpcache_data_t *exdata,  typed_buffer_descr_t *descr, 
        char *idata, long ilen, long flags);
extern NDRX_API int ndrx_cache_del_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char **odata, long *olen);
extern NDRX_API int ndrx_cache_get_ubf (ndrx_tpcallcache_t *cache,
        ndrx_tpcache_data_t *exdata, typed_buffer_descr_t *buf_type, 
        char *idata, long ilen, char **odata, long *olen, long flags);
extern NDRX_API int ndrx_cache_ruleval_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen,  char *errdet, int errdetbufsz);
extern NDRX_API int ndrx_cache_rulcomp_ubf (ndrx_tpcallcache_t *cache, 
        char *errdet, int errdetbufsz);
extern NDRX_API int ndrx_cache_keyget_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, char *okey, int okey_bufsz, 
        char *errdet, int errdetbufsz);
extern NDRX_API int ndrx_cache_refeval_ubf (ndrx_tpcallcache_t *cache, 
        char *idata, long ilen,  char *errdet, int errdetbufsz);

/* eventing: */
extern NDRX_API int ndrx_cache_broadcast(ndrx_tpcallcache_t *cache, char *svc, 
        char *idata, long ilen, int event_type, char *flags, int user1, long user2,
        int user3, long user4);
extern NDRX_API int ndrx_cache_inval_by_key_bcastonly(char *cachedbnm, char *key, 
        short nodeid);
extern NDRX_API int ndrx_cache_events_get(string_list_t **list);

/* keygroup: */

extern NDRX_API int ndrx_cache_keygrp_lookup(ndrx_tpcallcache_t *cache, 
            char *idata, long ilen, char **odata, long *olen, char *cachekey,
            long flags);

extern NDRX_API int ndrx_cache_keygrp_addupd(ndrx_tpcallcache_t *cache, 
            char *idata, long ilen, char *cachekey, char *have_keygrp, 
        int deleteop, EDB_txn *txn);

extern NDRX_API int ndrx_cache_keygrp_inval_by_key(ndrx_tpcache_db_t* db, 
        char *key, EDB_txn *txn, char *keyitem_dbname);
 
extern NDRX_API int ndrx_cache_keygrp_inval_by_data(ndrx_tpcallcache_t *cache, 
        char *idata, long ilen, EDB_txn *txn);

extern NDRX_API int ndrx_cache_keygrp_getkey_from_data(ndrx_tpcallcache_t* cache, 
        ndrx_tpcache_data_t *exdata, char *keyout, long keyout_bufsz);

#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_CACHE_H */

/* vim: set ts=4 sw=4 et smartindent: */
