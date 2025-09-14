#!/usr/bin/perl
use strict;
use warnings;
use Text::CSV;
my $file = shift || "results.csv";
open my $fh, '<', $file or die "open $file: $!";
my $csv = Text::CSV->new({binary=>1});
my @rows;
while (my $row = $csv->getline($fh)) {
    push @rows, $row;
}
close $fh;
print "Parsed ", scalar(@rows), " rows\n";
for my $r (@rows) {
    print join(", ", @$r), "\n";
}