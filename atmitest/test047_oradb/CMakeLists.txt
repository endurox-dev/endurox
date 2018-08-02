## 
## @brief Test server connection to Oracle DB from C - makefile
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
					 ${ENDUROX_SOURCE_DIR}/ubftest
                                         $ENV{ORACLE_HOME}/precomp/public)

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf $ENV{ORACLE_HOME}/lib)

############################# Test - executables ###############################

#
# NOTE: *proc* binary must be in path
# Also we use PARSE=PARTIAL because of ndebug.h requires C99 compiler and
# proc does not support it.
# See: https://stackoverflow.com/questions/18465187/why-the-proc-c-oracle-precompiler-can-not-parse-the-windows-h-header-file
#
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/atmisv47.c
    COMMAND proc iname=${CMAKE_CURRENT_SOURCE_DIR}/atmisv47.proc SQLCHECK=SYNTAX
        MODE=ANSI IRECLEN=255 ORECLEN=255 PARSE=PARTIAL INCLUDE=../../include
        ONAME=${CMAKE_CURRENT_SOURCE_DIR}/atmisv47.c)

add_executable (atmi.sv47 atmisv47.c ../../libatmisrv/rawmain_integra.c)
add_executable (atmiclt47 atmiclt47.c)
################################################################################
############################# Test - executables ###############################
# Link the executable to the ATMI library & others...
target_link_libraries (atmi.sv47 clntsh atmisrvinteg atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (atmiclt47 atmiclt atmi ubf nstd m pthread ${RT_LIB})

set_target_properties(atmi.sv47 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt47 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
################################################################################


