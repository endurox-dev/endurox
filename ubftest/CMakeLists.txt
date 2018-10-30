##
## @brief Enduro Execution platform's UBF library tests
##   Using Cgreen framework for tests.
##
## @file CMakeLists.txt
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL or Mavimax's license for commercial use.
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


# Make sure the compiler can find include files from UBF library
include_directories (${ENDUROX_SOURCE_DIR}/libubf
					 ${ENDUROX_SOURCE_DIR}/include
					 ${ENDUROX_SOURCE_DIR}/libnstd)

# Make sure the linker can find the UBF library once it is built.
link_directories (${ENDUROX_BINARY_DIR}/libubf) 

# Add debug options
# By default if RELEASE_BUILD is not defined, then we run in debug!
IF ($ENV{RELEASE_BUILD})
	# do nothing
ELSE ($ENV{RELEASE_BUILD})
	ADD_DEFINITIONS("-D UBF_DEBUG")
ENDIF ($ENV{RELEASE_BUILD})

# Exectuables
add_executable (ubfunit1 ubfunit1.c test_badd.c test_genbuf.c test_cbchg.c 
                test_cbget.c test_bdel.c test_expr.c test_bnext.c test_bproj.c
                test_mem.c test_bupdate.c test_bconcat.c test_find.c test_get.c
                test_print.c test_macro.c test_readwrite.c test_mkfldhdr.c
                test_nstd_crypto.c test_nstd_b64.c
                test_nstd_mtest.c test_nstd_mtest2.c test_nstd_mtest3.c
                test_nstd_mtest4.c test_nstd_mtest5.c test_nstd_mtest6_dupcursor.c
                test_bcmp.c test_nstd_macros.c test_nstd_debug.c test_nstd_growlist.c
                test_nstd_standard.c)

add_executable (testedbsync test_nstd_msync.c)

# Link the executable to the UBF library & others...

target_link_libraries (ubfunit1 ubf cgreen m nstd ${RT_LIB} pthread)
target_link_libraries (testedbsync ubf cgreen m nstd ${RT_LIB} pthread)

set_target_properties(ubfunit1 PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")
set_target_properties(testedbsync PROPERTIES LINK_FLAGS "$ENV{MYLDFLAGS}")

# vim: set ts=4 sw=4 et smartindent:
