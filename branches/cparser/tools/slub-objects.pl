#!/usr/bin/perl -w

use strict;

my %caches;

while (<>) {
    next if (! /TRACE ([^ ]+) (alloc|free) (0x[0-9a-f]+) inuse/);
    
    if ($2 eq "alloc") {
        $caches{$1}{$3}++;
    }
    else {
        $caches{$1}{$3}--;
    }
}

foreach my $id(keys %caches) {
    my $cnt = 0;
    foreach my $addr(sort keys %{$caches{$id}}) { 
        if ($caches{$id}{$addr} == 1) {
            print "$id $addr\n";
            $cnt++;
        }
        elsif ($caches{$id}{$addr} != 0) {
            print "ERROR: $id $addr => ".$caches{$id}{$addr}."\n";
        }
    }
    
    if ($cnt > 0) {
        print "# Total $id objects: $cnt\n";
      print "# ------------------------------------------------\n";
    }
}
