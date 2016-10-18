#!/usr/bin/perl

#
# @(#) Script is used to build Object API for XATMI and UBF
#

use Getopt::Std;
#use Regexp::Common;

our ($opt_i, $opt_o, $opt_h);
$opt_i = 0; # Input file
$opt_o = 0; # output name
$opt_h = 0; # Help
my $opt_ret = getopts ('i:o:h');

if (!$opt_ret || $opt_h || !$opt_i || !$opt_o)
{
        print STDERR "Usage: $0 -i <input file, c header> -o <output prefix>\n ";
        exit 1;
}

my $M_name_upper = uc $opt_o;
$M_name_upper = "O$M_name_upper";
$M_name = "o$opt_o";

#
# Remove whitespace from string
#
sub remove_white_space {

    my $ret = shift;
    
    
    $ret =~ s/^\s+//;
    $ret =~ s/\s+$//;

    return $ret;
}

#http://stackoverflow.com/questions/5049358/split-on-comma-but-only-when-not-in-parenthesis
sub split_by_comma_but_not_parenthesis {

    my $string=shift;
    my @array = ($string =~ /(
    [^,]*\([^)]*\)   # comma inside parens is part of the word
    |
    [^,]*)           # split on comma outside parens
    (?:,|$)/gx);
    
    return @array;
}

################################################################################
# Object API output file handling
################################################################################



#
# Have some file handers, one for header
# and one for c
#


################################################################################
# Open C header
################################################################################
sub open_h {

    $M_h_name = "${M_name}.h";
    print "Opening [$M_h_name]\n";
    open($M_h_fd, '>', $M_h_name) or die "Could not open file '$M_h_name' $!";

    my $title = "";
    my $defs = "";

    if ($M_name=~/^oatmisrv$/)
    {
        $title = "ATMI Server level Object API header (auto-generated)";
    }
    if ($M_name=~/^oatmi$/)
    {
        $title = "ATMI Object API header (auto-generated)";

        $defs = "\n#define Otperrno(P_CTXT) (*O_exget_tperrno_addr(P_CTXT))\n".
                "#define Otpurcode(P_CTXT) (*O_exget_tpurcode_addr(P_CTXT))";

    }
    elsif($M_name=~/oubf/)
    {
        $title = "UBF Object API header (auto-generated)";

        $defs = "\n#define OBerror(P_CTXT)   (*O_Bget_Ferror_addr(P_CTXT))";
    }
    
my $message = <<"END_MESSAGE";
/* 
** $title
**
** \@file ${M_name}.h
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact\@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#ifndef __${M_name_upper}_H
#define __${M_name_upper}_H

#ifdef  __cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <stdint.h>
#include <ubf.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/${defs}
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
END_MESSAGE
    
print $M_h_fd $message;

}

################################################################################
# Open C source file
################################################################################
sub open_c {


    $M_c_name = "${M_name}.c";
    print "Opening [$M_c_name]\n";
    open($M_c_fd, '>', $M_c_name) or die "Could not open file '$M_c_name' $!";

    my $title = "";

    if ($M_name=~/^oatmisrv$/)
    {
        $title = "ATMI Server Level Object API code (auto-generated)";
    }
    if ($M_name=~/^oatmi$/)
    {
        $title = "ATMI Object API code (auto-generated)";
    }
    elsif($M_name=~/oubf/)
    {
        $title = "UBF Object API code (auto-generated)";
    }
my $message = <<"END_MESSAGE";
/* 
** $title
**
** @file ${M_name}.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact\@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
END_MESSAGE

print $M_c_fd $message;

}


################################################################################
# Close C header
################################################################################
sub close_h {
$message = <<"END_MESSAGE";
#endif  /* __${M_name_upper}_H */

END_MESSAGE

print $M_h_fd $message;

close($M_h_fd)

}

################################################################################
# Close C source file
################################################################################
sub close_c {

$message = <<"END_MESSAGE";

END_MESSAGE

print $M_c_fd $message;

close($M_c_fd)

    
}


################################################################################
# Generate Object API func signature
################################################################################
sub gen_sig {

    my $func_type = shift;
    my $func_name = shift;
    my $func_args_list = shift;
    
    if ($func_args_list=~/^void$/)
    {
        return "$func_type O$func_name(TPCONTEXT_T *p_ctxt)";
    }
    else
    {
        return "$func_type O$func_name(TPCONTEXT_T *p_ctxt, $func_args_list)";
    }
}

################################################################################
# Write C object API header func
################################################################################
sub write_h {
    my $sig = shift;
    print $M_h_fd "extern NDRX_API $sig;\n";
}

################################################################################
# Write C object API function
################################################################################
sub write_c {
    my $func_type = $_[0];
    my $func_name = $_[1];
    my $sig = $_[2];
    my $func_args_list = $_[3];
    
    my @func_arg_type = @{$_[4]};
    my @func_arg_name = @{$_[5]};
    my @func_arg_def = @{$_[6]};
    
    
    #
    # Generate function call
    #
    my $invoke = "$func_name(";
    my $first = 1;
    
    if ($func_args_list!~m/^void$/)
    {
        # Generate from arguments list;
        foreach my $name ( @func_arg_name )
        {
            if ($first)
            {
                $first = 0;
            }
            else
            {
                $invoke = "$invoke, ";
            }
            
            $invoke = "$invoke$name";
        }
    }
    
    $invoke = "$invoke)";
    
    my $M_priv_flags = "";
    #
    # Calculate the flags, all UBF operations receive only UBF & TLS contexts
    # ATMI have ATMI too, plus somes have a TRAN
    #
    if ($M_name=~/^oatmi$/ 
        || $M_name=~/^oatmisrv$/
        )
    {
        if ($func_name=~/^tpcall$/ 
            ||$func_name=~/^tpacall$/
            ||$func_name=~/^tpgetrply$/
            ||$func_name=~/^tpconnect$/
            ||$func_name=~/^tpdiscon$/
            ||$func_name=~/^tpsend$/
            ||$func_name=~/^tprecv$/
            ||$func_name=~/^tpcancel$/
            ||$func_name=~/^tpreturn$/
            ||$func_name=~/^tpgetlev$/
            ||$func_name=~/^tpabort$/
            ||$func_name=~/^tpbegin$/
            ||$func_name=~/^tpcommit$/
            ||$func_name=~/^tpopen$/
            ||$func_name=~/^tpclose$/
            ||$func_name=~/^tppost$/
            ||$func_name=~/^tpenqueue$/
            ||$func_name=~/^tpdequeue$/
            ||$func_name=~/^tpenqueueex$/
            ||$func_name=~/^tpdequeueex$/
            ||$func_name=~/^tpgetctxt$/
            ||$func_name=~/^tpsetctxt$/
            ||$func_name=~/^tpfreectxt$/
            ||$func_name=~/^tpterm$/
        )
        {
            $M_priv_flags = "CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN";
        }
        else
        {
            $M_priv_flags = "CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN";
        }
        
    }
    elsif($M_name=~/oubf/)
    {
        $M_priv_flags = "CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN";
    }
    
    
    if ($func_type=~m/^int$/ 
        || $func_type=~m/^BFLDOCC$/
        || $func_type=~m/^BFLDID$/
        || $func_type=~m/^long$/
        )
    {
################################################################################
# integral type function
################################################################################
$message = <<"END_MESSAGE";

/**
 * Object-API wrapper for $func_name() - Auto generated.
 */
public $sig 
{
    $func_type ret = SUCCEED;
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        $M_priv_flags))
    {
        userlog("ERROR! $func_name() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = $invoke;

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
        $M_priv_flags))
    {
        userlog("ERROR! $func_name() failed to get context");
        FAIL_OUT(ret);
    }
out:    
    return ret; 
}

END_MESSAGE
################################################################################
    }
    elsif ($func_type=~m/^void$/)
    {
################################################################################
# Void function
################################################################################
$message = <<"END_MESSAGE";
/**
 * Object-API wrapper for $func_name() - Auto generated.
 */
public $sig 
{
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        $M_priv_flags))
    {
        userlog("ERROR! $func_name() failed to set context");
    }
    
    $invoke;

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        $M_priv_flags))
    {
        userlog("ERROR! $func_name() failed to get context");
    }
out:    
    return;
}

END_MESSAGE
################################################################################
    }
    elsif ($func_type=~m/^long \*$/ 
        || $func_type=~m/^char \*$/
        || $func_type=~m/^UBFH \*$/
        || $func_type=~m/^int \*$/
        )
    {
################################################################################
# pointer function
################################################################################
$message = <<"END_MESSAGE";

/**
 * Object-API wrapper for $func_name() - Auto generated.
 */
public $sig 
{
    $func_type ret = NULL;
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        $M_priv_flags))
    {
        userlog("ERROR! $func_name() failed to set context");
        ret = NULL;
        goto out;
    }
    
    ret = $invoke;

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        $M_priv_flags))
    {
        userlog("ERROR! $func_name() failed to get context");
        ret = NULL;
        goto out;
    }
out:    
    return ret; 
}

END_MESSAGE
################################################################################
    }
    else
    {
        # return if unsupported
        print "!!! Unsupported return type [$func_type]\n";
        return;
    }
    
    print $M_c_fd $message;
    
}


open_h();
open_c();
################################################################################
# Open source file
################################################################################
my $file = $opt_i;
open my $info, $file or die "Could not open $file: $!";

NEXT: while( my $line = <$info>)
{   
    chomp $line;
    
    if ($line =~ m/extern NDRX_API.*\(.*/)
    {
        # read next line, if line not completed
        if ($line !~ m/\);\s*/)
        {
            my $next_line = <$info>;
            chomp $next_line;

            $next_line =~ s/^\s+//;
            $next_line =~ s/\s+$//;

            $line = $line.$next_line;
        }

        #Strip off any comments found
        $line =~ s/(\/\*.*\*\/)//gi;

        # Now get the function return type, name and arguments list
        my ($func_type, $func_name, $func_args_list) = 
                    ($line=~m/^extern NDRX_API\s*([A-Za-z0-9_]+\s*\**)\s*([A-Za-z0-9_]+)\s*\((.*)\)\s*;/);


        $func_type = remove_white_space($func_type);
        $func_name = remove_white_space($func_name);
        $func_args_list = remove_white_space($func_args_list);

        print "Processing line [$line] M_name = [$M_name]\n";

        #
        # Skip the specific symbols, per output module
        #

        if ($M_name =~ m/^oatmisrv$/)
        {
            # Include only server commands for ATMISRV level
            if ($func_name !~ m/tpreturn/
                && $func_name !~ m/tpforward/
                && $func_name !~ m/tpadvertise_full/
                && $func_name !~ m/tpunadvertise/

                && $func_name !~ m/tpext_addpollerfd/
                && $func_name !~ m/tpext_delpollerfd/
                && $func_name !~ m/tpext_addb4pollcb/
                && $func_name !~ m/tpsrvsetctxdata/
                && $func_name !~ m/tpext_addperiodcb/
                && $func_name !~ m/ndrx_main/ 
                && $func_name !~ m/ndrx_main_integra/ 

                )
            {
                print "skip - next\n";
                next NEXT;
            }
        }
        elsif ($M_name =~ m/^oatmi$/)
        {
            # Skip the names from ATMI level
            if ($line =~ m/G_tpsvrinit__/
                || $line =~ m/G_tpsvrdone__/ 
                || $func_name =~ m/ndrx_main/ 
                || $func_name =~ m/ndrx_main_integra/ 
                || $func_name =~ m/ndrx_atmi_tls_get/
                || $func_name =~ m/ndrx_atmi_tls_set/
                || $func_name =~ m/ndrx_atmi_tls_free/
                || $func_name =~ m/ndrx_atmi_tls_new/
                # Skip the server stuff
                || $func_name =~ m/tpreturn/
                || $func_name =~ m/tpforward/
                || $func_name =~ m/tpadvertise_full/
                || $func_name =~ m/tpunadvertise/
                || $func_name =~ m/tpservice/
                || $func_name =~ m/tpext_addpollerfd/
                || $func_name =~ m/tpext_delpollerfd/
                || $func_name =~ m/tpext_addb4pollcb/
                || $func_name =~ m/tpsrvsetctxdata/
                || $func_name =~ m/tpext_addperiodcb/
                || $func_name =~ m/tpsvrinit/
                || $func_name =~ m/tpgetsrvid/
                || $func_name =~ m/tpsrvgetctxdata/
                || $func_name =~ m/tpext_delb4pollcb/
                || $func_name =~ m/tpsvrdone/
                || $func_name =~ m/tpcontinue/
                || $func_name =~ m/tpext_delperiodcb/
                )
            {
                print "skip - next\n";
                next NEXT;
            }
        }
        elsif ($M_name =~ m/^oubf$/)
        {
            # Skip some bits from UBF level
            if ($func_name =~ m/^ndrx_ubf_tls_get$/
                && $func_name =~ m/^ndrx_ubf_tls_set$/
                && $func_name =~ m/^ndrx_ubf_tls_free$/
                && $func_name =~ m/^ndrx_ubf_tls_new$/
                )
            {
                print "skip - next\n";
                next NEXT;
            }
        }


        # Process next, parse arguments and their types
        print "func_type      = [$func_type]\n";
        print "func_name      = [$func_name]\n";
        print "func_args_list = [$func_args_list]\n";

        my @func_arg_type = ();
        my @func_arg_name = ();
        my @func_arg_def = (); # in case if pointer to function used...

        #
        # Fix callbacks with commas
        #
        #my @args = split(/\,/, $func_args_list);
        my @args = split_by_comma_but_not_parenthesis($func_args_list);

        if ($func_args_list=~m/^void$/)
        {
            push @func_arg_type, "void";
        }
        else
        {
            # Extract the data type and ar

            SKIP_TYPE: foreach my $pair ( @args )
            {
                $pair = remove_white_space($pair);

                my $type = "";
                my $name = "";
                my $sign = "";
                my $def = $pair;


                if ($pair=~m/\(.*\)\(.*\)/)
                {
                    print "Got ptr to function...\n";
                    ($name) = ($pair=~m/\(\s*\*(.*)\)\(.*\)/);
                }
                else
                {
                    print "Normal type...\n";
                    ($sign, $type, $name) = 
                        ($pair=~m/^\s*(unsigned\s*)?([A-Za-z0-9_]+\s*\**)\s*([A-Za-z0-9_]+)/);

                    $sign = remove_white_space($sign);
                    $type = remove_white_space($type);
                    $name = remove_white_space($name);

                    if ($sign!~m/^$/)
                    {
                        $type = "$sign $type";
                    }
                }

                #
                # Seems like split_by_comma_but_not_parenthesis()
                # generates last block empty.
                #
                if ($name=~m/^$/)
                {
                    next SKIP_TYPE;
                }

                push @func_arg_def, $pair;

                print "got param (pair: [$pair]), type: [$type], name: [$name], def: [$def]\n";
                push @func_arg_type, $type;
                push @func_arg_name, $name; 
            }
        }
        
        # Generate signature
        my $sig = gen_sig($func_type, $func_name, $func_args_list);
        
        # Write header;
        write_h($sig);
        
        # write func out
        write_c($func_type, $func_name, $sig, $func_args_list, 
            \@func_arg_type, \@func_arg_name, \@func_arg_def);
    }
}

close $info;

close_h();
close_c();


