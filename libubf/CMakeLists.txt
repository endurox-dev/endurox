##
## @brief Enduro Execution platform's UBF library
##   We are using flex & bison for boolean expressions!
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

# Set minimum version
cmake_minimum_required(VERSION 2.6)

find_package(BISON)
find_package(FLEX)

BISON_TARGET(EXPRPARSER expr.y ${CMAKE_CURRENT_BINARY_DIR}/expr.tab.c)
FLEX_TARGET(EXPRSCANNER expr.l ${CMAKE_CURRENT_BINARY_DIR}/expr.lex.c)

#set(CMAKE_C_FLAGS_DEBUG "-DUBF_DEBUG" ${CMAKE_C_FLAGS_DEBUG})

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D UBF_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

# Make sure the compiler can find include files from our UBF library.
include_directories (. ${ENDUROX_SOURCE_DIR}/include ${ENDUROX_SOURCE_DIR}/libnstd)

# Create a library called "UBF" which includes the source files.
# The extension is already found. Any number of sources could be listed here. 
add_library (ubf SHARED
		ubf_impl.c
		ubf.c
		fproj_impl.c
		find_impl.c
		get_impl.c
		bprint_impl.c
		fmerge.c
		ferror.c 
		fdatatype.c
		fieldtable.c
		cf.c 
		expr_funcs.c
		utils.c
                b_readwrite.c
                ubf_tls.c
                view_null.c
                view_parser.c
                view_plot.c
                view_struct.c
                view_ubf.c
		view_access.c
		${BISON_EXPRPARSER_OUTPUTS} 
		${FLEX_EXPRSCANNER_OUTPUTS}
                bcmp.c
                fielddb.c
	)

if(${CMAKE_OS_NAME} STREQUAL "CYGWIN")
    target_link_libraries(ubf nstd)
elseif(${CMAKE_OS_NAME} STREQUAL "DARWIN")
    target_link_libraries(ubf nstd)
elseif(${CMAKE_OS_NAME} STREQUAL "AIX")
    # for XL tls
    target_link_libraries(ubf pthread)
endif()

install (TARGETS 
    ubf 
    DESTINATION "${INSTALL_LIB_DIR}"
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    COMPONENT libraries
    )
# vim: set ts=4 sw=4 et smartindent:
