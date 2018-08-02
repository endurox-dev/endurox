## 
## @brief Cluster bridge server.
##
## @file CMakeLists.txt
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
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
cmake_minimum_required(VERSION 2.8)

# Make sure the compiler can find include files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/libubf
					 ${ENDUROX_SOURCE_DIR}/include
					 ${ENDUROX_SOURCE_DIR}/libnstd
					 ${ENDUROX_SOURCE_DIR}/ubftest
					 # For some systems gpgme is here:
					 /usr/include/gpgme
					 )


# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 

############################# Executables ###############################

IF(DEFINE_DISABLEGPGME)
	add_executable (tpbridge bridgesvc.c
                         queue.c
                         network.c
                         clock.c
                         brutils.c
                         ../libatmisrv/rawmain_integra.c
                        )

	SET(EX_LIB_GPGME "")

ELSE (DEFINE_DISABLEGPGME)
	add_executable (tpbridge bridgesvc.c
                         queue.c
                         network.c
                         clock.c
                         brutils.c
			 gpgme_encrypt.c
                         ../libatmisrv/rawmain_integra.c
                        )

	SET(EX_LIB_GPGME "gpgme")
ENDIF (DEFINE_DISABLEGPGME)

IF (${CMAKE_OS_NAME} STREQUAL "SUNOS")
	SET(EX_LIB_NET "socket")
ENDIF()

target_link_libraries (tpbridge exnet atmisrvinteg atmi netproto ubf thpool nstd ${RT_LIB}
                ${EX_LIB_GPGME} ${EX_LIB_NET} pthread)

set_target_properties(tpbridge PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
################################################################################

install (TARGETS 
    tpbridge 
    DESTINATION bin)
