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
	$_ =~ s/\s+$//;
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
sqconfigdb::addDbPersistData( "PERSIST_PROCESS_KEYS", "DTM,PSD,WDG"  );
sqconfigdb::addDbPersistData( "DTM_PROCESS_NAME", "\$TM%nid+"  );
sqconfigdb::addDbPersistData( "DTM_PROCESS_TYPE", "DTM"  );
sqconfigdb::addDbPersistData( "DTM_PROGRAM_NAME", "tm"  );
sqconfigdb::addDbPersistData( "DTM_REQUIRES_DTM", "N"  );
sqconfigdb::addDbPersistData( "DTM_STDOUT", "stdout_DTM%nid"  );
sqconfigdb::addDbPersistData( "DTM_PERSIST_RETRIES", "2,30"  );
sqconfigdb::addDbPersistData( "DTM_PERSIST_ZONES", "%zid"  );
sqconfigdb::addDbPersistData( "PSD_PROCESS_NAME", "\$PSD%nid+"  );
sqconfigdb::addDbPersistData( "PSD_PROCESS_TYPE", "PSD"  );
sqconfigdb::addDbPersistData( "PSD_PROGRAM_NAME", "pstartd"  );
sqconfigdb::addDbPersistData( "PSD_REQUIRES_DTM", "N"  );
sqconfigdb::addDbPersistData( "PSD_STDOUT", "stdout_PSD%nid"  );
sqconfigdb::addDbPersistData( "PSD_PERSIST_RETRIES", "10,60"  );
sqconfigdb::addDbPersistData( "PSD_PERSIST_ZONES", "%zid"  );
sqconfigdb::addDbPersistData( "WDG_PROCESS_NAME", "\$WDG%nid+"  );
sqconfigdb::addDbPersistData( "WDG_PROCESS_TYPE", "WDG"  );
sqconfigdb::addDbPersistData( "WDG_PROGRAM_NAME", "sqwatchdog"  );
sqconfigdb::addDbPersistData( "WDG_REQUIRES_DTM", "N"  );
sqconfigdb::addDbPersistData( "WDG_STDOUT", "stdout_WDG%nid"  );
sqconfigdb::addDbPersistData( "WDG_PERSIST_RETRIES", "10,60"  );
sqconfigdb::addDbPersistData( "WDG_PERSIST_ZONES", "%zid"  );

my $en = $ENV{'SQ_NAMESERVER_ENABLED'};
if ($en == '1') {
	open(NSCONF, "<ns.conf");
	my $nscnt = 0;
	while (<NSCONF>) {
		$_ =~ s/\s+$//;
		sqconfigdb::addDbNameServer( $_ );
		$nscnt++;
	}
	if ($nscnt == 0) {
		sqconfigdb::addDbNameServer( 'n0' );
	}
}
