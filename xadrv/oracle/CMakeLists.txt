## 
## Build the Oracle XA/enduro driver libs
##
## @file CMakeLists.txt
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
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
## A commercial use license is available from ATR Baltic, SIA
## contact@atrbaltic.com
## -----------------------------------------------------------------------------
##

# Set minimum version
cmake_minimum_required(VERSION 2.8)

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

# Make sure the compiler can find include files from our UBF library.
include_directories (. ${ENDUROX_SOURCE_DIR}/include ${ENDUROX_SOURCE_DIR}/libnstd)

add_library (ndrxxaoras SHARED
                        oracle_s.c
                        oracle_common.c
   	        )

add_library (ndrxxaorad SHARED
                        oracle_d.c
                        oracle_common.c
   	        )

if(${CMAKE_OS_NAME} STREQUAL "CYGWIN")
    target_link_libraries(ndrxxaoras atmi ubf nstd)
    target_link_libraries(ndrxxaorad atmi ubf nstd)
elseif(${CMAKE_OS_NAME} STREQUAL "DARWIN")
    target_link_libraries(ndrxxaoras atmi ubf nstd)
    target_link_libraries(ndrxxaorad atmi ubf nstd)
endif()


# Install bin
install (TARGETS 
    ndrxxaoras
    ndrxxaorad
    DESTINATION "${INSTALL_LIB_DIR}"
    )
