#!/bin/bash

#
# @(#) Run the benchmarks on given platform
#

if [ "$#" -ne 1 ]; then
    echo "Usage: $(basename "$0") <configuration name>"
    exit 1
fi

export NDRX_BENCH_CONFIGNAME=$1

echo "Config name: $NDRX_BENCH_CONFIGNAME"

################################################################################
# 3. Threaded tpcalls.
################################################################################
export NDRX_BENCH_FILE=`pwd`/03_tpcall_threads.txt

if [[ "$NDRX_BENCH_CONFIGNAME" != "r" ]]; then
pushd .
cd ../../atmitest/test015_threads
./run-doc-bench-03.sh
popd
fi

#
# Generate the chart
#
export NDRX_BENCH_TITLE="5x clients, 5x servers, request+reply"
export NDRX_BENCH_X_LABEL="Msg Size (KB)"
export NDRX_BENCH_Y_LABEL="Calls Per Second (tpcall()/sec)"
export NDRX_BENCH_OUTFILE="03_tpcall_threads.png"
R -f genchart.r

################################################################################
# 1. First test case, basic benchmarking (pure tpcall)
# Single threaded (one client, one server), request + reply
################################################################################
export NDRX_BENCH_FILE=`pwd`/01_tpcall.txt

if [[ "$NDRX_BENCH_CONFIGNAME" != "r" ]]; then
pushd .
cd ../../atmitest/test001_basiccall
./run-doc-bench-01.sh
popd
fi

#
# Generate the chart
#
export NDRX_BENCH_TITLE="One client, one server, tpcall, request+reply"
export NDRX_BENCH_X_LABEL="Msg Size (KB)"
export NDRX_BENCH_Y_LABEL="Calls Per Second (tpcall()/sec)"
export NDRX_BENCH_OUTFILE="01_tpcall.png"
R -f genchart.r

################################################################################
# 2. Next we do network benchmark, client one side, server another
################################################################################
export NDRX_BENCH_FILE=`pwd`/02_tpcall_dom.txt

if [[ "$NDRX_BENCH_CONFIGNAME" != "r" ]]; then
pushd .
cd ../../atmitest/test001_basiccall
./run-doc-bench-02.sh
popd
fi

#
# Generate the chart
#
export NDRX_BENCH_TITLE="Networked: One client, one server, tpcall, request+reply"
export NDRX_BENCH_X_LABEL="Msg Size (KB)"
export NDRX_BENCH_Y_LABEL="Calls Per Second (tpcall()/sec)"
export NDRX_BENCH_OUTFILE="02_tpcall_network.png"
R -f genchart.r


