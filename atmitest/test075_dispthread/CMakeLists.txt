##
## @brief Test Enduro/X server dispatch threading - server
##
## @file CMakeLists.txt
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

cmake_minimum_required(VERSION 3.5)

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

############################# Test - executables ###############################
add_executable (atmi.sv75 atmisv75.c)
add_executable (atmi.sv75_dmn atmisv75_dmn.c)
add_executable (atmi.sv75_conv atmisv75_conv.c)
add_executable (atmiclt75 atmiclt75.c)
add_executable (atmiclt75_conv atmiclt75_conv.c)
add_executable (atmi.sv75_fail atmisv75_fail.c)
################################################################################
############################# Test - executables ###############################
# Link the executable to the ATMI library & others...
target_link_libraries (atmi.sv75 atmisrvinteg atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (atmi.sv75_dmn atmisrvinteg atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (atmi.sv75_conv atmisrvinteg atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (atmiclt75 atmiclt atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (atmiclt75_conv atmiclt atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (atmi.sv75_fail atmisrvinteg atmi ubf nstd m pthread ${RT_LIB})

set_target_properties(atmi.sv75 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmi.sv75_dmn PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmi.sv75_conv PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt75 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt75_conv PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmi.sv75_fail PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
################################################################################

# vim: set ts=4 sw=4 et smartindent:
