/* 
** NDRX helper functions.
**
** @file utils.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>


#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <nclopt.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
#if 0
/**
 * Get argument from command line
 * @param param - param to search, for example "-y"
 * @param argc - count of args
 * @param argv - list of args
 * @param out - where to put pointer of the value;
 * @return TRUE (found)/FALSE (not found)
 */
public int get_arg(char *param, int argc, char **argv, char **out)
{
    int i;

    for (i=0; i<argc; i++)
    {
        if (0==strcmp(param, argv[i]))
        {
            if (i+1<argc && NULL!=out)
            {
                /* return some value out there! */
                *out = argv[i+1];
            }
            return TRUE;
        }
    }
    
    return FALSE;
}
#endif

/**
 * If confirmation is required for command, then check it.
 * @param message
 * @param argc
 * @param argv
 * @return TRUE/FALSE
 */
public int chk_confirm(char *message, short is_confirmed)
{
    int ret=FALSE;
    char buffer[128];
    int ans_ok = FALSE;
    
    if (!is_confirmed)
    {
        if (isatty(0))
        {
            do
            {
                /* Ask Are you sure */
                fprintf(stderr, "%s [Y/N]: ", message);
                while (NULL==fgets(buffer, sizeof(buffer), stdin))
                {
                    /* do nothing */
                }

                if (toupper(buffer[0])=='Y' && '\n'==buffer[1] && EOS==buffer[2])
                {
                    ret=TRUE;
                    ans_ok=TRUE;
                }
                else if (toupper(buffer[0])=='N' && '\n'==buffer[1] && EOS==buffer[2])
                {
                    ret=FALSE;
                    ans_ok=TRUE;
                }

            } while (!ans_ok);
        }
        else
        {
            NDRX_LOG(log_warn, "Not tty, assuming no for: %s", message);
        }
    }
    else
    {
        ret=TRUE;
    }
    
    return ret;
}

/**
 * Quick wrapper for argument parsing...
 * @param message
 * @param artc
 * @param argv
 * @return 
 */
public int chk_confirm_clopt(char *message, int argc, char **argv)
{
    int ret = SUCCEED;
    short confirm = FALSE;
    
    ncloptmap_t clopt[] =
    {
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, "Confirm"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
    {
        fprintf(stderr, "Invalid options, see `help'.");
        FAIL_OUT(ret);
    }
    
    
    ret = chk_confirm(message, confirm);
    
out:
    return ret;
}

