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
#include <config.h>
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

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
NDRX_API ndrx_debug_t G_ubf_debug;
NDRX_API ndrx_debug_t G_ndrx_debug;
NDRX_API ndrx_debug_t G_stdout_debug;
/*---------------------------Statics------------------------------------*/
NDRX_API volatile int G_ndrx_debug_first = TRUE;
static __thread long M_threadnr = 0; /* Current thread nr */
MUTEX_LOCKDECL(M_dbglock);	/* For spinlock */
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialize operating thread number.
 * note default is zero.
 */
public NDRX_API void ndrx_dbg_setthread(long threadnr)
{
    M_threadnr = threadnr;
}

/**
 * Lock the debug output
 */
public NDRX_API void ndrx_dbg_lock(void)
{
    MUTEX_LOCK_V(M_dbglock);
}

/**
 * Unlock the debug output
 */
public NDRX_API void ndrx_dbg_unlock(void)
{
    MUTEX_UNLOCK_V(M_dbglock);
}

/**
 * This initializes debug out form ndebug.conf
 */
public NDRX_API void ndrx_init_debug(void)
{
    char *cfg_file = getenv("NDRX_DEBUG_CONF");
    FILE *f;
    char *p;
    char filename[PATH_MAX]={EOS};
    
    memset(&G_ubf_debug, 0, sizeof(G_ubf_debug));
    memset(&G_ndrx_debug, 0, sizeof(G_ndrx_debug));
    memset(&G_stdout_debug, 0, sizeof(G_stdout_debug));
    
    /* Initialize with defaults.. */
    G_ndrx_debug.dbg_f_ptr = stderr;
    G_ubf_debug.dbg_f_ptr = stderr;
    G_stdout_debug.dbg_f_ptr = stdout;
    
    G_ubf_debug.pid = G_ndrx_debug.pid = G_stdout_debug.pid = getpid();
    
    /* static coinf */
    G_stdout_debug.buf_lines = 1;
    G_stdout_debug.buffer_size = 1;
    G_stdout_debug.level = log_debug;
    
    /* default bufsz  */
    G_ubf_debug.buffer_size = G_ndrx_debug.buffer_size = 50000;

    G_ubf_debug.buf_lines = G_ndrx_debug.buf_lines = 1;
    G_ubf_debug.level = G_ndrx_debug.level = log_debug;

    if (NULL!=cfg_file &&
            NULL!=(f=fopen(cfg_file, "r")))
    {
        char buf[5000];
        char *saveptr=NULL;
        
        /* process line by line */
        while (NULL!=fgets(buf, sizeof(buf), f))
        {
            char *tok=NULL;
            char *prog;
            /*char *strtok_r(char *str, const char *delim, char **saveptr);*/
            if ('#'==buf[0] || '\n'==buf[0])
            {
                /* skip comments */
                continue;
            }
            if (buf[strlen(buf)-1]=='\n')
                buf[strlen(buf)-1]=EOS;
            
            tok=strtok_r (buf,"\t ", &saveptr);

            if ('*'==tok[0] || 0==strcmp(tok, EX_PROGNAME))
            {
                int do_break = ('*'!=tok[0]);
                do
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
                    else if (0==strncmp("lines", tok, cmplen))
                    {
                        G_ndrx_debug.buf_lines = G_ubf_debug.buf_lines = atoi(p+1);
                        if (G_ndrx_debug.buf_lines<0)
                            G_ndrx_debug.buf_lines=G_ubf_debug.buf_lines=0;
                    }
                    else if (0==strncmp("bufsz", tok, cmplen))
                    {
                        G_ndrx_debug.buffer_size = G_ubf_debug.buffer_size = atoi(p+1);
                        if (G_ndrx_debug.buffer_size<=0)
                            G_ubf_debug.buffer_size = G_ndrx_debug.buffer_size = 50000;
                    }
                    else if (0==strncmp("file", tok, cmplen))
                    {
                        strcpy(filename, p+1);
                    }
                    tok=strtok_r (NULL,"\t ", &saveptr);
                } while (NULL!=tok);

                if (do_break)
                    break;
            }
        }

        fclose(f);
    }
    else if (NULL!=cfg_file)
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

    /* open debug file.. */
    if (EOS!=filename[0])
    {
        ndrx_str_env_subs(filename);
        if (!(G_ndrx_debug.dbg_f_ptr = fopen(filename, "a")))
        {
                fprintf(stderr,"Failed to open %s\n",filename);
                G_ubf_debug.dbg_f_ptr=G_ndrx_debug.dbg_f_ptr=stderr;
        }
        else
        {
            setvbuf(G_ndrx_debug.dbg_f_ptr,   NULL, _IOFBF, G_ndrx_debug.buffer_size);
            G_ubf_debug.dbg_f_ptr=G_ndrx_debug.dbg_f_ptr;
            strcpy(G_ndrx_debug.filename, filename);
            strcpy(G_ubf_debug.filename, filename);
        }
    }

    /*
     Expected file syntax is:
     * file=/tmp/common.log ndrx=5 ubf=5 buf=1
     ndrxd file=/tmp/ndrxd.log
     xadmin file=/tmp/xadmin.log
     ...
     */
    G_ndrx_debug_first = FALSE;
}

/**
 * Return current NDRX debug level.
 * @return - debug level..
 */
public NDRX_API ndrx_debug_t * debug_get_ndrx_ptr(void)
{
    NDRX_DBG_INIT_ENTRY;
    return &G_ndrx_debug;
}

/**
 * Return current UBF debug level.
 * @return
 */
public NDRX_API ndrx_debug_t * debug_get_ubf_ptr(void)
{
    NDRX_DBG_INIT_ENTRY;
    return &G_ubf_debug;
}

/**
 * Return current NDRX debug level.
 * @return - debug level..
 */
public NDRX_API int debug_get_ndrx_level(void)
{
    NDRX_DBG_INIT_ENTRY;
    return G_ndrx_debug.level;
}

/**
 * Return current UBF debug level.
 * @return 
 */
public NDRX_API int debug_get_ubf_level(void)
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
public NDRX_API void __ndrx_debug_dump_diff__(ndrx_debug_t *dbg_ptr, int lev, char *mod, const char *file, 
        long line, const char *func, char *comment, void *ptr, void *ptr2, long len)
{
    
    int i;
    unsigned char buf[17];
    unsigned char buf2[17];
    unsigned char *cptr = (unsigned char*)ptr;
    unsigned char *cptr2 = (unsigned char*)ptr2;
    char print_line[256]={0};
    char print_line2[256]={0};
    
    /* NDRX_DBG_INIT_ENTRY; - called by master macro */
    __ndrx_debug__(dbg_ptr, lev, mod, file, line, func, "%s", comment);
    
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
    sprintf (print_line2 + strlen(print_line2), "  %s", buf);

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
public NDRX_API void __ndrx_debug_dump__(ndrx_debug_t *dbg_ptr, int lev, char *mod, const char *file, 
        long line, const char *func, char *comment, void *ptr, long len)
{
    int i;
    unsigned char buf[17];
    unsigned char *cptr = (unsigned char*)ptr;
    char print_line[256]={0};
    
    /* NDRX_DBG_INIT_ENTRY; - called by master macro */
    
    __ndrx_debug__(dbg_ptr, lev, mod, file, line, func, "%s", comment);

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
public NDRX_API void __ndrx_debug__(ndrx_debug_t *dbg_ptr, int lev, char *mod, const char *file, 
        long line, const char *func, char *fmt, ...)
{
    va_list ap;
    char line_start[128];
    long ldate, ltime;
    struct timeval  time_val;
    struct timezone time_zone;
    char *line_print;
    int len;
    /* NDRX_DBG_INIT_ENTRY; - called by master macro */
    
    if ((len=strlen(file)) > 8)
        line_print = (char *)file+len-8;
    else
        line_print = (char *)file;
    
    gettimeofday( &time_val, &time_zone );
    ndrx_get_dt_local(&ldate, &ltime);
    
    sprintf(line_start, "%s:%d:%5d:%03ld:%08ld:%06ld%03d:%-8.8s:%04ld:",
        mod, lev, (int)dbg_ptr->pid, M_threadnr, ldate, ltime, 
        (int)(time_val.tv_usec/10000), line_print, line);
    
    va_start(ap, fmt);    
    fputs(line_start, dbg_ptr->dbg_f_ptr);
    (void) vfprintf(dbg_ptr->dbg_f_ptr, fmt, ap);
    fputs("\n", dbg_ptr->dbg_f_ptr);
    va_end(ap);
    
    /* Handle some buffering... */
    BUFFER_CONTROL(dbg_ptr);
}

/**
 * Set debug level. Override one set in debug.conf
 * @param level
 */
public NDRX_API void ndrx_dbg_setlev(ndrx_debug_t *dbg_ptr, int level)
{
    NDRX_DBG_INIT_ENTRY;
    if (level < 0)
            level = 0;
    if (level > DBG_MAXLEV)
            level = DBG_MAXLEV;
    dbg_ptr->level = level;
}

/**
 * Initialize debug library
 * Currently default level is to use maximum.
 *
 * Debug buffer is set to default 0, can be overriden by <KEY>+DBGBUF
 * @param module - module form which debug is initialized
 */
public NDRX_API void ndrx_dbg_init(char *module, char *config_key)
{
   NDRX_DBG_INIT_ENTRY;
}

