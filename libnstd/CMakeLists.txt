##
## @brief Enduro standard library
##   Enduro Execution platform
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

# Set minimum version
cmake_minimum_required(VERSION 3.5)

# Make sure the compiler can find include files from our Hello library. 
include_directories (. ../include)

SET(NSTD_POLLER "invalid_poller.c")

if(DEFINED EX_USE_SYSVQ)
    SET(NSTD_POLLER "sys_svqpoll.c")
    SET(NSTD_POLLER_2 "sys_svq.c")
    SET(NSTD_POLLER_3 "sys_svqshm.c")
    SET(NSTD_POLLER_4 "sys_svqevent.c")
    SET(NSTD_POLLER_5 "sys_svqadmin.c")
elseif (DEFINED EX_USE_EMQ)
    SET(NSTD_POLLER "sys_poll.c")
    SET(NSTD_POLLER_2 "sys_emqueue.c")
elseif (DEFINED EX_USE_KQUEUE)
    SET(NSTD_POLLER "sys_kqueue.c")
elseif(DEFINED EX_USE_POLL)
    SET(NSTD_POLLER "sys_poll.c")
elseif(DEFINED EX_USE_FDPOLL)
    SET(NSTD_POLLER "sys_fdpoll.c")
elseif(DEFINED EX_USE_EPOLL)
    SET(NSTD_POLLER "sys_epoll.c")
elseif(DEFINED EX_USE_SVAPOLL)
    # AIX Poll - directly on systemv queues
    SET(NSTD_POLLER "sys_svapoll.c")
    SET(NSTD_POLLER_2 "sys_svq.c")
    SET(NSTD_POLLER_3 "sys_svqshm.c")
endif()

message("CMake CMAKE_OS_NAME  = ${CMAKE_OS_NAME}")

# Use emulated queues (if forced).
IF(DEFINE_FORCEEMQ)
    SET(NSTD_SYS_3 "sys_emqueue.c")
ENDIF()

# Select the OS specifics:
if (CMAKE_OS_NAME STREQUAL "LINUX")
    SET(NSTD_SYS "sys_linux.c")
    SET(NSTD_SYS_2 "sys_posixq.c")
elseif (CMAKE_OS_NAME STREQUAL "CYGWIN")
    SET(NSTD_SYS "sys_linux.c")
    SET(NSTD_SYS_2 "sys_posixq.c")
elseif (CMAKE_OS_NAME STREQUAL "SUNOS")
    SET(NSTD_SYS "sys_solaris.c")
    SET(NSTD_SYS_2 "sys_posixq.c")
elseif (CMAKE_OS_NAME STREQUAL "AIX")
    SET(NSTD_SYS "sys_aix.c")
    SET(NSTD_SYS_2 "sys_posixq.c")
elseif (CMAKE_OS_NAME STREQUAL "FREEBSD")
    SET(NSTD_SYS "sys_freebsd.c")
    SET(NSTD_SYS_2 "sys_posixq.c")
    SET(NSTD_SYS_3 "fmemopen-funopen.c")
elseif (CMAKE_OS_NAME STREQUAL "DARWIN")
    SET(NSTD_SYS "sys_darwin.c")
    SET(NSTD_SYS_2 "sys_emqueue.c")
    SET(NSTD_SYS_3 "fmemopen-funopen.c")
endif ()

add_library (objnstd OBJECT ndebug.c nstdutil.c nstopwatch.c nclopt.c benchmark.c
                        ini.c inicfg.c cconfig.c nerror.c nstd_tls.c ulog.c
                        sys_genunix.c 
                        ${NSTD_POLLER}
                        ${NSTD_POLLER_2}
                        ${NSTD_POLLER_3}
                        ${NSTD_POLLER_4}
                        ${NSTD_POLLER_5}
                        ${NSTD_SYS}
                        sys_common.c ${NSTD_SYS_2} ${NSTD_SYS_3} tplog.c
                        exregex.c platform.c msgsizemax.c exaes.c exsha1.c
                        exbase64.c crypto.c expluginbase.c lmdb/eidl.c lmdb/edb.c
                        edbutil.c crc32.c nstd_shmsv.c ${NSTD_SYS_4} ${NSTD_SYS_5}
                        nstd_sem.c ${NSTD_SYS_6} emb.c sys_test.c
                        linearhash.c fpalloc.c thpool.c strtokblk.c lcf.c ndebugfd.c
                        lcf_api.c sys_fsync.c stdcfgstr.c cid.c singlegrp.c exsm.c 
                        rbtree.c)

# shared libraries need PIC
set_property(TARGET objnstd PROPERTY POSITION_INDEPENDENT_CODE 1)

# shared and static libraries built from the same object files
add_library(nstd SHARED $<TARGET_OBJECTS:objnstd>)

if(CMAKE_OS_NAME STREQUAL "AIX")
    # for tls support
    target_link_libraries(nstd pthread)
endif()

install (TARGETS 
    nstd 
    DESTINATION "${INSTALL_LIB_DIR}"
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    COMPONENT libraries
)
# vim: set ts=4 sw=4 et smartindent:
