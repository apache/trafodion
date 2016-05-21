#!/usr/bin/perl
# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
#
use strict;

use sqconfigdb;

my $count = @ARGV[0];

sqconfigdb::openDb();

my $first_core = 0;
my $first_excl = -1;
my $last_core = 0;
my $last_excl = -1;
my $name_inx;
my $num_proc = 0;
my $node_name;
my $nid;
my $pnid;
my $role_set = 7;
open(CONF, "<nmap.conf");
my @node_names;
my $ninx = 0;
while (<CONF>) {
	$node_names[$ninx] = $_;
	$ninx++;
}
for $nid (0..$count-1) {
	$pnid = $nid;
	$name_inx = $pnid + 1;
	$node_name = "n${name_inx}";
	if ($ninx > 0) {
		if ($nid < $ninx) {
			$node_name = $node_names[$nid];
			$node_name =~ s/^\s+|\s+$//g;
		}
	}
	sqconfigdb::addDbPNode($pnid, $node_name, $first_excl, $last_excl);
	sqconfigdb::addDbLNode($nid, $pnid, $num_proc, $role_set, $first_core, $last_core);
}
