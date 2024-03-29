##
## @brief TMSRV State Driving verification - server
##
## @file atmisv87.c
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

cmake_minimum_required(VERSION 3.5)

# Make sure the compiler can find include files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/libubf
					 ${ENDUROX_SOURCE_DIR}/include
					 ${ENDUROX_SOURCE_DIR}/libnstd
					 ${ENDUROX_SOURCE_DIR}/ubftest)

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

#
# RM1, wjoin
#
add_library (l87_1 STATIC
        xabackend.c
    )
target_compile_definitions(l87_1 PUBLIC TEST_RM1)

#
# RM2, wjoin
#
add_library (l87_2 STATIC
        xabackend.c
    )
target_compile_definitions(l87_2 PUBLIC TEST_RM2)

#
# RM1, nojoin
#
add_library (l87_1nj STATIC
        xabackend.c
    )
target_compile_definitions(l87_1nj PUBLIC TEST_RM1 TEST_NOJOIN)

#
# RM2, njoin
#
add_library (l87_2nj STATIC
        xabackend.c
    )
target_compile_definitions(l87_2nj PUBLIC TEST_RM2 TEST_NOJOIN)

################################################################################

# vim: set ts=4 sw=4 et smartindent:
