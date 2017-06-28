/* 
** Conversation functions
**
** @file cf.h
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

#ifndef CF_H
#define	CF_H

#ifdef	__cplusplus
extern "C" {
#endif

/* In this case data is being passed in. I.e. Data is not provided buffer
 * and target is UBF buffer. In this case *carr* operations may assumes that
 * EOS at the end is OK - buffer length is OK (so that standard str functions )
 * could be reused */
#define CNV_DIR_IN   0
/* In this case data is being passed out. Data is in FB, and target is user buffer
 * in this case for CARR operations EOS at the end could not be allowed!
 * also buffer length checks must be made!
 */
#define CNV_DIR_OUT  1
struct conv_type {
    _UBF_SHORT from_type;
    _UBF_SHORT to_type;
    char * (*conv_fn) (struct conv_type *t, int cnv_dir, char *input_buf, int in_len,
                        char *output_buf, int *out_len);
};

typedef struct conv_type conv_type_t;

/* The main conversation function. This will not work in multi threaded mode! */
extern char * ubf_convert(int cnv_dir, int from_type, char *input_buf, int in_len,
                        int to_type, char *output_buf , int *out_len);

extern char * get_cbuf(int in_from_type, int in_to_type,
                        char *in_temp_buf, char *in_data, int in_len,
                        char **out_alloc_buf, int *alloc_size, int mode,
                        int extra_len);

#ifdef	__cplusplus
}
#endif

#endif	/* CF_H */

