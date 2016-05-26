## 
## Enduro standard library
## Enduro Execution platform
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
cmake_minimum_required(VERSION 2.6)

# Make sure the compiler can find include files from our Hello library. 
include_directories (. ../include)

# Set the poller
if (${CMAKE_OS_NAME} MATCHES "LINUX")
    SET(NSTD_POLLER "sys_epoll.c")
else ()
    SET(NSTD_POLLER "sys_poll.c")
endif ()

# Select the OS specifics:
if (${CMAKE_OS_NAME} MATCHES "LINUX")
    SET(NSTD_SYS "sys_linux.c")
elseif (${CMAKE_OS_NAME} MATCHES "SUNOS")
    SET(NSTD_SYS "sys_genunix.c")
    SET(NSTD_SYS_2 "sys_solaris.c")
endif ()


add_library (nstd SHARED ndebug.c nstdutil.c ntimer.c nclopt.c ${NSTD_POLLER} ${NSTD_SYS}
                            sys_common.c ${NSTD_SYS_2})

install (TARGETS 
    nstd 
    DESTINATION "${INSTALL_LIB_DIR}"
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    COMPONENT libraries
    )
