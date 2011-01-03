#!/usr/bin/perl -W

use strict;

#-------------------------------------------------------------------------------
sub usage
{
	print "Usage: ".__FILE__." <file1> [<file2> ...]\n";
	exit 0;
}

#-------------------------------------------------------------------------------
sub dot_product
{
	my $a = $_[0];
	my $b = $_[1];

	my $result = 0;

	for (my $i = 0; $i < @{$a}; $i++) {
		$result += $$a[$i] * $$b[$i];
# 		print "$i: \$result = $result\n";
	}

	return $result;
}


#-------------------------------------------------------------------------------

if (@ARGV < 1) {
	usage();
}

# my @vectors = ();
# my @types = ();
my @unchanged = ();

foreach my $file(@ARGV) {
	if (! -r $file) {
		print STDERR "File does not exist: $file\n";
		next;
	}

	if (!open(FILE, $file)) {
		print STDERR "Error opening file: $file\n";
		next;
	}

	my $line = 0;
	my $count = 0;
	my @prev_vec = ();
	my $len_vec = 0;
	my $len_prev = 0;
	
	while (<FILE>) {
		# Ignore comments and emtpy lines
		next if (/^#/ || /^\s*$/);
		my @vec = split;
		next if (@vec == 0);

		# The first row contains the type IDs
		if ($line++ == 0) {
# 			@types = @vec;
		}
		else {
			$len_vec = sqrt(dot_product(\@vec, \@vec));
			
			if ($count > 0) {
				my $dotp = dot_product(\@vec, \@prev_vec);
				my $cos = "-1";
				
				if ($len_vec && $len_prev) {
					$cos = sprintf "%.5f", $dotp / ($len_vec * $len_prev);
				}
				printf "%2d -> %2d: dot product = %7d, cosine = %s\n", $count, $count + 1, $dotp, $cos;
			}
			
			@prev_vec = @vec;
			$len_prev = $len_vec;
			
			$count++;
		}
	}
	close(FILE);

	
}
