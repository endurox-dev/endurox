##
## @brief Enduro Execution platform main Make descriptor
##
## @file CMakeLists.txt
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

cmake_minimum_required (VERSION 3.1) 
project (ENDUROX)

include(cmake/ex_osver.cmake)
include(cmake/ex_comp.cmake)

set(VERSION "8.0.8")
set(PROJ_NAME "Enduro/X")
set(RELEASE "1")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

string(REPLACE "." ";" VERSION_LIST ${VERSION})
list(GET VERSION_LIST 0 NDRX_VERSION_MAJOR)
list(GET VERSION_LIST 1 NDRX_VERSION_MINOR)
list(GET VERSION_LIST 2 NDRX_VERSION_PATCH)
MATH (EXPR NDRX_VERSION_NUM "${NDRX_VERSION_MAJOR} * 10000 + ${NDRX_VERSION_MINOR} * 100 + ${NDRX_VERSION_PATCH}")

################################################################################
# Check versions if defined, export build env
################################################################################

if(DEFINED ENV{NDRX_BLD_VERSION})
    IF(NOT ENV{NDRX_BLD_VERSION} MATCHES VERSION)
        message( FATAL_ERROR "Invalid versions: ENV: [$ENV{NDRX_BLD_VERSION}] Code: [${VERSION}]" )
    endif()
endif()

if(DEFINED ENV{NDRX_BLD_RELEASE})
    set(RELEASE $ENV{NDRX_BLD_RELEASE})
endif()

message("CMake RELEASE  = ${RELEASE}")

SET (NDRX_BLD_PRODUCT 	$ENV{NDRX_BLD_PRODUCT})
SET (NDRX_BLD_SYSTEM 	$ENV{NDRX_BLD_SYSTEM})
SET (NDRX_BLD_CONFIG 	$ENV{NDRX_BLD_CONFIG})
SET (NDRX_BLD_VERSION 	$ENV{NDRX_BLD_VERSION})
SET (NDRX_BLD_RELEASE 	$ENV{NDRX_BLD_RELEASE})
SET (NDRX_BLD_TAG 	$ENV{NDRX_BLD_TAG})
SET (NDRX_BLD_BRANCH 	$ENV{NDRX_BLD_BRANCH})
SET (NDRX_BLD_COMMIT 	$ENV{NDRX_BLD_COMMIT})
SET (NDRX_BLD_FLAGS 	$ENV{NDRX_BLD_FLAGS})


#
# Get the git version (the version which is
#
execute_process(
    COMMAND
        git rev-parse HEAD
    RESULT_VARIABLE
        gitres
    OUTPUT_VARIABLE
        NDRX_BLD_HASH
    )

string(REGEX REPLACE "\n$" "" NDRX_BLD_HASH "${NDRX_BLD_HASH}")


if (NOT gitres EQUAL 0)
    SET (NDRX_BLD_HASH 	"unknown")
endif ()

################################################################################
# OS Configuration
################################################################################

# configure compiler
ex_comp_settings()

# Enable oracle test to run
IF (ENABLE_TEST47)
    set(NDRX_ENABLE_TEST47 "1")
ENDIF()

IF(EX_USE_SYSVQ)
    set(EX_POLLER_STR   "SystemV")
ELSEIF(EX_USE_FDPOLL)
    set(EX_POLLER_STR   "fdpoll")
ELSEIF(EX_USE_EMQ)
    set(EX_POLLER_STR   "emq")
ELSEIF(EX_USE_POLL)
    set(EX_POLLER_STR   "poll")
ELSEIF(EX_USE_EPOLL)
    set(EX_POLLER_STR   "epoll")
ELSEIF(EX_USE_KQUEUE)
    set(EX_POLLER_STR   "kqueue")
ELSEIF(EX_USE_SVAPOLL)
    set(EX_POLLER_STR   "svapoll")
ELSE()
    message( FATAL_ERROR "ERROR! Invalid queue transport backend!" )
ENDIF()

message("CMake EX_ALIGNMENT_BYTES  = ${EX_ALIGNMENT_BYTES}")
message("Poller  = ${EX_POLLER_STR}")
message("ALIGNMENT_FORCE  = ${EX_ALIGNMENT_FORCE}")

################################################################################
# "Configure"
################################################################################

# In this file we are doing all of our 'configure' checks. Things like checking
# for headers, functions, libraries, types and size of types.

# Includes from osver package
ex_osver_include()

INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckCSourceCompiles.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckCXXSourceCompiles.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckSymbolExists.cmake)


CHECK_C_SOURCE_COMPILES("int main() {__builtin_expect(1, 1);}" HAVE_EXPECT)
CHECK_CXX_SOURCE_COMPILES("int main() {__builtin_expect(1, 1);}" HAVE_EXPECT_CXX)
CHECK_C_SOURCE_COMPILES("int main() { int X = 0; __sync_bool_compare_and_swap(&X, 0, 1); __sync_synchronize();}" HAVE_SYNC)
CHECK_C_SOURCE_COMPILES("#include <stdio.h> \n int main() {char buf[128]; size_t len; getline(buf, &len, stdin);}" HAVE_GETLINE)
CHECK_C_SOURCE_COMPILES("#include <sys/sem.h> \n int main(int argc, char **argv) {union semun u; return 0;}" EX_HAVE_SEMUN)

if(CMAKE_OS_NAME STREQUAL "AIX")
    # it compiles OK, but we have no header where it is defined
    # thus we get lots of warnings
    message("No strlcpy/strcat_s/asprintf/__progname test for AIX")
    set(HAVE_STRLCPY "0")
    set(EX_HAVE_STRCAT_S "0")
else()
    CHECK_C_SOURCE_COMPILES("#include <string.h> \n int main() {char dest[1]; char src[1]; strlcpy(dest, src, 1);}" HAVE_STRLCPY)
    CHECK_C_SOURCE_COMPILES("#include <string.h> \n int main() {char tmp[8]={0x0}; strcat_s(tmp, sizeof(tmp), \"abc\");}" EX_HAVE_STRCAT_S)
    CHECK_C_SOURCE_COMPILES("#include <string.h> \n int main() {char *tmp; asprintf(&tmp, \"HELLO\");}" EX_HAVE_ASPRINTF)
    CHECK_C_SOURCE_COMPILES("extern const char * __progname; int main() {return __progname[0];}" HAVE_PROGNAME)
endif()

CHECK_C_SOURCE_COMPILES("#include <string.h> \n int main() {strnlen(\"ABC\", 1);}" HAVE_STRNLEN)
CHECK_C_SOURCE_COMPILES("#include <stdatomic.h> \n int main() {int i=0; atomic_fetch_add( &i, 1 );}" EX_HAVE_STDATOMIC)
CHECK_C_SOURCE_COMPILES("int main() {int i=0; __sync_fetch_and_add( &i, 1 );}" EX_HAVE_SYNCFETCHADD)

# To check for an include file you do this: 
CHECK_INCLUDE_FILE("stdint.h"        HAVE_STDINT_H)
CHECK_INCLUDE_FILE("getopt.h"       HAVE_GETOPT_H)
	

# To check the size of a primitive type:
CHECK_TYPE_SIZE("int"   EX_SIZEOF_INT)
CHECK_TYPE_SIZE("long"  EX_SIZEOF_LONG)
CHECK_TYPE_SIZE("void*"  EX_SIZEOF_VOIDPTR)

MATH (EXPR EX_PLATFORM_BITS "${EX_SIZEOF_VOIDPTR} * 8")

IF (DEFINE_DISABLEPSCRIPT)
    set(NDRX_DISABLEPSCRIPT "1")
ENDIF()

IF (DEFINE_SANITIZE)
    set(NDRX_SANITIZE "1")
ENDIF()

IF (MUTEX_DEBUG)
    set(NDRX_MUTEX_DEBUG "1")
ENDIF()

IF (ENABLE_POSTGRES)
    set(NDRX_USE_POSTGRES "1")
ENDIF()

# Output the project version
set(NDRX_VERSION_STR "${PROJ_NAME} ${VERSION}")

# avoid configure warnings of not being used
# as they are used.
set(ignoreWarning "${DISABLE_ECPG} ${PostgreSQL_TYPE_INCLUDE_DIR}")
################################################################################
# Install dir config
################################################################################
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

message("LIB: ${LIB64}")

if(CMAKE_OS_NAME STREQUAL "DARWIN")
    set(LIB_SUFFIX "")
# Seems like BSD does not use lib64 folder
elseif (CMAKE_OS_NAME STREQUAL "FREEBSD")
    set(LIB_SUFFIX "")
elseif (CMAKE_OS_NAME STREQUAL "AIX")
    set(LIB_SUFFIX "")
elseif (LIB64 STREQUAL "TRUE")
    set(LIB_SUFFIX 64)
else()
    set(LIB_SUFFIX "")
endif()

message("LIB_SUFFIX: ${LIB_SUFFIX}")

set(INSTALL_LIB_DIR lib${LIB_SUFFIX} CACHE PATH "Installation directory for libraries")

message("INSTALL_LIB_DIR: ${INSTALL_LIB_DIR}")

mark_as_advanced(INSTALL_LIB_DIR) 
MESSAGE( STATUS "INSTALL_LIB_DIR: " ${INSTALL_LIB_DIR} )

################################################################################
# Options
################################################################################
OPTION(DEFINE_DISABLEGPGME "Use GPG-ME encryption (not used, for compatiblity)" OFF)
OPTION(DEFINE_DISABLEPSCRIPT "Disable Platform Script" OFF)
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
add_subdirectory (embedfile)
add_subdirectory (libcgreen)
add_subdirectory (libubf)
add_subdirectory (libnetproto)
add_subdirectory (mkfldhdr)
# Enduro/X sub-directories for ATMI
add_subdirectory (libatmi) # General Purpose ATMI library
add_subdirectory (libatmisrv) #  ATMI server library 
add_subdirectory (libatmiclt) #  ATMI client library 
add_subdirectory (libtux) #  Tux lib emulation
add_subdirectory (libexnet)
add_subdirectory (libexmemck)
add_subdirectory (libndrxconf)

IF (NOT DEFINE_DISABLEPSCRIPT)
    add_subdirectory (libps) # Platform script
    add_subdirectory (libpsstdlib) # Platform script standard library
ENDIF()

add_subdirectory (tpevsrv)
add_subdirectory (tpcachesv)
add_subdirectory (tpcached)
add_subdirectory (tprecover) # Special recovery server
add_subdirectory (tpbrdcstsv) # tpbroadcast remote machine dispatcher
add_subdirectory (ud)


IF (NOT DEFINE_DISABLEPSCRIPT)
    add_subdirectory (pscript) # Platform script interpreter
ENDIF()

add_subdirectory (viewc) # View compiler
add_subdirectory (ndrxd) # Enduro X daemon
add_subdirectory (tpadmsv) # Enduro/X Admin API Server
add_subdirectory (migration) # Migration tools
add_subdirectory (xadmin) # Enduro X command line interface
add_subdirectory (buildtools) # buildserver,buildclient,buildtms
add_subdirectory (lmdb_util) # LMDB/EXDB Admin utilities
add_subdirectory (xmemck) # Enduro X memcory checking tool
add_subdirectory (bridge) # Cluster bridge server
add_subdirectory (tmsrv) # Transaction Manager server for XA transactions
add_subdirectory (tmqueue) # Persistant Queue server
add_subdirectory (cpmsrv) # Client Process Monitor
add_subdirectory (xadrv) # XA Drivers for RMs
add_subdirectory (cconfsrv) # Common-Config server
add_subdirectory (exbench) # Enduro/X benchmark tools
add_subdirectory (plugins) # XA Drivers for RMs
add_subdirectory (enctools) # XA Drivers for RMs
add_subdirectory (libextest) # Shared test resource library
add_subdirectory (ubftest) # UBF library testing
add_subdirectory (atmitest) #  ATMI testing

IF(DEFINE_DISABLEDOC)
        message("Documentation disabled - not building")
ELSE (DEFINE_DISABLEDOC)
        add_subdirectory (doc) #  Documentation project.
ENDIF (DEFINE_DISABLEDOC)

add_subdirectory (include) #  For install
add_subdirectory (scripts) #  For install
add_subdirectory (cmake) #  For install

#
# Install license files
#
install (FILES
    LICENSE
    DESTINATION share/endurox)

#
# Install third party licenses
#
install (FILES
    doc/third_party_licences.adoc
    DESTINATION share/endurox
    RENAME THIRD_PARTY_LICENCES)

#
# Install stock RM file
#
install (FILES
    scripts/RM
    DESTINATION udataobj)

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

# Resolve OS versions...
ex_osver()

# Support only for Linux
MESSAGE( "CPack:Debug: CMAKE_SYSTEM_NAME = " ${CMAKE_SYSTEM_NAME} )
MESSAGE( "CPack:Debug: CMAKE_SYSTEM_PROCESSOR = " ${CMAKE_SYSTEM_PROCESSOR} )
set(CPACK_MONOLITHIC_INSTALL 1)

#
# Generate configuration 
#
configure_file ("${PROJECT_SOURCE_DIR}/include/ndrx_config.h.in"
                "${PROJECT_BINARY_DIR}/include/ndrx_config.h" )

# Test the available generators and then configure output
find_program(RPMPROG "rpmbuild")
find_program(APTPROG "dpkg")

# avoid file /usr/share/man from install of endurox-3.5.1-1.x86_64 conflicts with file from package filesystem-3.2-21.el7.x86_64
# problems...
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/share/man
		/usr/share/man/man3
		/usr/share/man/man5
		/usr/share/man/man8
		/usr/lib64/pkgconfig
		/usr/lib/pkgconfig)
message("Excl: ${CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION}")

set(CPACK_GENERATOR "TGZ")
if(RPMPROG)
    message("Outputting RPM")
    set(CPACK_GENERATOR "${CPACK_GENERATOR};RPM")
endif()

if(APTPROG)
    message("Outputting DEB")
    set(CPACK_GENERATOR "${CPACK_GENERATOR};DEB")
endif()

ex_cpuarch()


set(CPACK_PACKAGE_CONTACT "contact@mavimax.com")
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_VENDOR "Mavimax Ltd")
set(CPACK_RPM_PACKAGE_RELEASE ${RELEASE})
set(CPACK_RPM_PACKAGE_RELEASE_DIST ${RELEASE})
set(CPACK_DEBIAN_PACKAGE_RELEASE ${RELEASE})
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LO)
message("CPack:Debug: PROJECT NAME = ${PROJECT_NAME_LO}")
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME_LO}-${VERSION}-${RELEASE}.${LSB_RELEASE_OUTPUT_OS}${LSB_RELEASE_OUTPUT_VER}_${CMAKE_C_COMPILER_ID}_${EX_POLLER_STR}.${EX_CPU_ARCH})
string(TOLOWER ${CPACK_PACKAGE_FILE_NAME}  CPACK_PACKAGE_FILE_NAME)
message("CPack:Debug: CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Enduro/X Middleware Platform")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION  ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_PACKAGE_AUTOREQ "0")
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Debian/postinst")

include(CPack)
################################################################################
# Source indexing for quick symbol lookup
# note that "glimpse" package must be installed.
# to exclude any directory or name from indexing,
# put the name in the glimpse_index/.glimpse_exclude
################################################################################

add_custom_target(index glimpseindex -n -H ./glimpse_index ${PROJECT_SOURCE_DIR}
                    COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/glimpse_index/glim.cmake)
#this also shall generate the glimp command in the current project

# vim: set ts=4 sw=4 et smartindent:
