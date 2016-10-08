#!/usr/bin/perl

#
# @(#) Script is used to build Object API for XATMI and UBF
#

use Getopt::Std;


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


#
# Remove whitespace from string
#
sub remove_white_space  {

	my $ret = shift;
	
	
	$ret =~ s/^\s+//;
	$ret =~ s/\s+$//;

	return $ret;
}

################################################################################
# Object API output file handling
################################################################################

sub open_h {

	$message = "UNK!";

	if ($opt_o =~ /xatmi/)
	{
	
$message = <<'END_MESSAGE';

END_MESSAGE

	}
	elsif ($opt_o =~/ubf/)
	{
	$message = <<'END_MESSAGE';
		Dear $name,
		
		this is a message I plan to send to you.
		
		regards
		the Perl Maven
		END_MESSAGE
	}
	
}

sub open_c {
	
}

sub write_h {
	
}

sub write_c {
	
}

sub close_h {
	
}

sub close_c {
	
}

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
	
	print "Processing line [$line]\n";
	
	#
	# Skip the specific symbols
	#
	
	if ($line =~ m/G_tpsvrinit__/
		|| $line =~ m/G_tpsvrdone__/ 
		|| $line =~ m/ndrx_main/ 
		|| $line =~ m/ndrx_main_integra/ 
		|| $line =~ m/ndrx_atmi_tls_get/
		|| $line =~ m/ndrx_atmi_tls_set/
		|| $line =~ m/ndrx_atmi_tls_free/
		|| $line =~ m/ndrx_atmi_tls_new/
		)
	{
		print "skip - next\n";
		next NEXT;
	}
	
	# Process next, parse arguments and their types
	print "func_type      = [$func_type]\n";
	print "func_name      = [$func_name]\n";
	print "func_args_list = [$func_args_list]\n";
	
	my @func_arg_type = ();
	my @func_arg_name = ();
	my @func_arg_def = (); # in case if pointer to function used...
	
	my @args = split(/\,/, $func_args_list);
	
	if ($func_args_list=~m/^void$/)
	{
		push @func_arg_type, "void";
	}
	else
	{
		# Extract the data type and ar
		
		foreach my $pair ( @args )
		{
			$pair = remove_white_space($pair);
			
			my $type = "";
			my $name = "";
			my $def = $pair;
			
			
			if ($pair=~m/\(.*\)\(.*\)/)
			{
				print "Got ptr to function...\n";
				($name) = ($pair=~m/\(\s*\*(.*)\)\(.*\)/);
			}
			else
			{
				print "Normal type...\n";
				($type, $name) = 
					($pair=~m/^\s*([A-Za-z0-9_]+\s*\**)\s*([A-Za-z0-9_]+)/);
				
				$type = remove_white_space($type);
				$name = remove_white_space($name);
			}
			
			push @func_arg_def, $pair;
			
			print "got param (pair: [$pair]), type: [$type], name: [$name], def: [$def]\n";
			push @func_arg_type, $type;
			push @func_arg_name, $name;
				
		}
	}
    }
}

close $info;



