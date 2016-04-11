/* 
** NDR 'standard' common utilites
** Enduro Execution system platform library
**
** @file nstdutil.h
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
#ifndef NUTIL_H
#define	NUTIL_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <stdint.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

typedef struct longstrmap longstrmap_t;
struct longstrmap
{
    long from;
    char *to;
};

typedef struct charstrmap charstrmap_t;
struct charstrmap
{
    long from;
    char *to;
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern void nstdutil_get_dt_local(long *p_date, long *p_time);
extern unsigned long long nstdutil_utc_tstamp(void);
extern char * nstdutil_str_env_subs(char * str);

extern char *nstdutil_decode_num(long tt, int slot, int level, int levels);
extern char *nstdutil_str_strip(char *haystack, char *needle);

public int nstdutil_isint(char *str);
    
/* Mapping functions: */
extern char *dolongstrgmap(longstrmap_t *map, long val, long endval);
extern char *docharstrgmap(longstrmap_t *map, char val, char endval);

/* Threading functions */
extern uint64_t ndrx_gettid(void);

/* New env */
extern int load_new_env(char *file);

#ifdef	__cplusplus
}
#endif

#endif	/* NUTIL_H */

