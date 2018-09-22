/**
 * @brief NDR Debug library routines
 *   Enduro Execution system platform library
 *
 * @file ndebug.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#include <sys/time.h>                   /* purely for dbg_timer()       */
#include <sys/stat.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>
#include <sys_unix.h>
#include <cconfig.h>

#include "nstd_tls.h"
#include "userlog.h"
#include "utlist.h"
#include <expluginbase.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define BUFFER_CONTROL(dbg_p)\
    dbg_p->lines_written++;\
    if (dbg_p->lines_written >= dbg_p->buf_lines)\
    {\
        dbg_p->lines_written = 0;\
        fflush(dbg_p->dbg_f_ptr);\
    }

#define BUFFERED_PRINT_LINE(dbg_p, line)\
    fputs(line, dbg_p->dbg_f_ptr);\
    fputs("\n", dbg_p->dbg_f_ptr);\
    BUFFER_CONTROL(dbg_p)


#define DEFAULT_BUFFER_SIZE         50000

/*
 * Logger initializer 
 */
#define DEBUG_INITIALIZER(CODE, MODULE, FLAGS)   \
{\
    .level = 0,\
    .dbg_f_ptr = NULL,\
    .filename = "",\
    .filename_th_template="",\
    .pid = 0,\
    .buf_lines = 0,\
    .buffer_size = 0,\
    .lines_written = 0,\
    .module=MODULE,\
    .is_user=0,\
    .code=CODE,\
    .iflags="",\
    .is_threaded=0,\
    .threadnr=0,\
    .flags=FLAGS,\
    .memlog=NULL,\
    .hostnamecrc32=0x0\
}

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
ndrx_debug_t G_ubf_debug = DEBUG_INITIALIZER(LOG_CODE_UBF, "UBF ", LOG_FACILITY_UBF);
ndrx_debug_t G_ndrx_debug = DEBUG_INITIALIZER(LOG_CODE_NDRX, "NDRX", LOG_FACILITY_NDRX);
ndrx_debug_t G_tp_debug = DEBUG_INITIALIZER(LOG_CODE_TP, "USER", LOG_FACILITY_TP);

ndrx_debug_t G_stdout_debug;
/*---------------------------Statics------------------------------------*/
volatile int G_ndrx_debug_first = EXTRUE;
volatile unsigned M_thread_nr = 0;
exprivate int __thread M_is_initlock_owner = 0; /* for recursive logging calls */
MUTEX_LOCKDECL(M_dbglock);
MUTEX_LOCKDECL(M_thread_nr_lock);
MUTEX_LOCKDECL(M_memlog_lock);
/*---------------------------Prototypes---------------------------------*/

/**
 * Reply the cached log to the real/initilaized logger
 * @param dbg logger (after init)
 */
expublic void ndrx_dbg_reply_memlog(ndrx_debug_t *dbg)
{
    ndrx_memlogger_t *line, *tmp;
    
    /* This shall be done by one thread only...
     * Thus we need a lock.
     */
    MUTEX_LOCK_V(M_memlog_lock);
    DL_FOREACH_SAFE(dbg->memlog, line, tmp)
    {
        /* Bug #321 */
        if (line->level <= dbg->level)
        {
            BUFFERED_PRINT_LINE(dbg, line->line)
        }
        
        DL_DELETE(dbg->memlog, line);
        NDRX_FREE(line);
    }
    MUTEX_UNLOCK_V(M_memlog_lock);
}

/**
 * Function returns true if current thread init lock owner
 * @return TRUE (we are performing init) / FALSE (we are not performing init)
 */
expublic int ndrx_dbg_intlock_isset(void)
{
    return M_is_initlock_owner;
}

/**
 * Sets init lock to us
 */
expublic void ndrx_dbg_intlock_set(void)
{   
    M_is_initlock_owner++;
}

/**
 * Unset (decrement) lock & reply logs if we are at the end.
 */
expublic void ndrx_dbg_intlock_unset(void)
{   
    M_is_initlock_owner--;
    
    if (M_is_initlock_owner < 0)
    {
        M_is_initlock_owner = 0;
    }
    
    /*
     * Reply memory based logs to the file.
     * mem debugs are filled only if we are doing the init
     * and the init was doing some debug (while the debug it self was not
     * initialized).
     */
    if (0==M_is_initlock_owner)
    {  
        /* Check that logs are initialized (in case of call from 
         * other bootstrap sources, with out call  */
        NDRX_DBG_INIT_ENTRY;

        if (NULL!=G_ubf_debug.memlog)
        {
            ndrx_dbg_reply_memlog(&G_ubf_debug);
        }

        if (NULL!=G_ndrx_debug.memlog)
        {
            ndrx_dbg_reply_memlog(&G_ndrx_debug);
        }

        if (NULL!=G_tp_debug.memlog)
        {
            ndrx_dbg_reply_memlog(&G_tp_debug);
        }
    }
}

/**
 * Initialize operating thread number.
 * note default is zero.
 */
expublic void ndrx_dbg_setthread(long threadnr)
{
    NSTD_TLS_ENTRY;
    G_nstd_tls->M_threadnr = threadnr;
}

/**
 * Lock the debug output
 */
expublic void ndrx_dbg_lock(void)
{
    MUTEX_LOCK_V(M_dbglock);
}

/**
 * Unlock the debug output
 */
expublic void ndrx_dbg_unlock(void)
{
    MUTEX_UNLOCK_V(M_dbglock);
}

/**
 * Return the logger according to current thread settings.
 * Note the internals should no call the logging. as this will cause recursive loop...
 * @param dbg_ptr
 * @return 
 */
exprivate ndrx_debug_t * get_debug_ptr(ndrx_debug_t *dbg_ptr)
{
    static __thread int recursive = EXFALSE;
    long flags = 0;
    /* If tls is enabled and we run threaded modes */
    if (NULL!=G_nstd_tls && !recursive)
    {
        if (dbg_ptr->is_threaded &&
                ( 
                  ((dbg_ptr->flags & LOG_FACILITY_NDRX) && NULL==G_nstd_tls->threadlog_ndrx.dbg_f_ptr 
                        /* assign target logger */
                        && (flags = LOG_FACILITY_NDRX_THREAD)) ||
                  ((dbg_ptr->flags & LOG_FACILITY_UBF) && NULL==G_nstd_tls->threadlog_ubf.dbg_f_ptr 
                        /* assign target logger */
                        && (flags = LOG_FACILITY_UBF_THREAD)) ||
                  ((dbg_ptr->flags & LOG_FACILITY_TP) && NULL==G_nstd_tls->threadlog_tp.dbg_f_ptr 
                         /* assign target logger */
                        && (flags = LOG_FACILITY_TP_THREAD))
                )
            )
        {
            char new_file[PATH_MAX];

            /* format new line... */
            snprintf(new_file, sizeof(new_file), dbg_ptr->filename_th_template, 
                    (unsigned)G_nstd_tls->M_threadnr);
            
            /* configure the thread based logger.. */
            
            recursive = EXTRUE; /* forbid recursive function call.. when doing some logging... */
            if (EXFAIL==tplogconfig(flags, 
                    dbg_ptr->level, NULL, dbg_ptr->module, new_file))
            {
                userlog("Failed to configure thread based logger for thread %d file %s: %s",
                        G_nstd_tls->M_threadnr, new_file, Nstrerror(Nerror));
            }
            
            recursive = EXFALSE; /* forbid recursive function call.. when doing some logging... */
            
        }

        if (NULL!=G_nstd_tls && !recursive)
        {
            if (dbg_ptr == &G_tp_debug && NULL!=G_nstd_tls->requestlog_tp.dbg_f_ptr)
            {
                return &G_nstd_tls->requestlog_tp;
            }
            else if (dbg_ptr == &G_tp_debug && NULL!=G_nstd_tls->threadlog_tp.dbg_f_ptr)
            {
                return &G_nstd_tls->threadlog_tp;
            }
            else if (dbg_ptr == &G_ndrx_debug && NULL!=G_nstd_tls->requestlog_ndrx.dbg_f_ptr)
            {
                return &G_nstd_tls->requestlog_ndrx;
            }
            else if (dbg_ptr == &G_ndrx_debug && NULL!=G_nstd_tls->threadlog_ndrx.dbg_f_ptr)
            {
                return &G_nstd_tls->threadlog_ndrx;
            }
            else if (dbg_ptr == &G_ubf_debug && NULL!=G_nstd_tls->requestlog_ubf.dbg_f_ptr)
            {
                return &G_nstd_tls->requestlog_ubf;
            }
            else if (dbg_ptr == &G_ubf_debug && NULL!=G_nstd_tls->threadlog_ubf.dbg_f_ptr)
            {
                return &G_nstd_tls->threadlog_ubf;
            }
        }
    }
    
    return dbg_ptr;
}

/**
 * Parser sharing the functionality with common config & old style debug.conf
 * User update mode: tok1 == NULL, tok2 config string
 * CConfig mode: tok1 != NULL, tok2 != NULL
 * ndrxdebug.conf mode: tok1 !=NULL tok2 == NULL
 * @param tok1 (full string for ndrxdebug.conf, for CConfig binary name
 * @param tok2 (config string for CConfig or update mode)
 * @return 
 */
expublic int ndrx_init_parse_line(char *in_tok1, char *in_tok2, 
        int *p_finish_off, ndrx_debug_t *dbg_ptr)
{
    int ret = EXSUCCEED;
    char *saveptr=NULL;
    char *name;
    char *tok;
    int ccmode = EXFALSE;
    int upd_mode = EXFALSE; /* user update mode */
    char *p;
    /* have a own copies of params as we do the token over them... */
    char *tok1 = NULL;
    char *tok2 = NULL;
    ndrx_debug_t *tmp_ptr;
    
    if (NULL!=in_tok1)
    {
        if (NULL==(tok1 = strdup(in_tok1)))
        {
            userlog("Failed to strdup(): %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
    }
    
    if (NULL!=in_tok2)
    {
        if (NULL==(tok2 = strdup(in_tok2)))
        {
            userlog("Failed to strdup(): %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
    }
    
    if (NULL==tok1 && tok2!=NULL)
    {
        upd_mode = EXTRUE;
    } 
    else if (NULL!=tok2)
    {
        ccmode = EXTRUE;
    }
    
    if (ccmode)
    {
        name = tok1;
    }
    else if (!upd_mode)
    {
        name=strtok_r (tok1,"\t ", &saveptr);
        tok=strtok_r (NULL,"\t ", &saveptr);
    }
    
    /**
     * In update mode we do not have name
     */
    if (upd_mode || ('*'==name[0] || 0==strcmp(name, EX_PROGNAME)))
    {
        if (!upd_mode)
        {
            *p_finish_off = ('*'!=name[0]);
        }
        
        /* for non-cc mode we have already tokenised tok */
        if (ccmode || upd_mode)
        {
            tok=strtok_r (tok2,"\t ", &saveptr);
        }
        
        while( tok != NULL ) 
        {
            int cmplen;
            /* get the setting... */
            p = strchr(tok, '=');
            cmplen = p-tok;

            if (0==strncmp("ndrx", tok, cmplen))
            {
                G_ndrx_debug.level = atoi(p+1);
            }
            else if (0==strncmp("ubf", tok, cmplen))
            {
                G_ubf_debug.level = atoi(p+1);
            }
            else if (0==strncmp("tp", tok, cmplen))
            {
                int lev = atoi(p+1);
            
                if (NULL!=dbg_ptr)
                {
                    dbg_ptr->level = lev;
                }
                else
                {
                    G_tp_debug.level = lev;
                }
            }
            else if (0==strncmp("iflags", tok, cmplen))
            {
                /* Setup integration flags */
                
                /* Feature #312 apply to all facilities, as during integration
                 * these might be used too */
                NDRX_STRNCPY_SAFE(G_ndrx_debug.iflags, p+1, sizeof(G_ndrx_debug.iflags)-1);
                NDRX_STRNCPY_SAFE(G_ubf_debug.iflags, p+1, sizeof(G_ubf_debug.iflags)-1);
                NDRX_STRNCPY_SAFE(G_tp_debug.iflags, p+1, sizeof(G_tp_debug.iflags)-1);
            }
            else if (0==strncmp("lines", tok, cmplen))
            {
                int lines = atoi(p+1);
                
                if (lines < 0)
                {
                    lines = 0;
                }
                
                if (NULL!=dbg_ptr)
                {
                    dbg_ptr->buf_lines = lines;
                }
                else
                {
                    G_tp_debug.buf_lines = 
                            G_ndrx_debug.buf_lines = 
                            G_ubf_debug.buf_lines = lines;
                }
            }
            else if (0==strncmp("bufsz", tok, cmplen))
            {
                int bufsz = atoi(p+1);
                
                bufsz = atoi(p+1);
                
                if (bufsz<=0)
                {
                    bufsz = DEFAULT_BUFFER_SIZE;
                }

                if (NULL!=dbg_ptr)
                {
                    dbg_ptr->buffer_size = bufsz;
                }
                else
                {
                    G_tp_debug.buffer_size = 
                            G_ndrx_debug.buffer_size = 
                            G_ubf_debug.buffer_size = bufsz;
                }
            }
            else if (0==strncmp("file", tok, cmplen))
            {
                if (NULL!=dbg_ptr)
                {
                    NDRX_STRCPY_SAFE(dbg_ptr->filename, p+1);
                }
                else
                {
                    NDRX_STRCPY_SAFE(G_tp_debug.filename, p+1);
                    NDRX_STRCPY_SAFE(G_ubf_debug.filename, p+1);
                    NDRX_STRCPY_SAFE(G_ndrx_debug.filename, p+1);
                }
            } /* Feature #167 */
            else if (0==strncmp("threaded", tok, cmplen))
            {
                int val = EXFALSE;
                
                if (*(p+1) == 'Y' || *(p+1) == 'y')
                {
                    val = EXTRUE;
                }
                
                if (NULL!=dbg_ptr)
                {
                    dbg_ptr->is_threaded = val;
                }
                else
                {
                    G_tp_debug.is_threaded = val;
                    G_ubf_debug.is_threaded = val;
                    G_ndrx_debug.is_threaded = val;
                }
            }
            
            tok=strtok_r (NULL,"\t ", &saveptr);
        }
    }
    
    if (NULL!=dbg_ptr)
    {
        tmp_ptr = dbg_ptr;
    }
    else
    {
        tmp_ptr = &G_ndrx_debug;
    }

    /* Configure template for threads...
     * Feature #167
     */
    if (tmp_ptr->is_threaded && EXEOS!=tmp_ptr->filename[0])
    {
        int len;
        int len2;
        char *p;
        
        len = strlen(tmp_ptr->filename_th_template);
        len2 = 3; /* len of .%u */

        if (len+len2 <= sizeof(tmp_ptr->filename))
        {
            NDRX_STRCPY_SAFE(tmp_ptr->filename_th_template, tmp_ptr->filename);
            ndrx_str_env_subs_len(tmp_ptr->filename_th_template, 
                    sizeof(tmp_ptr->filename_th_template));
            
            /* Thread based logfile name... */
            if (NULL!=(p = strrchr(tmp_ptr->filename_th_template, '.')))
            {
                /* insert the" .%u", move other part to the back..*/
                memmove(p+len2, p, 4);
                NDRX_STRNCPY(p, ".%u", len2);
            }
            else
            {
                /* add the .%d */
                strcat(tmp_ptr->filename_th_template, ".%u");
            }
            
            if (NULL!=dbg_ptr)
            {
                NDRX_STRCPY_SAFE(G_ubf_debug.filename_th_template, 
                        G_ndrx_debug.filename_th_template);
                
                NDRX_STRCPY_SAFE(G_tp_debug.filename_th_template, 
                        G_ndrx_debug.filename_th_template);
            }
        }
    }

out:
    if (NULL!=tok1)
    {
        free(tok1);
    }

    if (NULL!=tok2)
    {
        free(tok2);
    }
    
    return ret;
}

/**
 * This initializes debug out form ndebug.conf
 * NOTE This also loads the common configuration at standard library level 
 * 
 */
expublic void ndrx_init_debug(void)
{
    char *cfg_file = getenv(CONF_NDRX_DEBUG_CONF);
    FILE *f;
    int finish_off = EXFALSE;
    ndrx_inicfg_section_keyval_t *conf = NULL, *cc;
    ndrx_inicfg_t *cconfig = NULL;
    char hostname[PATH_MAX];
    
    ndrx_dbg_intlock_set();
    
    /*
     * init in declaration...
    memset(&G_ubf_debug, 0, sizeof(G_ubf_debug));
    memset(&G_ndrx_debug, 0, sizeof(G_ndrx_debug));
    memset(&G_stdout_debug, 0, sizeof(G_stdout_debug));
    */
    
    G_tp_debug.pid = G_ubf_debug.pid = G_ndrx_debug.pid = G_stdout_debug.pid = getpid();
    
    ndrx_sys_get_hostname(hostname, sizeof(hostname));
    /* copy number of chars specified in hostname if debug config */
    
    G_tp_debug.hostnamecrc32 = G_ubf_debug.hostnamecrc32 = 
                G_ndrx_debug.hostnamecrc32 = G_stdout_debug.hostnamecrc32 = 
                ndrx_Crc32_ComputeBuf(0, hostname, strlen(hostname));
    
    cconfig = ndrx_get_G_cconfig();
    
    /* Initialize with defaults.. */
    G_ndrx_debug.dbg_f_ptr = stderr;
    G_ubf_debug.dbg_f_ptr = stderr;
    G_tp_debug.dbg_f_ptr = stderr;
    G_stdout_debug.dbg_f_ptr = stdout;
    
    
    /* static coinf */
    G_stdout_debug.buf_lines = 1;
    G_stdout_debug.buffer_size = 1;
    G_stdout_debug.level = log_debug;
    
    /* default bufsz  */
    G_ubf_debug.buffer_size = G_ndrx_debug.buffer_size = 50000;

    G_tp_debug.buf_lines = G_ubf_debug.buf_lines = G_ndrx_debug.buf_lines = 1;
    G_tp_debug.level = G_ubf_debug.level = G_ndrx_debug.level = log_debug;

    if (NULL==cconfig)
    {
        if (NULL!=cfg_file &&
                NULL!=(f=fopen(cfg_file, "r")))
        {
            char buf[PATH_MAX*2];

            /* process line by line */
            while (NULL!=fgets(buf, sizeof(buf), f))
            {
                if ('#'==buf[0] || '\n'==buf[0])
                {
                    /* skip comments */
                    continue;
                }
                if (buf[strlen(buf)-1]=='\n')
                {
                    buf[strlen(buf)-1]=EXEOS;
                }

                ndrx_init_parse_line(buf, NULL, &finish_off, NULL);

                if (finish_off)
                {
                    break;
                }
            }

            fclose(f);
        }
        else if (NULL==f && NULL!=cfg_file)
        {
            fprintf(stderr, "Failed to to open [%s]: %d/%s\n", cfg_file,
                                errno, strerror(errno));
        }
        else
        {
            /* no debug configuration set! */
            fprintf(stderr, "To control debug output, set debug"
                            "config file path in $NDRX_DEBUG_CONF\n");            
        }
    }
    else
    {
        /* CCONFIG in use, get the section */
        ndrx_cconfig_load(); /* load the global section... */
        if (EXSUCCEED==ndrx_cconfig_get(NDRX_CONF_SECTION_DEBUG, &conf))
        {
            /* 1. get he line by common & process */
            if (NULL!=(cc=ndrx_keyval_hash_get(conf, "*")))
            {
                ndrx_init_parse_line(cc->key, cc->val, &finish_off, NULL);
            }
            
            /* 2. get the line by binary name  */
            if (NULL!=(cc=ndrx_keyval_hash_get(conf, (char *)EX_PROGNAME)))
            {
                ndrx_init_parse_line(cc->key, cc->val, &finish_off, NULL);
            }   
        }
    }
    
    /* open debug file.. */
    if (EXEOS!=G_ndrx_debug.filename[0])
    {        
        ndrx_str_env_subs_len(G_ndrx_debug.filename, sizeof(G_ndrx_debug.filename));
        /* Opens the file descriptors */
        if (!(G_ndrx_debug.dbg_f_ptr = fopen(G_ndrx_debug.filename, "a")))
        {
            fprintf(stderr,"Failed to open %s\n",G_ndrx_debug.filename);
            G_tp_debug.dbg_f_ptr = G_ubf_debug.dbg_f_ptr=G_ndrx_debug.dbg_f_ptr=stderr;
        }
        else
        {   
            /* close descriptors of fork: Bug #176 */
	    if (EXSUCCEED!=fcntl(fileno(G_ndrx_debug.dbg_f_ptr), F_SETFD, FD_CLOEXEC))
            {
                userlog("WARNING: Failed to set FD_CLOEXEC: %s", strerror(errno));
            }
            setvbuf(G_ndrx_debug.dbg_f_ptr, NULL, _IOFBF, G_ndrx_debug.buffer_size);
            G_tp_debug.dbg_f_ptr = G_ubf_debug.dbg_f_ptr=G_ndrx_debug.dbg_f_ptr;
        }
    }

    /*
     Expected file syntax is:
     * file=/tmp/common.log ndrx=5 ubf=5 buf=1
     ndrxd file=/tmp/ndrxd.log  threaded=y
     xadmin file=/tmp/xadmin.log
     ...
     */
    
    if (NULL!=conf)
    {
        /* kill the conf */
        ndrx_keyval_hash_free(conf);
    }
    
    G_ndrx_debug_first = EXFALSE;
    
    ndrx_dbg_intlock_unset();
    
}

/**
 * Return current NDRX debug level.
 * @return - debug level..
 */
expublic ndrx_debug_t * debug_get_ndrx_ptr(void)
{
    /* NSTD_TLS_ENTRY; */
    NDRX_DBG_INIT_ENTRY;
    
    return get_debug_ptr(&G_ndrx_debug);
}

/**
 * Return TP pointer
 * @return 
 */
expublic ndrx_debug_t * debug_get_tp_ptr(void)
{
    /* NSTD_TLS_ENTRY; */
    NDRX_DBG_INIT_ENTRY;
    
    return get_debug_ptr(&G_tp_debug);
}

/**
 * Return current UBF debug level.
 * @return
 */
expublic ndrx_debug_t * debug_get_ubf_ptr(void)
{   /* NSTD_TLS_ENTRY; */
    NDRX_DBG_INIT_ENTRY;
    
    return get_debug_ptr(&G_ubf_debug);
}

/**
 * Return current NDRX debug level.
 * @return - debug level..
 */
expublic int debug_get_ndrx_level(void)
{
    NDRX_DBG_INIT_ENTRY;
    return G_ndrx_debug.level;
}

/**
 * Return current UBF debug level.
 * @return 
 */
expublic int debug_get_ubf_level(void)
{
    NDRX_DBG_INIT_ENTRY;
    return G_ubf_debug.level;
}

/**
 * Return current UBF debug level.
 * @return 
 */
expublic int debug_get_tp_level(void)
{
    NDRX_DBG_INIT_ENTRY;
    return G_tp_debug.level;
}

/**
 * Print buffer dump diff to log file. Diffing buffer sizes must match the "len"
 * @param dbg_ptr - debug config
 * @param lev - level
 * @param mod - module
 * @param file - source file
 * @param line - source line
 * @param func - calling func
 * @param comment - comment
 * @param ptr - buffer1
 * @param ptr2 - buffer2
 * @param len - buffer size
 */
expublic void __ndrx_debug_dump_diff__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
        long line, const char *func, char *comment, void *ptr, void *ptr2, long len)
{
    
    int i;
    unsigned char buf[17];
    unsigned char buf2[17];
    unsigned char *cptr = (unsigned char*)ptr;
    unsigned char *cptr2 = (unsigned char*)ptr2;
    char print_line[256]={0};
    char print_line2[256]={0};
    /* NSTD_TLS_ENTRY; */
    /* NDRX_DBG_INIT_ENTRY; - called by master macro */
    dbg_ptr = get_debug_ptr(dbg_ptr);
    
    if (dbg_ptr->level < lev)
    {
        return; /* the level is lowered by thread/request logger */
    }
        
    __ndrx_debug__(dbg_ptr, lev, file, line, func, "%s", comment);
    
    if (0==len)
    {
        __ndrx_debug__(dbg_ptr, lev, file, line, func, "Notice: Hex dump diff - "
                "nothing to dump: len=%d ptr=%p ptr2=%p", len, ptr, ptr2);
        
        return; /* nothing todo... */
    }
    
    for (i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {
            if (i != 0)
            {
                sprintf (print_line + strlen(print_line), "  %s", buf);
                sprintf (print_line2 + strlen(print_line2), "  %s", buf2);

                if (0!=strcmp(print_line, print_line2))
                {
                    /* print line 1 */
                    fputs("<", dbg_ptr->dbg_f_ptr);
                    fputs(print_line, dbg_ptr->dbg_f_ptr);
                    fputs("\n", dbg_ptr->dbg_f_ptr);
                    BUFFER_CONTROL(dbg_ptr);
                    
                    /* print line 2 */
                    fputs(">", dbg_ptr->dbg_f_ptr);
                    fputs(print_line2, dbg_ptr->dbg_f_ptr);
                    fputs("\n", dbg_ptr->dbg_f_ptr);
                    BUFFER_CONTROL(dbg_ptr);
                }

                print_line[0] = 0;
                print_line2[0] = 0;
            }

            sprintf (print_line + strlen(print_line), "  %04x ", i);
            sprintf (print_line2 + strlen(print_line2), "  %04x ", i);
        }

        sprintf (print_line + strlen(print_line), " %02x", cptr[i]);
        sprintf (print_line2 + strlen(print_line2), " %02x", cptr2[i]);

        if ((cptr[i] < 0x20) || (cptr[i] > 0x7e))
        {
            buf[i % 16] = '.';
        }
        else
        {
            buf[i % 16] = cptr[i];
        }
        buf[(i % 16) + 1] = '\0';

        if ((cptr2[i] < 0x20) || (cptr2[i] > 0x7e))
        {
            buf2[i % 16] = '.';
        }
        else
        {
            buf2[i % 16] = cptr2[i];
        }
        buf2[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0)
    {
        sprintf (print_line + strlen(print_line), "   ");
        sprintf (print_line2 + strlen(print_line2), "   ");
        i++;
    }

    sprintf (print_line + strlen(print_line), "  %s", buf);
    sprintf (print_line2 + strlen(print_line2), "  %s", buf2);

    if (0!=strcmp(print_line, print_line2))
    {
        /* print line 1 */
        fputs("<", dbg_ptr->dbg_f_ptr);
        fputs(print_line, dbg_ptr->dbg_f_ptr);
        fputs("\n", dbg_ptr->dbg_f_ptr);
        BUFFER_CONTROL(dbg_ptr);

        /* print line 2 */
        fputs(">", dbg_ptr->dbg_f_ptr);
        fputs(print_line2, dbg_ptr->dbg_f_ptr);
        fputs("\n", dbg_ptr->dbg_f_ptr);
        BUFFER_CONTROL(dbg_ptr);
    }
    print_line[0] = 0;
    print_line2[0] = 0;
    
}

/**
 * Print buffer dump to log file
 * @param dbg_ptr - debug config
 * @param lev - level
 * @param mod - module
 * @param file - source file
 * @param line - source line
 * @param func - calling func
 * @param comment - comment
 * @param ptr - buffer1
 * @param len - buffer size
 */
expublic void __ndrx_debug_dump__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
        long line, const char *func, char *comment, void *ptr, long len)
{
    int i;
    unsigned char buf[17];
    unsigned char *cptr = (unsigned char*)ptr;
    char print_line[256]={0};
    NSTD_TLS_ENTRY;
    /* NDRX_DBG_INIT_ENTRY; - called by master macro */
    
    dbg_ptr = get_debug_ptr(dbg_ptr);
    
    if (dbg_ptr->level < lev)
    {
        return; /* the level is lowered by thread/request logger */
    }
    
    __ndrx_debug__(dbg_ptr, lev, file, line, func, "%s (nr bytes: %ld)", 
            comment, len);
    
    if (0==len)
    {
        __ndrx_debug__(dbg_ptr, lev, file, line, func, "Notice: Hex dump - "
                "nothing to dump: len=%d ptr=%p", len, ptr);
        
        return; /* nothing todo... */
    }

    for (i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {
            if (i != 0)
            {
                sprintf (print_line + strlen(print_line), "  %s", buf);
                BUFFERED_PRINT_LINE(dbg_ptr, print_line);
                print_line[0] = 0;
            }

            sprintf (print_line + strlen(print_line), "  %04x ", i);
        }
        sprintf (print_line + strlen(print_line), " %02x", cptr[i]);

        if ((cptr[i] < 0x20) || (cptr[i] > 0x7e))
        {
            buf[i % 16] = '.';
        }
        else
        {
            buf[i % 16] = cptr[i];
        }
        buf[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0)
    {
        sprintf (print_line + strlen(print_line), "   ");
        i++;
    }

    /* And print the final ASCII bit. */
    sprintf (print_line + strlen(print_line), "  %s", buf);
    BUFFERED_PRINT_LINE(dbg_ptr, print_line);
    print_line[0] = 0;    
}

/**
 * Print stuff to trace file
 * @param dbg_ptr - debug conf
 * @param lev - level
 * @param mod - module
 * @param file - source file
 * @param line - source line
 * @param func - source func
 * @param fmt - format
 * @param ... - varargs
 */
expublic void __ndrx_debug__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
        long line, const char *func, char *fmt, ...)
{
    va_list ap;
    char line_start[128];
    long ldate, ltime, lusec;
    char *line_print;
    int len;
    ndrx_debug_t *org_ptr = dbg_ptr;
    long  thread_nr = 0;
    static __thread uint64_t ostid = 0;
    static __thread int first = EXTRUE;
    /* NSTD_TLS_ENTRY; */
    
    /* NDRX_DBG_INIT_ENTRY; - called by master macro */
    
    if (NULL!=G_nstd_tls)
    {
        thread_nr = G_nstd_tls->M_threadnr;
    }
    
    /* get the physical tid */
    if (first)
    {
        ostid = ndrx_gettid();
        first = EXFALSE;
    }
    
    if (!M_is_initlock_owner)
    {
        dbg_ptr = get_debug_ptr(dbg_ptr);

        if (dbg_ptr->level < lev)
        {
            return; /* the level is lowered by thread/request logger */
        }
    }
    
    if ((len=strlen(file)) > 8)
    {
        line_print = (char *)file+len-8;
    }
    else
    {
        line_print = (char *)file;
    }

    ndrx_get_dt_local(&ldate, &ltime, &lusec);
    
    snprintf(line_start, sizeof(line_start), 
        "%c:%s:%d:%08x:%5d:%08llx:%03ld:%08ld:%06ld%03d:%-8.8s:%04ld:%-8.8s",
        dbg_ptr->code, org_ptr->module, lev, (unsigned int)dbg_ptr->hostnamecrc32, 
            (int)dbg_ptr->pid, (unsigned long long)(ostid), thread_nr, ldate, ltime, 
        (int)(lusec/1000), line_print, line, func);
    
    if (!M_is_initlock_owner)
    {
        fputs(line_start, dbg_ptr->dbg_f_ptr);
        va_start(ap, fmt);    
        (void) vfprintf(dbg_ptr->dbg_f_ptr, fmt, ap);
        va_end(ap);
        fputs("\n", dbg_ptr->dbg_f_ptr);
        
        /* Handle some buffering... */
        BUFFER_CONTROL(dbg_ptr);
    }
    else
    {
        ndrx_memlogger_t *memline = NDRX_MALLOC(sizeof(ndrx_memlogger_t));
        
        if (NULL==memline)
        {
            userlog("Failed to malloc mem debug line: %s - skipping log entry", 
                    strerror(errno));
        }
        else
        {
            int len;
            memline->line[0] = EXEOS;
            memline->level = lev; /* user for log reply to actual logger */
            /* alloc the storage object */
            NDRX_STRCPY_SAFE(memline->line, line_start);
            
            len = strlen(memline->line);
            
            va_start(ap, fmt);    
            (void) vsnprintf(memline->line+len, sizeof(memline->line)-len, fmt, ap);
            va_end(ap);
            
            
            /* Add line to the logger */
            MUTEX_LOCK_V(M_memlog_lock);
            DL_APPEND(dbg_ptr->memlog, memline);
            MUTEX_UNLOCK_V(M_memlog_lock);
            
        }
    }
}

/**
 * Initialize debug library
 * Currently default level is to use maximum.
 *
 * Debug buffer is set to default 0, can be overriden by <KEY>+DBGBUF
 * @param module - module form which debug is initialized
 */
expublic void ndrx_dbg_init(char *module, char *config_key)
{
   NDRX_DBG_INIT_ENTRY;
}

/**
 * Debug version of malloc();
 * @param size
 * @param line
 * @param file
 * @param func
 * @return 
 */
expublic void *ndrx_malloc_dbg(size_t size, long line, const char *file, const char *func)
{
    void *ret;
    int errnosv;
    
    ret=malloc(size);
    errnosv = errno;
    
    userlog("[%p] <= malloc(size=%d):%s %s:%ld", ret, size, func, file, line);
    
    errno = errnosv;
    
    return ret;
}

/**
 * Debug version of free();
 * @param size
 * @param line
 * @param file
 * @param func
 * @return 
 */
expublic void ndrx_free_dbg(void *ptr, long line, const char *file, const char *func)
{
    userlog("[%p] => free(ptr=%p):%s %s:%ld", ptr, ptr, func, file, line);
    free(ptr);
}

/**
 * Debug version of calloc();
 * @param size
 * @param line
 * @param file
 * @param func
 * @return 
 */
expublic void *ndrx_calloc_dbg(size_t nmemb, size_t size, long line, const char *file, const char *func)
{
    void *ret;
    int errnosv;
    
    ret=calloc(nmemb, size);
    errnosv = errno;
    userlog("[%p] <= calloc(nmemb=%d, size=%d):%s %s:%ld", ret, nmemb, 
            size, func, file, line);
    
    errno = errnosv;
    
    return ret;
}

/**
 * Debug version of realloc();
 * @param size
 * @param line
 * @param file
 * @param func
 * @return 
 */
expublic void *ndrx_realloc_dbg(void *ptr, size_t size, long line, const char *file, const char *func)
{   
    void *ret;
    int errnosv;
    
    ret= realloc(ptr, size);
    
    errnosv = errno;
            
    userlog("[%p] <= realloc(ptr=[%p], size=%d):%s %s:%ld", ret, ptr, 
            size, func, file, line);
    
    errno = errnosv;
    return ret;
}


/**
 * Memory logging version of fopen(3)
 * @param path
 * @param mode
 * @return 
 */
expublic FILE *ndrx_fopen_dbg(const char *path, const char *mode, 
        long line, const char *file, const char *func)
{
    FILE *ret;
    int errnosv;
    
    ret = fopen(path, mode);
    errnosv = errno;
   
    userlog("[%p] <= fopen(path=%s, mode=%s):%s %s:%ld", ret,  path, mode,
            func, file, line);
    
    errno = errnosv;
    
    return ret;
}


/**
 * Memory logging version of fclose(3)
 * @param fp
 * @return 
 */
expublic int ndrx_fclose_dbg(FILE *fp, long line, const char *file, const char *func)
{
    int ret;
    int errnosv;
    
    ret = fclose(fp);
    
    errnosv = errno;
            
    userlog("[%p] => fclose(fp=%p) => %d:%s %s:%ld", fp, fp, ret, 
            func, file, line);
    
    errno = errnosv;
    
    return ret;
    
}

/**
 * Debug version of strdup();
 * @return 
 */
expublic char *ndrx_strdup_dbg(char *ptr, long line, const char *file, const char *func)
{
    char *ret;
    int errnosv;
    
    ret=strdup(ptr);
    errnosv = errno;
    
    userlog("[%p] <= strdup(ptr=%p):%s %s:%ld", ret, ptr, func, file, line);
    
    errno = errnosv;
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
