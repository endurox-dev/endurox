##
## @brief Platform script build support module
##
## @file pscript.cmake
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

cmake_minimum_required (VERSION 3.1) 

#
# Embed platform script
# 
# @param source_file full name of platform script file
# @param output_prefix output file name without extension. finally will gate suffix .c
#
macro(pscript_embed source_file output_prefix)


add_custom_command(
  OUTPUT ${output_prefix}.c
  COMMAND pscript -c -o ${output_prefix}.cnut ${source_file}
  COMMAND exembedfile ${output_prefix}.cnut ${output_prefix}
  COMMAND rm ${output_prefix}.cnut
  DEPENDS ${source_file})

endmacro(pscript_embed source_file)

# vim: set ts=4 sw=4 et smartindent:
