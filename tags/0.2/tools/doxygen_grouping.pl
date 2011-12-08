#!/usr/bin/perl -W
#
# Author: chrschn
#
# This script is used by Doxygen to automatically group souce files into
# modules "insight", "insightd" and "libinsight". It expects the path to a
# single source file name as argument and outputs the file verbatim in between
# doxygen commands as follows:
#
# /** \addtogroup <module> <module>
#  @} */
# <file contents>
# /** @} */
#
# For more information about the grouping function of Doxygen, see
# http://www.stack.nl/~dimitri/doxygen/grouping.html.
#

use strict;
use Cwd;
use Cwd 'abs_path';

sub process_file
{
    my $module = shift;
    my $file = shift;

    # Process file
    open IN, $file || die("Error opening file $file\n");
    print "/** \\addtogroup $module $module\n \@{ */ ";
    while (<IN>) {
        print $_;
    }
    print "/** \@} */ \n";
    close IN;
}

# Check the arguments
if (@ARGV != 1 || ! -f $ARGV[0]) {
    printf STDERR "Usage: $0 <srcfile>\n";
    exit;
}

# Find absolute source path
my $src_root = abs_path(getcwd()."/../");

# Process supplied file(s)
foreach my $file (@ARGV) {
    if (! -f $file) {
        printf STDERR "Warning: \"$file\" not a regular file, skipping!\n";
        next;
    }

    # Strip the source root directory from absolute path
    $file = abs_path($file);
    $file =~ /^$src_root\/*([^\/]+)\//;
    my $module = $1;

    process_file($module, $file);
}
