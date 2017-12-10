
/* Result Sets Interface */
#ifndef SQL_CRSR
#  define SQL_CRSR
  struct sql_cursor
  {
    unsigned int curocn;
    void *ptr1;
    void *ptr2;
    unsigned int magic;
  };
  typedef struct sql_cursor sql_cursor;
  typedef struct sql_cursor SQL_CURSOR;
#endif /* SQL_CRSR */

/* Thread Safety */
typedef void * sql_context;
typedef void * SQL_CONTEXT;

/* Object support */
struct sqltvn
{
  unsigned char *tvnvsn; 
  unsigned short tvnvsnl; 
  unsigned char *tvnnm;
  unsigned short tvnnml; 
  unsigned char *tvnsnm;
  unsigned short tvnsnml;
};
typedef struct sqltvn sqltvn;

struct sqladts
{
  unsigned int adtvsn; 
  unsigned short adtmode; 
  unsigned short adtnum;  
  sqltvn adttvn[1];       
};
typedef struct sqladts sqladts;

static struct sqladts sqladt = {
  1,1,0,
};

/* Binding to PL/SQL Records */
struct sqltdss
{
  unsigned int tdsvsn; 
  unsigned short tdsnum; 
  unsigned char *tdsval[1]; 
};
typedef struct sqltdss sqltdss;
static struct sqltdss sqltds =
{
  1,
  0,
};

/* File name & Package Name */
struct sqlcxp
{
  unsigned short fillen;
           char  filnam[69];
};
static struct sqlcxp sqlfpn =
{
    68,
    "/home/mvitolin/projects/endurox/atmitest/test047_oradb/atmisv47.proc"
};


static unsigned int sqlctx = 901091479;


static struct sqlexd {
   unsigned long  sqlvsn;
   unsigned int   arrsiz;
   unsigned int   iters;
   unsigned int   offset;
   unsigned short selerr;
   unsigned short sqlety;
   unsigned int   occurs;
            short *cud;
   unsigned char  *sqlest;
            char  *stmt;
   sqladts *sqladtp;
   sqltdss *sqltdsp;
   unsigned char  **sqphsv;
   unsigned long  *sqphsl;
            int   *sqphss;
            short **sqpind;
            int   *sqpins;
   unsigned long  *sqparm;
   unsigned long  **sqparc;
   unsigned short  *sqpadto;
   unsigned short  *sqptdso;
   unsigned int   sqlcmax;
   unsigned int   sqlcmin;
   unsigned int   sqlcincr;
   unsigned int   sqlctimeout;
   unsigned int   sqlcnowait;
            int   sqfoff;
   unsigned int   sqcmod;
   unsigned int   sqfmod;
   unsigned char  *sqhstv[2];
   unsigned long  sqhstl[2];
            int   sqhsts[2];
            short *sqindv[2];
            int   sqinds[2];
   unsigned long  sqharm[2];
   unsigned long  *sqharc[2];
   unsigned short  sqadto[2];
   unsigned short  sqtdso[2];
} sqlstm = {12,2};

/* SQLLIB Prototypes */
extern sqlcxt ( void **, unsigned int *,
                   struct sqlexd *, struct sqlcxp * );
extern sqlcx2t( void **, unsigned int *,
                   struct sqlexd *, struct sqlcxp * );
extern sqlbuft( void **, char * );
extern sqlgs2t( void **, char * );
extern sqlorat( void **, unsigned int *, void * );

/* Forms Interface */
static int IAPSUCC = 0;
static int IAPFAIL = 1403;
static int IAPFTL  = 535;
extern void sqliem( unsigned char *, signed int * );

typedef struct { unsigned short len; unsigned char arr[1]; } VARCHAR;
typedef struct { unsigned short len; unsigned char arr[1]; } varchar;

/* CUD (Compilation Unit Data) Array */
static short sqlcud0[] =
{12,4128,1,27,0,
5,0,0,1,55,0,4,87,0,0,2,1,0,1,0,2,3,0,0,1,97,0,0,
28,0,0,2,54,0,3,166,0,0,2,2,0,1,0,1,97,0,0,1,3,0,0,
51,0,0,3,22,0,2,234,0,0,0,0,0,1,0,
};


/* 
** Oracle PRO*C Sample database server
**
** @file atmisv47.proc
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

#ifndef ORA_PROC
#include <ndebug.h>
#endif

#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include "test47.h"

#include <sqlca.h> 


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Read "account" balance from DB
 * This will work with out transactions (as read only)
 */
void BALANCE (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    /* EXEC SQL BEGIN DECLARE SECTION; */ 

    long h_balance;
    char h_accnum[1024];
    /* EXEC SQL END DECLARE SECTION; */ 

    
    NDRX_LOG(log_debug, "%s got call", __func__);

    /* Just print the buffer */
    Bprint(p_ub);
    
    if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, h_accnum, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    NDRX_LOG(log_info, "Search for account: %s", h_accnum);
    
    /* EXEC SQL SELECT balance INTO :h_balance
             FROM accounts
             WHERE accnum = :h_accnum; */ 

{
    struct sqlexd sqlstm;
    sqlstm.sqlvsn = 12;
    sqlstm.arrsiz = 2;
    sqlstm.sqladtp = &sqladt;
    sqlstm.sqltdsp = &sqltds;
    sqlstm.stmt = "select balance into :b0  from accounts where accnum=:b1";
    sqlstm.iters = (unsigned int  )1;
    sqlstm.offset = (unsigned int  )5;
    sqlstm.selerr = (unsigned short)1;
    sqlstm.cud = sqlcud0;
    sqlstm.sqlest = (unsigned char  *)&sqlca;
    sqlstm.sqlety = (unsigned short)4352;
    sqlstm.occurs = (unsigned int  )0;
    sqlstm.sqhstv[0] = (unsigned char  *)&h_balance;
    sqlstm.sqhstl[0] = (unsigned long )sizeof(long);
    sqlstm.sqhsts[0] = (         int  )0;
    sqlstm.sqindv[0] = (         short *)0;
    sqlstm.sqinds[0] = (         int  )0;
    sqlstm.sqharm[0] = (unsigned long )0;
    sqlstm.sqadto[0] = (unsigned short )0;
    sqlstm.sqtdso[0] = (unsigned short )0;
    sqlstm.sqhstv[1] = (unsigned char  *)h_accnum;
    sqlstm.sqhstl[1] = (unsigned long )1024;
    sqlstm.sqhsts[1] = (         int  )0;
    sqlstm.sqindv[1] = (         short *)0;
    sqlstm.sqinds[1] = (         int  )0;
    sqlstm.sqharm[1] = (unsigned long )0;
    sqlstm.sqadto[1] = (unsigned short )0;
    sqlstm.sqtdso[1] = (unsigned short )0;
    sqlstm.sqphsv = sqlstm.sqhstv;
    sqlstm.sqphsl = sqlstm.sqhstl;
    sqlstm.sqphss = sqlstm.sqhsts;
    sqlstm.sqpind = sqlstm.sqindv;
    sqlstm.sqpins = sqlstm.sqinds;
    sqlstm.sqparm = sqlstm.sqharm;
    sqlstm.sqparc = sqlstm.sqharc;
    sqlstm.sqpadto = sqlstm.sqadto;
    sqlstm.sqptdso = sqlstm.sqtdso;
    sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
}


    
    NDRX_LOG(log_info, "Result: %d", sqlca.sqlcode);
    
    if (sqlca.sqlcode != 0)
    {
        NDRX_LOG(log_error, "Failed to select: %d: %.70s", 
                sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==Bchg(p_ub, T_LONG_FLD, 0, (char *)&h_balance, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to set h_balance: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * This service will make an account
 */
void ACCOPEN (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    int tran_started=EXFALSE;
    /* EXEC SQL BEGIN DECLARE SECTION; */ 

    long h_balance;
    char h_accnum[1024];
    /* EXEC SQL END DECLARE SECTION; */ 

    
    NDRX_LOG(log_debug, "%s got call", __func__);

    if (!tpgetlev())
    {
        /* start a transaction */
        if (EXSUCCEED!=tpbegin(60, 0))
        {
            NDRX_LOG(log_error, "Failed to start transaction: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        tran_started=EXTRUE;
    }
    
    /* Just print the buffer */
    Bprint(p_ub);
    
    if (EXFAIL==Bget(p_ub, T_STRING_FLD, 0, h_accnum, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_FLD: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL==Bget(p_ub, T_LONG_FLD, 0, (char *)&h_balance, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get h_balance: %s", 
                 Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    NDRX_LOG(log_info, "Opening account: [%s] balance: [%ld]", 
            h_accnum, h_balance);
    
    
    /* EXEC SQL INSERT INTO accounts(accnum, balance)
        VALUES(:h_accnum, :h_balance); */ 

{
    struct sqlexd sqlstm;
    sqlstm.sqlvsn = 12;
    sqlstm.arrsiz = 2;
    sqlstm.sqladtp = &sqladt;
    sqlstm.sqltdsp = &sqltds;
    sqlstm.stmt = "insert into accounts (accnum,balance) values (:b0,:b1)";
    sqlstm.iters = (unsigned int  )1;
    sqlstm.offset = (unsigned int  )28;
    sqlstm.cud = sqlcud0;
    sqlstm.sqlest = (unsigned char  *)&sqlca;
    sqlstm.sqlety = (unsigned short)4352;
    sqlstm.occurs = (unsigned int  )0;
    sqlstm.sqhstv[0] = (unsigned char  *)h_accnum;
    sqlstm.sqhstl[0] = (unsigned long )1024;
    sqlstm.sqhsts[0] = (         int  )0;
    sqlstm.sqindv[0] = (         short *)0;
    sqlstm.sqinds[0] = (         int  )0;
    sqlstm.sqharm[0] = (unsigned long )0;
    sqlstm.sqadto[0] = (unsigned short )0;
    sqlstm.sqtdso[0] = (unsigned short )0;
    sqlstm.sqhstv[1] = (unsigned char  *)&h_balance;
    sqlstm.sqhstl[1] = (unsigned long )sizeof(long);
    sqlstm.sqhsts[1] = (         int  )0;
    sqlstm.sqindv[1] = (         short *)0;
    sqlstm.sqinds[1] = (         int  )0;
    sqlstm.sqharm[1] = (unsigned long )0;
    sqlstm.sqadto[1] = (unsigned short )0;
    sqlstm.sqtdso[1] = (unsigned short )0;
    sqlstm.sqphsv = sqlstm.sqhstv;
    sqlstm.sqphsl = sqlstm.sqhstl;
    sqlstm.sqphss = sqlstm.sqhsts;
    sqlstm.sqpind = sqlstm.sqindv;
    sqlstm.sqpins = sqlstm.sqinds;
    sqlstm.sqparm = sqlstm.sqharm;
    sqlstm.sqparc = sqlstm.sqharc;
    sqlstm.sqpadto = sqlstm.sqadto;
    sqlstm.sqptdso = sqlstm.sqtdso;
    sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
}


    
    NDRX_LOG(log_info, "Result: %d", sqlca.sqlcode);
    
    if (sqlca.sqlcode != 0)
    {
        NDRX_LOG(log_error, "Failed to select: %d: %.70s", 
                sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
        EXFAIL_OUT(ret);
    }
    
out:
        
    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=tpcommit(0))
            {
                NDRX_LOG(log_error, "Failed to commit: %s", tpstrerror(tperrno))
                ret=EXFAIL;
            }
        }
        
        if (EXFAIL==ret)
        {
            if (EXSUCCEED!=tpabort(0))
            {
                NDRX_LOG(log_error, "Failed to abort: %s", tpstrerror(tperrno))
            }
        }
    }

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}


/**
 * This service will make an account
 */
void ACCCLEAN (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    int tran_started=EXFALSE;
    /* EXEC SQL BEGIN DECLARE SECTION; */ 

    long h_balance;
    char h_accnum[1024];
    /* EXEC SQL END DECLARE SECTION; */ 

    
    NDRX_LOG(log_debug, "%s got call", __func__);

    if (!tpgetlev())
    {
        /* start a transaction */
        if (EXSUCCEED!=tpbegin(60, 0))
        {
            NDRX_LOG(log_error, "Failed to start transaction: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        tran_started=EXTRUE;
    }
    
    /* EXEC SQL DELETE FROM ACCOUNTS; */ 

{
    struct sqlexd sqlstm;
    sqlstm.sqlvsn = 12;
    sqlstm.arrsiz = 2;
    sqlstm.sqladtp = &sqladt;
    sqlstm.sqltdsp = &sqltds;
    sqlstm.stmt = "delete  from ACCOUNTS ";
    sqlstm.iters = (unsigned int  )1;
    sqlstm.offset = (unsigned int  )51;
    sqlstm.cud = sqlcud0;
    sqlstm.sqlest = (unsigned char  *)&sqlca;
    sqlstm.sqlety = (unsigned short)4352;
    sqlstm.occurs = (unsigned int  )0;
    sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
}


    
    NDRX_LOG(log_info, "Result: %d", sqlca.sqlcode);
    
    if (sqlca.sqlcode != 0 && sqlca.sqlcode != 100)
    {
        NDRX_LOG(log_error, "Failed to delete: %d: %.70s", 
                sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
        EXFAIL_OUT(ret);
    }
    
out:
        
    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=tpcommit(0))
            {
                NDRX_LOG(log_error, "Failed to commit: %s", tpstrerror(tperrno))
                ret=EXFAIL;
            }
        }
        
        if (EXFAIL==ret)
        {
            if (EXSUCCEED!=tpabort(0))
            {
                NDRX_LOG(log_error, "Failed to abort: %s", tpstrerror(tperrno))
            }
        }
    }

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                NULL,
                0L,
                0L);
}


/**
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Failed to open DB!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("BALANCE", BALANCE))
    {
        NDRX_LOG(log_error, "Failed to initialize BALANCE!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("ACCOPEN", ACCOPEN))
    {
        NDRX_LOG(log_error, "Failed to initialize ACCOPEN!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpadvertise("ACCCLEAN", ACCCLEAN))
    {
        NDRX_LOG(log_error, "Failed to initialize ACCCLEAN!");
        EXFAIL_OUT(ret);
    }
    
out:
    return EXSUCCEED;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    tpclose();
    
}

