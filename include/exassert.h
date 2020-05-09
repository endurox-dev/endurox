/**
 * @brief Enduro/X testing support library
 *
 * @file exassert.h
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
#ifndef EXASSERT_H
#define	EXASSERT_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>

#include "xatmi.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
/**
 * Check the expression is true, if find out false
 * print the TP error to the log, ret=EXFAIL, goto out;
 * 
 * @param expr - test expression, expected true
 * @param msg - extra message for failure, format string
 * @param ... - params to format string
 */
#define NDRX_ASSERT_TP_OUT(expr, msg, ...) \
    do {\
        if (!(expr)) {\
            char tmp_assert_buf_[PATH_MAX];\
            snprintf(tmp_assert_buf_, sizeof(tmp_assert_buf_), msg, ##__VA_ARGS__);\
            NDRX_LOG(log_always, "TESTERROR %s: expr [%s] is false: %s", tmp_assert_buf_, #expr, tpstrerror(tperrno));\
            EXFAIL_OUT(ret);\
        }\
    } while (0)

/**
 * Check the expression is true, if find out false
 * print the UBF error to the log, ret=EXFAIL, goto out;
 * 
 * @param expr - test expression, expected true
 * @param msg - extra message for failure, format string
 * @param ... - params to format string
 */    
#define NDRX_ASSERT_UBF_OUT(expr, msg, ...) \
    do {\
        if (!(expr)) {\
            char tmp_assert_buf_[PATH_MAX];\
            snprintf(tmp_assert_buf_, sizeof(tmp_assert_buf_), msg, ##__VA_ARGS__);\
            NDRX_LOG(log_always, "TESTERROR %s: expr [%s] is false: %s", tmp_assert_buf_, #expr, Bstrerror(Berror));\
            EXFAIL_OUT(ret);\
        }\
    } while (0)
    
/**
 * Check the expression is true, if find out false
 * print the error
 * 
 * @param expr - test expression, expected true
 * @param msg - extra message for failure
 * @param ... - params to format string
 */    
#define NDRX_ASSERT_VAL_OUT(expr, msg, ...) \
    do {\
        if (!(expr)) {\
            char tmp_assert_buf_[PATH_MAX];\
            snprintf(tmp_assert_buf_, sizeof(tmp_assert_buf_), msg, ##__VA_ARGS__);\
            NDRX_LOG(log_always, "TESTERROR %s: expr [%s] is false", tmp_assert_buf_, #expr);\
            EXFAIL_OUT(ret);\
        }\
    } while (0)
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
#ifdef	__cplusplus
}
#endif

#endif	/* EXASSERT_H */

/* vim: set ts=4 sw=4 et smartindent: */
