/**
 * @brief UBF library
 *   Contains auxiliary functions
 *
 * @file utils.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ubf.h>
#include <ubf_int.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define IS_NOT_PRINTABLE(X) !(isprint(X) && !iscntrl(X))
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
char HEX_TABLE[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Returns requested space required for chracter masking.
 * Note that for strings we provide the string len not including EOS. Meaning
 * if string is ok, it should return 0 non-printable count (EOS left out!).
 *
 * @param str - string into which we should look
 * @param len - length of the resource.
 * @return count of non printable chracters
 */
expublic int ndrx_get_nonprintable_char_tmpspace(char *str, int len)
{
    int ret=0;
    int i;

    for (i=0; i<len; i++)
    {
        if (IS_NOT_PRINTABLE(str[i]))
        {
            ret+=3;
        }
        else if ('\\'==str[i])
        {
            ret+=2;
        }
        else
        {
            ret++;
        }
    }
    return ret;
}
/**
 * Builds printable version out 
 */
expublic void ndrx_build_printable_string(char *out, int out_len, char *in, int in_len)
{
    int i;
    int cur = 0;
    
    for (i=0; i<in_len; i++)
    {
        if (IS_NOT_PRINTABLE(in[i]))
        {
            /* we shall leave space for EOS, thus having not >= but > and same
             * bellow.
             */
            if (out_len - cur > 3)
            {
                /* make int hexy */
                out[cur++] = '\\';
                out[cur++] = HEX_TABLE[(in[i]>>4)&0x0f];
                out[cur++] = HEX_TABLE[in[i]&0x0f];
            }
            else
            {
                break;
            }
        }
        else if ('\\'==in[i])
        {
            /* Convert \ to two \\ */
            if (out_len - cur > 2)
            {
                out[cur++] = '\\';
                out[cur++] = '\\';
            }
            else
            {
                break;
            }
        }
        else if (out_len - cur > 1)
        {
            /* copy off printable char */
            out[cur++] = in[i];
        }
        else
        {
            break;
        }
    }
    
    if (cur<out_len)
    {
        out[cur] = EXEOS;
    }
}

/**
 * Returns decimal number out from hex digit
 * @param c - hex digit 0..f
 * @return -1 (on FAIL)/decimal number
 */
expublic int ndrx_get_num_from_hex(char c)
{
    int ret=EXFAIL;
    int i;

    for (i=0; i< sizeof(HEX_TABLE); i++)
    {
        if (toupper(HEX_TABLE[i])==toupper(c))
        {
            ret=i;
            break;
        }
    }

    return ret;
}


/**
 * Function builds string back from value built by build_printable_string
 * We will re-use the same string, because by the case normalized version is shorter
 * that printable version
 * @param str - coded string with build_printable_string function
 */
expublic int ndrx_normalize_string(char *str, int *out_len)
{
    int ret=EXSUCCEED;
    int real=0;
    int data=0;
    int len = strlen(str);
    int high, low;
    while (data<len)
    {
        if (str[data]=='\\')
        {
            if (data+1>=len)
            {
                UBF_LOG(log_error, "String terminates after prefix \\");
                return EXFAIL; /* << RETURN! */
            }
            else if (str[data+1]=='\\') /* this is simply \ */
            {
                str[real] = str[data];
                data+=2;
                real++;
            } 
            else if (data+2>=len)
            {
                UBF_LOG(log_error, "Hex code does not follow at the"
                                                " end of string for \\");
                return EXFAIL; /* << RETURN! */
            }
            else
            {
                /* get high, low parts */
                high = ndrx_get_num_from_hex(str[data+1]);
                low = ndrx_get_num_from_hex(str[data+2]);
                if (EXFAIL==high||EXFAIL==low)
                {
                    UBF_LOG(log_error, "Invalid hex number 0x%c%c",
                                                    str[data+1], str[data+2]);
                    return EXFAIL; /* << RETURN! */
                }
                else
                {
                    /* Construct the value back */
                    str[real]= high<<4 | low;
                    real++;
                    data+=3;
                }
            }
        }
        else /* nothing special about this */
        {
            str[real] = str[data];
            real++;
            data++;
        }
    }
    /* Finish up the string with EOS! 
    str[real] = EOS;*/
    *out_len = real;
    
    return ret;
}

/**
 * Dump the UBF buffer to log file
 * @lev - debug level
 * @title - debug title
 * @p_ub - pointer to UBF buffer
 */
expublic void ndrx_debug_dump_UBF(int lev, char *title, UBFH *p_ub)
{
    ndrx_debug_t * dbg = debug_get_ndrx_ptr();
    if (dbg->level>=lev)
    {
        NDRX_LOG(lev, "%s", title);
        Bfprint(p_ub, dbg->dbg_f_ptr);
    }
}


/**
 * Dump the UBF buffer to log file, UBF logger
 * @lev - debug level
 * @title - debug title
 * @p_ub - pointer to UBF buffer
 */
expublic void ndrx_debug_dump_UBF_ubflogger(int lev, char *title, UBFH *p_ub)
{
    ndrx_debug_t * dbg = debug_get_ubf_ptr();
    if (dbg->level>=lev)
    {
        UBF_LOG(lev, "%s", title);
        Bfprint(p_ub, dbg->dbg_f_ptr);
    }
}


/* vim: set ts=4 sw=4 et smartindent: */
