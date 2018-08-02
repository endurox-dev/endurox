## 
## @brief Encryption tools
##
## @file CMakeLists.txt
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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

# Set a minimum version
cmake_minimum_required(VERSION 2.8)


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
