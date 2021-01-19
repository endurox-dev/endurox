#!/usr/bin/perl


################################################################################
# @(#) Generate license texts
# Run manually once new licese is added
################################################################################

use strict;
use warnings;

my @files = glob("*.txt");

my $outfile="../doc/third_party_licences.adoc";

# here i 'open' the file, saying i want to write to it with the '>>' symbol
open (FILE, "> $outfile") || die "problem opening $outfile\n";


print FILE <<EOS;
Third party library licenses used by Enduro/X
=============================================

:sectnums:
:chapter-label:
:doctype: book

EOS

#
# Loop over the license texts 
#
foreach my $file (@files)
{

	# Parse the file name
	# mask: <module>_<lic_type>.txt
	# module may contain _, which later needs to be replaced with
	# spaces
	
	my ($module, $ltype) = ($file=~m/^(.*)_([^_]+)\.txt$/);
	$module =~ s/_/ /g;
	
	
	# convert the string to lowercase
	$ltype = lc $ltype;
	$ltype = ucfirst $ltype;
	
	#print "$file module [$module] type [$ltype]\n";
	
	
	print FILE "== $ltype for \"$module\" library\n\n";
	
	print FILE "--------------------------------------------------------------------------------\n\n";
	open(READH, '<', $file)
		or die "Could not open file '$file' $!";
	
	while (my $row = <READH>) 
	{
		chomp $row; 
		chop($row) if ($row =~ m/\r$/);
		print FILE "$row\n";
	}
	
	print FILE "\n";

	close(READH);
	
	print FILE "\n--------------------------------------------------------------------------------\n\n";
	
}

close(FILE);

exit 0;
