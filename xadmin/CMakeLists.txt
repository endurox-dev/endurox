##
## @brief `ndrx' EnduroX Administration utility.
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

# Set a minimum version
cmake_minimum_required(VERSION 3.5)


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

include(pscript)

set(SCRIPTS "")

IF (NOT DEFINE_DISABLEPSCRIPT)
    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/provision.pscript provision)
    list(APPEND SCRIPTS provision.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_go_server.pscript gen_go_server)
    list(APPEND SCRIPTS gen_go_server.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_go_client.pscript gen_go_client)
    list(APPEND SCRIPTS gen_go_client.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_ubf_tab.pscript gen_ubf_tab)
    list(APPEND SCRIPTS gen_ubf_tab.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_c_client.pscript gen_c_client)
    list(APPEND SCRIPTS gen_c_client.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_c_server.pscript gen_c_server)
    list(APPEND SCRIPTS gen_c_server.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_java_client.pscript gen_java_client)
    list(APPEND SCRIPTS gen_java_client.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_java_server.pscript gen_java_server)
    list(APPEND SCRIPTS gen_java_server.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_test_local.pscript gen_test_local)
    list(APPEND SCRIPTS gen_test_local.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_python_client.pscript gen_python_client)
    list(APPEND SCRIPTS gen_python_client.c)

    pscript_embed(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_python_server.pscript gen_python_server)
    list(APPEND SCRIPTS gen_python_server.c)

ENDIF()

add_custom_command(
  OUTPUT Exfields.c
  COMMAND exembedfile ${CMAKE_CURRENT_SOURCE_DIR}/../include/Exfields Exfields
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../include/Exfields)

add_custom_command(
  OUTPUT ndrx_config.c
  COMMAND exembedfile ../include/ndrx_config.h ndrx_config
  DEPENDS ../include/ndrx_config.h)

if(DEFINED EX_USE_SYSVQ OR DEFINED EX_USE_SVAPOLL)
    SET(SOURCE1 "cmd_svmaps.c")
endif()

  
# Exectuables
add_executable (xadmin xadmin.c exec.c config.c
    cmd_config.c utils.c cmd_startstop.c cmd_psc.c cmd_fdown.c
    cmd_at.c cmd_reload.c cmd_unadv.c cmd_ppm.c
    cmd_shm_psvc.c cmd_shm_psrv.c cmd_readv.c cmd_pq.c cmd_pqa.c
    cmd_pt.c cmd_abort.c cmd_commit.c cmd_env.c cmd_pc.c
    cmd_mqlq.c cmd_mqlc.c cmd_mqlm.c cmd_mqdm.c cmd_mqrc.c
    cmd_mqch.c cmd_mqrm.c cmd_mqmv.c cmd_killall.c cmd_qrm.c
    ${SCRIPTS} Exfields.c cmd_provision.c cmd_gen.c
    cmd_cs.c cmd_cd.c cmd_ci.c
    cmd_pubfdb.c ${SOURCE1} cmd_svids.c ndrx_config.c cmd_util.c
    cmd_appconfig.c cmd_dping.c linenoise.c cmd_tranlocal.c
    cmd_tmib.c taboutput.c cmd_lcf.c cmd_prtsvc.c cmd_psg.c)

set_target_properties(xadmin PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# Link the executable to the UBF library & others...

# add pscript, if not disabled...
IF (NOT DEFINE_DISABLEPSCRIPT)
    SET(PS_LIB psstdlib ps)
ENDIF()

target_link_libraries (xadmin tpadm ${PS_LIB} atmi nstd ubf ${RT_LIB} pthread m)

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
# vim: set ts=4 sw=4 et smartindent:
