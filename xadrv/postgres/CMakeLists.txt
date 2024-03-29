##
## @brief PosgreSQL XA Switch Makefile
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

find_package(PostgreSQL REQUIRED)

# Make sure the compiler can find include files from our UBF library.
include_directories (. ${ENDUROX_SOURCE_DIR}/include ${ENDUROX_SOURCE_DIR}/libnstd
                        ${PostgreSQL_INCLUDE_DIRS})

link_directories(${PostgreSQL_LIBRARY_DIRS})

add_library (ndrxxapq SHARED
                    pgswitch.c
                    pgconfig.c
                    pgxid.c
                    pq.c
   	        )
target_link_libraries(ndrxxapq atmi ubf nstd ${PostgreSQL_LIBRARIES})

IF(NOT DEFINED DISABLE_ECPG)

    add_library (ndrxxaecpg SHARED
                        pgswitch.c
                        pgconfig.c
                        pgxid.c
                        ecpg.c
                    )

    target_link_libraries(ndrxxaecpg atmi ubf nstd ecpg ${PostgreSQL_LIBRARIES})

ENDIF()

add_executable (pgtest
                    codestest.c)
target_link_libraries (pgtest nstd ${RT_LIB} pthread pq)
set_target_properties(pgtest PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# Install bin
install (TARGETS 
    ndrxxapq
    DESTINATION "${INSTALL_LIB_DIR}"
    )

IF(NOT DEFINED DISABLE_ECPG)

    # Install bin
    install (TARGETS 
        ndrxxaecpg
        DESTINATION "${INSTALL_LIB_DIR}"
        )

ENDIF()

# vim: set ts=4 sw=4 et smartindent:
