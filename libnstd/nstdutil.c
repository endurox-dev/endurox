/**
 * @brief NDR 'standard' common utilites
 *   Enduro Execution system platform library
 *
 * @file nstdutil.c
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
#include <ndrstandard.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>
#include <nstd_tls.h>

#include "nstdutil.h"
#include "ndebug.h"
#include "userlog.h"
#include "atmi_int.h"
#include <errno.h>
#include <excrypto.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define _MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Compre 3 segmeneted number
 * @param a1
 * @param a2
 * @param a3
 * @param b1
 * @param b2
 * @param b3
 * @return 
 */
expublic int ndrx_compare3(long a1, long a2, long a3, long b1, long b2, long b3)
{
    
    long res1 =  a1 - b1;
    long res2 =  a2 - b2;
    long res3 =  a3 - b3;
    
    
    if (res1!=0)
        return (int)res1;
    
    if (res2!=0)
        return (int)res2;
    
    return (int)res3;
    
}
/**
 * Return -1 in case if t1/tusec1 is less than t2/tusec2
 * return 0 in case if t1/tusec1 equals t2/tusec2
 * return 1 in case if t1/tusec1 greater t2/tusec2
 * @param t1 tstamp1
 * @param tusec1 tstamp1 (microsec)
 * @param t2 tstamp2
 * @param tusec2 tstamp2 (microsec)
 * @return see descr
 */
expublic int ndrx_utc_cmp(long *t1, long *tusec1, long *t2, long *tusec2)
{
    if (*t1 < *t2 || *t1 == *t2 && *tusec1 < *tusec2)
    {
        return -1;
    }
    else if (*t1 == *t2 && *tusec1 == *tusec2)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * Return timestamp split in two fields
 * @param t
 * @param tusec
 */
expublic void ndrx_utc_tstamp2(long *t, long *tusec)
{
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    
    *t = tv.tv_sec;
    *tusec = tv.tv_usec;
}

/**
 * Return YYYY-MM-DD HH:MI:SS timestamp from two field epoch
 * @param ts
 * @return 
 */
expublic char * ndrx_get_strtstamp2(int slot, long t, long tusec)
{
    time_t tfmt;
    struct tm utc;
    NSTD_TLS_ENTRY;
    
    tfmt = t;
    gmtime_r(&tfmt, &utc);
    strftime(G_nstd_tls->util_buf1[slot], 
            sizeof(G_nstd_tls->util_buf1[slot]), "%Y-%m-%d %H:%M:%S", &utc);
    
    return G_nstd_tls->util_buf1[slot];
}


/**
 * Return timstamp UTC, milliseconds since epoch date.
 * This assumes that platform uses 64bit long long.
 * Or we can drop the milliseconds if the platform does not handle that.
 */
expublic unsigned long long ndrx_utc_tstamp(void)
{
    struct timeval tv;
    unsigned long long ret;

    gettimeofday(&tv, NULL);

    /* so basically we need 6 byte storage or more */
    if (sizeof(unsigned long long)>=8) 
    {
        ret =
            (unsigned long long)(tv.tv_sec) * 1000 +
            (unsigned long long)(tv.tv_usec) / 1000;
    }
    else
    {
        ret = (unsigned long long)(tv.tv_sec);
    }
    
    return ret;
}

/**
 * Return timestamp in microseconds
 * @return 
 */
expublic unsigned long long ndrx_utc_tstamp_micro(void)
{
    struct timeval tv;
    unsigned long long ret;

    gettimeofday(&tv, NULL);
    
    if (sizeof(unsigned long long)>=8) 
    {
        ret =
            (unsigned long long)(tv.tv_sec) * 1000000 +
            (unsigned long long)(tv.tv_usec);
    }
    else
    {
        ret = (unsigned long long)(tv.tv_sec);
    }
    
    return ret;
}

/**
 * Return YYYY-MM-DD HH:MI:SS timestamp from micro seconds based timestamp.
 * @param ts
 * @return 
 */
expublic char * ndrx_get_strtstamp_from_sec(int slot, long ts)
{
    time_t t;
    struct tm utc;
    
    NSTD_TLS_ENTRY;
    
    t = ts;
    gmtime_r(&t, &utc);
    strftime(G_nstd_tls->util_buf2[slot], 
            sizeof(G_nstd_tls->util_buf2[slot]), "%Y-%m-%d %H:%M:%S", &utc);
    
    return G_nstd_tls->util_buf2[slot];
}

/**
 * Get tick count in one second for current platform
 * @return 
 */
expublic unsigned long long ndrx_get_micro_resolution_for_sec(void)
{
    unsigned long long ret;    
    
    if (sizeof(unsigned long long)>=8) 
    {
        ret = 1000000;
    }
    else
    {
        ret = 1;
    }
    
    return ret;
}


/**
 * Return date/time local 
 * @param p_date - ptr to store long date, format YYYYMMDD
 * @param p_time - ptr to store long time, format HHMISS
 * @param p_usec - ptr to store microseconds
 */
expublic void ndrx_get_dt_local(long *p_date, long *p_time, long *p_usec)
{
    struct tm       *p_tm;
    long            lret;
    struct timeval  timeval;
    struct timezone timezone_val;

    gettimeofday (&timeval, &timezone_val);
    p_tm = localtime(&timeval.tv_sec);
    *p_time = 10000L*p_tm->tm_hour+100*p_tm->tm_min+1*p_tm->tm_sec;
    *p_date = 10000L*(1900 + p_tm->tm_year)+100*(1+p_tm->tm_mon)+1*(p_tm->tm_mday);
    *p_usec = timeval.tv_usec;

    return;
}

/**
 * Replace string with other string, return malloced new copy
 * @param orig original string
 * @param rep string to replace
 * @param with replace with
 * @return NULL (if orgin is NULL) or newly allocated string
 */
expublic char *ndrx_str_replace(char *orig, char *rep, char *with) {
    char *result; /* the return string */
    char *ins;    /* the next insert point */
    char *tmp;    /* varies */
    int len_rep;  /* length of rep */
    int len_with; /* length of with */
    int len_front; /* distance between rep and end of last rep */
    int count;    /* number of replacements */

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count)
    {
        ins = tmp + len_rep;
    }

    /* first time through the loop, all the variable are set correctly 
     * from here on,
     *    tmp points to the end of the result string
     *    ins points to the next occurrence of rep in orig
     *    orig points to the remainder of orig after "end of rep"
     */
    tmp = result = NDRX_MALLOC(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--)
    {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        NDRX_STRNCPY(tmp, orig, len_front);
        tmp =  tmp + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; /* move to next "end of rep" */
    }
    strcpy(tmp, orig);
    return result;
}

/**
 * Substitute environment 
 * TODO: Implement $[<func>:data] substitution. Currently available functions:
 * $[dec:<encrypted base64 string>] or just will copy to new function
 * 
 * @param str
 * @param buf_len buffer len for overrun checks
 * @return 
 */
expublic char * ndrx_str_env_subs_len(char * str, int buf_size)
{
    char *p, *p2, *p3;
    char *next = str;
    char envnm[1024];
    char *env;
    char *out;
    char *empty="";
    char *malloced;
    char *pval;
    char *tempbuf = NULL;
    char *close;
    
#define FUNCTION_SEPERATOR  '='
    
    while (NULL!=(p=strstr(next, "${")))
    {
        p2=strstr(next, "\\${");
        p3=strstr(next, "\\\\${");
        
        /* this is escaped stuff, we shall ignore that */
        if (p == p3+2)
        {
            /* This is normally escaped \, thus ignore & continue 
             * Does not affects our value
             */
        }
        else if (p == (p2+1))
        {
            /* This is our placeholder escaped, thus skip 
             * But we need to kill the escape
             */
            memmove(p2, p, strlen(p)+1);
            next=p+3; 
            continue;
        }
        
        /* Bug #317 */
        close =strchr(p, '}');
        if (NULL!=close)
        {
            long bufsz;
            int cpylen = close-p-2;
            int envlen;
            /* do substitution */
            NDRX_STRNCPY(envnm, p+2, cpylen);
            envnm[cpylen] = EXEOS;
            
            if (NULL==(pval=strchr(envnm, FUNCTION_SEPERATOR)))
            {
                env = getenv(envnm);
                
                if (NULL!=env)
                    out = env;
                else
                    out = empty;
            }
            else
            {
                *pval=EXEOS;
                pval++;
                
                if (0==(bufsz=strlen(pval)))
                {
                    userlog("Invalid encrypted data (zero len, maybe invalid sep? not =?) "
                            "for: [%s] - fill empty", envnm);
                    out = empty;
                }
                else
                {
                    int err;
                    tempbuf = NDRX_MALLOC(bufsz);
                    
                    if (NULL==tempbuf)
                    {
                        err = errno;
                        userlog("Failed to allocate %ld bytes for decryption buffer: %s", 
                                bufsz, strerror(errno));
                        NDRX_LOG_EARLY(log_error, "Failed to allocate %ld bytes "
                                "for decryption buffer: %s", 
                                bufsz, strerror(errno));
                        goto out;
                    }
                    
                    /* So function is:  
                     * - 'envnm'
                     * and the value is 'p'
                     * So syntax:
                     * ${dec=<encrypted string>}
                     */    
                    if (0==strcmp(envnm, "dec"))
                    {
                        /* About to decrypt the value... of p 
                         * space of the data will be shorter or the same size or smaller
                         * encrypted block.
                         */
                        if (EXSUCCEED!=ndrx_crypto_dec_string(pval, tempbuf, bufsz))
                        {
                            userlog("Failed to decrypt [%s] string: %s",
                                    pval, Nstrerror(Nerror));
                            NDRX_LOG_EARLY(log_error, "Failed to decrypt [%s] string: %s",
                                    pval, Nstrerror(Nerror));
                            out = empty;
                        }
                        out = tempbuf;
                    }
                    else
                    {
                        userlog("Unsupported substitution function: [%s] - skipping", 
                                pval);
                        NDRX_LOG_EARLY(log_error, "Failed to decrypt [%s] string: %s",
                                pval, Nstrerror(Nerror));
                        out = empty;
                    }
                    
                } /* if data > 0 */
            } /* if is function instead of env variable */
            


            envlen = strlen(out);
            
            /* fix up the buffer!!! */
            if (cpylen+3==envlen)
            {
                memcpy(p, out, envlen);
            }
            else if (cpylen+3 > envlen)
            {
                /* if buf_len == 0, skip the checks. */
                if (buf_size > 0 && 
                        strlen(str) + (cpylen+3 - envlen) > buf_size-1 /*incl EOS*/)
                {
                    if (NULL!=tempbuf)
                    {
                        NDRX_FREE(tempbuf);
                    }
                    /* cannot continue it is buffer overrun! */
                    return str;
                }
                
                /*int overleft = cpylen+2 - envlen; */
                /* copy there, and reduce total len */
                memcpy(p, out, envlen);
                /* copy left overs after } at the end of the env, including eos */
                memmove(p+envlen, close+1, strlen(close+1)+1);
            }
            else if (cpylen+3 < envlen)
            {
                int missing = envlen - (cpylen+2);
                
                /* we have to stretch that stuff and then copy in, including eos */
                memmove(close+missing, close+1, strlen(close+1)+1);
                memcpy(p, out, envlen);
            }
            
            /* free-up if temp buffer allocated. */
            
            next = p+envlen;
        }
        else
        {
            /* just step forward... */
            next+=2;
        }
        
        if (NULL!=tempbuf)
        {
            /* fix #268 */
            NDRX_FREE(tempbuf);
            tempbuf=NULL;
        }
    }
    
out:

    if (NULL!=tempbuf)
    {
        /* fix #268 */
        NDRX_FREE(tempbuf);
        tempbuf=NULL;
    }

    /* replace '\\' -> '\'  */
    if (strstr(str, "\\"))
    {
        malloced = ndrx_str_replace(str, "\\\\", "\\");
        strcpy(str, malloced);
        NDRX_FREE(malloced);
    }
    
    return str;
}

/* TODO: we need a callback for value getter. Also how we determine 
 * temp buffer size? Alloc temp space in the size of the buf_size? 
 * Also we need to have configurable open/close symbols.
 * We will have two data pointers
 * This works with NDRX logger.
 * Note that value cannot contain '}' for the functions and env variables
 */
expublic int ndrx_str_subs_context(char * str, int buf_size, char opensymb, char closesymb,
        void *data1, void *data2, void *data3, void *data4,
        int (*pf_get_data) (void *data1, void *data2, void *data3, void *data4,
            char *symbol, char *outbuf, long outbufsz))
{
    char *p, *p2, *p3;
    char *next = str;
    char symbol[1024];
    char *malloced;
    char *tempbuf = NULL;
    char open1[]={'$',opensymb,EXEOS};
    char open2[]={'\\', '$', opensymb, EXEOS};
    char open3[]={'\\', '\\', '$', opensymb, EXEOS};
    char *outbuf = NULL;
    int ret = EXSUCCEED;
    char *close;
    
    NDRX_MALLOC_OUT(outbuf, buf_size, char);
    
    while (NULL!=(p=strstr(next, open1)))
    {
        p2=strstr(next, open2);
        p3=strstr(next, open3);
        
        /* this is escaped stuff, we shall ignore that */
        if (p == p3+2)
        {
            /* This is normally escaped \, thus ignore & continue 
             * Does not affects our value
             */
        }
        else if (p == (p2+1))
        {
            /* This is our placeholder escaped, thus skip 
             * But we need to kill the escape
             */
            memmove(p2, p, strlen(p)+1);
            next=p+3; 
            continue;
        }
        
        /* Bug #317*/
        close =strchr(p, closesymb);
        if (NULL!=close)
        {
            long bufsz;
            int cpylen = close-p-2;
            int envlen;
            /* do substitution */
            NDRX_STRNCPY(symbol, p+2, cpylen);
            symbol[cpylen] = EXEOS;
            
            if (EXSUCCEED!=(ret=pf_get_data(data1, data2, data3, data4,
                    symbol, outbuf, buf_size)))
            {
                NDRX_LOG(log_error, "Failed to substitute [%s] error: %d", symbol, ret);
                goto out;
            }
            
            envlen = strlen(outbuf);
            
            /* fix up the buffer!!! */
            if (cpylen+3==envlen)
            {
                memcpy(p, outbuf, envlen);
            }
            else if (cpylen+3 > envlen)
            {
                int totlen;
                /* if buf_len == 0, skip the checks. */
                if (buf_size > 0 && 
                        (totlen=(strlen(str) + (cpylen+3 - envlen))) > buf_size-1 /*incl EOS*/)
                {
                    if (NULL!=tempbuf)
                    {
                        NDRX_FREE(tempbuf);
                    }
                    /* cannot continue it is buffer overrun! Maybe fail here? */
                    NDRX_LOG(log_error, "buffer overrun in string "
                            "formatting totlen=%d, bufsz-1=%d", totlen, buf_size-1);
                    EXFAIL_OUT(ret);
                }
                
                /*int overleft = cpylen+2 - envlen; */
                /* copy there, and reduce total len */
                memcpy(p, outbuf, envlen);
                /* copy left overs after } at the end of the env, including eos */
                memmove(p+envlen, close+1, strlen(close+1)+1);
            }
            else if (cpylen+3 < envlen)
            {
                int missing = envlen - (cpylen+2);
                
                /* we have to stretch that stuff and then copy in, including eos */
                memmove(close+missing, close+1, strlen(close+1)+1);
                memcpy(p, outbuf, envlen);
            }
            
            /* free-up if temp buffer allocated. */
            
            next = p+envlen;
        }
        else
        {
            /* just step forward... */
            next+=2;
        }
        
        if (NULL!=tempbuf)
        {
            /* fix #268 */
            tempbuf=NULL;
            NDRX_FREE(tempbuf);
        }
    }
    
out:
    /* replace '\\' -> '\'  */
    if (strstr(str, "\\"))
    {
        malloced = ndrx_str_replace(str, "\\\\", "\\");
        strcpy(str, malloced);
        NDRX_FREE(malloced);
    }

    if (NULL!=outbuf)
    {
        NDRX_FREE(outbuf);
    }

    return ret;
}


/**
 * Unknown buffer len.
 * @param str
 * @return 
 */
expublic char * ndrx_str_env_subs(char * str)
{
    return ndrx_str_env_subs_len(str, 0);
}

/**
 * Decode numbers from config file ending with K, M, G
 * NOTE! This does change the str value!!!!
 * @param str
 * @return number parsed/built
 */
expublic double ndrx_num_dec_parsecfg(char * str)
{
    double ret = 0;
    double multipler = 1;
    int len = strlen(str);
    int mapplied = EXFALSE;
    
    if (len>1)
    {
        switch (str[len-1])
        {
            case 'k':
            case 'K':
                multipler = 1000.0f;
                mapplied = EXTRUE;
                break;
            case 'm':
            case 'M':
                multipler = 1000000.0f;
                mapplied = EXTRUE;
                break;
            case 'g':
            case 'G':
                multipler = 1000000000.0f;
                mapplied = EXTRUE;
                break;
        }
        /* Avoid precision issues... */
        if (mapplied)
        {
            str[len-1] = EXEOS;
        }
    }
    
    ret = atof(str);
    
    ret*=multipler;
    
    return ret;
}

/**
 * Parse milli-seconds based record
 * @param str NOTE string is modified (last postfix removed for parsing)
 * @return parsed number of milliseconds
 */
expublic double ndrx_num_time_parsecfg(char * str)
{
    double ret = 0;
    double multipler = 1;
    int len = strlen(str);
    int mapplied = EXFALSE;
    
    if (len>1)
    {
        switch (str[len-1])
        {
            case 's':
                /* second */
                multipler = 1000.0f;
                mapplied = EXTRUE;
                break;
            case 'm':
                /* minute */
                multipler = 60.0f * 1000.0f;
                mapplied = EXTRUE;
                break;
            case 'h':
                /* hour */
                multipler = 60.0f * 60.0f * 1000.0f;
                mapplied = EXTRUE;
                break;
        }
        /* Avoid precision issues... */
        if (mapplied)
        {
            str[len-1] = EXEOS;
        }
    }
    
    ret = atof(str);
    
    ret*=multipler;
    
    return ret;
}


/**
 * Decode number
 * @param t
 * @param slot
 * @return 
 */
expublic char *ndrx_decode_num(long tt, int slot, int level, int levels)
{
    char tmp[128];
    long next_t=0;
    long t = tt;
#define DEC_K  ((long)1000)
#define DEC_M  ((long)1000*1000)
#define DEC_B  ((long)1000*1000*1000)
#define DEC_T  ((long long)1000*1000*1000*1000)

    NSTD_TLS_ENTRY;
    
    level++;

    if ((double)t/DEC_K < 1.0) /* Less that thousand */
    {
        snprintf(tmp, sizeof(tmp), "%ld", t);
    }
    else if ((double)t/DEC_M < 1.0) /* less than milliion */
    {
        snprintf(tmp, sizeof(tmp), "%ldK", t/DEC_K);
        
        if (level<levels)
            next_t = t%DEC_K;
    }
    else if ((double)t/DEC_B < 1.0) /* less that billion */
    {
        snprintf(tmp, sizeof(tmp), "%ldM", t/DEC_M);
        
        if (level<levels)
            next_t = t%DEC_M;
    }
    else if ((double)t/DEC_T < 1.0) /* less than trillion */
    {
        snprintf(tmp, sizeof(tmp), "%ldB", t/DEC_B);
        
        if (level<levels)
            next_t = t%DEC_B;
    }
    
    if (level==1)
    {
        NDRX_STRCPY_SAFE(G_nstd_tls->util_text[slot], tmp);
    }
    else
    {
        strcat(G_nstd_tls->util_text[slot], tmp);
    }
    
    if (next_t)
        ndrx_decode_num(next_t, slot, level, levels);
    
    return G_nstd_tls->util_text[slot];
}

/**
 * Strip specified char from string
 * @param haystack - string to strip
 * @param needle - chars to strip
 * @return 
 */
expublic char *ndrx_str_strip(char *haystack, char *needle)
{
    char *dest;
    char *src;
    int len = strlen(needle);
    int i;
    int have_found;
    dest = src = haystack;

    for (; EXEOS!=*src; src++)
    {
        have_found = EXFALSE;
        for (i=0; i<len; i++)
        {
            if (*src == needle[i])
            {
                have_found = EXTRUE;
                continue;
            }
        }
        /* Copy only if have found... */
        if (!have_found)
        {
            *dest = *src;
            dest++;
        }
    }
    
    *dest = EXEOS;
    
    return haystack;
}

/**
 * Strip off given chars from string ending
 * @param s string to process
 * @param needle chars to search and strip off from end
 * @return same s string
 */
expublic char* ndrx_str_rstrip(char* s, char *needle)
{
    int i;    
    char* p = s + strlen(s);
    while (p > s)
    {
        p--;
        
        if (strchr(needle, *p))
        {
            *p = '\0';
        }
        else
        {
            /* we are done */
            break;
        }
    }
    return s;
}

/**
 * Return pointer to data where first non-matched char starts
 * @param s string to process
 * @param needle char to strip off
 * @return ptr to start non matched char
 */
expublic char* ndrx_str_lstrip_ptr(char* s, char *needle)
{
    int len = strlen(s);
    int i;
    char *p =s;
    
    for (i=0; i<len; i++)
    {
        if (strchr(needle, *p))
        {
            p++;
        }
        else
        {
            break;
        }
    }
    
    return p;
}

/**
 * Check is string a integer
 * @param str string to test
 * @return TRUE/FALSE
 */
expublic int ndrx_isint(char *str)
{
   if (*str == '-')
   {
      ++str;
   }

   if (!*str)
   {
      return EXFALSE;
   }

   while (*str)
   {
      if (!isdigit(*str))
      {
         return EXFALSE;
      }
      else
      {
         ++str;
      }
   }
   
   return EXTRUE;
}

/**
 * Count the number of specified chars in string
 * @param str
 * @param character
 * @return 
 */
expublic int ndrx_nr_chars(char *str, char character)
{
    char *p = str;
    int count = 0;
    
    do
    {
        if (*p == character)
        {
            count++;
        }
    } while (*(p++));

    return count;
}

/**
 * Returns the string mapped to long value
 * @param map - mapping table
 * @param val - value to map
 * @param endval - List end/default value
 * @return ptr to maping str
 */
expublic char *ndrx_dolongstrgmap(longstrmap_t *map, long val, long endval)
{
    do 
    {
        if (map->from == val)
        {
            return map->to;
        }
        map++;
    } while (endval!=map->from);
    
    return map->to;
}


/**
 * Returns the string mapped to long value
 * @param map - mapping table
 * @param val - value to map
 * @param endval - List end/default value
 * @return ptr to maping str
 */
expublic char *ndrx_docharstrgmap(longstrmap_t *map, char val, char endval)
{
    do 
    {
        if (map->from == val)
        {
            return map->to;
        }
        map++;
    } while (endval!=map->from);
    
    return map->to;
}


/**
 * Get thread id (not the very portable way...)
 * @return 
 */
expublic uint64_t ndrx_gettid(void) 
{
    pthread_t ptid = pthread_self();
    uint64_t threadId = 0;
    memcpy(&threadId, &ptid, _MIN(sizeof(threadId), sizeof(ptid)));
    return threadId;
}

/**
 * Tests for file existance
 * @param filename path + filename
 * @return TRUE if exists / FALSE not exists
 */
expublic int ndrx_file_exists(char *filename)
{
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

/**
 * Test if given path is regular file
 * @param path
 * @return 
 */
expublic int ndrx_file_regular(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

/**
 * read from stdin with stripping of trailing \r or \n
 * @return NULL or allocated stdin string read
 */
expublic char * ndrx_fgets_stdin_strip(char *buf, int bufsz)
{
    int len;

    if (NULL==fgets(buf, bufsz, stdin))
    {
        userlog("%s: fgets fail: %s", __func__, strerror(errno));
        return NULL;
    }
    
    len = strlen(buf);
    
    if (len>0)
    {
        len--;
        
        /* strip off newline */
        if (buf[len]=='\n')
        {
            buf[len] = 0;
            len--;
        }
        
        /* strip off \r */
        if (len>= 0 && buf[len]=='\r')
        {
            buf[len] = 0;
        }
    }
        
    return buf;
}


/**
 * Enduro/X Cross platform getline version (system version, more close to GNU)
 * @param lineptr must be pre-allocated (for Macos will use fgets on this buffer)
 * @param n buffer size (ptr to)
 * @param stream file to read from
 * @return number bytes read for Macos will return just 1 or -1
 */
expublic ssize_t ndrx_getline(char **lineptr, size_t *n, FILE *stream)
{
#ifdef HAVE_GETLINE
    
    return getline(lineptr, n, stream);
    
#else
    if (NULL==fgets(*lineptr, *n, stream))
    {
        return EXFAIL;
    }
    else
    {
        return EXTRUE;
    }
#endif
}

/**
 * Calculate crc32 of given file
 * @param file
 * @return CRC32 or FAIL (-1)
 */
expublic int ndrx_get_cksum(char *file)
{
    unsigned char checksum = 0;
    int ret = EXSUCCEED;
    
    FILE *fp = fopen(file,"rb");
    
    if (NULL!=fp)
    {
        
        while (!feof(fp) && !ferror(fp)) {
           checksum ^= fgetc(fp);
        }

        fclose(fp);
    }
    else
    {
        ret = EXFAIL;
    }
    
    if (EXSUCCEED==ret)
    {
        return checksum;
    }
    else
    {
        return EXFAIL;
    }
}

/**
 * Get the path from executable
 * @param out_path  Out buffer (full binary name where lives...)
 * @param bufsz Out buffer size
 * @param in_binary Binary to search for
 * @return NULL (if not found) or ptr to out_path if found
 */
expublic char * ndrx_get_executable_path(char * out_path, size_t bufsz, char * in_binary)
{
    char * systemPath = NULL;
    char * candidateDir = NULL;
    int found = EXFALSE;
    char *ret;
    
    systemPath = getenv ("PATH");
    if (systemPath != NULL)
    {
        systemPath = strdup (systemPath);
        for (candidateDir = strtok (systemPath, ":"); 
                candidateDir != NULL; 
                candidateDir = strtok (NULL, ":"))
        {
            snprintf(out_path, bufsz, "%s/%s", candidateDir, in_binary);
            
            if (access(out_path, F_OK) == 0)
            {
                found = EXTRUE;
                goto out;
            }
        }
    }

out:
    
    if (systemPath)
    {
        free(systemPath);
    }

    if (found) 
    {
        ret = out_path;
    }
    else
    {
        out_path[0] = EXEOS;
        ret = NULL;
    }
    
    return ret;
}

/**
 * Allocate & duplicate the ptr to which org points
 * @param org memroy to copy
 * @param len length to copy
 * @return NULL or allocated memory
 */
expublic char * ndrx_memdup(char *org, size_t len)
{
    char *ret;
    
    if (NULL!=(ret = NDRX_MALLOC(len)))
    {
        memcpy(ret, org, len);
        return ret;
    }
    
    return NULL;
}

/**
 * Locale independent atof
 * Basically this assumes that home decimal separator is '.'. I.e. \r str must
 * contain only '.'.
 * @param str string to convert to float, decimal separator is '.'.
 * @return converted decimal value
 */
expublic double ndrx_atof(char *str)
{
    char test[5];
    char buf[128];
    char *p;
    int len, i;
    
    /* extract the decimal separator... */
    snprintf(test, sizeof(test), "%.1f", 0.0f);
    
    if (NDRX_LOCALE_STOCK_DECSEP!=test[1])
    {
        NDRX_STRCPY_SAFE(buf, str);
        len = strlen(buf);
        
        for (i=0; i<len; i++)
        {
            if (NDRX_LOCALE_STOCK_DECSEP==buf[i])
            {
                buf[i] = test[1];
            }
        }
        
        p = buf;
    }
    else
    {
        p = str;
    }
    
    return atof(p);
}

/**
 * Extract tokens from string
 * @param str
 * @param fmt   Format string for scanf
 * @param tokens
 * @param tokens_elmsz
 * @param len
 * @return 0 - no tokens extracted
 */
expublic int ndrx_tokens_extract(char *str1, char *fmt, void *tokens, 
        int tokens_elmsz, int len)
{
    int ret = 0;
    char *str = NDRX_STRDUP(str1);
    char *ptr;
    char *token;
    char *str_first = str;
    
    if (NULL==str)
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to duplicate [%s]: %s", str1, strerror(err));
        userlog("Failed to duplicate [%s]: %s", str1, strerror(err));
        goto out;
    }
    
    while ((token = strtok_r(str_first, "\t ", &ptr)))
    {
        if (NULL!=str_first)
        {
            str_first = NULL; /* now loop over the string */
        }
        
        if (ret<len)
        {
            sscanf(token, fmt, tokens);
            tokens+=tokens_elmsz;
        }
        else
        {
            break;
        }
        
        ret++;
    }
    
out:

    if (NULL!=str)
    {
        NDRX_FREE(str);
    }
    return ret;
}

/**
 * Remove trailing newlines
 * @param str
 * @return 
 */
expublic void ndrx_chomp(char *str)
{
    int len = strlen(str);

    while (len>1 && (str[len-1]=='\n' || str[len-1]=='\r'))
    {
        str[len-1] = EXEOS;
        len--;
    }
}

/**
 * 32bit rotate left
 * @param x variable to rotate bits left
 * @param n number of bits to rotate
 * @return return value
 */
expublic uint32_t ndrx_rotl32b (uint32_t x, uint32_t n)
{
  if (!n) return x;
  return (x<<n) | (x>>(32-n));
}


/**
 * Return string len, but do it until the max position
 * @param str string to test
 * @param max max len to test to
 * @return string len
 */
expublic size_t ndrx_strnlen(char *str, size_t max)
{
    char *p;
    
    for(p = str; *p && max; ++p)
    {
        max--;
    }
    
    return(p - str);
}

/**
 * Initialize the grow list
 * @param list ptr to list (can be un-initialized memory)
 * @param step number of elements by which to reallocate ahead
 * @param size number of element
 */
expublic void ndrx_growlist_init(ndrx_growlist_t *list, int step, size_t size)
{
    list->maxindexused = EXFAIL;
    list->items = 0;
    list->step = step;
    list->size = size;
    list->mem = NULL;
}

/**
 * Add element to the list. Allocate/reallocate linear array as needed.
 * @param list list struct pointer
 * @param item ptr to item
 * @param index zero based item index in the memory
 * @return EXSUCCEED (all OK), EXFAIL (failed to allocate)
 */
expublic int ndrx_growlist_add(ndrx_growlist_t *list, void *item, int index)
{
    int ret = EXSUCCEED;
    int next_blocks;
    size_t new_size;
    char *p;

    if (NULL==list->mem)
    {
        new_size = list->step * list->size;
        if (NULL==(list->mem = NDRX_MALLOC(list->step * list->size)))
        {
            userlog("Failed to alloc %d bytes: %s", new_size,
                        strerror(errno));
            
            EXFAIL_OUT(ret);
        }
        
        list->items+=list->step;
    }
    
    while (index+1 > list->items)
    {
        list->items+=list->step;
        
        next_blocks = list->items / list->step;
        
        new_size = next_blocks * list->step * list->size;
        /*
        NDRX_LOG(log_debug, "realloc: new_size: %d (index: %d items: %d)", 
                new_size, index, list->items);
        */
        if (NULL==(list->mem = NDRX_REALLOC(list->mem, new_size)))
        {
            userlog("Failed to realloc %d bytes (%d blocks): %s", new_size,
                        next_blocks, strerror(errno));
            
            EXFAIL_OUT(ret);
        }
    }
    
    /* finally we are ready for data */
    p = list->mem;
    p+=(index * list->size);
    
    /*
    NDRX_LOG(log_debug, "Ptr: %p, write ptr %p, from %p (size: %d, index: %d)",
            list->mem, p, item, (int)list->size, index);
    
    NDRX_DUMP(log_debug, "data for writting", item, list->size);
    */
    memcpy( p, item, list->size);
    
    
    if (index > list->maxindexused)
    {
        list->maxindexused = index;
    }
    
out:

    return ret;
    
}

/**
 * Append entry (at the end of the array)
 * @param list list to be appended
 * @param item item to be added to the end of the array
 * @return EXSUCCEED/EXFAIL (out of the mem)
 */
expublic int ndrx_growlist_append(ndrx_growlist_t *list, void *item)
{
    return ndrx_growlist_add(list, item, list->maxindexused+1);
}

/**
 * Free up the grow list. User is completely responsible for any data to be freed
 * from the mem block
 * @param list ptr to list
 */
expublic void ndrx_growlist_free(ndrx_growlist_t *list)
{
    NDRX_FREE(list->mem);
}

/* vim: set ts=4 sw=4 et smartindent: */
