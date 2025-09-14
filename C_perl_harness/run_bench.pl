#!/usr/bin/perl
use strict;
use warnings;
use Getopt::Long;
use Time::HiRes qw(gettimeofday tv_interval);
use Text::CSV;

my $client = "../B_gpudrv/user/gpudrv_client";
my $out = "results.csv";
GetOptions("client=s" => \$client, "out=s" => \$out);

my @sizes = (1024, 65536, 262144, 1048576);
my $csv = Text::CSV->new({binary=>1, eol=>"\n"});
open my $fh, ">>", $out or die "open $out: $!";

# header if empty
if (-s $out == 0) { $csv->print($fh, ["timestamp", "n", "elapsed_ms", "notes"]); }

for my $n (@sizes) {
    my $t0 = [gettimeofday];
    my $cmd = "$client $n 2>&1";
    print "Running: $cmd\n";
    my $outstr = `$cmd`;
    my $elapsed = tv_interval($t0) * 1000.0;
    # try to parse daemon line "Daemon: processed n=... kernel_ms=..."
    my ($k) = $outstr =~ /Kernel time:\s*([\d\.]+)/;
    my $note = $k ? "kernel_ms=$k" : "";
    $csv->print($fh, [scalar(localtime), $n, sprintf("%.3f", $elapsed), $note]);
    sleep 1;
}
close $fh;
print "Results appended to $out\n";