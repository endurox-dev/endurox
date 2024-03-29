##
## @brief Enduro/X test library
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

# Set minimum version
cmake_minimum_required(VERSION 3.5)

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

add_custom_command(
  DEPENDS ../viewc/viewc
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  OUTPUT test_view.h test_view.V
  COMMAND ../viewc/viewc -n -L "${PROJECT_SOURCE_DIR}/libatmi:${PROJECT_SOURCE_DIR}/libatmiclt:${PROJECT_SOURCE_DIR}/libnstd:${PROJECT_SOURCE_DIR}/libubf" 
        -b "../buildtools/buildclient -a '${CMAKE_C_FLAGS} -I${PROJECT_SOURCE_DIR}/include -L${PROJECT_SOURCE_DIR}/libatmi -L${PROJECT_SOURCE_DIR}/libatmiclt -L${PROJECT_SOURCE_DIR}/libnstd -L${PROJECT_SOURCE_DIR}/libubf'" 
        ${CMAKE_CURRENT_SOURCE_DIR}/test_view.v_in
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_view.v_in)

add_custom_target(ubftestvhdrs DEPENDS test_view.h test_view.V)

# Make sure the compiler can find include files from our UBF library.
include_directories (. ${ENDUROX_SOURCE_DIR}/include ${ENDUROX_SOURCE_DIR}/libnstd)

# Create a library called "SRVNDRX" which includes the source files.
# The extension is already found. Any number of sources could be listed here. 
add_library (extest STATIC
   	        testfunc.c)

add_dependencies (extest ubftestvhdrs)

if(${CMAKE_OS_NAME} STREQUAL "CYGWIN")
    target_link_libraries(extest atmisrvinteg atmi nstd)
elseif(${CMAKE_OS_NAME} STREQUAL "DARWIN")
    target_link_libraries(extest atmisrvinteg atmi nstd)
endif()

# vim: set ts=4 sw=4 et smartindent:
