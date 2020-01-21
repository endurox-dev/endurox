##
## @brief Transaction Monitor server default tmsrv & library
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
cmake_minimum_required(VERSION 3.1)

# Make sure the compiler can find include files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/libubf
					 ${ENDUROX_SOURCE_DIR}/include
					 ${ENDUROX_SOURCE_DIR}/libnstd
					 ${ENDUROX_SOURCE_DIR}/ubftest
                        )


# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF (ENV{RELEASE_BUILD})
	# do nothing
ELSE (ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF (ENV{RELEASE_BUILD})

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 

############################# Libs #############################################
add_library (objtms OBJECT
                    tmsrv.c 
                    xasrvutil.c 
                    log.c 
                    local.c 
                    background.c 
                    tmapi.c
                    btid.c
                    statedrv.c)



# shared libraries need PIC
set_property(TARGET objtms PROPERTY POSITION_INDEPENDENT_CODE 1)

# shared and static libraries built from the same object files
add_library(tms SHARED $<TARGET_OBJECTS:objtms>)

if(CMAKE_OS_NAME STREQUAL "CYGWIN")
    target_link_libraries(tms atmisrvinteg atmi ubf exuuid exthpool nstd ${RT_LIB} pthread)
elseif(CMAKE_OS_NAME STREQUAL "DARWIN")
    target_link_libraries(tms atmisrvinteg atmi ubf exuuid exthpool nstd ${RT_LIB} pthread)
elseif(CMAKE_OS_NAME STREQUAL "AIX")
    # 
    # linking lib against pthread for XL C on aix, otherwise we get error like: 
    #  ld: 0711-768 WARNING: Object CMakeFiles/ubf.dir/ferror.c.o, section 1, function .__tls_get_addr:
    #        The branch at address 0x154 is not followed by a rec
    # Or
    # ld: 0711-768 WARNING: Object CMakeFiles/ubf.dir/ferror.c.o, section 1, function .__tls_get_addr:
    #   The branch at address 0x154 is not followed by a rec
    target_link_libraries(tms pthread)
endif()


############################# Executables ######################################
add_executable (tmsrv 
                tmsrvmain.c
                )
target_link_libraries (tmsrv tms atmisrvinteg atmi ubf exuuid exthpool nstd ${RT_LIB} pthread)
set_target_properties(tmsrv PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
################################################################################

#
# Run off the build server, use NULL server in default build
#
install (TARGETS
    tms 
    DESTINATION "${INSTALL_LIB_DIR}"
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    COMPONENT libraries)

# Install bin
install (TARGETS 
    tmsrv
    DESTINATION bin)

# vim: set ts=4 sw=4 et smartindent:
