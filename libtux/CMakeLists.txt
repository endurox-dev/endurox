##
## @brief Tuxedo compatibility library (dependecy loader)
##
## @file CMakeLists.txt
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

# Set minimum version
cmake_minimum_required(VERSION 2.8)

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D NDRX_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

add_library(tux SHARED $<TARGET_OBJECTS:objnstd>
                       $<TARGET_OBJECTS:objubf>
                       $<TARGET_OBJECTS:objatmi>
                       $<TARGET_OBJECTS:objatmisrvinteg>)
target_link_libraries(tux ${RT_LIB} pthread)

#
# having issues with golang on aix.. cannot load -brtl libraries
#
SET(ADDLIB "")

if(CMAKE_OS_NAME STREQUAL "AIX")

add_library(ndrxaix SHARED $<TARGET_OBJECTS:objnstd>
                       $<TARGET_OBJECTS:objubf>
                       $<TARGET_OBJECTS:objatmi>
                       $<TARGET_OBJECTS:objatmisrvinteg>)
SET(ADDLIB "ndrxaix")
# special library for aix / go
target_link_libraries(ndrxaix ${RT_LIB} pthread)
set_target_properties(ndrxaix PROPERTIES SUFFIX ".a")
endif()

#FML Emul
add_library (fml SHARED 
                    dum.c
                        )

#FML32 Emul
add_library (fml32 SHARED 
                    dum.c
                        )
#
# Also needs to provide dummy fml32 library for NetXMS Build
#
install (TARGETS 
    tux fml fml32 ${ADDLIB}
    DESTINATION "${INSTALL_LIB_DIR}"
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    COMPONENT libraries
    )

# vim: set ts=4 sw=4 et smartindent:
