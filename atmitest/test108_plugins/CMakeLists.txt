##
## @brief Plugin interface tests - server
##
## @file atmisv108.c
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
add_executable (atmiclt108 atmiclt108.c)
############################# Test - executables ###############################
# Link the executable to the ATMI library & others...
target_link_libraries (atmiclt108 atmiclt atmi ubf nstd m pthread ${RT_LIB})

set_target_properties(atmiclt108 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
################################################################################

add_library (t108_lcf1 SHARED
                plugin1.c
            )
if(CMAKE_OS_NAME STREQUAL "CYGWIN")
    target_link_libraries(t108_lcf1 nstd)
elseif(CMAKE_OS_NAME STREQUAL "DARWIN")
    target_link_libraries(t108_lcf1 nstd)
elseif(CMAKE_OS_NAME STREQUAL "AIX")
    target_link_libraries(t108_lcf1 pthread)
endif()

add_library (t108_lcf2 SHARED
                plugin2.c
            )
if(CMAKE_OS_NAME STREQUAL "CYGWIN")
    target_link_libraries(t108_lcf2 nstd)
elseif(CMAKE_OS_NAME STREQUAL "DARWIN")
    target_link_libraries(t108_lcf2 nstd)
elseif(CMAKE_OS_NAME STREQUAL "AIX")
    target_link_libraries(t108_lcf2 pthread)
endif()

add_library (t108_lcf3 SHARED
                plugin3.c
            )
if(CMAKE_OS_NAME STREQUAL "CYGWIN")
    target_link_libraries(t108_lcf3 nstd)
elseif(CMAKE_OS_NAME STREQUAL "DARWIN")
    target_link_libraries(t108_lcf3 nstd)
elseif(CMAKE_OS_NAME STREQUAL "AIX")
    target_link_libraries(t108_lcf3 pthread)
endif()


# vim: set ts=4 sw=4 et smartindent:
