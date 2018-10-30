/**
 * @brief TP LOG, full feature logging system for user, mainly for exporting to
 *   to other languages. C/C++ can use ndebug.h
 *
 * @file tplog.c
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
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Close the file if only one logger is using that
 * @param p
 */
exprivate void logfile_close(FILE *p)
{
    ndrx_debug_t *fd_arr[9];
    int i;
    int cnt = 0;
    int num;
    /*API_ENTRY;  set TLS too work with out context... */
    
    if (p == stdout || p == stderr)
    {
        return; /* nothing todo. */
    }
    
    fd_arr[0] = &G_ndrx_debug;
    fd_arr[1] = &G_ubf_debug;
    fd_arr[2] = &G_tp_debug;
    num = 3;
    if (NULL!=G_nstd_tls)
    {
        fd_arr[3] = &G_nstd_tls->threadlog_tp;
        fd_arr[4] = &G_nstd_tls->requestlog_tp;
        
        
        fd_arr[5] = &G_nstd_tls->threadlog_ndrx;
        fd_arr[6] = &G_nstd_tls->requestlog_ndrx;
        
        fd_arr[7] = &G_nstd_tls->threadlog_ubf;
        fd_arr[8] = &G_nstd_tls->requestlog_ubf;
        
        num = 9;
    }
    
    for (i=0; i<num; i++)
    {
        if (fd_arr[i]->dbg_f_ptr == p)
        {
            cnt++;
        }
    }
    
    if (cnt<2)
    {
        NDRX_FCLOSE(p);
    }
}

/**
 * Close any TLS loggers if open
 * @param tls thread local storage for ATMI lib
 */
expublic void ndrx_nstd_tls_loggers_close(nstd_tls_t *tls)
{
    
    ndrx_debug_t *logger[] = {&tls->threadlog_ndrx, &tls->threadlog_ubf, 
        &tls->threadlog_tp, &tls->requestlog_ndrx, &tls->requestlog_ubf, 
        &tls->requestlog_tp, NULL};
    
    int i=0;
    
    while (NULL!=logger[i])
    {
        if (NULL!=logger[i]->dbg_f_ptr && stderr!=logger[i]->dbg_f_ptr)
        {
            /* Close intelligently to avoid closing  */
            if (logger[i]->dbg_f_ptr!=G_ndrx_debug.dbg_f_ptr &&
                    logger[i]->dbg_f_ptr!=G_ubf_debug.dbg_f_ptr &&
                    logger[i]->dbg_f_ptr!=G_tp_debug.dbg_f_ptr)
            {
                NDRX_FCLOSE(logger[i]->dbg_f_ptr);
                logger[i]->dbg_f_ptr = NULL;
            }
        }
        i++;
    }
    
}

/**
 * Set the thread based log file
 * TODO: Later we need a wrapper to set file from buffer.
 * @param logger logging sub-system
 * @param file full path to file (optional) if present, does compare
 * @return 
 */
exprivate int logfile_change_name(int logger, char *filename)
{
    ndrx_debug_t *l;
    int ret = EXSUCCEED;
    
    API_ENTRY; /* set TLS too */
    
    switch (logger)
    {
        case LOG_FACILITY_NDRX:
            l = &G_ndrx_debug;
            break;
        case LOG_FACILITY_UBF:
            l = &G_ubf_debug;
            break;
        case LOG_FACILITY_TP:
            l = &G_tp_debug;
            break;
        case LOG_FACILITY_TP_THREAD:
            l = &G_nstd_tls->threadlog_tp;
            break;
        case LOG_FACILITY_TP_REQUEST:
            /* set request file shall do th thread init/init level.. */
            l = &G_nstd_tls->requestlog_tp;
            break;    
        case LOG_FACILITY_NDRX_THREAD:
            l = &G_nstd_tls->threadlog_ndrx;
            break;
        case LOG_FACILITY_NDRX_REQUEST:
            /* set request file shall do th thread init/init level.. */
            l = &G_nstd_tls->requestlog_ndrx;
            break;
        case LOG_FACILITY_UBF_THREAD:
            l = &G_nstd_tls->threadlog_ubf;
            break;
        case LOG_FACILITY_UBF_REQUEST:
            /* set request file shall do th thread init/init level.. */
            l = &G_nstd_tls->requestlog_ubf;
            break;
        default:
            _Nset_error_fmt(NEINVAL, "tplogfileset: Invalid logger: %d", logger);
            EXFAIL_OUT(ret);
            break;
    }
    
    if (NULL!=filename)
    {
        NDRX_LOG(log_debug, "Logger = %d change name to: [%s]", logger, filename);
        if (0==strcmp(l->filename, filename))
        {
            goto out;
        }
        else
        {
            NDRX_STRCPY_SAFE(l->filename, filename);
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Logger = %d change name to: [%s]", logger, l->filename);
    }
    
    /* name already changed no need to compare 
     * the caller must decide that.
     */
    if (l->dbg_f_ptr)
    {
        logfile_close(l->dbg_f_ptr);
    }

    /* open the file */
    if (EXEOS==l->filename[0])
    {
        /* log to stderr. */
        l->dbg_f_ptr = stderr;
    }
    else if (NULL==(l->dbg_f_ptr = NDRX_FOPEN(l->filename, "a")))
    {
        int err = errno;
        userlog("Failed to open %s: %s",l->filename, strerror(err));
        
        _Nset_error_fmt(NESYSTEM, "Failed to open %s: %s", 
                l->filename, strerror(err));
        
        l->filename[0] = EXEOS;
        l->dbg_f_ptr = stderr;
    }
    else
    {   
        setvbuf(l->dbg_f_ptr, NULL, _IOFBF, l->buffer_size);
    }
     
out:
    NDRX_LOG(log_debug, "Logger = %d logging to: [%s]", logger, l->filename);
    return ret;
    
}

/**
 * Setup the request logging file.
 * NOTE that we need: 
 * 1. tplogsetreqfile(<buffer>, <filename> (optional, not needed if name already in buffer))
 * 2. tploggetreqfile(fname (out)) return TRUE/FALSE if we have, and string with the value of fname
 * 3. tplogclosereqfile(); close thre request logging.
 * File will be set to EX_REQLOG = <Full path to request log file>
 * @param filename file name to log to
 */
expublic void tplogsetreqfile_direct(char *filename)
{
    API_ENTRY; /* set TLS too */
    
    /* Level not set, then there was no init */
    if (EXFAIL==G_nstd_tls->requestlog_tp.level)
    {
        /* file is null, we want to copy off the settings  */
        if (NULL!=G_nstd_tls->threadlog_tp.dbg_f_ptr)
        {
            memcpy(&G_nstd_tls->requestlog_tp, 
                    &G_nstd_tls->threadlog_tp, sizeof(G_nstd_tls->threadlog_tp));
        }
        else
        {
            /* Copy from TPlog */
            memcpy(&G_nstd_tls->requestlog_tp, &G_tp_debug, sizeof(G_tp_debug));
        }
        G_nstd_tls->requestlog_tp.code = LOG_CODE_TP_REQUEST;
    }
    
    /* ok now open then file */
    logfile_change_name(LOG_FACILITY_TP_REQUEST, filename);
    
}

/**
 * Close the request file (back to other loggers)
 * @param filename
 */
expublic void tplogclosereqfile(void)
{
    /* Only if we have a TLS... */
    if (G_nstd_tls)
    {
        if (G_nstd_tls->requestlog_tp.dbg_f_ptr)
        {
            logfile_close(G_nstd_tls->requestlog_tp.dbg_f_ptr);
        }
        G_nstd_tls->requestlog_tp.filename[0] = EXEOS;
        G_nstd_tls->requestlog_tp.dbg_f_ptr = NULL;
    }
}


/**
 * Reconfigure loggers.
 * 
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
        if (loggers[i] == LOG_FACILITY_NDRX && (logger & LOG_FACILITY_NDRX))
        {
            l = &G_ndrx_debug;
        }
        else if (loggers[i] == LOG_FACILITY_UBF && (logger & LOG_FACILITY_UBF))
        {
            l = &G_ubf_debug;
        }
        else if (loggers[i] == LOG_FACILITY_TP && (logger & LOG_FACILITY_TP))
        {
            l = &G_tp_debug;
        }
        else if (loggers[i] == LOG_FACILITY_TP_THREAD && (logger & LOG_FACILITY_TP_THREAD))
        {
            /* if thread was not set, then inherit the all stuff from tp
             */
            if (EXFAIL==G_nstd_tls->threadlog_tp.level)
            {
                memcpy(&G_nstd_tls->threadlog_tp, &G_tp_debug, sizeof(G_tp_debug));
                G_nstd_tls->threadlog_tp.code = LOG_CODE_TP_THREAD;
            }
            l = &G_nstd_tls->threadlog_tp;
        }
        else if (loggers[i] == LOG_FACILITY_TP_REQUEST && logger & (LOG_FACILITY_TP_REQUEST))
        {
            if (EXFAIL==G_nstd_tls->requestlog_tp.level)
            {
                memcpy(&G_nstd_tls->requestlog_tp, &G_tp_debug, sizeof(G_tp_debug));
                G_nstd_tls->requestlog_tp.code = LOG_CODE_TP_REQUEST;
            }
            
            l = &G_nstd_tls->requestlog_tp;
        }
        else if (loggers[i] == LOG_FACILITY_NDRX_THREAD && (logger & LOG_FACILITY_NDRX_THREAD))
        {
            /* if thread was not set, then inherit the all stuff from tp
             */
            if (EXFAIL==G_nstd_tls->threadlog_ndrx.level)
            {
                memcpy(&G_nstd_tls->threadlog_ndrx, &G_ndrx_debug, sizeof(G_ndrx_debug));
                G_nstd_tls->threadlog_ndrx.code = LOG_FACILITY_NDRX_THREAD;
            }
            l = &G_nstd_tls->threadlog_ndrx;
        }
        else if (loggers[i] == LOG_FACILITY_NDRX_REQUEST && logger & (LOG_FACILITY_NDRX_REQUEST))
        {
            if (EXFAIL==G_nstd_tls->requestlog_ndrx.level)
            {
                memcpy(&G_nstd_tls->requestlog_ndrx, &G_ndrx_debug, sizeof(G_ndrx_debug));
                G_nstd_tls->requestlog_ndrx.code = LOG_CODE_NDRX_REQUEST;
            }
            
            l = &G_nstd_tls->requestlog_ndrx;
        }
        else if (loggers[i] == LOG_FACILITY_UBF_THREAD && (logger & LOG_FACILITY_UBF_THREAD))
        {
            /* if thread was not set, then inherit the all stuff from tp
             */
            if (EXFAIL==G_nstd_tls->threadlog_ubf.level)
            {
                memcpy(&G_nstd_tls->threadlog_ubf, &G_ubf_debug, sizeof(G_ubf_debug));
                G_nstd_tls->threadlog_ubf.code = LOG_CODE_UBF_THREAD;
            }
            l = &G_nstd_tls->threadlog_ubf;
        }
        else if (loggers[i] == LOG_FACILITY_UBF_REQUEST && logger & (LOG_FACILITY_UBF_REQUEST))
        {
            if (EXFAIL==G_nstd_tls->requestlog_ubf.level)
            {
                memcpy(&G_nstd_tls->requestlog_ubf, &G_ubf_debug, sizeof(G_ubf_debug));
                G_nstd_tls->requestlog_ubf.code = LOG_CODE_UBF_REQUEST;
            }
            
            l = &G_nstd_tls->requestlog_ubf;
        }
        else
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
            if (EXSUCCEED!= (ret = ndrx_init_parse_line(NULL, debug_string, NULL, l)))
            {
                _Nset_error_msg(NEFORMAT, "Failed to parse debug string");
                EXFAIL_OUT(ret);
            }

            /* only if new name is not passed in */
            if (0!=strcmp(tmp_filename, l->filename) && 
                    (NULL==new_file || EXEOS==new_file[0]))
            {
                /* open new log file... (to what ever level we run...) */
                if (EXSUCCEED!=(ret = logfile_change_name(loggers[i], NULL)))
                {
                    _Nset_error_msg(NESYSTEM, "Failed to change log name");
                    EXFAIL_OUT(ret);
                }
            }
        }

        if (EXFAIL!=lev)
        {
            l->level = lev;
        }
        
        /* new file passed in: */
        if (NULL!=new_file && EXEOS!=new_file[0] && 0!=strcmp(new_file, l->filename))
        {            
            /* open new log file... (to what ever level we run...) */
            NDRX_STRCPY_SAFE(l->filename, new_file);
            if (EXSUCCEED!=(ret = logfile_change_name(loggers[i], NULL)))
            {
                _Nset_error_msg(NESYSTEM, "Failed to change log name");
                EXFAIL_OUT(ret);
            }
        }

    }
    
out:
    return ret;
}

/**
 * Close thread logger (if open)
 * @return 
 */
expublic void tplogclosethread(void)
{
    if (NULL!=G_nstd_tls && NULL!=G_nstd_tls->threadlog_tp.dbg_f_ptr)
    {
        logfile_close(G_nstd_tls->threadlog_tp.dbg_f_ptr);
        G_nstd_tls->threadlog_tp.level = EXFAIL;
        G_nstd_tls->threadlog_tp.filename[0] = EXEOS;
        G_nstd_tls->threadlog_tp.dbg_f_ptr = NULL;
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
            NDRX_STRNCPY(filename, G_nstd_tls->requestlog_tp.filename, bufsize-1);
            filename[bufsize-1] = EXEOS;
        }
        else
        {
            NDRX_STRNCPY_SAFE(filename, G_nstd_tls->requestlog_tp.filename, bufsize);
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
