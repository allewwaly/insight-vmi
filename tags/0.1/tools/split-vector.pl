#!/usr/bin/perl -W

use strict;

#-------------------------------------------------------------------------------
sub usage
{
	print "Usage: ".__FILE__." <file1> [<file2> ...]\n";
	exit 0;
}

#-------------------------------------------------------------------------------

if (@ARGV < 1) {
	usage();
}

foreach my $file(@ARGV) {

	print "Processing file $file\n";

	if (! -r $file) {
		print STDERR "File does not exist: $file\n";
		next;
	}

	if (!open(FILE, $file)) {
		print STDERR "Error opening file: $file\n";
		next;
	}

	my $outfile1 = $file."1";
	my $outfile2 = $file."2";
	if (!open(OUT1, ">$outfile1")) {
		print STDERR "Error writing file: $outfile1\n";
		next;
	}
	if (!open(OUT2, ">$outfile2")) {
		print STDERR "Error writing file: $outfile2\n";
		next;
	}

	my $line = 0;
	my $count = 0;

	while (<FILE>) {
		# Ignore comments and emtpy lines
		if (/^#/ || /^\s*$/) {
			printf OUT1 $_;
			printf OUT2 $_;
			next;
		}
		my @vec = split;
		if (@vec == 0) {
			print OUT1 $_;
			print OUT2 $_;
		}

		# The first row contains the type IDs
		if ($line++ == 0) {
			print OUT1 $_;
			print OUT2 $_;
		}
		else {
			my $i = 0;
			while ($i < @vec) {
				print OUT1 $vec[$i++] . " ";
				print OUT2 $vec[$i++] . " ";
			}
			print OUT1 "\n";
			print OUT2 "\n";
			$count++;
		}
	}
	close(FILE);
	close(OUT1);
	close(OUT2);

	print "Wrote file $outfile1\n";
	print "Wrote file $outfile2\n";
}
