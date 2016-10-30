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
# Standard library O-API, Error handling
################################################################################
./build_object_api.pl -i ../include/nerror.h -onerror
mv onerror.h ../include
mv onerror.c ../libatmi

################################################################################
# Standard library O-API, Logging
################################################################################
./build_object_api.pl -i ../include/ndebug.h -ondebug
mv ondebug.h ../include
mv ondebug.c ../libatmi

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
# Normal API
./build_object_api.pl -i ../include/xatmi.h -oatmisrv
mv oatmisrv.h ../include
mv oatmisrv.c ../libatmisrv
# Integration API
./build_object_api.pl -i ../include/xatmi.h -oatmisrv_integra
mv oatmisrv_integra.h ../include
mv oatmisrv_integra.c ../libatmisrv

echo "Done..."
