## 
## Enduro Execution platform's ATMI library/ies tests
## Using Cgreen framework for tests.
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
cmake_minimum_required(VERSION 2.8)

# Make sure the compiler can find include files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/libubf
					 ${ENDUROX_SOURCE_DIR}/include
					 ${ENDUROX_SOURCE_DIR}/libnstd
					 ${ENDUROX_SOURCE_DIR}/ubftest)

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 

# Sub Tests
add_subdirectory (test000_system)
add_subdirectory (test001_basiccall)
add_subdirectory (test002_basicforward)
add_subdirectory (test003_basicconvers)
add_subdirectory (test004_basicevent)
add_subdirectory (test005_convconfload)
add_subdirectory (test006_ulog)
add_subdirectory (test007_advertise)
add_subdirectory (test008_extensions)
add_subdirectory (test009_srvdie)
add_subdirectory (test010_strtracecond)
add_subdirectory (test011_tout)
add_subdirectory (test012_admqfull)
add_subdirectory (test013_procnorsp)
add_subdirectory (test015_threads)
add_subdirectory (test017_srvthread)
add_subdirectory (test018_tpacalltout)
add_subdirectory (test021_xafull)
add_subdirectory (test022_typedstring)
add_subdirectory (test023_typedcarray)
add_subdirectory (test024_json)
add_subdirectory (test026_typedjson)

IF (NOT DEFINE_DISABLEPSCRIPT)
	add_subdirectory (test027_pscript)
ENDIF()

add_subdirectory (test028_tmq)

################################################################################
# Master test case driver
add_executable (atmiunit1 atmiunit1.c)
################################################################################
# Master test case driver
target_link_libraries (atmiunit1 ubf cgreen m nstd ${RT_LIB} pthread)
set_target_properties(atmiunit1 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

