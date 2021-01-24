/**
 * @brief TP LOG, full feature logging system for user, mainly for exporting to
 *   to other languages. C/C++ can use ndebug.h
 *   TODO: tplog* func shall have pull in of init in case if they are called first
 *
 * @file tplog.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <sys_unix.h>
#include <userlog.h>
#include <nstd_tls.h>
#include <errno.h>

#include "atmi_tls.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define API_ENTRY {_Nunset_error();}

/**
 * Get logger mapping,
 * used in source code to perform inits
 */
#define LOGGER_MAP { \
        {&G_nstd_tls->requestlog_tp, &G_nstd_tls->threadlog_tp, &G_tp_debug} \
        ,{&G_nstd_tls->requestlog_ndrx, &G_nstd_tls->threadlog_ndrx, &G_ndrx_debug} \
        ,{&G_nstd_tls->requestlog_ubf, &G_nstd_tls->threadlog_ubf, &G_ubf_debug} \
    }


/**
 * Define save fields for module init
 */
#define LOGGER_SAVE_FIELDS_DEF \
    char sav_code; \
    long sav_flags; \
    char sav_module[NDRX_LOG_MODULE_LEN+1];
    
/**
 * Do save op
 */
#define LOGGER_SAVE_FIELDS(LOGGERPTR) \
    sav_code = LOGGERPTR->code; \
    sav_flags = LOGGERPTR->flags; \
    NDRX_STRCPY_SAFE(sav_module, LOGGERPTR->module);
    
/**
 * Do field restore op
 */
#define LOGGER_RESTORE_FIELDS(LOGGERPTR) \
    LOGGERPTR->code = sav_code; \
    LOGGERPTR->flags = sav_flags; \
    NDRX_STRCPY_SAFE(LOGGERPTR->module, sav_module);
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Debug mapping for req log open/close
 */
typedef struct
{
    ndrx_debug_t *req;  /**< request logging settings               */
    ndrx_debug_t *th;   /**< thread logging settings                */
    ndrx_debug_t *proc; /**< base logging settings (non TLS)        */
} debug_map_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate ndrx_debug_t * ndrx_tplog_getlogger(int logger);

/**
 * Close any TLS loggers if open
 * @param tls thread local storage for ATMI lib
 */
expublic void ndrx_nstd_tls_loggers_close(nstd_tls_t *tls)
{
    ndrx_debug_t *logger[8] = {&tls->threadlog_ndrx, &tls->threadlog_ubf, 
        &tls->threadlog_tp, &tls->requestlog_ndrx, &tls->requestlog_ubf, 
        &tls->requestlog_tp, NULL};
    int i=0;
    
    while (NULL!=logger[i])
    {
        if (NULL!=logger[i]->dbg_f_ptr)
        {
            ndrx_debug_unset_sink(logger[i]->dbg_f_ptr, EXTRUE, EXFALSE);
            logger[i]->dbg_f_ptr=NULL;
            /* clear the file names */
            logger[i]->filename[0]=EXEOS;
            logger[i]->level = EXFAIL;
        }
        i++;
    }
    
}

/**
 * Setup the request logging file.  
 * NOTE that we need: 
 * 1. tplogsetreqfile(<buffer>, <filename> (optional, not needed if name already in buffer))
 * 2. tploggetreqfile(fname (out)) return TRUE/FALSE if we have, and string with the value of fname
 * 3. tplogclosereqfile(); close thre request logging.
 * File will be set to EX_REQLOG = <Full path to request log file>
 * Feature #470 This now applies TP/NDRX/UBF facilities.
 * @param filename file name to log to
 */
expublic void tplogsetreqfile_direct(char *filename)
{
    debug_map_t map[] = LOGGER_MAP;
    int i, dosetup=EXFALSE;
        
    API_ENTRY; /* set TLS too */
    /* have a scope: */
    do 
    {
        
        LOGGER_SAVE_FIELDS_DEF;

        for (i=0; i<N_DIM(map); i++)
        {
            /* Level not set, then there was no init */
            if (EXFAIL==map[i].req->level)
            {	
                LOGGER_SAVE_FIELDS(map[i].req);

                /* file is null, we want to copy off the settings  */
                if (NULL!=map[i].th->dbg_f_ptr)
                {
                    memcpy(map[i].req, map[i].th, sizeof(ndrx_debug_t));
                    ndrx_debug_addref(map[i].req->dbg_f_ptr);
                }
                else
                {
                    /* Copy from TPlog */
                    memcpy(map[i].req, map[i].proc, sizeof(ndrx_debug_t));
                    ndrx_debug_addref(map[i].req->dbg_f_ptr);
                }
    
                /* restore the fields... */
                LOGGER_RESTORE_FIELDS(map[i].req);
                dosetup=EXTRUE;
            }
            else if (0!=strcmp(map[i].req->filename, filename))
            {
                dosetup=EXTRUE;
            }
        }

        /* setup only once, if was setup, ignore the function call */
        /* copy off the pointers, use the same file pointer... */
        if (dosetup)
        {
            for (i=0; i<3; i++)
            {
                /* register sinks for each of the loggers, inherit the flags, if any...
                 * What here we can inherit mkdir & flock
                 *  
                 */
                /* swithc loggers if any...  */
                
                if (NULL==map[i].req->dbg_f_ptr)
                {
                    ndrx_debug_get_sink(filename, EXTRUE, map[i].req);
                }
                else
                {
                    /* leave sink and open new name */
                    ndrx_debug_changename(filename, EXTRUE, map[i].req);
                }
                
                /* set final name -?
                NDRX_STRCPY_SAFE(map[i].req->filename, map[i].req->filename);
                 * */

            }
        }
    } while(0);
}

/**
 * Loop over the loggers and get the request one with full initialization
 * if required
 * @param logger code
 * @return logger found or NULL if invalid flag
 */
exprivate ndrx_debug_t * ndrx_tplog_getlogger(int logger)
{
    int i;
    ndrx_debug_t *ret = NULL;
    debug_map_t map[] = LOGGER_MAP;
    LOGGER_SAVE_FIELDS_DEF;
    
    for (i=0; i<N_DIM(map); i++)
    {
        if (logger & map[i].proc->flags)
        {
            ret = map[i].proc;
            break;
        }
        else if (logger & map[i].th->flags)
        {
            if (EXFAIL==map[i].th->level)
            {
                LOGGER_SAVE_FIELDS(map[i].th);

                /* thread logger inherits from base */
                memcpy(map[i].th, map[i].proc, sizeof(ndrx_debug_t));

                /* increment usage
                 * proc is stable never removed, thus can surely reference it
                 */
                ndrx_debug_addref(map[i].th->dbg_f_ptr);
                
                LOGGER_RESTORE_FIELDS(map[i].th);
                
            }
            ret = map[i].th;
            break;
        }
        else if (logger & map[i].req->flags)
        {
            if (EXFAIL==map[i].req->level)
            {
                LOGGER_SAVE_FIELDS(map[i].req);

                /* request logger inherits from thread
                 * or from base - witch one is closer
                 */
                if (NULL!=map[i].th->dbg_f_ptr)
                {
                    memcpy(map[i].req, map[i].th, sizeof(ndrx_debug_t));
                    
                    /* our thread, thus logger always will have reference */
                    ndrx_debug_addref(map[i].req->dbg_f_ptr);
                }
                else
                {
                    memcpy(map[i].req, map[i].proc, sizeof(ndrx_debug_t));
                    
                    /* process loggers are never removed */
                    ndrx_debug_addref(map[i].req->dbg_f_ptr);
                }
                
                LOGGER_RESTORE_FIELDS(map[i].req);
            }
            ret = map[i].req;
            break;
        }
    }
    
out:
    return ret;
}

/**
 * Close the request file (back to other loggers)
 * This applies to all TP/NDRX/UBF facilities.
 * @param filename
 */
expublic void tplogclosereqfile(void)
{
    int i;
    /* Only if we have a TLS... */
    if (G_nstd_tls)
    {
        debug_map_t map[] = {
            {&G_nstd_tls->requestlog_tp}
            ,{&G_nstd_tls->requestlog_ndrx}
            ,{&G_nstd_tls->requestlog_ubf}
        };
        
        for (i=0; i<N_DIM(map); i++)
        {
            if (map[i].req->dbg_f_ptr)
            {
                /* 
                 * OK we leave the sink
                 */
                ndrx_debug_unset_sink(map[i].req->dbg_f_ptr, EXTRUE, EXFALSE);
                map[i].req->dbg_f_ptr = NULL;
                map[i].req->level = EXFAIL;
                
            }
            map[i].req->filename[0] = EXEOS;
        }
    }
}

/**
 * Reconfigure loggers.
 * TODO: Add fallback re-initialize the same way as done in tplogsetreqfile_direct()
 * @param logger See LOG_FACILITY_*
 * @param lev 0..5 (if -1 (FAIL) then ignored)
 * @param config_line ndrx config line (if NULL/empty then ignored)
 * @param module 4 char module code (which uses the logger)
 * @param new_file New logging file (this overrides the stuff from debug string)
 * @return SUCCEED/FAIL
 */
expublic int tplogconfig(int logger, int lev, char *debug_string, char *module, 
        char *new_file)
{
    int ret = EXSUCCEED;
    ndrx_debug_t *l;
    long new_flags;
    char tmp_filename[PATH_MAX];
    int loggers[] = {LOG_FACILITY_NDRX, 
                    LOG_FACILITY_UBF, 
                    LOG_FACILITY_TP,
                    LOG_FACILITY_TP_THREAD,
                    LOG_FACILITY_TP_REQUEST,
                    LOG_FACILITY_NDRX_THREAD,
                    LOG_FACILITY_NDRX_REQUEST,
                    LOG_FACILITY_UBF_THREAD,
                    LOG_FACILITY_UBF_REQUEST    
    };
    int i;
    
    API_ENTRY; /* set TLS too */
    NDRX_DBG_INIT_ENTRY; /* Do the debug entry (so that we load defaults...) */
 
    for (i=0; i<N_DIM(loggers); i++)
    {
        int curlogger = logger & loggers[i];

        if (!curlogger)
        {
            continue;
        }
            
        l = ndrx_tplog_getlogger(curlogger);

        if (NULL==l)
        {
            /*
            _Nset_error_fmt(NEINVAL, "tplogconfig: Invalid logger: %d", logger);
            FAIL_OUT(ret);
             */
            continue;
        }

        if (NULL!=module && EXEOS!=module[0] && 
                loggers[i] != LOG_FACILITY_NDRX &&
                loggers[i] != LOG_FACILITY_UBF &&
                loggers[i] != LOG_FACILITY_NDRX_THREAD &&
                loggers[i] != LOG_FACILITY_UBF_THREAD &&
                loggers[i] != LOG_FACILITY_NDRX_REQUEST &&
                loggers[i] != LOG_FACILITY_UBF_REQUEST
            )
        {
            NDRX_STRNCPY(l->module, module, 4);
            l->module[4] = EXEOS;
        }

        if (NULL!= debug_string && EXEOS!=debug_string[0])
        {
            /* parse out the logger 
             * Check if file is changed? If changed, then we shall
             * close the old file & open newone...
             */
            NDRX_STRCPY_SAFE(tmp_filename, l->filename);
            if (EXSUCCEED!= (ret = ndrx_init_parse_line(NULL, debug_string, 
                    NULL, l, NULL, 0)))
            {
                _Nset_error_msg(NEFORMAT, "Failed to parse debug string");
                EXFAIL_OUT(ret);
            }

            /* only if new name is not passed in */
            if (0!=strcmp(tmp_filename, l->filename) && 
                    (NULL==new_file || EXEOS==new_file[0]))
            {
                ndrx_debug_changename(l->filename, EXTRUE, l);
            }
        }

        if (EXFAIL!=lev)
        {
            l->level = lev;
        }
        
        /* new file passed in: */
        if (NULL!=new_file && EXEOS!=new_file[0] && 0!=strcmp(new_file, l->filename))
        {            
            ndrx_debug_changename(new_file, EXTRUE, l);
        }

    }
    
out:
    return ret;
}

/**
 * Close thread logger (if open)
 */
expublic void tplogclosethread(void)
{
    int i;
    /* Only if we have a TLS... */
    if (G_nstd_tls)
    {
        debug_map_t map[] = {
            {&G_nstd_tls->threadlog_tp}
            ,{&G_nstd_tls->threadlog_ubf}
            ,{&G_nstd_tls->threadlog_ndrx}
        };
        
        for (i=0; i<N_DIM(map); i++)
        {
            if (map[i].req->dbg_f_ptr)
            {
                /* 
                 * OK we leave the sink
                 */
                ndrx_debug_unset_sink(map[i].req->dbg_f_ptr, EXTRUE, EXFALSE);
                map[i].req->dbg_f_ptr = NULL;
                map[i].req->level = EXFAIL;
                
            }
            map[i].req->filename[0] = EXEOS;
        }
    }
}

/**
 * Get the current request file
 * @param filename (optional) optional to return file name
 * @return TRUE (request file used) / FALSE (request file not used)
 */
expublic int tploggetreqfile(char *filename, int bufsize)
{
    int ret = EXFALSE;
    
    if (NULL==G_nstd_tls->requestlog_tp.dbg_f_ptr)
    {
        ret=EXFALSE;
        goto out;
    }
    
    ret = EXTRUE;

    if (NULL!=filename)
    {
        if (bufsize>0)
        {
            NDRX_STRCPY_SAFE_DST(filename, G_nstd_tls->requestlog_tp.filename, bufsize);
        }
        else
        {
            strcpy(filename, G_nstd_tls->requestlog_tp.filename);
        }
    }
    
out:
    return ret;
}

/**
 * Log the message to configured device
 * @param lev
 * @param message
 */
expublic void tplog(int lev, char *message)
{
    /* do not want to interpret the format string */
    TP_LOG(lev, "%s", message);
}

/**
 * Extended logging (TP)
 * @param lev
 * @param message
 */
expublic void tplogex(int lev, char *file, long line, char *message)
{
    /* do not want to interpret the format string */
    TP_LOGEX(lev, file, line, "%s", message);
}

/**
 * Get integration flags
 * @return integration flags set (EOS or some infos from debug config)
 */
expublic char * tploggetiflags(void)
{
    TP_LOGGETIFLAGS;
}

/**
 * Do the hex dump
 * @param lev
 * @param comment
 * @param ptr
 * @param len
 */
expublic void tplogdump(int lev, char *comment, void *ptr, int len)
{
    TP_DUMP(lev, comment, (char *)ptr, len);
}

/**
 * Print the differences of to binary arrays
 * @param lev
 * @param comment
 * @param ptr1
 * @param ptr2
 * @param len
 */
expublic void tplogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len)
{
    TP_DUMP_DIFF(lev, comment, (char *)ptr1, (char *)ptr2, len);
}
 
/**
 * Log the message to configured device, ndrx facility
 * @param lev
 * @param message
 */
expublic void ndrxlog(int lev, char *message)
{
    /* do not want to interpret the format string */
    NDRX_LOG(lev, "%s", message);
}

/**
 * Extended logging (NDRX)
 * @param lev log level
 * @param file source file name
 * @param line source line number
 * @param message log message
 */
expublic void ndrxlogex(int lev, char *file, long line, char *message)
{
    /* do not want to interpret the format string */
    NDRX_LOGEX(lev, file, line, "%s", message);
}

/**
 * Do the hex dump, ndrx facility
 * @param lev
 * @param comment
 * @param ptr
 * @param len
 */
expublic void ndrxlogdump(int lev, char *comment, void *ptr, int len)
{
    NDRX_DUMP(lev, comment, (char *)ptr, len);
}

/**
 * Print the differences of to binary arrays, ndrx facility
 * @param lev
 * @param comment
 * @param ptr1
 * @param ptr2
 * @param len
 */
expublic void ndrxlogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len)
{
    NDRX_DUMP_DIFF(lev, comment, (char *)ptr1, (char *)ptr2, len);
}
 
/**
 * Log the message to configured device, ubf facility
 * @param lev
 * @param message
 */
expublic void ubflog(int lev, char *message)
{
    /* do not want to interpret the format string */
    UBF_LOG(lev, "%s", message);
}

/**
 * Extended logging (UBF)
 * @param lev log level
 * @param file source file name
 * @param line source line number
 * @param message log message
 */
expublic void ubflogex(int lev, char *file, long line, char *message)
{
    /* do not want to interpret the format string */
    UBF_LOGEX(lev, file, line, "%s", message);
}

/**
 * Do the hex dump, ubf facility
 * @param lev
 * @param comment
 * @param ptr
 * @param len
 */
expublic void ubflogdump(int lev, char *comment, void *ptr, int len)
{
    UBF_DUMP(lev, comment, (char *)ptr, len);
}

/**
 * Print the differences of to binary arrays, ubf facility
 * @param lev
 * @param comment
 * @param ptr1
 * @param ptr2
 * @param len
 */
expublic void ubflogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len)
{
    UBF_DUMP_DIFF(lev, comment, (char *)ptr1, (char *)ptr2, len);
}

/**
 * TPLOG Query Information
 * Oldest byte (signed) is log level, next TPLOGQI flags, then log facility follows
 * Feature #312
 * @lev debug level (normally 1..5, 6 - dump)
 * @flags see TPLOGQI_GET flags
 * @return signed 32bit integer
 */
expublic long tplogqinfo(int lev, long flags)
{
    int ret  = 0;
    int tmp;
    ndrx_debug_t *dbg;
    
    API_ENTRY;
    
    if (flags & TPLOGQI_GET_NDRX)
    {
        dbg = debug_get_ndrx_ptr();
    }
    else if (flags & TPLOGQI_GET_UBF)
    {
        dbg = debug_get_ubf_ptr();
    }
    else if (flags & TPLOGQI_GET_TP)
    {
        dbg = debug_get_tp_ptr();
    }
    else 
    {
        _Nset_error_fmt(NEINVAL, "%s: Invalid flags: %ld", __func__, flags);
        EXFAIL_OUT(ret);
    }
    
    if (!(flags & TPLOGQI_EVAL_RETURN) && lev > dbg->level)
    {
        goto out;
    }
    
    ret|=(((int)dbg->flags) & LOG_FACILITY_MASK);
    
    if (flags & TPLOGQI_EVAL_DETAILED)
    {
        if (NULL!=strstr(dbg->iflags, LOG_IFLAGS_DETAILED))
        {
            ret|=TPLOGQI_RET_HAVDETAILED;
        }
    }
    tmp = dbg->level;
    
    tmp <<= TPLOGQI_RET_DBGLEVBITS;
    ret|=tmp;
    
out:
            
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
