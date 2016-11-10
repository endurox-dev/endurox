/* 
** NDR Debug library routines
** Enduro Execution system platform library
**
** @file ndebug.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
ndrx_debug_t G_tp_debug;
ndrx_debug_t G_ubf_debug;
ndrx_debug_t G_ndrx_debug;
ndrx_debug_t G_stdout_debug;
/*---------------------------Statics------------------------------------*/
volatile int G_ndrx_debug_first = TRUE;
MUTEX_LOCKDECL(M_dbglock);	/* For spinlock */
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialize operating thread number.
 * note default is zero.
 */
public void ndrx_dbg_setthread(long threadnr)
{
    NSTD_TLS_ENTRY;
    G_nstd_tls->M_threadnr = threadnr;
}

/**
 * Lock the debug output
 */
public void ndrx_dbg_lock(void)
{
    MUTEX_LOCK_V(M_dbglock);
}

/**
 * Unlock the debug output
 */
public void ndrx_dbg_unlock(void)
{
    MUTEX_UNLOCK_V(M_dbglock);
}

/**
 * Return the logger according to curren thread settings.
 * @param dbg_ptr
 * @return 
 */
private ndrx_debug_t * get_debug_ptr(ndrx_debug_t *dbg_ptr)
{
    if (NULL!=G_nstd_tls && NULL!=G_nstd_tls->threadlog.dbg_f_ptr)
    {
        return &G_nstd_tls->threadlog;
    }
    else if (NULL!=G_nstd_tls && NULL!=G_nstd_tls->requestlog.dbg_f_ptr)
    {
        return &G_nstd_tls->requestlog;
    }
    else
    {
        return dbg_ptr;
    }
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
public int ndrx_init_parse_line(char *in_tok1, char *in_tok2, 
        int *p_finish_off, ndrx_debug_t *dbg_ptr)
{
    int ret = SUCCEED;
    char *saveptr=NULL;
    char *name;
    char *tok;
    int ccmode = FALSE;
    int upd_mode = FALSE; /* user update mode */
    char *p;
    /* have a own copies of params as we do the token over them... */
    char *tok1 = NULL;
    char *tok2 = NULL;
    
    if (NULL!=in_tok1)
    {
        if (NULL==(tok1 = strdup(in_tok1)))
        {
            userlog("Failed to strdup(): %s", strerror(errno));
            FAIL_OUT(ret);
        }
    }
    
    if (NULL!=in_tok2)
    {
        if (NULL==(tok2 = strdup(in_tok2)))
        {
            userlog("Failed to strdup(): %s", strerror(errno));
            FAIL_OUT(ret);
        }
    }
    
    
    if (NULL==tok1 && tok2!=NULL)
    {
        upd_mode = TRUE;
    } 
    else if (NULL!=tok2)
    {
        ccmode = TRUE;
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
                    strcpy(dbg_ptr->filename, p+1);
                }
                else
                {
                    strcpy(G_tp_debug.filename, p+1);
                    strcpy(G_ubf_debug.filename, p+1);
                    strcpy(G_ndrx_debug.filename, p+1);
                }
            }
            
            tok=strtok_r (NULL,"\t ", &saveptr);
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
 */
public void ndrx_init_debug(void)
{
    char *cfg_file = getenv("NDRX_DEBUG_CONF");
    FILE *f;
    char *p;
    int finish_off = FALSE;
    ndrx_inicfg_t *cconfig = ndrx_get_G_cconfig();
    ndrx_inicfg_section_keyval_t *conf = NULL, *cc;
    memset(&G_ubf_debug, 0, sizeof(G_ubf_debug));
    memset(&G_ndrx_debug, 0, sizeof(G_ndrx_debug));
    memset(&G_stdout_debug, 0, sizeof(G_stdout_debug));
    
    /* Initialize with defaults.. */
    G_ndrx_debug.dbg_f_ptr = stderr;
    G_ubf_debug.dbg_f_ptr = stderr;
    G_tp_debug.dbg_f_ptr = stderr;
    G_stdout_debug.dbg_f_ptr = stdout;
    
    strcpy(G_ubf_debug.module, "UBF ");
    strcpy(G_ndrx_debug.module, "NDRX");
    strcpy(G_tp_debug.module, "USER");
    
    G_ubf_debug.code = LOG_CODE_UBF;
    G_ndrx_debug.code = LOG_CODE_NDRX;
    G_tp_debug.code = LOG_CODE_TP;
    
    G_tp_debug.pid = G_ubf_debug.pid = G_ndrx_debug.pid = G_stdout_debug.pid = getpid();
    
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
                    buf[strlen(buf)-1]=EOS;
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
        if (SUCCEED==ndrx_cconfig_get(NDRX_CONF_SECTION_DEBUG, &conf))
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
    if (EOS!=G_ndrx_debug.filename[0])
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
            setvbuf(G_ndrx_debug.dbg_f_ptr, NULL, _IOFBF, G_ndrx_debug.buffer_size);
            G_tp_debug.dbg_f_ptr = G_ubf_debug.dbg_f_ptr=G_ndrx_debug.dbg_f_ptr;
        }
    }

    /*
     Expected file syntax is:
     * file=/tmp/common.log ndrx=5 ubf=5 buf=1
     ndrxd file=/tmp/ndrxd.log
     xadmin file=/tmp/xadmin.log
     ...
     */
    
    if (NULL!=conf)
    {
        /* kill the conf */
        ndrx_keyval_hash_free(conf);
    }
    
    G_ndrx_debug_first = FALSE;
}

/**
 * Return current NDRX debug level.
 * @return - debug level..
 */
public ndrx_debug_t * debug_get_ndrx_ptr(void)
{
    /* NSTD_TLS_ENTRY; */
    NDRX_DBG_INIT_ENTRY;
    
    return get_debug_ptr(&G_ndrx_debug);
}

/**
 * Return TP pointer
 * @return 
 */
public ndrx_debug_t * debug_get_tp_ptr(void)
{
    /* NSTD_TLS_ENTRY; */
    NDRX_DBG_INIT_ENTRY;
    
    return get_debug_ptr(&G_tp_debug);
}

/**
 * Return current UBF debug level.
 * @return
 */
public ndrx_debug_t * debug_get_ubf_ptr(void)
{   /* NSTD_TLS_ENTRY; */
    NDRX_DBG_INIT_ENTRY;
    
    return get_debug_ptr(&G_ubf_debug);
}

/**
 * Return current NDRX debug level.
 * @return - debug level..
 */
public int debug_get_ndrx_level(void)
{
    NDRX_DBG_INIT_ENTRY;
    return G_ndrx_debug.level;
}

/**
 * Return current UBF debug level.
 * @return 
 */
public int debug_get_ubf_level(void)
{
    NDRX_DBG_INIT_ENTRY;
    return G_ubf_debug.level;
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
public void __ndrx_debug_dump_diff__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
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
    __ndrx_debug__(dbg_ptr, lev, file, line, func, "%s", comment);
    
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
public void __ndrx_debug_dump__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
        long line, const char *func, char *comment, void *ptr, long len)
{
    int i;
    unsigned char buf[17];
    unsigned char *cptr = (unsigned char*)ptr;
    char print_line[256]={0};
    NSTD_TLS_ENTRY;
    /* NDRX_DBG_INIT_ENTRY; - called by master macro */
    
    dbg_ptr = get_debug_ptr(dbg_ptr);
    
    __ndrx_debug__(dbg_ptr, lev, file, line, func, "%s", comment);

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
public void __ndrx_debug__(ndrx_debug_t *dbg_ptr, int lev, const char *file, 
        long line, const char *func, char *fmt, ...)
{
    va_list ap;
    char line_start[128];
    long ldate, ltime, lusec;
    char *line_print;
    int len;
    ndrx_debug_t *org_ptr = dbg_ptr;
    long  thread_nr = 0;
    /* NSTD_TLS_ENTRY; */
    
    /* NDRX_DBG_INIT_ENTRY; - called by master macro */
    
    if (NULL!=G_nstd_tls)
    {
        thread_nr = G_nstd_tls->M_threadnr;
    }
    
    dbg_ptr = get_debug_ptr(dbg_ptr);
    
    if ((len=strlen(file)) > 8)
        line_print = (char *)file+len-8;
    else
        line_print = (char *)file;
    

    ndrx_get_dt_local(&ldate, &ltime, &lusec);
    
    sprintf(line_start, "%c:%s:%d:%5d:%03ld:%08ld:%06ld%03d:%-8.8s:%04ld:",
        dbg_ptr->code, org_ptr->module, lev, (int)dbg_ptr->pid, 
        thread_nr, ldate, ltime, 
        (int)(lusec/1000), line_print, line);
    
    va_start(ap, fmt);    
    fputs(line_start, dbg_ptr->dbg_f_ptr);
    (void) vfprintf(dbg_ptr->dbg_f_ptr, fmt, ap);
    fputs("\n", dbg_ptr->dbg_f_ptr);
    va_end(ap);
    
    /* Handle some buffering... */
    BUFFER_CONTROL(dbg_ptr);
}
/**
 * Initialize debug library
 * Currently default level is to use maximum.
 *
 * Debug buffer is set to default 0, can be overriden by <KEY>+DBGBUF
 * @param module - module form which debug is initialized
 */
public void ndrx_dbg_init(char *module, char *config_key)
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
public void *ndrx_malloc_dbg(size_t size, long line, const char *file, const char *func)
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
public void ndrx_free_dbg(void *ptr, long line, const char *file, const char *func)
{
    userlog("[%p] => free(ptr=%p):%s %s:%ld", ptr, ptr, func, file, line);
    return free(ptr);
}

/**
 * Debug version of calloc();
 * @param size
 * @param line
 * @param file
 * @param func
 * @return 
 */
public void *ndrx_calloc_dbg(size_t nmemb, size_t size, long line, const char *file, const char *func)
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
public void *ndrx_realloc_dbg(void *ptr, size_t size, long line, const char *file, const char *func)
{   void *ret;
    int errnosv;
    
    ret= realloc(ptr, size);
    
    errnosv = errno;
            
    userlog("[%p] <= realloc(ptr=[%p], size=%d):%s %s:%ld", ret, ptr, 
            size, func, file, line);
    
    errno = errnosv;
    return ret;
    
}

