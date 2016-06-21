## 
## Build documentation
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
cmake_minimum_required(VERSION 2.8)

find_program(A2X_EXECUTABLE NAMES a2x)
find_program(DBLATEX "dblatex")

set(PDF_NAMES ex_devguide ex_overview ex_adminman building_guide 
                       getting_started_tutorial persistent_message_queues_overview
                        osx_notes aix_notes)
set(HTML_NAMES ex_devguide ex_overview ex_adminman building_guide
                       getting_started_tutorial persistent_message_queues_overview
                        osx_notes aix_notes)

# Generate PDFs...
if(DBLATEX)
set(A2X_OPTS_PDF -L -f pdf --doctype book)
set(PDF_FILES)
foreach(m IN LISTS PDF_NAMES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/${m}.pdf)
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.adoc)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS_PDF} ${ms}
    DEPENDS ${ms}
    WORKING_DIRECTORY .
    COMMENT "Building PDF ${mf}"
    VERBATIM)
  list(APPEND PDF_FILES ${mf})

  # Install the docs
  install (FILES
    ${mf}
    DESTINATION share/endurox/doc/pdf)

endforeach()
add_custom_target(pdf ALL DEPENDS ${PDF_FILES})
endif()

# Generate HTMLs
set(A2X_OPTS_HTML -D . -f xhtml)
set(HTML_FILES)
foreach(m IN LISTS HTML_NAMES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/${m}.html)
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.adoc)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS_HTML} ${ms}
    DEPENDS ${ms}
    WORKING_DIRECTORY .
    COMMENT "Building HTML ${mf}"
    VERBATIM)
  list(APPEND HTML_FILES ${mf})

  # Install the docs
  install (FILES
    ${mf}
    DESTINATION share/endurox/doc/html)

endforeach()
add_custom_target(html ALL DEPENDS ${HTML_FILES})


add_subdirectory (manpage)
# end.


