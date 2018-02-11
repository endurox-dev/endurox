## 
## `ndrx' EnduroX Administration utility.
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

add_custom_command(
  OUTPUT provision.c
  COMMAND embedfile scripts/provision.pscript provision
  DEPENDS scripts/provision.pscript)

add_custom_command(
  OUTPUT gen_go_server.c
  COMMAND embedfile scripts/gen_go_server.pscript gen_go_server
  DEPENDS scripts/gen_go_server.pscript)

add_custom_command(
  OUTPUT gen_go_client.c
  COMMAND embedfile scripts/gen_go_client.pscript gen_go_client
  DEPENDS scripts/gen_go_client.pscript)
  
add_custom_command(
  OUTPUT gen_ubf_tab.c
  COMMAND embedfile scripts/gen_ubf_tab.pscript gen_ubf_tab
  DEPENDS scripts/gen_ubf_tab.pscript)

add_custom_command(
  OUTPUT Exfields.c
  COMMAND embedfile ../include/Exfields Exfields
  DEPENDS ../include/Exfields)
  
add_custom_command(
  OUTPUT gen_c_client.c
  COMMAND embedfile scripts/gen_c_client.pscript gen_c_client
  DEPENDS scripts/gen_c_client.pscript)

add_custom_command(
  OUTPUT gen_c_server.c
  COMMAND embedfile scripts/gen_c_server.pscript gen_c_server
  DEPENDS scripts/gen_c_server.pscript)

add_custom_command(
  OUTPUT gen_test_local.c
  COMMAND embedfile scripts/gen_test_local.pscript gen_test_local
  DEPENDS scripts/gen_test_local.pscript)
  
  
# Exectuables
add_executable (xadmin xadmin.c exec.c config.c
		cmd_config.c utils.c cmd_startstop.c cmd_psc.c cmd_fdown.c
		cmd_at.c cmd_reload.c cmd_unadv.c cmd_ppm.c
                cmd_shm_psvc.c cmd_shm_psrv.c cmd_readv.c cmd_pq.c cmd_pqa.c
                cmd_pt.c cmd_abort.c cmd_commit.c cmd_env.c cmd_pc.c
                cmd_mqlq.c cmd_mqlc.c cmd_mqlm.c cmd_mqdm.c cmd_mqrc.c
                cmd_mqch.c cmd_mqrm.c cmd_mqmv.c cmd_killall.c cmd_qrm.c
                provision.c Exfields.c cmd_provision.c gen_go_server.c cmd_gen.c
                gen_ubf_tab.c gen_go_client.c gen_c_client.c gen_c_server.c
                gen_test_local cmd_cs.c)

set_target_properties(xadmin PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# Link the executable to the UBF library & others...

# add pscript, if not disabled...
IF (NOT DEFINE_DISABLEPSCRIPT)
    SET(PS_LIB psstdlib ps)
ENDIF()

target_link_libraries (xadmin ${PS_LIB} atmi nstd ubf m ${RT_LIB} pthread)

# Install bin
install (TARGETS 
    xadmin 
    DESTINATION bin)

# Install bin, scripts
install (PROGRAMS 
    tmadmin
    tmboot
    tmshutdown
    DESTINATION bin)
