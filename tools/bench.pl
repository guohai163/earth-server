#! /usr/bin/perl

printf("bench start ... \n");

use IO::Socket::INET;

@ARGV == 1
    or die "Usage: $FindBin::Script HOST:PORT\n";

my $addr = $ARGV[0];
my $count = 1_000_000;


my $sock = IO::Socket::INET->new(PeerAddr => $addr,
                                 Timeout  => 3);
die "$!\n" unless $sock;

use Time::HiRes qw(gettimeofday tv_interval);

my $start = [gettimeofday];
foreach (1 .. $count) {
        print $sock "add home 1.1 1.1\r\n";
        scalar<$sock> unless $noreply;
        print $sock "delete home 1.1 1.1\r\n";
        scalar<$sock> unless $noreply;
}
my $end = [gettimeofday];

printf("update commands: %.2f secs\n\n", tv_interval($start, $end));