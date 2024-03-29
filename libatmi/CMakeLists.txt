##
## @brief Enduro Execution platform's ATMI library
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

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF (ENV{RELEASE_BUILD})
	# do nothing
ELSE (ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF (ENV{RELEASE_BUILD})

# Make sure the compiler can find include files from our UBF library.
include_directories (. ${ENDUROX_SOURCE_DIR}/include ${ENDUROX_SOURCE_DIR}/libnstd
                ${ENDUROX_SOURCE_DIR}/libubf) 
include_directories (/opt/freeware/include) 
# Create a library called "ATMI" which includes the source files.
# The extension is already found. Any number of sources could be listed here. 
add_library (objatmi OBJECT
                atmi.c
                tperror.c
                tpcall.c
                typed_buf.c
                typed_ubf.c
                typed_tpinit.c
                typed_null.c
                typed_string.c
                typed_json.c
                typed_carray.c
                typed_view.c
                atmiutils.c
                init.c
                conversation.c
                tpevents.c
                shm.c
                sem.c
                brsupport.c
                xa.c
                xautils.c
                exparson.c
                ubf2exjson.c
                view2exjson.c
                newenv.c
                typed_xcvt.c
                ubfutil.c
                qcommon.c
                sysutil.c
                atmi_tls.c
                atmi_tplog.c
                oatmi.c
                oubf.c
                ondebug.c
                onerror.c
                tpnotify.c
                identifiers.c
                atmi_cache_init.c
                atmi_cache_edb.c
                atmi_cache_ops.c
                atmi_cache_ubf.c
                atmi_cache_events.c
                atmi_cache_inval.c
                atmi_cache_mgt.c
                atmi_cache_keygrp.c
                tpimport.c
                tpexport.c
                tx.c
                cltshm.c
                tmnull_switch.c
                tpcrypto.c
                multibuf.c
                ddr_atmi.c
                txsinglegrp.c
            )

# shared libraries need PIC
set_property(TARGET objatmi PROPERTY POSITION_INDEPENDENT_CODE 1)

# shared and static libraries built from the same object files
add_library(atmi SHARED $<TARGET_OBJECTS:objatmi>)

if(CMAKE_OS_NAME STREQUAL "CYGWIN")
    target_link_libraries(atmi ubf nstd dl)
elseif(CMAKE_OS_NAME STREQUAL "DARWIN")
    target_link_libraries(atmi ubf nstd dl)
elseif(CMAKE_OS_NAME STREQUAL "FREEBSD")
    target_link_libraries(atmi ubf nstd pthread)
elseif(CMAKE_OS_NAME STREQUAL "AIX")
    # 
    # linking lib against pthread for XL C on aix, otherwise we get error like: 
    #  ld: 0711-768 WARNING: Object CMakeFiles/ubf.dir/ferror.c.o, section 1, function .__tls_get_addr:
    #        The branch at address 0x154 is not followed by a rec
    # Or
    # ld: 0711-768 WARNING: Object CMakeFiles/ubf.dir/ferror.c.o, section 1, function .__tls_get_addr:
    #   The branch at address 0x154 is not followed by a rec
    target_link_libraries(atmi ubf nstd pthread)
endif()

install (TARGETS 
    atmi 
    DESTINATION "${INSTALL_LIB_DIR}"
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    COMPONENT libraries)

# vim: set ts=4 sw=4 et smartindent:
