##
## @brief `ndrxd' EnduroX Monitor Daemon
##
## @file CMakeLists.txt
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 3 of the License, or (at your option) any later
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

# Set a minimum version
cmake_minimum_required(VERSION 2.8)

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

find_package(LibXml2 REQUIRED)
# Make sure the compiler can find include files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/libubf
                        ${ENDUROX_SOURCE_DIR}/libatmi
                        ${ENDUROX_SOURCE_DIR}/libatmiclt
                        ${ENDUROX_SOURCE_DIR}/include
                        ${ENDUROX_SOURCE_DIR}/libnstd
			${LIBXML2_INCLUDE_DIR})
# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 

# Exectuables
add_executable (ndrxd   ndrxd.c
                        appconfig.c
                        pmodel.c
			admerror.c
			cmd_processor.c
			cmd_config.c
			cmd_reply.c
			cmd_startstop.c
			cmd_srvinfo.c
			cmd_psc.c
			cmd_at.c
			cmd_reload.c
                        sanity.c
                        respawn.c
                        restart.c
                        cmd_unadv.c
                        cmd_readv.c
                        cmd_adv.c
                        cmd_ppm.c
                        cmd_shm_psvc.c
                        cmd_shm_psrv.c
                        svc_q_remove.c
                        bridge.c
                        cmd_bridge.c
			cmd_ping.c
			cmd_pq.c
                        cmd_env.c
                        reloadonchange.c)

# Link the executable to the UBF library & others...
target_link_libraries (ndrxd atmiclt atmi ndrxconf ubf nstd m 
        ${RT_LIB} ${LIBXML2_LIBRARIES} pthread)
set_target_properties(ndrxd PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# Install bin
install (TARGETS 
    ndrxd 
    DESTINATION bin)

