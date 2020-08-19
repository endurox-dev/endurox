##
## @brief Build documentation, XATMI API
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

set(MAN3_NAMES 
        tpcall
        tpcancel
        tpconnect
        tpreturn
        tpforward
        tpdiscon
        tprecv
        tpsend
        tpservice
        tpbegin
        tpalloc
        tprealloc
        tpfree
        tptypes
        tpadvertise
        tpunadvertise
        tpgetrply
        tpabort
        tpcommit
        tpsuspend
        tpresume
        tpopen
        tpgetlev
        tpstrerror
        tpsubscribe
        tpunsubscribe
        tppost
        tpsrvgetctxdata 
        tpsrvsetctxdata
        tpsrvfreectxdata
        tpgetctxt
        tpsetctxt
        tpnewctxt
        tpfreectxt
        tpcontinue
        tpgetnodeid
        tpclose
        tpinit
        tpterm
        tpsvrdone
        tpsvrinit
        tpext_addpollerfd
        tpext_delpollerfd
        tpext_addperiodcb
        tpext_delperiodcb
        tpext_addb4pollcb
        tpext_delb4pollcb
        tpgetsrvid
        tpjsontoubf
        tpubftojson
        tpenqueue
        tpdequeue
        ndrx_main
        ndrx_main_integra
        tpacall
        tplogdumpdiff
        tplogdump
        tplog
        tploggetreqfile
        tplogconfig
        tplogclosereqfile
        tplogclosethread
        tplogsetreqfile_direct
        tplogsetreqfile
        tploggetbufreqfile
        tplogdelbufreqfile
        tplogprintubf
        tplogqinfo
        tpisautobuf
        tpsetunsol
        tpnotify
        tpbroadcast
        tpchkunsol
        tpviewtojson
        tpjsontoview
        tpconvert
        tpurcode
        tptoutset
        tpimport
        tpexport
        ndrx_fork
        ndrx_atfork_prepare
        ndrx_atfork_parent
        ndrx_atfork_child
        tuxgetenv
        tx_begin
        tx_close
        tx_info
        tx_commit
        tx_rollback
        tx_open
        tx_set_commit_return
        tx_set_transaction_control
        tx_set_transaction_timeout
        _tmstartserver
        tpgetconn
        tpscmt
        tpsvrthrinit
        tpsvrthrdone
        tpencrypt
        tpdecrypt
)
set(HTML_MAN_NAMES ${MAN3_NAMES})

# Generate MAN3
set(A2X_OPTS_MAN3 --format=manpage -D man3)
set(MAN3_FILES)
foreach(m IN LISTS MAN3_NAMES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/man3/${m}.3)
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.adoc)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS_MAN3} ${ms}
    DEPENDS ${ms}
    WORKING_DIRECTORY .
    COMMENT "Building MAN 3 ${mf}"
    VERBATIM)
  list(APPEND MAN3_FILES ${mf})
  # Install the docs
  install (FILES
    ${mf}
    DESTINATION share/man/man3)

endforeach()
add_custom_target(man3_xatmi ALL DEPENDS ${MAN3_FILES})

set(A2X_OPTS_XHTML_MAN -f xhtml -D html)
set(HTML_FILES)
foreach(m IN LISTS HTML_MAN_NAMES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/html/${m}.html)
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.adoc)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS_XHTML_MAN} ${ms}
    DEPENDS ${ms}
    WORKING_DIRECTORY .
    COMMENT "Building HTML_MAN ${mf}"
    VERBATIM)
  list(APPEND HTML_FILES ${mf})

  # Install the docs
  install (FILES
    ${mf}
    DESTINATION share/endurox/doc/html/api/xatmi)

endforeach()

install (FILES
  ${CMAKE_CURRENT_BINARY_DIR}/html/docbook-xsl.css
  DESTINATION share/endurox/doc/html/api/xatmi
  OPTIONAL)

add_custom_target(html_man3_xatmi ALL DEPENDS ${HTML_FILES})

# end.


# vim: set ts=4 sw=4 et smartindent:
