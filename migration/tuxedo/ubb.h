/**
 * @brief Tuxedo ubb config parser -> Enduro/X translator
 *
 * @file ubb.h
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
#ifndef UBB_H
#define	UBB_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <ndrstandard.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/* used for ddr parser */
typedef char ndrx_routcritseq_dl_t;

/**
 * Support structure for parse time
 */
typedef struct
{
    int     error;       /**< error is not set                           */
    int     line;        /**< Line number of config                      */
    ndrx_growlist_t  stringbuffer;   /**< list used for string build */
    char errbuf[1024];
    char *parsebuf;     /**< currently parsing buffer ptr, for ddr       */
} ndrx_ubb_parser_t;

/*---------------------------Globals------------------------------------*/
extern ndrx_ubb_parser_t ndrx_G_ubbp;      /**< Parsing time attributes, ubb*/
extern ndrx_ubb_parser_t ndrx_G_ddrp;      /**< Parsing time attributes, ddr*/

extern int ndrx_G_ubbcolumn;
extern int ndrx_G_ubbline;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern void ubberror(char *s, ...);
extern void ddrerror(char *s, ...);
extern int ubbaddarg(char *arg);
extern int ubbaddres(char *arg);

extern int ubb_add_val(char *arg);
extern int ubb_add_res_parm(char *arg);
extern int ubb_add_sect_parm(char *arg);
extern int ubb_add_sect_keyw(char *arg);
extern int ubb_add_sect(char *arg);
extern int init_vm(char *script_nm, char *opt_n, char *opt_y, char *opt_L,
        char *opt_A, char *opt_P);
extern int call_add_func(const char *func, char *arg);

extern char *ndrx_ddr_new_rangeval(char *range, int is_negative, int dealloc);
extern ndrx_routcritseq_dl_t * ndrx_ddr_new_rangeexpr(char *range_min, char *range_max);
extern int ndrx_ddr_add_group(ndrx_routcritseq_dl_t * seq, char *grp, int is_mallocd);


#ifdef	__cplusplus
}
#endif

#endif	/* UBB_H */

/* vim: set ts=4 sw=4 et smartindent: */
