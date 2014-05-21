#! /usr/bin/perl -w -I /opt/hp/nv/lib/perl
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@

use strict;
use warnings;
use 5.008000;
use POSIX;

my $vilicfg = $ARGV[0];
my $mode = $ARGV[1];
my $freq= $ARGV[2];

if (!$vilicfg || !$mode || !$freq) {
	&usage();
	exit (1);
}

my $flt1 = "-vili[ =]$vilicfg";
my $flt2 = "-mode[ =]$mode";
my $flt3 = "-frequency[ =]$freq";
my $flt4 = "-publication";
my $node = `hostname -s`;

my $output = `ps -u $< -o pid,cmd | grep -P -- "$flt1" | grep -P -- "$flt2" | grep -P -- "$flt3" | grep -P -- "$flt4"`;
my @lines = split /\n/, $output;

for my $vili (@lines) {
	my ($pid, $cmd) = ($1, $2) if ($vili =~ /^(?:\s*)?(\d+)\s+(.+)$/);
	next if (!$pid || !$cmd);	# This is not a valid line.
	next if ($cmd =~ /^grep /);	# This a grep process
	`kill -9 $pid`;
	if (WIFEXITED($?) && (WEXITSTATUS($?) == 0)) {
		print "The workflow $cmd on node $node has run timeout and been killed.\n";
	}
}

sub usage {
	print "Usage: vili_cleanup.pl <vilicfg> <mode> <frequency>\n";
}
