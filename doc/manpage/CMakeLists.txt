##
## @brief Build documentation
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
cmake_minimum_required(VERSION 2.8)

find_program(A2X_EXECUTABLE NAMES a2x)

set(MAN5_NAMES ex_env ndrxdebug.conf ndrxconfig.xml ex_envover q.conf
                viewfile tpcache.ini ubfdb.ini tm_mib)

set(MAN8_NAMES xadmin ndrxd ud mkfldhdr tpbridge tpevsrv tmsrv
           tprecover cpmsrv tmqueue cconfsrv tpbrdcstsv uvlog xmemck
           viewc buildclient buildserver buildtms exencrypt exdecrypt libcryptohost
           edb_dump edb_copy edb_stat edb_load tpcachebtsv tpcached tpcachesv
           libndrxxapq libndrxxaecpg libndrxxawsmqs libndrxxaoras tpadmsv libndrxxanulls
           libndrxxatmsx)

set(XHTML_MAN_NAMES ${MAN5_NAMES} ${MAN8_NAMES})


# Generate MAN5
set(A2X_OPTS_MAN5 --format=manpage -D man5)
set(MAN5_FILES)
foreach(m IN LISTS MAN5_NAMES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/man5/${m}.5)
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.adoc)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS_MAN5} ${ms}
    DEPENDS ${ms}
    WORKING_DIRECTORY .
    COMMENT "Building MAN 5 ${mf}"
    VERBATIM)
  list(APPEND MAN5_FILES ${mf})
  # Install the docs
  install (FILES
    ${mf}
    DESTINATION share/man/man5)

endforeach()
add_custom_target(man5 ALL DEPENDS ${MAN5_FILES})

# Generate MAN8
set(A2X_OPTS_MAN8 --format=manpage -D man8)
set(MAN8_FILES)
foreach(m IN LISTS MAN8_NAMES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/man8/${m}.8)
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.adoc)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS_MAN8} ${ms}
    DEPENDS ${ms}
    WORKING_DIRECTORY .
    COMMENT "Building MAN 8 ${mf}"
    VERBATIM)
  list(APPEND MAN8_FILES ${mf})

  # Install the docs
  install (FILES
    ${mf}
    DESTINATION share/man/man8)

endforeach()
add_custom_target(man8 ALL DEPENDS ${MAN8_FILES})

#
# html man pages, man 5
#
set(A2X_OPTS_XHTML_MAN -f xhtml -D html5)
set(XHTML_MAN_NAMES)
foreach(m IN LISTS MAN5_NAMES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/html5/${m}.html)
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.adoc)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS_XHTML_MAN} ${ms}
    DEPENDS ${ms}
    WORKING_DIRECTORY .
    COMMENT "Building HTML_MAN ${mf}"
    VERBATIM)
  list(APPEND XHTML_MAN_NAMES ${mf})

  # Install the docs
  install (FILES
    ${mf}
    DESTINATION share/endurox/doc/html/man5)

endforeach()
add_custom_target(man5html ALL DEPENDS ${XHTML_MAN_NAMES})

install (FILES
  ${CMAKE_CURRENT_BINARY_DIR}/html5/docbook-xsl.css
  DESTINATION share/endurox/doc/html/man5)


#
# html man pages, man 8
#
set(A2X_OPTS_XHTML_MAN -f xhtml -D html8)
set(XHTML_MAN_NAMES)
foreach(m IN LISTS MAN8_NAMES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/html8/${m}.html)
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.adoc)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS_XHTML_MAN} ${ms}
    DEPENDS ${ms}
    WORKING_DIRECTORY .
    COMMENT "Building HTML_MAN ${mf}"
    VERBATIM)
  list(APPEND XHTML_MAN_NAMES ${mf})

  # Install the docs
  install (FILES
    ${mf}
    DESTINATION share/endurox/doc/html/man8)

endforeach()

add_custom_target(man8html ALL DEPENDS ${XHTML_MAN_NAMES})


install (FILES
  ${CMAKE_CURRENT_BINARY_DIR}/html8/docbook-xsl.css
  DESTINATION share/endurox/doc/html/man8)

# end.


# vim: set ts=4 sw=4 et smartindent:
