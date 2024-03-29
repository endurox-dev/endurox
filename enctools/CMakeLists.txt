##
## @brief Encryption tools
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

# Set a minimum version
cmake_minimum_required(VERSION 3.5)


# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

# Make sure the compiler can find inclndrxe files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/include ${ENDUROX_SOURCE_DIR}/libnstd
					 ${ENDUROX_SOURCE_DIR}/enctools)
 
# Executables
add_executable (exencrypt exencrypt.c)
add_executable (exdecrypt exdecrypt.c)

set_target_properties(exencrypt PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(exdecrypt PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# Link the executable to the UBF library & others...

target_link_libraries (exencrypt ${PS_LIB} atmi nstd ubf m ${RT_LIB} pthread)
target_link_libraries (exdecrypt ${PS_LIB} atmi nstd ubf m ${RT_LIB} pthread)

# Install bin
install (TARGETS 
    exencrypt
    exdecrypt
    DESTINATION bin)
# vim: set ts=4 sw=4 et smartindent:
