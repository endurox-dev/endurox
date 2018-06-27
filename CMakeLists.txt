## 
## Enduro Execution platform main Make descriptor
##
## @file CMakeLists.txt
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
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
## A commercial use license is available from Mavimax, Ltd
## contact@mavimax.com
## -----------------------------------------------------------------------------
##

cmake_minimum_required (VERSION 2.6) 
project (ENDUROX) 
set(VERSION "5.3.18")
set(PROJ_NAME "Enduro/X")
set(RELEASE "1")


################################################################################
# OS Confiugration
################################################################################

EXEC_PROGRAM(uname ARGS -a OUTPUT_VARIABLE _TMP_CMAKE_OS_NAME)
string(REGEX MATCH "^[a-zA-Z0-9-]*" _TMP_CMAKE_OS_NAME_EXTR ${_TMP_CMAKE_OS_NAME})
string(TOUPPER ${_TMP_CMAKE_OS_NAME_EXTR} CMAKE_OS_NAME)

message("CMake CMAKE_OS_NAME  = ${CMAKE_OS_NAME}")
message("CMake CMAKE_C_COMPILER_ID  = ${CMAKE_C_COMPILER_ID}")
message("CMake CMAKE_CXX_COMPILER_ID  = ${CMAKE_CXX_COMPILER_ID}")

OPTION(DEFINE_FORCEPOLL "Force POLL (even on linux)" OFF)
OPTION(DEFINE_FORCEFDPOLL "Force FDPOLL" OFF)
OPTION(DEFINE_FORCEKQUEUE "Force Kqueue (BSD poll framework)" OFF)

OPTION(DEFINE_RELEASEBUILD "Do relesae build" OFF)
OPTION(DEFINE_FORCEEMQ "Force Emulated Message Queue" OFF)


SET ( CMAKE_CXX_FLAGS "-O2 -fno-exceptions -fno-rtti" CACHE STRING "compile flags" FORCE)

# Support #310 Ubuntu 18.04 prints lot of unneeded warnings...
SET(CMAKE_C_FLAGS "-Wno-format-truncation ${CMAKE_C_FLAGS}")

# Enable release only if specified.
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF (DEFINE_RELEASEBUILD)
	MESSAGE( RELEASE_BUILD )
	SET(CMAKE_BUILD_TYPE release)
        SET(CMAKE_C_FLAGS "-fstack-protector -O2 ${CMAKE_C_FLAGS}")
ELSE ()
	MESSAGE( DEBUG_BUILD )
	SET(CMAKE_BUILD_TYPE debug)
# Memory debugging:
#       SET(CMAKE_C_FLAGS "-fsanitize=alignment -fsanitize=address -fno-omit-frame-pointer -O1 -ggdb ${CMAKE_C_FLAGS}")
#       SET(CMAKE_C_FLAGS "-fsanitize=address -fno-omit-frame-pointer -O1 -ggdb ${CMAKE_C_FLAGS}")
        SET(CMAKE_C_FLAGS "-fstack-protector -O2 -ggdb ${CMAKE_C_FLAGS}")
ENDIF ()

SET(RT_LIB rt dl)
if( ${CMAKE_OS_NAME} STREQUAL "LINUX" )
        set(EX_OS_LINUX "1")
        set(NDRX_LD_LIBRARY_PATH "LD_LIBRARY_PATH")
        # Enable poll only
        IF(DEFINE_FORCEPOLL)
            message("Linux - Using poll()")
            set(EX_USE_POLL "1")
        ELSEIF(DEFINE_FORCEFDPOLL)
            message("Linux - Using fdpoll()")
        ELSE()
            message("Linux - Using epoll()")
            set(EX_USE_EPOLL "1")
        ENDIF()
elseif(${CMAKE_OS_NAME} STREQUAL "AIX")
# Configure the compiler, we support XLC too..

	message("No threads mode for CPM - bit slower child process exit status check...")
	# Seems aix 7.1 does not like threads mixed with fork...
   	set(EX_CPM_NO_THREADS "1")
        set(NDRX_LD_LIBRARY_PATH "LD_LIBRARY_PATH")

	if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		message("GNU compiler")
		set ( CMAKE_C_FLAGS "-D_SEM_SEMUN_UNDEFINED -D_THREAD_SAFE -pthread -maix64 ${CMAKE_C_FLAGS}")
	elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "XL")
		message("XL compiler")
		set ( CMAKE_C_FLAGS " -D_SEM_SEMUN_UNDEFINED -D_THREAD_SAFE ")
                set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qwarn64 -qpic -bexpfull -b64")
                set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -brtl -qtls -q64 -qlanglvl=extc99")
                set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qsuppress=1506-997 -qprocimported=setjmp -lbsd")

		if (NOT DEFINE_RELEASEBUILD)
			message("XL compiler: with -g (debug symbols...)")
			set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
		endif()
	endif()

    set(EX_USE_POLL "1")
	set(EX_OS_AIX "1")
    # For AIX we do not have other options that use file system
    # base queue registry.
    set(USE_FS_REGISTRY "1")
elseif(${CMAKE_OS_NAME} STREQUAL "HP-UX")
    set(EX_USE_POLL "1")
	set(EX_OS_HPUX "1")
    set(NDRX_LD_LIBRARY_PATH "SHLIB_PATH")
elseif(${CMAKE_OS_NAME} STREQUAL "SUNOS")
	set(EX_OS_SUNOS "1")
    set(EX_USE_POLL "1")
    set(NDRX_LD_LIBRARY_PATH "LD_LIBRARY_PATH")
	# Bug #219 !!! _REENTRANT makes errno thread safe
	SET(CMAKE_C_FLAGS "-D_REENTRANT")
	if ("${CMAKE_C_COMPILER_ID}" STREQUAL "SunPro")
		message("SUN Studio compiler (building for 64bit) ")

		# Bug #219 !!! _REENTRANT makes errno thread safe
		# For GNU I guess 
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -errtags -fast -m64 -xc99=all -erroff=E_PTR_TO_VOID_IN_ARITHMETIC ")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -erroff=E_EMPTY_MBR_DECLARATION -erroff=E_NO_IMPLICIT_DECL_ALLOWED")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -erroff=E_EMPTY_DECLARATION -erroff=E_ARGUEMENT_MISMATCH -erroff=E_ZERO_OR_NEGATIVE_SUBSCRIPT")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -erroff=E_ARG_INCOMPATIBLE_WITH_ARG_L")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -erroff=E_BAD_PTR_INT_COMBINATION")

		SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -m64")
		# Add Sun Studio C runtime
		SET(RT_LIB ${RT_LIB} Crun)
	endif()
elseif(${CMAKE_OS_NAME} STREQUAL "FREEBSD")
    SET(RT_LIB rt)
	# seem that threading model also does not work here...
    set(EX_CPM_NO_THREADS "1")
	set(EX_OS_FREEBSD "1")
	# Best option...
    set(EX_USE_KQUEUE "1")
    set(NDRX_LD_LIBRARY_PATH "LD_LIBRARY_PATH")
elseif(${CMAKE_OS_NAME} STREQUAL "CYGWIN")
	set(EX_OS_CYGWIN "1")
    set(EX_USE_POLL "1")
    set(NDRX_LD_LIBRARY_PATH "LD_LIBRARY_PATH")
elseif(${CMAKE_OS_NAME} STREQUAL "DARWIN")
    SET(RT_LIB dl)
    set(EX_OS_DARWIN "1")
    set(EX_USE_EMQ "1")
    set(EX_USE_POLL "1")
    set(NDRX_LD_LIBRARY_PATH "DYLD_FALLBACK_LIBRARY_PATH")
# Unknown OS:
else()
	message( FATAL_ERROR "Unsupported OS" )
endif()

# Use emulated message queue
IF(DEFINE_FORCEEMQ)
    message("Using emulated message queue")
    set(EX_USE_EMQ "1")
    set(EX_USE_POLL "1")
ENDIF()

# Use emulated message queue
IF(DEFINE_FORCEFDPOLL)
    message("Using poll on MQ mqd_t as file descriptor")
    set(EX_USE_FDPOLL "1")
ENDIF()

IF(DEFINE_FORCEKQUEUE)
    message("Using Kqueue event mechanism")
    set(EX_USE_KQUEUE "1")
ENDIF()

################################################################################
# "Configure"
################################################################################

# In this file we are doing all of our 'configure' checks. Things like checking
# for headers, functions, libraries, types and size of types.
INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckCSourceCompiles.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckCXXSourceCompiles.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckSymbolExists.cmake)


CHECK_C_SOURCE_COMPILES("extern const char * __progname; int main() {return __progname[0];}" HAVE_PROGNAME)

CHECK_C_SOURCE_COMPILES("int main() {__builtin_expect(1, 1);}" HAVE_EXPECT)

CHECK_C_SOURCE_COMPILES("int main() { int X = 0; __sync_bool_compare_and_swap(&X, 0, 1); __sync_synchronize();}" HAVE_SYNC)

CHECK_C_SOURCE_COMPILES("#include <stdio.h> \n int main() {char buf[128]; size_t len; getline(buf, &len, stdin);}" HAVE_GETLINE)

CHECK_C_SOURCE_COMPILES("#include <string.h> \n int main() {char dest[1]; char src[1]; strlcpy(dest, src, 1);}" HAVE_STRLCPY)

CHECK_C_SOURCE_COMPILES("#include <string.h> \n int main() {strnlen(\"ABC\", 1);}" HAVE_STRNLEN)

# To check for an include file you do this: 
CHECK_INCLUDE_FILE("stdint.h"        HAVE_STDINT_H)
CHECK_INCLUDE_FILE("getopt.h"       HAVE_GETOPT_H)
	

# To check the size of a primitive type:
CHECK_TYPE_SIZE("int"   SIZEOF_INT)

IF (DEFINE_DISABLEPSCRIPT)
	set(NDRX_DISABLEPSCRIPT "1")
ENDIF()

# Output the project version
set(NDRX_VERSION_STR "${PROJ_NAME} ${VERSION}")

################################################################################
# Install dir config
################################################################################
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

message("LIB: ${LIB64}")

if(${CMAKE_OS_NAME} STREQUAL "DARWIN")
    set(LIB_SUFFIX "")
# Seems like BSD does not use lib64 folder
elseif ("${CMAKE_OS_NAME}" STREQUAL "FREEBSD")
    set(LIB_SUFFIX "")
elseif ("${LIB64}" STREQUAL "TRUE")
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

# Option to disable gpg-me (gpg-me enabled by default)

OPTION(DEFINE_DISABLEGPGME "Use GPG-ME encryption" OFF)
 
IF(DEFINE_DISABLEGPGME)
    ADD_DEFINITIONS(-DDISABLEGPGME)
ENDIF(DEFINE_DISABLEGPGME)

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
add_subdirectory (libexuuid)
add_subdirectory (libubf)
add_subdirectory (libnetproto)
add_subdirectory (libthpool)
add_subdirectory (ubftest)
add_subdirectory (mkfldhdr)
# EnduroX sub-directories for ATMI
add_subdirectory (libatmi) # General Purpose ATMI library
add_subdirectory (libatmisrv) #  ATMI server library 
add_subdirectory (libatmiclt) #  ATMI client library 
add_subdirectory (libexnet)
add_subdirectory (libexmemck)

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
add_subdirectory (xadmin) # Enduro X command line interface
add_subdirectory (lmdb_util) # LMDB/EXDB Admin utilities
add_subdirectory (xmemck) # Enduro X memcory checking tool
add_subdirectory (bridge) # Cluster bridge server
add_subdirectory (tmsrv) # Transaction Manager server for XA transactions
add_subdirectory (tmqueue) # Persistant Queue server
add_subdirectory (cpmsrv) # Client Process Monitor
add_subdirectory (xadrv) # XA Drivers for RMs
add_subdirectory (cconfsrv) # Common-Config server
add_subdirectory (plugins) # XA Drivers for RMs
add_subdirectory (enctools) # XA Drivers for RMs
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
        set(LSB_RELEASE_OUTPUT_OS ${CMAKE_OS_NAME})

        string(REPLACE "." "_" _TMP_LSB_RELEASE_OUTPUT_VER ${CMAKE_SYSTEM_VERSION})
        string(REPLACE "-" "_" LSB_RELEASE_OUTPUT_VER ${_TMP_LSB_RELEASE_OUTPUT_VER})
	
	# If it is AIX, then we can extract version from uname
	if(${CMAKE_OS_NAME} STREQUAL "AIX")
		execute_process(COMMAND uname -v
			OUTPUT_VARIABLE _TMP_OS_MAJOR_VER OUTPUT_STRIP_TRAILING_WHITESPACE)
		execute_process(COMMAND uname -r
			OUTPUT_VARIABLE _TMP_OS_MINOR_VER OUTPUT_STRIP_TRAILING_WHITESPACE)
		set(LSB_RELEASE_OUTPUT_VER ${_TMP_OS_MAJOR_VER}_${_TMP_OS_MINOR_VER})
	endif()
	
endif()

message("LSB_RELEASE OS  = ${LSB_RELEASE_OUTPUT_OS}")
message("LSB_RELEASE VER = ${LSB_RELEASE_OUTPUT_VER}")
set(EX_LSB_RELEASE_VER ${LSB_RELEASE_OUTPUT_VER})

string(REPLACE "_" ";" LSB_VERSION_LIST ${LSB_RELEASE_OUTPUT_VER})

list( LENGTH LSB_VERSION_LIST TMP_LIST_LEN ) 

if (TMP_LIST_LEN LESS 1)
        # No version at all!!
        set(EX_LSB_RELEASE_VER_MAJOR "0")
        set(EX_LSB_RELEASE_VER_MINOR "0")
elseif (TMP_LIST_LEN LESS 2)
        # Bug #198 - FedoraCore do not have minor version numbers
        list(GET LSB_VERSION_LIST 0 EX_LSB_RELEASE_VER_MAJOR)
        set(EX_LSB_RELEASE_VER_MINOR "0")

else ()
        list(GET LSB_VERSION_LIST 0 EX_LSB_RELEASE_VER_MAJOR)
        list(GET LSB_VERSION_LIST 1 EX_LSB_RELEASE_VER_MINOR)
endif()

MESSAGE( "EX_LSB_RELEASE_VER_MAJOR = " ${EX_LSB_RELEASE_VER_MAJOR} )
MESSAGE( "EX_LSB_RELEASE_VER_MINOR = " ${EX_LSB_RELEASE_VER_MINOR} )

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

set(CPACK_PACKAGE_CONTACT "madars.vitolins@gmail.com")
set(CPACK_PACKAGE_VERSION ${VERSION})
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxml2,openssl,libgpgme11")
#set(CPACK_RPM_PACKAGE_REQUIRES "libxml2,openssl,libssl,libgpgme11")
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LO)
message("CPack:Debug: PROJECT NAME = ${PROJECT_NAME_LO}")
set(CPACK_PACKAGE_FILE_NAME         ${PROJECT_NAME_LO}-${VERSION}-${RELEASE}.${LSB_RELEASE_OUTPUT_OS}${LSB_RELEASE_OUTPUT_VER}_${CMAKE_C_COMPILER_ID}.${CMAKE_SYSTEM_PROCESSOR})
message("CPack:Debug: CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Enduro/X Middleware Platform")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION  ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Debian/postinst")

include(CPack)
################################################################################



