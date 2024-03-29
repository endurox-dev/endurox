##
## @brief Enduro/X Execution platform's UBF library's buildserver utility
##
## @file CMakeLists.txt
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL or Mavimax's license for commercial use.
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

# Make sure the compiler can find include files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/libubf
					 ${ENDUROX_SOURCE_DIR}/include
					 ${ENDUROX_SOURCE_DIR}/libnstd)

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 

# Executables
add_executable (buildserver buildserver.c codegen_c.c compile_c.c getrm.c)
add_executable (buildclient buildclient.c codegen_c.c compile_c.c getrm.c)
add_executable (buildtms buildtms.c codegen_c.c compile_c.c getrm.c)

# Link the executable to the UBF library & others...
target_link_libraries (buildserver atmiclt atmi ubf m nstd ${RT_LIB} pthread)
target_link_libraries (buildclient atmiclt atmi ubf m nstd ${RT_LIB} pthread)
target_link_libraries (buildtms atmiclt atmi ubf m nstd ${RT_LIB} pthread)

install (TARGETS 
    buildserver 
    buildclient
    buildtms
    DESTINATION bin)
# vim: set ts=4 sw=4 et smartindent:
