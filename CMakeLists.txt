## 
## Enduro Execution platform main Make descriptor
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

cmake_minimum_required (VERSION 2.6) 
project (ENDUROX) 
set(VERSION "2.5.1")
set(PROJ_NAME "Enduro/X")
set(RELEASE "1")


################################################################################
# OS Confiugration
################################################################################


EXEC_PROGRAM(uname ARGS -a OUTPUT_VARIABLE _TMP_CMAKE_OS_NAME)
string(REGEX MATCH "^[a-zA-Z0-9-]*" _TMP_CMAKE_OS_NAME_EXTR ${_TMP_CMAKE_OS_NAME})
string(TOUPPER ${_TMP_CMAKE_OS_NAME_EXTR} CMAKE_OS_NAME)

message("CMake CMAKE_OS_NAME  = ${CMAKE_OS_NAME}")

if( ${CMAKE_OS_NAME} STREQUAL "LINUX" )
	ADD_DEFINITIONS(-DEX_OS_LINUX)
elseif(${CMAKE_OS_NAME} STREQUAL "AIX")
	ADD_DEFINITIONS(-DEX_OS_AIX)
elseif(${CMAKE_OS_NAME} STREQUAL "HP-UX")
	ADD_DEFINITIONS(-DEX_OS_HPUX)
elseif(${CMAKE_OS_NAME} STREQUAL "SUNOS")
	ADD_DEFINITIONS(-DEX_OS_SUNOS)
elseif(${CMAKE_OS_NAME} STREQUAL "FREEBSD")
	ADD_DEFINITIONS(-DEX_OS_FREEBSD)
# Unkonwn OS:
else()
	message( FATAL_ERROR "Unsupported OS" )
endif()


################################################################################
# "Configure"
################################################################################

# In this file we are doing all of our 'configure' checks. Things like checking
# for headers, functions, libraries, types and size of types.
INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckCXXSourceCompiles.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckSymbolExists.cmake)

# To check for an include file you do this:
CHECK_INCLUDE_FILE("stdint.h"        HAVE_STDINT_H)

# To check the size of a primitive type:
CHECK_TYPE_SIZE("int"   SIZEOF_INT)

# Output the project version
set(NDRX_VERSION_STR "${PROJ_NAME} ${VERSION}")

configure_file ("${PROJECT_SOURCE_DIR}/include/config.h.in"
                "${PROJECT_BINARY_DIR}/include/config.h" )

################################################################################
# Build configuration
################################################################################
# Enable release only if specified.
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	MESSAGE( RELEASE_BUILD )
	SET(CMAKE_BUILD_TYPE release)
        SET(CMAKE_C_FLAGS -O2)
ELSE ($ENV{RELEASE_BUILD})
	MESSAGE( DEBUG_BUILD )
	SET(CMAKE_BUILD_TYPE debug)
        SET(CMAKE_C_FLAGS -ggdb)
ENDIF ($ENV{RELEASE_BUILD})

################################################################################
# Install dir config
################################################################################
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if ("${LIB64}" STREQUAL "TRUE")
    set(LIB_SUFFIX 64)
else()
    set(LIB_SUFFIX "")
endif()

set(INSTALL_LIB_DIR     lib${LIB_SUFFIX} CACHE PATH "Installation directory for libraries")
mark_as_advanced(INSTALL_LIB_DIR) 
MESSAGE( STATUS "INSTALL_LIB_DIR: " ${INSTALL_LIB_DIR} )

################################################################################
# Option to disable gpg-me (gpg-me enabled by default)
OPTION(DEFINE_DISABLEGPGME "Use GPG-ME encryption" OFF)
 
IF(DEFINE_DISABLEGPGME)
    ADD_DEFINITIONS(-DDISABLEGPGME)
ENDIF(DEFINE_DISABLEGPGME)
###############################################################################

################################################################################
# Option to disable documentation build (dos are enabled by default)
OPTION(DEFINE_DISABLEDOC "Disable documentation generation" OFF) 
################################################################################

################################################################################
# Submakes
################################################################################
# Recurse into the project subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through 
# the project's entire directory structure. 
add_subdirectory (libnstd)
add_subdirectory (libubf)
add_subdirectory (libnetproto)
add_subdirectory (libexnet)
add_subdirectory (libthpool)
add_subdirectory (ubftest)
add_subdirectory (mkfldhdr)
# EnduroX sub-directories for ATMI
add_subdirectory (libatmi) # General Purpose ATMI library
add_subdirectory (libatmisrv) #  ATMI server library 
add_subdirectory (libatmiclt) #  ATMI client library 
add_subdirectory (libps) # Platform script
add_subdirectory (libpsstdlib) # Platform script standard library
add_subdirectory (tpevsrv)
add_subdirectory (tprecover) # Special recovery server
add_subdirectory (ud)
add_subdirectory (pscript) # Platform script interpreter
add_subdirectory (ndrxd) # Enduro X daemon
add_subdirectory (xadmin) # Enduro X command line interface
add_subdirectory (bridge) # Cluster bridge server
add_subdirectory (tmsrv) # Transaction Manager server for XA transactions
add_subdirectory (tmqueue) # Persistant Queue server
add_subdirectory (cpmsrv) # Client Process Monitor
add_subdirectory (xadrv) # XA Drivers for RMs
add_subdirectory (atmitest) #  ATMI testing

IF(DEFINE_DISABLEDOC)
        message("Documentation disabled - not building")
ELSE (DEFINE_DISABLEDOC)
        add_subdirectory (doc) #  Documentation project.
ENDIF (DEFINE_DISABLEDOC)

add_subdirectory (include) #  For install
add_subdirectory (scripts) #  For install
add_subdirectory (sampleconfig) #  For install


################################################################################
# uninstall target
################################################################################
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
################################################################################
# Packages
################################################################################

find_program(LSB_RELEASE_EXECUTABLE lsb_release)
if(LSB_RELEASE_EXECUTABLE)
        execute_process(COMMAND ${LSB_RELEASE_EXECUTABLE} -si
                OUTPUT_VARIABLE _TMP_LSB_RELEASE_OUTPUT_OS
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)

        string(TOLOWER 
                ${_TMP_LSB_RELEASE_OUTPUT_OS}
        LSB_RELEASE_OUTPUT_OS)
        string(REPLACE " " "_" LSB_RELEASE_OUTPUT_OS ${LSB_RELEASE_OUTPUT_OS})

        execute_process(COMMAND ${LSB_RELEASE_EXECUTABLE} -sr
                OUTPUT_VARIABLE _TMP_LSB_RELEASE_OUTPUT_VER
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(REPLACE "." "_" LSB_RELEASE_OUTPUT_VER ${_TMP_LSB_RELEASE_OUTPUT_VER})
        #string(REGEX MATCH "^[0-9]+" LSB_RELEASE_OUTPUT_VER ${_TMP_LSB_RELEASE_OUTPUT_VER})
else ()
        set(LSB_RELEASE_OUTPUT_OS "unknown")
        set(LSB_RELEASE_OUTPUT_VER "0")
endif()

message("CPack:Debug: LSB_RELEASE OS  = ${LSB_RELEASE_OUTPUT_OS}")
message("CPack:Debug: LSB_RELEASE VER = ${LSB_RELEASE_OUTPUT_VER}")


# Support only for Linux
MESSAGE( "CPack:Debug: CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME} )
MESSAGE( "CPack:Debug: CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR} )
set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_GENERATOR "DEB;RPM;TGZ")
set(CPACK_PACKAGE_CONTACT "madars.vitolins@gmail.com")
set(CPACK_PACKAGE_VERSION ${VERSION})
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxml2,openssl,libgpgme11")
#set(CPACK_RPM_PACKAGE_REQUIRES "libxml2,openssl,libssl,libgpgme11")
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LO)
message("CPack:Debug: PROJECT NAME = ${PROJECT_NAME_LO}")
set(CPACK_PACKAGE_FILE_NAME         ${PROJECT_NAME_LO}-${VERSION}-${RELEASE}.${LSB_RELEASE_OUTPUT_OS}${LSB_RELEASE_OUTPUT_VER}.${CMAKE_SYSTEM_PROCESSOR})
message("CPack:Debug: CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Enduro/X Middleware Platform")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION  ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Debian/postinst")

include(CPack)
################################################################################



