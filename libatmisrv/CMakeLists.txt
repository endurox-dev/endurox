##
## @brief Enduro Execution platform's ATMI server library
##
## @file CMakeLists.txt
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
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
cmake_minimum_required(VERSION 3.1)

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF (ENV{RELEASE_BUILD})
	# do nothing
ELSE (ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF (ENV{RELEASE_BUILD})

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
                nomain.c
   	    )
# Integration library
add_library (objatmisrvinteg OBJECT
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

# shared libraries need PIC
set_property(TARGET objatmisrvinteg PROPERTY POSITION_INDEPENDENT_CODE 1)

# shared and static libraries built from the same object files
add_library(atmisrvinteg SHARED $<TARGET_OBJECTS:objatmisrvinteg>)

if(CMAKE_OS_NAME STREQUAL "CYGWIN")
    target_link_libraries(atmisrvdum nstd) # just for cywing build, anyway we need to use integration mode...
    target_link_libraries(atmisrv atmisrvdum atmi ubf nstd)
    target_link_libraries(atmisrvnomain atmisrvdum atmi ubf nstd)
    target_link_libraries(atmisrvinteg atmi ubf nstd)
elseif(CMAKE_OS_NAME STREQUAL "DARWIN")
    target_link_libraries(atmisrvdum nstd) # just for cywing build, anyway we need to use integration mode...
    target_link_libraries(atmisrv atmisrvdum atmi ubf nstd)
    target_link_libraries(atmisrvnomain atmisrvdum atmi ubf nstd)
    target_link_libraries(atmisrvinteg atmi ubf nstd)
elseif(CMAKE_OS_NAME STREQUAL "AIX")
    target_link_libraries(atmisrv atmi ubf nstd pthread)
    target_link_libraries(atmisrvnomain atmi ubf nstd pthread)
    target_link_libraries(atmisrvinteg atmi ubf nstd pthread)
    target_link_libraries(atmisrvdum atmi ubf nstd pthread)
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

# there is special mode for aix
# seems that go linker does not recongnize .so files...

if(CMAKE_OS_NAME STREQUAL "AIX")
SET(ATMISRV_INTEG_LIBS "-lndrxaix -lrt -ldl")
else()

SET(ATMISRV_INTEG_LIBS "-latmisrvinteg -latmi -lubf -lnstd -lpthread -lm")
foreach(LIB ${RT_LIB})
    list(APPEND ATMISRV_INTEG_LIBS "-l${LIB}")
endforeach()
string(REPLACE ";" " " ATMISRV_INTEG_LIBS "${ATMISRV_INTEG_LIBS}")

endif()


# Produce a pkg-config file
configure_file("atmisrvinteg.pc.in" "atmisrvinteg.pc" @ONLY)

install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/atmisrvinteg.pc"
        DESTINATION "${INSTALL_LIB_DIR}/pkgconfig/")
#################################################################################

# vim: set ts=4 sw=4 et smartindent:
