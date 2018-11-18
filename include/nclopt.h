/**
 * @brief Common command line options handler
 *
 * @file nclopt.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
extern NDRX_API int nstd_parse_clopt(ncloptmap_t *opts, int print_values, 
        int argc, char **argv, int ignore_unk);

#ifdef	__cplusplus
}
#endif

#endif	/* NCLOPT_H */

/* vim: set ts=4 sw=4 et smartindent: */
