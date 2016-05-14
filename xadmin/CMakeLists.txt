## 
## `ndrx' EnduroX Administration utility.
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

# Set a minimum version
cmake_minimum_required(VERSION 2.8)


# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

# Make sure the compiler can find inclndrxe files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/include ${ENDUROX_SOURCE_DIR}/libnstd
					 ${ENDUROX_SOURCE_DIR}/xadmin)

# Make sure the linker can find the UBF library once it is built.
#link_directories (${ENDUROX_BINARY_DIR}/libubf) 

# Exectuables
add_executable (xadmin xadmin.c exec.c config.c
		cmd_config.c utils.c cmd_startstop.c cmd_psc.c cmd_fdown.c
		cmd_at.c cmd_reload.c cmd_unadv.c cmd_ppm.c
                cmd_shm_psvc.c cmd_shm_psrv.c cmd_readv.c cmd_pq.c cmd_pqa.c
                cmd_pt.c cmd_abort.c cmd_commit.c cmd_env.c cmd_pc.c
                cmd_scbp.c cmd_mqlq.c cmd_mqlc.c cmd_mqlm.c cmd_mqdm.c cmd_mqrc.c)

# Link the executable to the UBF library & others...
target_link_libraries (xadmin m nstd rt atmi ubf dl)

# Install bin
install (TARGETS 
    xadmin 
    DESTINATION bin)

# Install bin, scripts
install (PROGRAMS 
    fdown.sh
    tmadmin
    tmboot
    tmshutdown
    DESTINATION bin)
