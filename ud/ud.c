/* 
 * @brief Send buffer from stdin to specified service,
 *   i.e. using SRVCNM.
 *
 * @file ud.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <atmi_int.h>
#include <ndrstandard.h>
#include <Exfields.h>
#include <ubf.h>
#include <ubf_int.h>
#include <fdatatype.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Main entry point for `ud' utility
 */
int main(int argc, char** argv)
{

    int ret=EXSUCCEED;
    UBFH *p_ub;
    UBFH *p_ub_ret=NULL;
    char svc[XATMI_SERVICE_NAME_LENGTH+1];
    BFLDLEN  len=sizeof(svc);
    long rsp_len;

    /* Allocate memory, currenlty max that could be supported, but exclude some header
     stuff out... */
    if (NULL==(p_ub=(UBFH *)tpalloc("UBF", NULL, MAX_CALL_DATA_SIZE-sizeof(UBF_header_t))))
    {
        fprintf(stderr, "%s\n", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }

    /* Read the buffer from external source */
    if (EXSUCCEED!=Bextread(p_ub, stdin))
    {
        fprintf(stderr, "%s\n", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    /* Get service name */
    if (EXSUCCEED!=Bget(p_ub, SRVCNM, 0, svc, &len))
    {
        fprintf(stderr, "Failed to get SRVCNM: %s\n", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    /* remove SRVCNM from buffer */
    Bdel(p_ub, SRVCNM, 0);

    /* call the service now! */
    if (EXSUCCEED!=tpcall(svc, (char *)p_ub, 0L, (char **)&p_ub_ret, &rsp_len, 0L))
    {
        ret=EXFAIL;

	/* print the result */
        fprintf(stderr, "%s\n", tpstrerror(tperrno));

    	/* print the buffer - print the buffers anyway...!
    	Bprint(p_ub);
        goto out;
	*/
    }
    fprintf(stdout, "SENT pkt(1) is :\n");
    /* print the buffer */
    Bprint(p_ub);
    fprintf(stdout, "\n");

    /* print the buffer */
    fprintf(stdout, "RTN pkt(1) is :\n");
    Bprint(p_ub_ret);
    fprintf(stdout, "\n");

out:
    /* un-initalize */
    tpterm();
    return ret;
}

