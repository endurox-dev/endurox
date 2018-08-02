#!/bin/bash
## 
## @brief @(#) Test021 - XA testing, common functions, include
##
## @file test-func-include.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
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

##
# Ensure that given number of keys exists in db
##
ensure_keys () {

        db=$1
        count=$2

        REC_OUT=`xadmin cs -d $db | wc | awk '{ print $1 }'`
        echo "Record count: [$REC_OUT] must be: [$count]"
        if [[ "$REC_OUT" != "$count" ]]; then
                echo "TESTERROR: Invalid record count in cache db!"
                exit 1
        fi
}

##
# Ensure that given field is cached somewhere in dump
##
ensure_field () {

        db=$1
        key=$2
        fb=$3
        value=$4
        count=$5

        xadmin cd -d $db -k $key -i | grep $fb | grep $value

        REC_OUT=`xadmin cd -d $db -k $key -i | grep $fb | grep $value | wc | awk '{ print $1 }'`

        echo "Buffer $fb/$value: [$REC_OUT] must be: [$count]"

        if [ $count -eq 0 ]; then

            if [[ $REC_OUT -ne 0 ]]; then
                    echo "TESTERROR: Field must not be present!"
                    exit 1
            fi

        else
            if [[ $REC_OUT -lt 1 ]]; then
                    echo "TESTERROR: Field must be present!"
                    exit 1
            fi
        fi
}
