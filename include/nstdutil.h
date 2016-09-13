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
#include <ndrx_config.h>
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

extern NDRX_API void ndrx_get_dt_local(long *p_date, long *p_time);
extern NDRX_API unsigned long long ndrx_utc_tstamp(void);
extern NDRX_API unsigned long long ndrx_utc_tstamp_micro(void);
extern NDRX_API char * ndrx_get_strtstamp_from_micro(int slot, unsigned long long ts);
extern NDRX_API unsigned long long ndrx_get_micro_resolution_for_sec(void);
extern NDRX_API char * ndrx_str_env_subs(char * str);
extern char * ndrx_str_env_subs_len(char * str, int buf_size);
extern NDRX_API char *ndrx_str_replace(char *orig, char *rep, char *with);
extern NDRX_API void ndrx_utc_tstamp2(long *t, long *tusec);
extern NDRX_API char * ndrx_get_strtstamp2(int slot, long t, long tusec);
extern NDRX_API int ndrx_compare3(long a1, long a2, long a3, long b1, long b2, long b3);

extern NDRX_API char *ndrx_decode_num(long tt, int slot, int level, int levels);
extern NDRX_API char *ndrx_str_strip(char *haystack, char *needle);

extern NDRX_API int ndrx_isint(char *str);
extern NDRX_API int ndrx_nr_chars(char *str, char character);

extern NDRX_API int ndrx_file_exists(char *filename);
extern NDRX_API int ndrx_file_regular(char *path);

/* Mapping functions: */
extern NDRX_API char *ndrx_dolongstrgmap(longstrmap_t *map, long val, long endval);
extern NDRX_API char *ndrx_docharstrgmap(longstrmap_t *map, char val, char endval);

/* Threading functions */
extern NDRX_API uint64_t ndrx_gettid(void);

/* Internal testing */
extern NDRX_API int ndrx_bench_write_stats(double msgsize, double callspersec);

/* Standard library TLS: */
extern NDRX_API void * ndrx_nstd_tls_get(void);
extern NDRX_API void ndrx_nstd_tls_set(void *data);
extern NDRX_API void ndrx_nstd_tls_free(void *data);
extern NDRX_API void * ndrx_nstd_tls_new(int auto_destroy, int auto_set);

#ifdef	__cplusplus
}
#endif

#endif	/* NUTIL_H */

