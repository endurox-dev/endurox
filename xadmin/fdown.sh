#!/bin/bash
## 
## File:   fdown.sh
## Created on Aug 12, 2011, 12:09:58 AM
## This is invoked by ndrx command "fdown"
##
## @file fdown.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
## version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
## PARTICULAR PURPOSE. See the GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place, Suite 330, Boston, MA 02111-1307 USA
##
## -----------------------------------------------------------------------------
## A commercial use license is available from ATR Baltic, SIA
## contact@atrbaltic.com
## -----------------------------------------------------------------------------
##

LOG_FILE=1
NDRX_PARENT=$1
FMTSEP=","
function log()
{
	date_fmt=`date`
	if [ $LOG_FILE -eq 1 ]; then
		echo "fdown.sh:$date_fmt:$1" >> $NDRX_LOG
	else
		# no file loggin.
		echo "fdown.sh:$date_fmt:$1"
	fi
}
################################# CHECK ENV ####################################
# Check for log env
if [ "X${NDRX_LOG}" == "X" ]; then
	LOG_FILE=0
fi

# Check for presence of Key
if [ "X${NDRX_RNDK}" == "X" ]; then
	log "Env variable NDRX_RNDK is not set!"
	exit 1;
fi

# Check for q prefix
if [ "X${NDRX_QPREFIX}" == "X" ]; then
	log "Env variable NDRX_QPREFIX is not set!"
	exit 1;
fi

# Check NDRX_QPATH
if [ "X${NDRX_QPATH}" == "X" ]; then
	log "Env variable NDRX_QPATH is not set!"
	exit 1;
fi

# Check NDRX_SHMPATH
if [ "X${NDRX_SHMPATH}" == "X" ]; then
	log "Env variable NDRX_SHMPATH is not set!"
	exit 1;
fi
# Check NDRX_SHMPATH
if [ "X${NDRX_SHMPATH}" == "X" ]; then
	log "Env variable NDRX_SHMPATH is not set!"
	exit 1;
fi

# Check NDRX_DPID
if [ "X${NDRX_DPID}" == "X" ]; then
	log "Env variable NDRX_DPID is not set!"
	exit 1;
fi

if [ "X${NDRX_IPCKEY}" == "X" ]; then
	log "Env variable NDRX_IPCKEY is not set!"
	exit 1;
fi

################################# DO FORCE SHUTDOWN ############################
log "--- Doing shutdown of the app server ---"
log "Server key     : [$NDRX_RNDK]"
log "SHM & MQ prefix: [$NDRX_QPREFIX]"
log "MQ Path        : [$NDRX_QPATH]"
log "SHM Path       : [$NDRX_SHMPATH]"
log "User           : [$USER]"
log "NDRX_DPID      : [$NDRX_DPID]"
log "Parent         : [$NDRX_PARENT]"
log "IPCKEY         : [$NDRX_IPCKEY]"
################################ KILL ALL CLIENTS BY QUEUE #####################
log "Removing client processes"
# Now list client processes (by queue IDS...)
CLTS=`ls -lC1 $NDRX_QPATH$NDRX_QPREFIX${FMTSEP}clt${FMTSEP}reply${FMTSEP}* 2>/dev/null`
CMD="";
IFS=$'\n';
# Remove any client processes..
SLEEPED=0
if [ "X$CLTS" != "X" ]; then
	for sig in 15 9; do
		for c in $CLTS; do
			prog=`echo $c| cut -d \${FMTSEP} -f4`
			pid=`echo $c| cut -d \${FMTSEP} -f5`

			# Check do we have such process?
			PROC=`ps -ef | grep $USER | grep -v grep | grep $pid | grep $prog`
			if [ "X$PROC" != "X" ]; then
				# Have some sleep on second loop
				if [ $sig -eq 9 ]; then
					if [ $SLEEPED -eq 0 ]; then
						sleep 2
						SLEEPED=1
					fi
				fi
				log "About to kill: $prog/$pid with kill $sig"
				kill -$sig ${pid}
			fi
		done
	done
fi

############################ REMOVES SERVERS & NDRXD ###########################
log "Removing processes"
LIST=`ps -ef | grep $NDRX_RNDK | grep $USER | grep -v grep`
for sig in 15 9; do
	for l in $LIST; do
		#echo $l;
		kpid=`echo $l | awk '{ print $2 }'`
		log "Killing -$sig $kpid: ($l)";
		kill -$sig $kpid
	done

	if [ $sig -eq 15 ]; then
		LIST=`ps -ef | grep $NDRX_RNDK | grep $USER | grep -v grep`
		if [ "X$LIST" != "X" ]; then
			sleep 2
			# List again
			LIST=`ps -ef | grep $NDRX_RNDK | grep $USER | grep -v grep`
		fi
	fi
done;
############################ ALL OTHER NDRXES  #################################
log "Removing dead ndrx admin tools, except current admin ($NDRX_PARENT)"
LIST=`ps -ef | grep "xadmin" | grep $USER | grep -v grep | grep -v $NDRX_PARENT`
for sig in 15 9; do
	for l in $LIST; do
		#echo $l;
		kpid=`echo $l | awk '{ print $2 }'`
		echo "Killing -$sig $kpid: ($l)";
		kill -$sig ${kpid}
	done

	if [ $sig -eq 15 ]; then
		LIST=`ps -ef | grep "xadmin" | grep $USER | grep -v grep | grep -v $NDRX_PARENT`
		if [ "X$LIST" != "X" ]; then
			sleep 2
			# List again
			LIST=`ps -ef | grep "xadmin" | grep $USER | grep -v grep | grep -v $NDRX_PARENT`
		fi
	fi
done;
############################ REMOVE QUEUES #####################################
log "Removing queues by       : $NDRX_QPATH$NDRX_QPREFIX${FMTSEP}*"
err=`(rm $NDRX_QPATH$NDRX_QPREFIX${FMTSEP}* 2>&1)`
log $err
log "Removing shared memory by: $NDRX_SHMPATH$NDRX_QPREFIX${FMTSEP}*"
err=`(rm $NDRX_SHMPATH$NDRX_QPREFIX${FMTSEP}* 2>&1)`
log $err
log "Removing ndrxd pidfile"
err=`(rm $NDRX_DPID 2>&1)`

log "Removing System-V IPC resources..."
ipcrm -S 0x$NDRX_IPCKEY

log "Completed!"
