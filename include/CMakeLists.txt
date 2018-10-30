##
## @brief Makefile for header file for distribution
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

cmake_minimum_required (VERSION 2.8)

install (FILES
    userlog.h
    fml.h
    fml32.h
    ubf.h
    ubfutil.h
    atmi.h
    xatmi.h
    pscript.h
    exparson.h
    ndebug.h
    ndebugcmn.h
    ndrstandard.h
    nstdutil.h
    nstd_tls.h
    thlock.h
    exhash.h
    cconfig.h 
    inicfg.h
    nerror.h
    xa.h
    ndrx_config.h
    sys_unix.h
    sys_mqueue.h 
    sys_emqueue.h
    oubf.h
    odebug.h
    ondebug.h
    onerror.h
    oatmisrv_integra.h
    oatmi.h
    oatmisrv.h
    Exfields.h
    Excompat.h
    tpadm.h
    evt_mib.h
    view_cmn.h
    exdb.h
    nstopwatch.h
    DESTINATION include)

install (FILES
    Exfields
    Excompat
    DESTINATION share/endurox/ubftab)
# vim: set ts=4 sw=4 et smartindent:
