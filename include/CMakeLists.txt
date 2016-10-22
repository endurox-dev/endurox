## 
## Makefile for header file for distribution
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

cmake_minimum_required (VERSION 2.8)

install (FILES
    userlog.h
    fml.h
    fml32.h
    ubf.h
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
    oubf.h
    odebug.h
    ondebug.h
    onerror.h
    oatmi.h
    oatmisrv.h
    Exfields.h
    DESTINATION include)

install (FILES
    Exfields
    DESTINATION share/endurox/ubftab)
