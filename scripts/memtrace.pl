#!/usr/bin/perl

#
# @(#) Trace Enduro/X memroy allocation
#


#
# Track down the stuff
#
%M_allocs = ();

#
# Read from file or stdin.
#
my $file = shift @ARGV;

my $ifh;
my $is_stdin = 0;
if (defined $file){
	open $ifh, "<", $file or die $!;
} 
else
{
	$ifh = *STDIN;
	$is_stdin++;
}

while (<$ifh>){
	# Process
	my $line = $_;
	
	
	#print $line;
	
	chomp $line;
	
	if ($line =~/\<\= malloc/ || $line =~/\<\= calloc/ || $line =~/\<\= fopen/)
	{
		# 4465:20161111:19454501:server      :[0xefbcc0] <= malloc(size=100):Balloc /home/mvitolin/projects/endurox/libubf/ubf.c:1217
		 my ($addr) = ($line=~m/\[(0x.*)\]/);
		 
		 $M_allocs{$addr} = $line;
	}
	elsif ($line =~/\=\> free/ || $line =~/\=\> fclose/)
	{
		#  4465:20161111:19454501:server      :[0xebd100] => free(ptr=0xebd100):_tpfree /home/mvitolin/projects/endurox/libatmi/typed_buf.c:353
		
		my ($addr) = ($line=~m/\[(0x.*)\]/);
		 
		if ($M_allocs{$addr})
		{
			delete $M_allocs{$addr};
		}
		else
		{
			print "!!! ERROR Double free: [$line]\n";
		}
	}
	elsif ($line =~ /\<\= realloc/)
	{
		#  4472:20161111:19455456:client      :[0x7faa743ba600] <= realloc(ptr=[0x7faa743ba600], size=128):Brealloc /home/mvitolin/projects/endurox/libubf/ubf.c:1273
		my ($addr, $addr_old) = ($line=~m/\[(0x.*)\].*\[(0x.*)\]/);
		
		if ($M_allocs{$addr_old})
		{
			delete $M_allocs{$addr_old};
		}
		else
		{
			print "!!! ERROR Double free (for realloc): [$line]\n";
		}
		
		$M_allocs{$addr} = $line;
	}
}

# Print the stuff in 

foreach my $key (keys %M_allocs)
{
	my $line = $M_allocs{$key};
	print "!!! WARNING Still in memory: [$line]\n";
}

# Close the file
close $ifh unless $is_stdin;


