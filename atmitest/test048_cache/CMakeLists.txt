##
## @brief TP Cache tests - makefile
##
## @file CMakeLists.txt
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
cmake_minimum_required(VERSION 2.8)

# Make sure the compiler can find include files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/libubf
					 ${ENDUROX_SOURCE_DIR}/include
					 ${ENDUROX_SOURCE_DIR}/libnstd
					 ${ENDUROX_SOURCE_DIR}/ubftest)


# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 


add_library (test48 STATIC testfuncs.c)
target_link_libraries(test48 nstd)

############################# Test - executables ###############################
add_executable (atmi.sv48 atmisv48.c ../../libatmisrv/rawmain_integra.c)
add_executable (01_atmiclt48 01_atmiclt48.c)
add_executable (testtool48 testtool48.c)
add_executable (atmiclt48_bench atmiclt48_bench.c)
################################################################################
############################# Test - executables ###############################
# Link the executable to the ATMI library & others...
target_link_libraries (atmi.sv48 test48 atmisrvinteg atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (01_atmiclt48 atmiclt test48 atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (testtool48 atmiclt test48 atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (atmiclt48_bench atmiclt test48 atmi ubf nstd m pthread ${RT_LIB})

set_target_properties(atmi.sv48 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(01_atmiclt48 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(testtool48 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt48_bench PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
################################################################################

# vim: set ts=4 sw=4 et smartindent:
