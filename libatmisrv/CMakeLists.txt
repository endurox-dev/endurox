## 
## Enduro Execution platform's ATMI server library
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

# Create a library called "SRVNDRX" which includes the source files.
# The extension is already found. Any number of sources could be listed here. 
add_library (atmisrv SHARED
                srvmain.c
                svqdispatch.c
                init.c
                serverapi.c
                tpreturn.c
                rawmain.c
                ndrxdapi.c
                dynadv.c
                pollextension.c
                standard.c
                oatmisrv.c 
            )

add_library (atmisrvnomain SHARED
                svqdispatch.c
                init.c
                serverapi.c
                tpreturn.c
                srvmain.c
                ndrxdapi.c
                dynadv.c
                pollextension.c
                standard.c
                oatmisrv.c
   	    )
# Integration library
add_library (atmisrvinteg SHARED
                svqdispatch.c
                init.c
                serverapi.c
                tpreturn.c
                srvmain.c
                ndrxdapi.c
                dynadv.c
                pollextension.c
                integra.c
                oatmisrv.c
                oatmisrv_integra.c
   	    )

add_library (atmisrvdum SHARED
		dum.c)

if(${CMAKE_OS_NAME} STREQUAL "CYGWIN")
    target_link_libraries(atmisrvdum nstd) # just for cywing build, anyway we need to use integration mode...
    target_link_libraries(atmisrv atmisrvdum atmi ubf nstd)
    target_link_libraries(atmisrvnomain atmisrvdum atmi ubf nstd)
    target_link_libraries(atmisrvinteg atmi ubf nstd)
elseif(${CMAKE_OS_NAME} STREQUAL "DARWIN")
    target_link_libraries(atmisrvdum nstd) # just for cywing build, anyway we need to use integration mode...
    target_link_libraries(atmisrv atmisrvdum atmi ubf nstd)
    target_link_libraries(atmisrvnomain atmisrvdum atmi ubf nstd)
    target_link_libraries(atmisrvinteg atmi ubf nstd)
elseif(${CMAKE_OS_NAME} STREQUAL "AIX")
    target_link_libraries(atmisrv pthread)
    target_link_libraries(atmisrvnomain pthread)
    target_link_libraries(atmisrvinteg pthread)
endif()

install (TARGETS 
    atmisrv 
    atmisrvdum
    atmisrvnomain
    atmisrvinteg
    DESTINATION "${INSTALL_LIB_DIR}"
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    COMPONENT libraries
    )

#################################################################################
# Have some package config, usable for golang for example...
#################################################################################
SET(ATMISRV_INTEG_LIBS "-latmisrvinteg -latmi -lubf -lnstd -lpthread -lm")

foreach(LIB ${RT_LIB})
    list(APPEND ATMISRV_INTEG_LIBS "-l${LIB}")
endforeach()

string(REPLACE ";" " " ATMISRV_INTEG_LIBS "${ATMISRV_INTEG_LIBS}")


# Produce a pkg-config file
configure_file("atmisrvinteg.pc.in" "atmisrvinteg.pc" @ONLY)

install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/atmisrvinteg.pc"
        DESTINATION "${INSTALL_LIB_DIR}/pkgconfig")
#################################################################################

