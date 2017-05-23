/* 
** TP LOG, full feature logging system for user, mainly for exporting to
** to other languages. C/C++ can use ndebug.h
**
** @file tplog.c
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
private void logfile_close(FILE *p)
{
    ndrx_debug_t *fd_arr[5];
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
        fd_arr[3] = &G_nstd_tls->threadlog;
        fd_arr[4] = &G_nstd_tls->requestlog;
        num = 5;
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
 * Set the thread based log file
 * TODO: Later we need a wrapper to set file from buffer.
 * @param logger logging sub-system
 * @param file full path to file (optional) if present, does compare
 * @return 
 */
private int logfile_change_name(int logger, char *filename)
{
    ndrx_debug_t *l;
    int ret = SUCCEED;
    
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
            l = &G_nstd_tls->threadlog;
            break;
        case LOG_FACILITY_TP_REQUEST:
            /* set request file shall do th thread init/init level.. */
            l = &G_nstd_tls->requestlog;
            break;
        default:
            _Nset_error_fmt(NEINVAL, "tplogfileset: Invalid logger: %d", logger);
            FAIL_OUT(ret);
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
            strcpy(l->filename, filename);
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
    if (EOS==l->filename[0])
    {
        /* log to stderr. */
        l->dbg_f_ptr = stderr;
    }
    else if (NULL==(l->dbg_f_ptr = NDRX_FOPEN(l->filename, "a")))
    {
        NDRX_LOG(log_error,"Failed to open %s: %s\n",l->filename, strerror(errno));
        l->filename[0] = EOS;
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
public void tplogsetreqfile_direct(char *filename)
{
    API_ENTRY; /* set TLS too */
    
    /* Level not set, then there was no init */
    if (FAIL==G_nstd_tls->requestlog.level)
    {
        /* file is null, we want to copy off the settings  */
        if (NULL!=G_nstd_tls->threadlog.dbg_f_ptr)
        {
            memcpy(&G_nstd_tls->requestlog, 
                    &G_nstd_tls->threadlog, sizeof(G_nstd_tls->threadlog));
        }
        else
        {
            /* Copy from TPlog */
            memcpy(&G_nstd_tls->requestlog, &G_tp_debug, sizeof(G_tp_debug));
        }
        G_nstd_tls->requestlog.code = LOG_CODE_TP_REQUEST;
    }
    
    /* ok now open then file */
    logfile_change_name(LOG_FACILITY_TP_REQUEST, filename);
    
}

/**
 * Close the request file (back to other loggers)
 * @param filename
 */
public void tplogclosereqfile(void)
{
    /* Only if we have a TLS... */
    if (G_nstd_tls)
    {
        if (G_nstd_tls->requestlog.dbg_f_ptr)
        {
            logfile_close(G_nstd_tls->requestlog.dbg_f_ptr);
        }
        G_nstd_tls->requestlog.filename[0] = EOS;
        G_nstd_tls->requestlog.dbg_f_ptr = NULL;
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
public int tplogconfig(int logger, int lev, char *debug_string, char *module, 
        char *new_file)
{
    int ret = SUCCEED;
    ndrx_debug_t *l;
    char tmp_filename[PATH_MAX];
    int loggers[] = {LOG_FACILITY_NDRX, 
                    LOG_FACILITY_UBF, 
                    LOG_FACILITY_TP,
                    LOG_FACILITY_TP_THREAD,
                    LOG_FACILITY_TP_REQUEST
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
            if (FAIL==G_nstd_tls->threadlog.level)
            {
                memcpy(&G_nstd_tls->threadlog, &G_tp_debug, sizeof(G_tp_debug));
                G_nstd_tls->threadlog.code = LOG_CODE_TP_THREAD;
            }
            l = &G_nstd_tls->threadlog;
        }
        else if (loggers[i] == LOG_FACILITY_TP_REQUEST && logger & (LOG_FACILITY_TP_REQUEST))
        {
            if (FAIL==G_nstd_tls->requestlog.level)
            {
                memcpy(&G_nstd_tls->requestlog, &G_tp_debug, sizeof(G_tp_debug));
                G_nstd_tls->requestlog.code = LOG_CODE_TP_REQUEST;
            }
            
            l = &G_nstd_tls->requestlog;
        }
        else
        {
            /*
            _Nset_error_fmt(NEINVAL, "tplogconfig: Invalid logger: %d", logger);
            FAIL_OUT(ret);
             */
            continue;
        }

        if (NULL!=module && EOS!=module[0] && 
                loggers[i] != LOG_FACILITY_NDRX &&
                loggers[i] != LOG_FACILITY_UBF
            )
        {
            strncpy(l->module, module, 4);
            l->module[4] = EOS;
        }

        if (NULL!= debug_string && EOS!=debug_string[0])
        {
            /* parse out the logger 
             * Check if file is changed? If changed, then we shall
             * close the old file & open newone...
             */
            strcpy(tmp_filename, l->filename);
            if (SUCCEED!= (ret = ndrx_init_parse_line(NULL, debug_string, NULL, l)))
            {
                _Nset_error_msg(NEFORMAT, "Failed to parse debug string");
                FAIL_OUT(ret);
            }

            /* only if new name is not passed in */
            if (0!=strcmp(tmp_filename, l->filename) && 
                    (NULL==new_file || EOS==new_file[0]))
            {
                /* open new log file... (to what ever level we run...) */
                if (SUCCEED!=(ret = logfile_change_name(loggers[i], NULL)))
                {
                    _Nset_error_msg(NESYSTEM, "Failed to change log name");
                    FAIL_OUT(ret);
                }
            }
        }

        if (FAIL!=lev)
        {
            l->level = lev;
        }
        
        /* new file passed in: */
        if (NULL!=new_file && EOS!=new_file[0] && 0!=strcmp(new_file, l->filename))
        {            
            /* open new log file... (to what ever level we run...) */
            strcpy(l->filename, new_file);
            if (SUCCEED!=(ret = logfile_change_name(loggers[i], NULL)))
            {
                _Nset_error_msg(NESYSTEM, "Failed to change log name");
                FAIL_OUT(ret);
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
public void tplogclosethread(void)
{
    if (NULL!=G_nstd_tls && NULL!=G_nstd_tls->threadlog.dbg_f_ptr)
    {
        logfile_close(G_nstd_tls->threadlog.dbg_f_ptr);
        G_nstd_tls->threadlog.level = FAIL;
        G_nstd_tls->threadlog.filename[0] = EOS;
        G_nstd_tls->threadlog.dbg_f_ptr = NULL;
    }
}

/**
 * Get the current request file
 * @param filename (optional) optional to return file name
 * @return TRUE (request file used) / FALSE (request file not used)
 */
public int tploggetreqfile(char *filename, int bufsize)
{
    int ret = FALSE;
    
    if (NULL==G_nstd_tls->requestlog.dbg_f_ptr)
    {
        ret=FALSE;
        goto out;
    }
    
    ret = TRUE;

    if (NULL!=filename)
    {
        if (bufsize>0)
        {
            strncpy(filename, G_nstd_tls->requestlog.filename, bufsize-1);
            filename[bufsize-1] = EOS;
        }
        else
        {
            strcpy(filename, G_nstd_tls->requestlog.filename);
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
public void tplog(int lev, char *message)
{
    /* do not want to interpret the format string */
    TP_LOG(lev, "%s", message);
}

/**
 * Extended logging
 * @param lev
 * @param message
 */
public void tplogex(int lev, char *file, long line, char *message)
{
    /* do not want to interpret the format string */
    TP_LOGEX(lev, file, line, "%s", message);
}

/**
 * Get integration flags
 * @return integration flags set (EOS or some infos from debug config)
 */
public char * tploggetiflags(void)
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
public void tplogdump(int lev, char *comment, void *ptr, int len)
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
public void tplogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len)
{
    TP_DUMP_DIFF(lev, comment, (char *)ptr1, (char *)ptr2, len);
}
 
/**
 * Log the message to configured device, ndrx facility
 * @param lev
 * @param message
 */
public void ndrxlog(int lev, char *message)
{
    /* do not want to interpret the format string */
    NDRX_LOG(lev, "%s", message);
}

/**
 * Do the hex dump, ndrx facility
 * @param lev
 * @param comment
 * @param ptr
 * @param len
 */
public void ndrxlogdump(int lev, char *comment, void *ptr, int len)
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
public void ndrxlogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len)
{
    NDRX_DUMP_DIFF(lev, comment, (char *)ptr1, (char *)ptr2, len);
}

 
/**
 * Log the message to configured device, ubf facility
 * @param lev
 * @param message
 */
public void ubflog(int lev, char *message)
{
    /* do not want to interpret the format string */
    UBF_LOG(lev, "%s", message);
}

/**
 * Do the hex dump, ubf facility
 * @param lev
 * @param comment
 * @param ptr
 * @param len
 */
public void ubflogdump(int lev, char *comment, void *ptr, int len)
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
public void ubflogdumpdiff(int lev, char *comment, void *ptr1, void *ptr2, int len)
{
    UBF_DUMP_DIFF(lev, comment, (char *)ptr1, (char *)ptr2, len);
}



