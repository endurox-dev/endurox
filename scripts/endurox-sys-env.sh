#!/bin/bash
## 
## @brief @(#) Initialize system environment
##
## @file endurox-sys-env.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 3 of the License, or (at your option) any later
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
## A commercial use license is available from Mavimax, Ltd
## contact@mavimax.com
## -----------------------------------------------------------------------------
##

# Also remember to configure /etc/secuiryt/limits.conf by adding lines:
# *               soft    msgqueue        -1
# *               hard    msgqueue        -1

if [ ! -d /dev/mqueue ]; then
	echo "Configuring system environment"
	mkdir /dev/mqueue
	mount -t mqueue none /dev/mqueue
	echo 32000 > /proc/sys/fs/mqueue/msg_max
	echo 32000 > /proc/sys/fs/mqueue/msgsize_max
	echo 10000 > /proc/sys/fs/mqueue/queues_max
else
	echo "Env already initialized"
fi

