##
## @brief Persistent queue limits tests
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
add_executable (atmi.sv86 atmisv86.c ../../libatmisrv/rawmain_integra.c)
add_executable (atmiclt86 atmiclt86.c abortrules.c errorq.c crashloop.c)
################################################################################
############################# Test - executables ###############################
# Link the executable to the ATMI library & others...
target_link_libraries (atmi.sv86 atmisrvinteg atmi ubf nstd m pthread ${RT_LIB})
target_link_libraries (atmiclt86 atmiclt atmi ubf nstd ${RT_LIB} m pthread)

set_target_properties(atmi.sv86 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt86 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
################################################################################

############################# LCF test points ##################################
add_library (t86_lcf SHARED
                t86_lcf.c
            )
if(CMAKE_OS_NAME STREQUAL "CYGWIN")
    target_link_libraries(t86_lcf atmi ubf nstd)
elseif(CMAKE_OS_NAME STREQUAL "DARWIN")
    target_link_libraries(t86_lcf atmi ubf nstd)
elseif(CMAKE_OS_NAME STREQUAL "AIX")
    target_link_libraries(t86_lcf pthread)
endif()

################################################################################


# vim: set ts=4 sw=4 et smartindent:
