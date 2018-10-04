##
## @brief Basic Call Testing
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

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 

############################# Test - executables ###############################
add_executable (atmi.sv21 atmisv21.c ../../libatmisrv/rawmain_integra.c)
add_executable (atmiclt21 atmiclt21.c)
add_executable (atmiclt21-105 atmiclt21-105.c)
add_executable (atmiclt21-try atmiclt21-try.c)
add_executable (atmiclt21-startfail atmiclt21-startfail.c)
add_executable (atmiclt21-tpconvert atmiclt21-tpconvert.c)

# Conversations
add_executable (convsv21 convsv21.c ../../libatmisrv/rawmain_integra.c)
add_executable (convclt21 convclt21.c)

# shell testing
add_executable (atmiclt21-cli atmiclt21-cli.c)

# Oapi tests
add_executable (atmiclt21-oapi atmiclt21-oapi.c)
################################################################################
############################# Test - executables ###############################
# Link the executable to the ATMI library & others...
target_link_libraries (atmi.sv21 atmisrvinteg atmi ubf nstd  ${RT_LIB} xadrv pthread m)
target_link_libraries (atmiclt21 atmiclt atmi ubf nstd ${RT_LIB} xadrv pthread m)
target_link_libraries (atmiclt21-105 atmiclt atmi ubf nstd ${RT_LIB} xadrv pthread m)
target_link_libraries (atmiclt21-try atmiclt atmi ubf nstd ${RT_LIB} xadrv pthread m)
target_link_libraries (atmiclt21-startfail atmiclt atmi ubf nstd ${RT_LIB} xadrv pthread m)
target_link_libraries (atmiclt21-tpconvert atmiclt atmi ubf nstd ${RT_LIB} xadrv pthread m)

# Conversations test
target_link_libraries (convsv21 atmisrvinteg atmi ubf nstd  ${RT_LIB} xadrv pthread m)
target_link_libraries (convclt21 atmiclt atmi ubf nstd ${RT_LIB} xadrv pthread m)

# shell testing
target_link_libraries (atmiclt21-cli atmiclt atmi ubf nstd ${RT_LIB} xadrv pthread m)

# oapi testing
target_link_libraries (atmiclt21-oapi atmiclt atmi ubf nstd ${RT_LIB} xadrv pthread m)

set_target_properties(atmi.sv21 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt21 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt21-105 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt21-startfail PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(atmiclt21-tpconvert PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# Conversation test
set_target_properties(convsv21 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(convclt21 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# shell testing
set_target_properties(atmiclt21-cli PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# Oapi tests
set_target_properties(atmiclt21-oapi PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")


add_library (xadrv SHARED
                xabackend.c
            )

add_library (xadrv_s SHARED
                xabackend_s.c
                xabackend_common.c
            )

add_library (xadrv_s-105 SHARED
                xabackend_s-105.c
                xabackend_common.c
            )


add_library (xadrv_s-tryok SHARED
                xabackend_s-tryok.c
                xabackend_common.c
            )

add_library (xadrv_s-tryfail SHARED
                xabackend_s-tryfail.c
                xabackend_common.c
             )
             
add_library (xadrv_s-startfail SHARED
                xabackend_s-startfail.c
                xabackend_common.c
            )

add_library (xadrv_d SHARED
                xabackend_d.c
                xabackend_common.c
            )


if(${CMAKE_OS_NAME} STREQUAL "CYGWIN")
    target_link_libraries(xadrv atmi ubf nstd)
    target_link_libraries(xadrv_s atmi ubf nstd)
    target_link_libraries(xadrv_s-105 atmi ubf nstd)
    target_link_libraries(xadrv_s-tryok atmi ubf nstd)
    target_link_libraries(xadrv_s-tryfail atmi ubf nstd)
    target_link_libraries(xadrv_s-startfail atmi ubf nstd)
    target_link_libraries(xadrv_d atmi ubf nstd)
elseif(${CMAKE_OS_NAME} STREQUAL "DARWIN")
    target_link_libraries(xadrv atmi ubf nstd)
    target_link_libraries(xadrv_s atmi ubf nstd)
    target_link_libraries(xadrv_s-105 atmi ubf nstd)
    target_link_libraries(xadrv_s-tryok atmi ubf nstd)
    target_link_libraries(xadrv_s-tryfail atmi ubf nstd)
    target_link_libraries(xadrv_s-startfail atmi ubf nstd)
    target_link_libraries(xadrv_d atmi ubf nstd)
elseif(${CMAKE_OS_NAME} STREQUAL "AIX")
    # Fixes on TLS issues: Bug #228 __tls_get_addr:..
    target_link_libraries(xadrv pthread)
    target_link_libraries(xadrv_s pthread)
    target_link_libraries(xadrv_s-105 pthread)
    target_link_libraries(xadrv_s-tryok pthread)
    target_link_libraries(xadrv_s-tryfail pthread)
    target_link_libraries(xadrv_s-startfail pthread)
    target_link_libraries(xadrv_d pthread)
endif()
################################################################################
# vim: set ts=4 sw=4 et smartindent:
