/* 
** TP LOG, full feature logging system for user, mainly for exporting to
** to other languages. C/C++ can use ndebug.h
**
** @file tplog.c
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
    API_ENTRY; /* set TLS too */
    
    if (p == stdout || p == stderr)
    {
        return; /* nothing todo. */
    }
    
    fd_arr[0] = &G_ndrx_debug;
    fd_arr[1] = &G_ubf_debug;
    fd_arr[2] = &G_tp_debug;
    fd_arr[3] = &G_nstd_tls->threadlog;
    fd_arr[4] = &G_nstd_tls->requestlog;
    
    for (i=0; i<N_DIM(fd_arr); i++)
    {
        if (fd_arr[i]->dbg_f_ptr == p)
        {
            cnt++;
        }
    }
    
    if (cnt<2)
    {
        fclose(p);
    }
}

/**
 * Set the thread based log file
 * TODO: Later we need a wrapper to set file from buffer.
 * @param file full path to file
 * @return 
 */
public int logfile_change_name(int logger, char *file)
{
    ndrx_debug_t *l;
    int ret = SUCCEED;
    
    API_ENTRY; /* set TLS too */
    
    
    switch (logger)
    {
        case LOG_FACILITY_NDRX:
            l = &G_ndrx_debug;
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
    
    if (0!=strcmp(file, l->filename))
    {
        logfile_close(l->dbg_f_ptr);
        
        /* open the file */
        if (EOS==l->filename[0])
        {
            /* log to stderr. */
            l->dbg_f_ptr = stderr;
        }
        else if (!(l->dbg_f_ptr = fopen(l->filename, "a")))
        {
            fprintf(stderr,"Failed to open %s: %s\n",l->filename, strerror(errno));
            l->filename[0] = EOS;
            l->dbg_f_ptr = stderr;
        }
        else
        {   
            setvbuf(l->dbg_f_ptr, NULL, _IOFBF, l->buffer_size);
            strcpy(l->filename, file); /* save the actual file */
        }
    }
     
out:
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
    
    if (NULL!=G_nstd_tls->requestlog.dbg_f_ptr)
    {
        logfile_close(G_nstd_tls->requestlog.dbg_f_ptr);
    }
    /* Level not set, then there was no init */
    else if (FAIL==G_nstd_tls->requestlog.level)
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
    }
    
    /* ok now open then file */
    logfile_change_name(LOG_FACILITY_TP_REQUEST, filename);
    
}

/**
 * Close the request file (back to other loggers)
 * @param filename
 */
public void tplogclosereqfile(char *filename)
{
    API_ENTRY; /* set TLS too */
    
    logfile_close(G_nstd_tls->requestlog.dbg_f_ptr);
    G_nstd_tls->requestlog.filename[0] = EOS;
    G_nstd_tls->requestlog.dbg_f_ptr = NULL;
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
    char filename[PATH_MAX];
    int loggers[] = {LOG_FACILITY_NDRX, 
                    LOG_FACILITY_UBF, 
                    LOG_FACILITY_TP,
                    LOG_FACILITY_TP_THREAD,
                    LOG_FACILITY_TP_REQUEST
    };
    int i;
    
    API_ENTRY; /* set TLS too */
    
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
            }
            l = &G_nstd_tls->threadlog;
        }
        else if (loggers[i] == LOG_FACILITY_TP_REQUEST && logger & (LOG_FACILITY_TP_REQUEST))
        {
            if (FAIL==G_nstd_tls->threadlog.level)
            {
                memcpy(&G_nstd_tls->requestlog, &G_tp_debug, sizeof(G_tp_debug));
            }
            
            l = &G_nstd_tls->requestlog;
        }
        else
        {
            _Nset_error_fmt(NEINVAL, "tplogconfig: Invalid logger: %d", logger);
            FAIL_OUT(ret);
        }

        if (NULL!=module && EOS!=module[0])
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
            strcpy(filename, l->filename);
            if (SUCCEED!= (ret = ndrx_init_parse_line(NULL, debug_string, NULL, l)))
            {
                _Nset_error_msg(NESYSTEM, "Failed to parse debug string");
                FAIL_OUT(ret);
            }

            /* only if new name is not passed in */
            if (0!=strcmp(filename, l->filename) && 
                    (NULL==new_file || EOS==new_file[0]))
            {
                /* open new log file... (to what ever level we run...) */
                l->filename[0] = EOS; /* it will do the compare inside... */
                if (SUCCEED!=(ret = logfile_change_name(logger, l->filename)))
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

        if (NULL!=new_file && EOS!=new_file[0] && 0!=strcmp(filename, l->filename))
        {            
            /* open new log file... (to what ever level we run...) */
            if (SUCCEED!=(ret = logfile_change_name(logger, filename)))
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
    
    if (NULL!=filename)
    {
        ret = TRUE;
        
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
    TP_LOG(lev, message);
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
    TP_LOGDUMP(lev, comment, (char *)ptr, len);
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
    TP_LOGDUMP_DIFF(lev, comment, (char *)ptr1, (char *)ptr2, len);
}
 
