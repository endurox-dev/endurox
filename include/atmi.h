/* 
** User defines for XATMI standard.
**
** @file atmi.h
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

#ifndef ATMI_H
#define	ATMI_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <xatmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define TPABSOLUTE      0x00000040      /* absolute value on tmsetprio */
#define TPACK           0x00002000

/* Flags to tpinit() */
#define TPU_MASK        0x00000047      /* unsolicited notification mask */
#define TPU_SIG         0x00000001      /* signal based notification */
#define TPU_DIP         0x00000002      /* dip-in based notification */
#define TPU_IGN         0x00000004      /* ignore unsolicited messages */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/



#ifndef _QADDON
#define _QADDON

/* START QUEUED MESSAGES ADD-ON */

#define TMQNAMELEN	15
#define TMMSGIDLEN	32
#define TMCORRIDLEN	32

struct tpqctl_t {		/* control parameters to queue primitives */
	long flags;		/* indicates which of the values are set */
	long deq_time;		/* absolute/relative  time for dequeuing */
	long priority;		/* enqueue priority */
	long diagnostic;	/* indicates reason for failure */
	char msgid[TMMSGIDLEN];	/* id of message before which to queue */
	char corrid[TMCORRIDLEN];/* correlation id used to identify message */
	char replyqueue[TMQNAMELEN+1];	/* queue name for reply message */
	char failurequeue[TMQNAMELEN+1];/* queue name for failure message */
	CLIENTID cltid;		/* client identifier for originating client */
	long urcode;		/* application user-return code */
	long appkey;		/* application authentication client key */
	long delivery_qos;      /* delivery quality of service  */
	long reply_qos;         /* reply message quality of service  */
	long exp_time;          /* expiration time  */
};
typedef struct tpqctl_t TPQCTL;

/* structure elements that are valid - set in flags */
#ifndef TPNOFLAGS
#define TPNOFLAGS	0x00000
#endif
#define	TPQCORRID	0x00001		/* set/get correlation id */
#define	TPQFAILUREQ	0x00002		/* set/get failure queue */
#define	TPQBEFOREMSGID	0x00004		/* enqueue before message id */
#define	TPQGETBYMSGIDOLD	0x00008	/* deprecated */
#define	TPQMSGID	0x00010		/* get msgid of enq/deq message */
#define	TPQPRIORITY	0x00020		/* set/get message priority */
#define	TPQTOP		0x00040		/* enqueue at queue top */
#define	TPQWAIT		0x00080		/* wait for dequeuing */
#define	TPQREPLYQ	0x00100		/* set/get reply queue */
#define	TPQTIME_ABS	0x00200		/* set absolute time */
#define	TPQTIME_REL	0x00400		/* set absolute time */
#define	TPQGETBYCORRIDOLD	0x00800	/* deprecated */
#define	TPQPEEK		0x01000		/* peek */
#define TPQDELIVERYQOS   0x02000         /* delivery quality of service */
#define TPQREPLYQOS     0x04000         /* reply message quality of service */
#define TPQEXPTIME_ABS  0x08000         /* absolute expiration time */
#define TPQEXPTIME_REL  0x10000         /* relative expiration time */
#define TPQEXPTIME_NONE 0x20000        	/* never expire */
#define	TPQGETBYMSGID	0x40008		/* dequeue by msgid */
#define	TPQGETBYCORRID	0x80800		/* dequeue by corrid */

/* Valid flags for the quality of service fileds in the TPQCTLstructure */
#define TPQQOSDEFAULTPERSIST  0x00001   /* queue's default persistence policy */
#define TPQQOSPERSISTENT      0x00002   /* disk message */
#define TPQQOSNONPERSISTENT   0x00004   /* memory message */

#ifndef _TMDLLENTRY
#define _TMDLLENTRY
#endif
#ifndef _TM_FAR
#define _TM_FAR
#endif

#if defined(__cplusplus)
extern "C" {
#endif

extern int _TMDLLENTRY tpenqueue (char _TM_FAR *qspace, char _TM_FAR *qname, TPQCTL _TM_FAR *ctl, char _TM_FAR *data, long len, long flags);
extern int _TMDLLENTRY tpdequeue (char _TM_FAR *qspace, char _TM_FAR *qname, TPQCTL _TM_FAR *ctl, char _TM_FAR * _TM_FAR *data, long _TM_FAR *len, long flags);
#if defined(_TMPROTOTYPES) && !defined(_H_SYS_TIME) && !defined(_SYS_TIME_INCLUDED) && !defined(_TIME_H)
struct tm;
#endif

#if defined(__cplusplus)
}
#endif


/* THESE MUST MATCH THE DEFINITIONS IN qm.h */
#define QMEINVAL	-1
#define QMEBADRMID	-2
#define QMENOTOPEN	-3
#define QMETRAN		-4
#define QMEBADMSGID	-5
#define QMESYSTEM	-6
#define QMEOS		-7
#define QMEABORTED	-8
#define QMENOTA		QMEABORTED
#define QMEPROTO	-9
#define QMEBADQUEUE	-10
#define QMENOMSG	-11
#define QMEINUSE	-12
#define QMENOSPACE	-13
#define QMERELEASE      -14     
#define QMEINVHANDLE    -15   
#define QMESHARE        -16

/* END QUEUED MESSAGES ADD-ON */
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_H */

