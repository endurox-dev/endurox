/* 
** Common command line options handler
**
** @file nclopt.h
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
#ifndef NCLOPT_H
#define	NCLOPT_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NCLOPT_MAND               0x1        /* Mandatory field             */
#define NCLOPT_OPT                0x2        /* Optiona field               */
#define NCLOPT_HAVE_VALUE         0x4        /* Have value                  */
#define NCLOPT_TRUEBOOL           0x8        /* Boolean flag (default)      */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

typedef struct ncloptmap ncloptmap_t;
struct ncloptmap
{
    char key;
    int datatype; /* UBF based data type */
    void *ptr;
    int buf_size;    /* Number of chars in buffer, used for strings... */
    int flags;
    char *descr;            /* Field description */
    int have_loaded;        /* Dynamic field, used per parse session... */
};
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int nstd_parse_clopt(ncloptmap_t *opts, int print_values, 
        int argc, char **argv);

#ifdef	__cplusplus
}
#endif

#endif	/* NCLOPT_H */

