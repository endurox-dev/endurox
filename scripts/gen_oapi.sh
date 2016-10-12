#!/bin/bash

#
# @(#) Generate object API, runner
#


################################################################################
# Clean up (if any stuff left here...)
################################################################################
rm oatmi* 2>/dev/null
rm oubf* 2>/dev/null

################################################################################
# UBF level (but context switching is done in ATMI)
################################################################################
./build_object_api.pl -i ../include/ubf.h -oubf
mv oubf.h ../include
mv oubf.c ../libatmi

################################################################################
# ATMI level
################################################################################
./build_object_api.pl -i ../include/xatmi.h -oatmi
mv oatmi.h ../include
mv oatmi.c ../libatmi

################################################################################
# ATMI Server level
################################################################################
./build_object_api.pl -i ../include/xatmi.h -oatmisrv
mv oatmisrv.h ../include
mv oatmisrv.c ../libatmisrv

echo "Done..."
