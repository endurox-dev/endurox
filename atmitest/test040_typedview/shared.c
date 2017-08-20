/* 
** View unit tests, shared functions between unit testing & integration testing
**
** @file shared.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "ndebug.h"
#include <fdatatype.h>

#include "test040.h"

/**
 * Initi view 3
 * @param v
 */
expublic void init_MYVIEW3(struct MYVIEW3 *v)
{
    v->tshort1 = 1;
    v->tshort2 = 2;
    v->tshort3 = 3;
}

/**
 * Init the 
 * @param v
 */
expublic void init_MYVIEW1(struct MYVIEW1 *v)
{
    int i, j;
        
    v->tshort1=15556;	/* null=2000 */

    v->C_tshort2=2;
    v->tshort2[0]=9999;	/* null=2001 */
    v->tshort2[1]=8888;	/* null=2001 */

    v->C_tshort3 = 2;
    v->tshort3[0]=7777;	/* null=2001 */
    v->tshort3[1]=-7777;	/* null=2001 */
    v->tshort3[2]=-7777;	/* null=2001 */

    v->tshort4=-10;	/* null=NONE */

    v->tlong1=33333333;	/* null=0 */

    v->tint2[0]=54545;	/* null=0 */
    v->tint2[1]=23232;	/* null=0 */
    v->tint3=-100;
    v->tint4[0]=1010101;	/* null=-1 */
    v->tint4[1]=989898;	/* null=-1 */

    v->tchar1='A';	/* null="\n" */

    v->C_tchar2=5;
    v->tchar2[0]='A';	/* null="A" */
    v->tchar2[1]='B';
    v->tchar2[2]='C';
    v->tchar2[3]='\n';
    v->tchar2[4]='\t';

    v->C_tchar3=0;
    v->tchar3[0]='C';	/* null="A" */
    v->tchar3[1]='D';	/* null="A" */

    v->tfloat1[0]=-0.11;	/* null=1.1 */
    v->tfloat1[1]=-0.22;	/* null=1.1 */
    v->tfloat1[2]=0.33;	/* null=1.1 */
    v->tfloat1[3]=0.44;	/* null=1.1 */

    v->tfloat2[0]=100000.1;	/* null=1.1 */
    v->tfloat2[1]=200000.2;	/* null=1.1 */

    v->tfloat3=333333.111;	/* null=9999.99 */

    v->tdouble1[0]=99999.111111;	/* null=55555.99 */
    v->tdouble1[1]=11111.999999;	/* null=55555.99 */
    v->tdouble2=-999.123;	/* null=-999.123 */

    NDRX_STRCPY_SAFE(v->tstring0[0], "HELLO Enduro/X");	/* null="\n\t\f\\\'\"\vHELLOWORLD" */
    NDRX_STRCPY_SAFE(v->tstring0[1], "");	/* null="\n\t\f\\\'\"\vHELLOWORLD" */
    NDRX_STRCPY_SAFE(v->tstring0[2], "\nABC\n");	/* null="\n\t\f\\\'\"\vHELLOWORLD" */

    NDRX_STRCPY_SAFE(v->tstring1[0], "Pack my box");	/* null="HELLO WORLDB" */
    NDRX_STRCPY_SAFE(v->tstring1[1], "BOX MY PACK");	/* null="HELLO WORLDB" */
    NDRX_STRCPY_SAFE(v->tstring1[2], "\nEnduro/X\n");	/* null="HELLO WORLDB" */

    /* Test the L length indicator, must be set to number of bytes transfered */
    v->C_tstring2=2;

    NDRX_STRCPY_SAFE(v->tstring2[0], "CCCCAAAADDDD");
    NDRX_STRCPY_SAFE(v->tstring2[1], "EEEFFFGGG");
    NDRX_STRCPY_SAFE(v->tstring2[2], "IIIIJJJJKKK");

    v->C_tstring3=4;

    NDRX_STRCPY_SAFE(v->tstring3[0], "LLLLLL");	/* null="TESTEST" */
    NDRX_STRCPY_SAFE(v->tstring3[1], "MMMMMM");	/* null="TESTEST" */
    NDRX_STRCPY_SAFE(v->tstring3[2], "");	/* null="TESTEST" */
    NDRX_STRCPY_SAFE(v->tstring3[3], "NNNNNN");	/* null="TESTEST" */

    NDRX_STRCPY_SAFE(v->tstring4, "Some string value");	/* null="HELLO TEST" */

    for (i=0; i<30; i++)
    {
        v->tcarray1[i]=i;
    }

    v->L_tcarray2 = 5;
    for (i=0; i<25; i++)
    {
        v->tcarray2[i]=i+1;
    }
    
    v->C_tcarray3 = 9;
            
    for (j=0; j<9; j++)
    {
        v->L_tcarray3[j]=j+1;
        
        for (i=0; i<16+j; i++)
        {
            v->tcarray3[j][i]=i+2;
        }
    }
    
    for (i=0; i<5; i++)
    {
        v->tcarray4[i]=i+3;	/* null="ABC" */
    }
    
    for (i=0; i<5; i++)
    {
        v->tcarray5[i]=i+4;	/* null="ABC" */
    }
    
}
