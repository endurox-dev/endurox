#!/bin/bash
##
## @brief Check the <services> and <routing> syntax - test launcher
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
## See LICENSE file for full text.
## -----------------------------------------------------------------------------
## AGPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU Affero General Public License, version 3 as published
## by the Free Software Foundation;
##
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
## PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
## for more details.
##
## You should have received a copy of the GNU Affero General Public License along 
## with this program; if not, write to the Free Software Foundation, Inc., 
## 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##
## -----------------------------------------------------------------------------
## A commercial use license is available from Mavimax, Ltd
## contact@mavimax.com
## -----------------------------------------------------------------------------
##

export TESTNAME="test083_ddrsyntax"

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
    # Do nothing 
    echo > /dev/null
else
    # started from parent folder
    pushd .
    echo "Doing cd"
    cd $TESTNAME
fi;

. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR
export NDRX_ULOG=$TESTDIR
export NDRX_TOUT=10
export NDRX_SILENT=Y

export NDRX_RTSVCMAX=20
export NDRX_RTCRTMAX=800

#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    popd 2>/dev/null
    exit $1
}

#
# validate invalid config
#
validate_invalid() {
    config=$1
    msg=$2

    echo "---------------------------------------------------------------------"
    echo "*** Testing: $msg ($config)"
    echo "---------------------------------------------------------------------"
    cp $TESTDIR/$1 ndrxconfig-dom1.xml

    OUT=`xadmin reload`
    RET=$?

    if [[ "X$RET" == "X0" ]]; then
        echo "Config failed [$config] must fail, but didn't"
        go_out -1
    fi

    # print the output
    echo $OUT
    
}

#
# validate syntax with combined xml
#
validate_syntax() {
    start=$1
    stop=$2
    syntax=$3
    msg=$4
    ftype=$5

    echo "---------------------------------------------------------------------"
    echo "*** Testing syntax: $msg [$syntax] type $ftype"
    echo "---------------------------------------------------------------------"

    # copy first part + second part of the files and put syntax inside
    cat $TESTDIR/$start > ndrxconfig-dom1.xml

    echo "<ranges>" >> ndrxconfig-dom1.xml
    echo $3 >> ndrxconfig-dom1.xml
    echo "</ranges>" >> ndrxconfig-dom1.xml
    echo "<fieldtype>$ftype</fieldtype>" >> ndrxconfig-dom1.xml

    cat $TESTDIR/$stop >> ndrxconfig-dom1.xml

    OUT=`xadmin reload`
    RET=$?

    if [[ "$msg" == "OK" ]]; then

        if [[ "X$RET" != "X0" ]]; then
            echo "Expected OK [$syntax] got error"
            go_out -1
        fi

    else
        if [[ "X$RET" == "X0" ]]; then
            echo "Config failed [$syntax] must fail, but didn't"
            go_out -1
        fi
    fi

    # print the output
    echo $OUT

}

rm *.log

#
# Start with OK config
#
cp $TESTDIR/ndrxconfig-ok.xml ndrxconfig-dom1.xml

set_dom1;
xadmin down -y
xadmin idle || go_out 1
xadmin ldcf

echo "Testing <services> tag..."
validate_invalid "ndrxconfig-dupsvc.xml" "Duplicate services is not allowed" 
validate_invalid "ndrxconfig-toolong.xml" "Service name too long" 
validate_invalid "ndrxconfig-norout.xml" "Routing not defined" 
validate_invalid "ndrxconfig-emptysvc.xml" "Emtpy service name" 
validate_invalid "ndrxconfig-invalfield.xml" "Invalid field" 

echo "Testing <routing> tag..."

validate_invalid "ndrxconfig-rtnoname.xml" "Missing routing name" 
validate_invalid "ndrxconfig-rtranges.xml" "Two ranges present" 
validate_invalid "ndrxconfig-rtfldmis.xml" "Routing field is missing" 
validate_invalid "ndrxconfig-rtfieldtype.xml" "Invalid field type" 

validate_invalid "ndrxconfig-rtbuftype.xml" "Inalid buffer type" 
validate_invalid "ndrxconfig-rtbuftype2.xml" "Only UBF supported" 
validate_invalid "ndrxconfig-rtdup.xml" "Duplicate routing entry" 


echo "Ranges syntax...(syntax engine)"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "MIN" "Inval expr" "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "MIN-MIN" "Inval expr"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "MIN-MIN:GRP" "Inval"  "LONG"# this shall not fail
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "MIN-MAX:GRP" "OK"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "MIN-MAX:GRP,MIN-MIN:G2" "Inval expr"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "*:*" "OK"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "5-9:CC," "Fail"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "5-9:CC,10-11:OK" "OK"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "5-9:CC,10-11:OK,MAX:*" "Fail"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "5-9:CC,10-11:OK,MAX-*:*" "Fail"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "5-9:CC,10-11:OK,*:*" "OK"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "," "Fail"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "" "Fail"  "LONG"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "A-B:C" "Fail"  "LONG" # this is long
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "A-B:C" "OK"  "STRING" # ok for string
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "1-B:C" "Fail"  "LONG" # this is long upper bad
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "1-9:'GRP'" "OK"  "LONG" # numbers
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "1.11-9.99:'GRP'" "Fail"  "LONG" # not a long
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "1.11-9.99:'GRP'" "OK"  "DOUBLE" # ok for double

validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "-1.11-9.99:'GRP'" "OK"  "DOUBLE" # ok for double
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "-1.11--9.99:'GRP'" "fail"  "DOUBLE" # double minus
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "-1.11-+9.99:'GRP'" "OK"  "DOUBLE" # minus + plus ?
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "-1.11-++9.99:'GRP'" "fail"  "DOUBLE" # invalid double..

validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "9-1:'GRP'" "fail"  "DOUBLE"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "'AA'-'ZZ':'GRP'" "OK"  "STRING"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "'ZZ' - 'AA':'GRP'" "fail"  "STRING"

validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "MIN - 'AA':'GRP'" "OK"  "STRING"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "'ZZZ' - MAX:'GRP'" "OK"  "STRING"
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "'ZZZ' - 'MAX':'GRP'" "fail"  "STRING" # max is smaller than aaa

validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "'ZZZ - 'MAX':'GRP'" "fail"  "STRING" # quotes fails
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "''ZZZ - 'MAX':'GRP'" "fail"  "STRING" #
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "'AAA\'' - 'BBB\'':'GRP'" "OK"  "STRING" #
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "AA:*" "OK"  "STRING" # goes to def group
validate_syntax "ndrxconfig-rtsyn_start.xml" "ndrxconfig-rtsyn_end.xml" "-55:G" "OK"  "LONG" # Single value


echo "Limits check"

validate_invalid "ndrxconfig-toomanysvcs.xml" "Too many entries in service shm" 
validate_invalid "ndrxconfig-toomanyroutes.xml" "Too many routing criterions" 

echo "---------------------------------------------------------------------"

# TODO: Test for number of service slots -> must fail on given count reached.
# TODO: Also test some 200 bytes criterion -> shall 


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

go_out 0

# vim: set ts=4 sw=4 et smartindent:
