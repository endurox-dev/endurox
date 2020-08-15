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

set(PDF_NAMES ex_devguide ex_overview ex_adminman building_guide 
                       getting_started_tutorial persistent_message_queues_overview
                        macos_notes aix_notes solaris_notes freebsd_notes
						benchmark third_party_licences)
set(HTML_NAMES ex_devguide ex_overview ex_adminman building_guide
                       getting_started_tutorial persistent_message_queues_overview
                        macos_notes aix_notes solaris_notes freebsd_notes
						benchmark third_party_licences)

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
    DESTINATION share/endurox/doc/html/guides)

endforeach()
add_custom_target(html ALL DEPENDS ${HTML_FILES})


install (FILES
    class_diagram.png
    cluster_detail.png
    cluster_links_center.png
    cluster_links_mix.png
    cluster_links_nocentre.png
    common_config.png
    endurox_overview.png
    epoll_message_dispatch_mode.png
    event_processing.png
    exforward_forward.png
    exforward_tpcall.png
    module_dependency.png
    netxms_avg_rsp.png
    netxms_clients_list.png
    netxms_new_dci2.png
    netxms_new_dci.png
    netxms_qsize.png
    netxms_service_chk.png
    netxms_succ.png
    netxms_tps.png
    netxms_tps_transf.png
    poll_message_dispatch_mode.png
    reqlogging.png
    rt-patching.png
    server_monitoring_and_recovery.png
    systemv_message_dispatch_mode.png
    tmqinternals.png
    tpcache.png
    tpenqueue_auto.png
    tpenqueue_tpdequeue.png
    tpnotify.png
    transaction_groups.png
    endurox-load-balance.jpg
    Endurox-product.jpg
    excache.jpg
    benchmark/01_tpcall.png
    benchmark/02_tpcall_network.png
    benchmark/03_tpcall_threads.png
    benchmark/04_tpacall.png
    benchmark/05_persistent_storage.png
    benchmark/06_tpcache.png
  DESTINATION share/endurox/doc/html/guides
  OPTIONAL)

install (FILES
    benchmark/01_tpcall.png
    benchmark/02_tpcall_network.png
    benchmark/03_tpcall_threads.png
    benchmark/04_tpacall.png
    benchmark/05_persistent_storage.png
    benchmark/06_tpcache.png
  DESTINATION share/endurox/doc/html/guides/benchmark
  OPTIONAL)

install (FILES
  ${CMAKE_CURRENT_BINARY_DIR}/docbook-xsl.css
  DESTINATION share/endurox/doc/html/guides
  OPTIONAL)

# install images 
add_subdirectory (manpage)
add_subdirectory (api)
#add_subdirectory (doxygen)
# end.


# vim: set ts=4 sw=4 et smartindent:
