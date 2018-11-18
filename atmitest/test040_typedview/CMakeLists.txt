##
## @brief Tests for STRING buffer testing
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
cmake_minimum_required(VERSION 2.8)

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


add_custom_command(
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  OUTPUT t40.h t40.V t40_2.h t40_2.V
  COMMAND ../../viewc/viewc -n -L "../../libatmi:../../libatmiclt:../../libnstd:../../libubf" -b "../../scripts/buildclient -a '${CMAKE_C_FLAGS} -I../../include -L../../libatmi -L../../libatmiclt -L../../libnstd -L../../libubf'" t40.v_in t40_2.v_in
  DEPENDS t40.v_in t40.v_in)

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 

############################# Test - executables ###############################
add_executable (viewunit1 viewunit1.c shared.c vaccget.c vaccutil.c
			vaccset.c t40.h t40.V t40_2.h t40_2.V)
add_executable (atmisv40 atmisv40.c shared.c ../../libatmisrv/rawmain_integra.c t40.h t40.V t40_2.h t40_2.V)
add_executable (atmiclt40 atmiclt40.c shared.c t40.h t40.V t40_2.h t40_2.V)
################################################################################
############################# Test - executables ###############################
# Link the executable to the ATMI library & others...
target_link_libraries (viewunit1 ubf cgreen m nstd ${RT_LIB} pthread)
target_link_libraries (atmisv40  atmisrvinteg atmi ubf nstd m ${RT_LIB} pthread)
target_link_libraries (atmiclt40 atmiclt atmi ubf nstd m ${RT_LIB} pthread)

set_target_properties(viewunit1 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmisv40 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt40 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
################################################################################
# vim: set ts=4 sw=4 et smartindent:
