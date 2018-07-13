/* 
** ATMI tpimport/tpexport function implementation (Common version)
** TODO
** TODO
**
** @file tpimport.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>
//#include <sys_mqueue.h>
//#include <fcntl.h>

#include <atmi.h>
#include <userlog.h>
#include <ndebug.h>
#include <tperror.h>
#include <typed_buf.h>
#include <atmi_int.h>

//#include "../libatmisrv/srv_int.h"
//#include "ndrxd.h"
//#include "utlist.h"
//#include <thlock.h>
//#include <xa_cmn.h>
//#include <atmi_shm.h>
//#include <atmi_tls.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

expublic int ndrx_tpimportex(ndrx_expbufctl_t *bufctl,
        char *idata, long ilen, char **odata, long *olen, long flags)
{
    int ret=EXSUCCEED;
    
    NDRX_LOG(log_debug, "%s: enter", __func__);
    

out:
    NDRX_LOG(log_debug, "%s: return %d", __func__, ret);

    return ret;
}
