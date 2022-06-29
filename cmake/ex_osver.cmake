##
## @brief Operating system versions resolver
##
## @file ex_osver.cmake
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

cmake_minimum_required (VERSION 3.1) 

#
# Required includes for macro package
#
macro(ex_osver_include)
    INCLUDE (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
endmacro(ex_osver_include)

#
# Set operating system version flags
# 
macro(ex_osver)

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
    elseif (EXISTS /etc/os-release)

            execute_process(COMMAND bash -c "cat /etc/os-release | egrep '^NAME="
                    OUTPUT_VARIABLE _TMP_LSB_RELEASE_OUTPUT_OS
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

            # extract the os name by regex
            if (_TMP_LSB_RELEASE_OUTPUT_OS MATCHES "^NAME=['\"]*(.*)['\"]*")
                set(_TMP_LSB_RELEASE_OUTPUT_OS ${CMAKE_MATCH_1})
            else()
                message(FATAL_ERROR "Failed to extract OS version from /etc/os-release")
            endif()

            string(TOLOWER 
                    ${_TMP_LSB_RELEASE_OUTPUT_OS}
            LSB_RELEASE_OUTPUT_OS)
            string(REPLACE " " "_" LSB_RELEASE_OUTPUT_OS ${LSB_RELEASE_OUTPUT_OS})

            # fixes for CentOs 7.1810, having something like "7 (Core)" in output.
            execute_process(COMMAND bash -c "cat /etc/os-release | egrep '^VERSION=' | cut -d '=' -f2 | cut -d ' ' -f1 | cut -d '\"' -f2"
                    OUTPUT_VARIABLE _TMP_LSB_RELEASE_OUTPUT_VER
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

endmacro(ex_osver)


#
# Common CPU architecture builder
#
macro(ex_cpuarch)

    CHECK_TYPE_SIZE("void*"  EX_OSVER_SIZEOF_VOIDPTR)
    MATH (EXPR EX_OSVER_PLATFORM_BITS "${EX_OSVER_SIZEOF_VOIDPTR} * 8")

    #
    # Fix arch for AIX
    #
    IF(CMAKE_SYSTEM_PROCESSOR MATCHES "^powerpc")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "ppc")
    ENDIF()

    set(EX_CPU_ARCH "${CMAKE_SYSTEM_PROCESSOR}_${EX_OSVER_PLATFORM_BITS}")

    #
    # MAP the CPU name
    #
    if( (${EX_CPU_ARCH} STREQUAL "x86_64_64") OR (${EX_CPU_ARCH} STREQUAL "amd64_64") OR
          (${EX_CPU_ARCH} STREQUAL "386_64") OR (${EX_CPU_ARCH} STREQUAL "amd64")
          OR (${EX_CPU_ARCH} STREQUAL "i386_64") )
        set(EX_CPU_ARCH "x86_64")
    endif()

endmacro(ex_cpuarch)


# vim: set ts=4 sw=4 et smartindent:
