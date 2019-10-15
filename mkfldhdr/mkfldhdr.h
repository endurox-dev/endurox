/**
 * @brief UBF Header generator
 *
 * @file mkfldhdr.h
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
#ifndef MKFLDHDR_H_
#define MKFLDHDR_H_

/*------------------------------Includes--------------------------------------*/
#include <fdatatype.h>
/*------------------------------Externs---------------------------------------*/
extern int G_langmode;  /* Programming Language mode                          */
extern char G_privdata[];
extern char *G_output_dir;
extern FILE *G_outf;
extern char G_active_file[];
/*------------------------------Macros----------------------------------------*/
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/*
 * Typed buffer descriptor table
 */
typedef struct renderer_descr renderer_descr_t;
struct renderer_descr
{
    int lang_mode;
    void (*get_fullname)(char *data); /* Get the full file name (with path) */
    int (*put_text_line) (char *text); /* callback for putting text line */
    int (*put_def_line) (UBF_field_def_t *def);  /* callback for writting definition */
    int (*put_got_base_line) (char *base); /* callback for base line */
    int (*file_open) (char *fname); /* event when file is open for write      */
    int (*file_close) (char *fname); /* event when file is closed (at finish) */
};
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

/* C lang: */
extern void c_get_fullname(char *data);
extern int c_put_text_line (char *text);
extern int c_put_got_base_line(char *base);
extern int c_put_def_line (UBF_field_def_t *def);
extern int c_file_open (char *fname);
extern int c_file_close (char *fname);

/* GO lang: */
extern void go_get_fullname(char *data);
extern int go_put_text_line (char *text);
extern int go_put_got_base_line(char *base);
extern int go_put_def_line (UBF_field_def_t *def);
extern int go_file_open (char *fname);
extern int go_file_close (char *fname);

/* Java lang: */
extern void java_get_fullname(char *data);
extern int java_put_text_line (char *text);
extern int java_put_got_base_line(char *base);
extern int java_put_def_line (UBF_field_def_t *def);
extern int java_file_open (char *fname);
extern int java_file_close (char *fname);

#endif /* MKFLDHDR_H_ */
/* vim: set ts=4 sw=4 et smartindent: */
