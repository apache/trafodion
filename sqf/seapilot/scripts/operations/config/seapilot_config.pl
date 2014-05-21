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
use 5.008000;
use warnings;
use POSIX;

# The PERL5LIB env variable isn't available to us, so we dynamically add
# $MY_SQROOT/export/lib to @INC here.
my $SQROOTLIB;
BEGIN { $SQROOTLIB = $ENV{'MY_SQROOT'} . "/export/lib"; }

use lib $SQROOTLIB;

# use library to access Vili functions
use Vili::Functions ':all';

# use library to access Vili helper routines
use Vili::Helper ':all';

#******** NOTE TO THOSE ADDING IN NEW CODE HERE *****
# You will want to do the following things when adding
# in new brokers.
# 1) determine a new key name for your broker, e.g. YADDA
# 2) determine the node you would prefer your broker to start on
# 	no real magic here -- just try to spread things out
# 3) once you know where you want your broker to start then
#	add your key and entry value to the starthash
# 4) update the spreadhash array with the spread of nodes per broker
# 5) add your broker key to the numbrokerhash initialized to 0
# 6) determine whether your broker is a part of an already existing
#	category, such as event, or performance, or if you want to
#	introduce a new category.  If you do, then you must add a
#	new argument to this operation and the processing for that 
#	new variable, otherwise, incorporate your code with the
#	preexisting category that it belongs to.  If you add a new
#	argument then you must update the short and long help text
# 7) add code to determine the placement of your brokers, similar 
#	to what is done already in other places.  NOTE: brokers like
#	event, perf, health, consolidating brokers get added as soon
#	as they are one past their spread.  For example, if you have
# 	one broker every 4 nodes, when you have 5 nodes you get must
#	configure 2 brokers.  BUT, the UNCPERF and the UNCHEALTH
#	brokers are added one by one as the number of nodes goes
#	up. In that case, if you have 4 every 10 nodes, when you hit
#	11 nodes, you get 5; 12 nodes gets 6; and so on up to 15
#	nodes where you stay at 8 brokers until you hit the next
#	multiple of 10.  So, you also need to determine whether 
#	you need to ensure that your number of brokers is right, 
#	or whether you add them as the number of nodes goes up.
# 8) finally, once you have created your arrays holding your 
#	broker information, you will need to update the 
#	format_output routine to print out your output correctly.

# global variable used by the operation
my $all = 0; # set to true if not particular set of brokers was requested

# the spread of brokers per node
# NOTE: these values will never be changed in this script
my %spreadhash = (
	EVENT => 4,
	PERF => 10,
	HEALTH => 101,
	CONEVENT => 16, # 1 per 4 events which is 1 per 4 nodes = 1 per 16 nodes
	CONPERF => 100, # 1 per 10 perfs which is 1 per 10 nodes = 1 per 100 nodes
	CONHEALTH => 10201, # 1 x 101 healths at 1 x 101 nodes = 1 per 10201 nodes
	UNCEVENT => 1, # 4 x 1 events at 1 x 4 nodes = 1 per 1 nodes
	UNCEVENTTOTAL => 4, # 4 UNCs per event broker
	UNCPERF1 => 10, # 1 of 4 x 1 perfs at 1 x 10 nodes = 1 per 10 nodes
	UNCPERF2 => 10, # 1 of 4 x 1 perfs at 1 x 10 nodes = 1 per 10 nodes
	UNCPERF3 => 10, # 1 of 4 x 1 perfs at 1 x 10 nodes = 1 per 10 nodes
	UNCPERF4 => 10, # 1 of 4 x 1 perfs at 1 x 10 nodes = 1 per 10 nodes
	UNCPERFTOTAL => 4, # 4 UNCs per perf broker
	UNCHEALTH1 => 101, # 1 of 4 x 1 healths at 1 x 101 nodes = 1 per 101 
	UNCHEALTH2 => 101, # 1 of 4 x 1 healths at 1 x 101 nodes = 1 per 101 
	UNCHEALTH3 => 101, # 1 of 4 x 1 healths at 1 x 101 nodes = 1 per 101 
	UNCHEALTH4 => 101, # 1 of 4 x 1 healths at 1 x 101 nodes = 1 per 101 	
	UNCHEALTHTOTAL => 4, # 4 UNCs per health broker
);

# the preferred starting node for each broker
# these values may be changed, depending upon the number of nodes in the cluster
my %starthash = (
	EVENT => 0,
	PERF => 1,
	HEALTH => 2,
	CONEVENT => 3,
	CONPERF => 5, # let's skip 4 since EVENT will land there
	CONHEALTH => 6, 
	UNCEVENT => 0, # run on every node 
	UNCPERF1 => 0, 
	UNCPERF2 => 1, 
	UNCPERF3 => 2, 
	UNCPERF4 => 3, 
	UNCHEALTH1 => 4,
	UNCHEALTH2 => 5,
	UNCHEALTH3 => 6,
	UNCHEALTH4 => 7,
);

# the offset between the primary broker node and the backup broker offset
my %backupoffset = (
	EVENT => 1,
	PERF => -1,
	HEALTH => 1,
	CONEVENT => -1,
	CONPERF => -1,
	CONHEALTH => -5,
	UNCEVENT => 0,  # UNCEs has no backup
	UNCPERF => 6,
	UNCHEALTH => 4,
);

# the number of brokers required given the number of nodes
# these values must be set once we know the number of nodes
# NOTE:  we don't handle the UNCPERF or UNCHEALTH this way..
my %numbrokerhash = (
	EVENT => 0,
	PERF => 0,
	HEALTH => 0,
	CONEVENT => 0,
	CONPERF => 0,
	CONHEALTH => 0,
	UNCEVENT => 0,
);

# arrays to hold the nodes that our brokers will be placed
my @event_array = ();
my @conevent_array = ();
my @uncevent_array = ();
my @event_backup_array = ();
my @conevent_backup_array = ();
my @uncevent_backup_array = ();
my @event_conmap = ();
my @unc_eventmap = ();
my @perf_array = ();
my @conperf_array = ();
my @uncperf1_array = ();
my @uncperf2_array = ();
my @uncperf3_array = ();
my @uncperf4_array = ();
my @comb_uncperf_array = ();
my @sorted_comb_uncperf_array = ();
my @perf_backup_array = ();
my @conperf_backup_array = ();
my @uncperf_backup_array = ();
my @perf_conmap = ();
my @unc_perfmap = ();
my @health_array = ();
my @conhealth_array = ();
my @unchealth1_array = ();
my @unchealth2_array = ();
my @unchealth3_array = ();
my @unchealth4_array = ();
my @comb_unchealth_array = ();
my @sorted_comb_unchealth_array = ();
my @health_backup_array = ();
my @conhealth_backup_array = ();
my @unchealth_backup_array = ();
my @health_conmap = ();
my @unc_healthmap = ();


# define the variables that will be passed in to the constructor
my $op = "";
my $seapilot_config_version = "1.0.0";
my $seapilot_config_help_text = "seapilot_config is an operation that will identify the best placement of
Seapilot processes on the nodes in an instance provided the number of nodes in the instance. If no specific type
of processes are requested, then information on all of the processes will be returned.
This operation accepts the following arguments:
 	-nodes={number of nodes in instance} - provides the number of nodes in the instance
 	-event - this optional argument requests only information associated with event brokers (optional)
 	-performance - this optional argument requests only information associated with performance brokers (optional)
 	-health - this optional argument requests only information associated with health and state brokers (optional)
 	-external - this optional argument requests only information associated with external facing brokers (optional)
	-verbose - this optional argument provides debug information (optional)
	-list - indicates that output should be provided in human readable format; this is the default value (optional)
	-token - indicates that output should be provided in an easily parsible format (optional)
	-version - returns the version information of the operation (optional)
	-help - returns the long usage information for the operation (optional)
	-long_help - returns the man page for the operation (optional)
	 ";
my $seapilot_config_long_help_text = "seapilot_config

Usage: seapilot_config -nodes={# of nodes} -event -performance -health -external -verbose -list -token -version -help -long_help

This operation accepts the following arguments:
 	-nodes={number of nodes in instance} - provides the number of nodes in the instance
 	-event - this optional argument requests only information associated with event brokers (optional)
 	-performance - this optional argument requests only information associated with performance brokers (optional)
 	-health - this optional argument requests only information associated with health and state brokers (optional)
 	-external - this optional argument requests only information associated with external facing brokers (optional)
        -verbose - gives debug information (optional)
        -list - indicates that output should be provided in human readable format; this is the default value (optional)
        -token - indicates that output should be provided in an easily parsible format (optional)
        -version - returns the version information of the operation (optional)
	-help - returns the long usage information for the operation (optional)
	-long_help - returns the man page for the operation (optional)


This operation is called to identify the best placement of Seapilot processes on the nodes in an instance provided
the number of nodes in an instance.   The operation attempts to spread out the configuration of the processes in an
effort to prevent one node from being overloaded with processes.  The number of nodes argument is required.  If the 
event argument is provided then only information about all processes associated with events will be returned.  Similarly, if the performance, health, or external options are provided, then only the information about those processes will be returned.  If neither the event, performance, health or external options are provided, then information on all of the processes will be returned.

If there is a problem executing this configuration then the operation will return the value unknown(3).  If the 
operation succeeds then the operation will return ok(0).  ";

#
# This sub formats the output from the arrays created
# and provides it in the requested output format
#
sub format_output
{
my $formatoutput =  "";
my $i = 0;
my $j = 0;
my $string = "";
my $string_count = 0;
		 
	# now, based on output mode, let's generate our output
	if ($all || $op->{event}) {
		if ($op->{list}) {
			$formatoutput .= "Event Brokers: "; 
			for ($i = 0; $i < scalar(@event_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@event_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$event_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@event_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup Event Brokers: ";
			for ($i = 0; $i < scalar(@event_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@event_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$event_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@event_backup_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Consolidating Event Brokers: "; 
			for ($i = 0; $i < scalar(@conevent_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conevent_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conevent_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conevent_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup Consolidating Event Brokers: "; 
			for ($i = 0; $i < scalar(@conevent_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conevent_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conevent_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conevent_backup_array) > 1);
			$formatoutput .= "\n";

			for ($i = 0; $i < scalar(@conevent_array); $i++) {
				$formatoutput .= "Consolidating Event Broker: "; 
				$formatoutput .= "$conevent_array[$i]";
				$formatoutput .= "\n";
				$formatoutput .= "Associated Event Brokers: "; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the broker is associated
				# with the con broker, then print the event broker node number
				for ($j = 0; $j < scalar(@event_conmap); $j++) {
					if ($event_conmap[$j] == $conevent_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $event_array[$j]; 
						$string_count++; 
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\n";
			}

			$formatoutput .= "UNC Event Brokers: "; 
			for ($i = 0; $i < scalar(@uncevent_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@uncevent_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$uncevent_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@uncevent_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup UNC Event Brokers: "; 
			$formatoutput .= "()";	# No backups for UNCEs
			$formatoutput .= "\n";

			for ($i = 0; $i < scalar(@event_array); $i++) {
				$formatoutput .= "Event Broker: "; 
				$formatoutput .= "$event_array[$i]";
				$formatoutput .= "\n";
				$formatoutput .= "Associated UNC Event Brokers: "; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the unc broker is associated
				# with the broker, then print the unc broker node number
				for ($j = 0; $j < scalar(@unc_eventmap); $j++) {
					if ($unc_eventmap[$j] == $event_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $uncevent_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\n";
			}
		} else {
			$formatoutput .= "Event Brokers\t"; 
			for ($i = 0; $i < scalar(@event_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@event_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$event_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@event_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup Event Brokers\t"; 
			for ($i = 0; $i < scalar(@event_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@event_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$event_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@event_backup_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Consolidating Event Brokers\t"; 
			for ($i = 0; $i < scalar(@conevent_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conevent_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conevent_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conevent_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup Consolidating Event Brokers\t"; 
			for ($i = 0; $i < scalar(@conevent_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conevent_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conevent_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conevent_backup_array) > 1);
			$formatoutput .= "\t";

			for ($i = 0; $i < scalar(@conevent_array); $i++) {
				$formatoutput .= "Consolidating Event Broker\t"; 
				$formatoutput .= "$conevent_array[$i]";
				$formatoutput .= "\t";
				$formatoutput .= "Associated Event Brokers\t"; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the broker is associated
				# with the con broker, then print the event broker node number
				for ($j = 0; $j < scalar(@event_conmap); $j++) {
					if ($event_conmap[$j] == $conevent_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $event_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ( $string_count > 1);
				$formatoutput .= "\t";
			}

			$formatoutput .= "UNC Event Brokers\t"; 
			for ($i = 0; $i < scalar(@uncevent_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@uncevent_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$uncevent_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@uncevent_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup UNC Event Brokers\t"; 
			$formatoutput .= "()";	# No backups for UNCEs
			$formatoutput .= "\t";

			for ($i = 0; $i < scalar(@event_array); $i++) {
				$formatoutput .= "Event Broker\t"; 
				$formatoutput .= "$event_array[$i]";
				$formatoutput .= "\t";
				$formatoutput .= "Associated UNC Event Brokers\t"; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the unc broker is associated
				# with the broker, then print the unc broker node number
				for ($j = 0; $j < scalar(@unc_eventmap); $j++) {
					if ($unc_eventmap[$j] == $event_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $uncevent_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\t";
			}

		}

	}

	if ($all || $op->{performance}) {
		if ($op->{list}) {
			$formatoutput .= "Performance Brokers: "; 
			for ($i = 0; $i < scalar(@perf_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@perf_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$perf_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@perf_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup Performance Brokers: "; 
			for ($i = 0; $i < scalar(@perf_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@perf_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$perf_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@perf_backup_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Consolidating Performance Brokers: "; 
			for ($i = 0; $i < scalar(@conperf_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conperf_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conperf_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conperf_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup Consolidating Performance Brokers: "; 
			for ($i = 0; $i < scalar(@conperf_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conperf_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conperf_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conperf_backup_array) > 1);
			$formatoutput .= "\n";

			for ($i = 0; $i < scalar(@conperf_array); $i++) {
				$formatoutput .= "Consolidating Performance Broker: "; 
				$formatoutput .= "$conperf_array[$i]";
				$formatoutput .= "\n";
				$formatoutput .= "Associated Performance Brokers: "; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the broker is associated
				# with the con broker, then print the perf broker node number
				for ($j = 0; $j < scalar(@perf_conmap); $j++) {
					if ($perf_conmap[$j] == $conperf_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $perf_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\n";
			}

			$formatoutput .= "UNC Performance Brokers: "; 
			for ($i = 0; $i < scalar(@sorted_comb_uncperf_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@sorted_comb_uncperf_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$sorted_comb_uncperf_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@sorted_comb_uncperf_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup UNC Performance Brokers: ("; 
			for ($i = 0; $i < scalar(@uncperf_backup_array); $i++) {
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$uncperf_backup_array[$i]";
			}
			$formatoutput .= ")";
			$formatoutput .= "\n";

			for ($i = 0; $i < scalar(@perf_array); $i++) {
				$formatoutput .= "Performance Broker: "; 
				$formatoutput .= "$perf_array[$i]";
				$formatoutput .= "\n";
				$formatoutput .= "Associated UNC Performance Brokers: "; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the unc broker is associated
				# with the broker, then print the unc broker node number
				for ($j = 0; $j < scalar(@unc_perfmap); $j++) {
					if ($unc_perfmap[$j] == $perf_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $sorted_comb_uncperf_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\n";
			}

		} else {
			$formatoutput .= "Performance Brokers\t"; 
			for ($i = 0; $i < scalar(@perf_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@perf_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$perf_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@perf_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup Performance Brokers\t"; 
			for ($i = 0; $i < scalar(@perf_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@perf_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$perf_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@perf_backup_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Consolidating Performance Brokers\t"; 
			for ($i = 0; $i < scalar(@conperf_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conperf_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conperf_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conperf_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup Consolidating Performance Brokers\t"; 
			for ($i = 0; $i < scalar(@conperf_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conperf_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conperf_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conperf_backup_array) > 1);
			$formatoutput .= "\t";

			for ($i = 0; $i < scalar(@conperf_array); $i++) {
				$formatoutput .= "Consolidating Performance Broker\t"; 
				$formatoutput .= "$conperf_array[$i]";
				$formatoutput .= "\t";
				$formatoutput .= "Associated Performance Brokers\t"; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the broker is associated
				# with the con broker, then print the perf broker node number
				for ($j = 0; $j < scalar(@perf_conmap); $j++) {
					if ($perf_conmap[$j] == $conperf_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $perf_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\t";
			}

			$formatoutput .= "UNC Performance Brokers\t"; 
			for ($i = 0; $i < scalar(@sorted_comb_uncperf_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@sorted_comb_uncperf_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$sorted_comb_uncperf_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@sorted_comb_uncperf_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup UNC Performance Brokers\t("; 
			for ($i = 0; $i < scalar(@uncperf_backup_array); $i++) {
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$uncperf_backup_array[$i]";
			}
			$formatoutput .= ")";
			$formatoutput .= "\t";

			for ($i = 0; $i < scalar(@perf_array); $i++) {
				$formatoutput .= "Performance Broker\t"; 
				$formatoutput .= "$perf_array[$i]";
				$formatoutput .= "\t";
				$formatoutput .= "Associated UNC Performance Brokers\t"; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the unc broker is associated
				# with the broker, then print the unc broker node number
				for ($j = 0; $j < scalar(@unc_perfmap); $j++) {
					if ($unc_perfmap[$j] == $perf_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $sorted_comb_uncperf_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\t";
			}

		}
			
	}

	if ($all || $op->{health}) {
		if ($op->{list}) {
			$formatoutput .= "Health and State Brokers: "; 
			for ($i = 0; $i < scalar(@health_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@health_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$health_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@health_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup Health and State Brokers: "; 
			for ($i = 0; $i < scalar(@health_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@health_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$health_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@health_backup_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Consolidating Health and State Brokers: "; 
			for ($i = 0; $i < scalar(@conhealth_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conhealth_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conhealth_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conhealth_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup Consolidating Health and State Brokers: "; 
			for ($i = 0; $i < scalar(@conhealth_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conhealth_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conhealth_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conhealth_backup_array) > 1);
			$formatoutput .= "\n";

			for ($i = 0; $i < scalar(@conhealth_array); $i++) {
				$formatoutput .= "Consolidating Health and State Broker: "; 
				$formatoutput .= "$conhealth_array[$i]";
				$formatoutput .= "\n";
				$formatoutput .= "Associated Health and State Brokers: "; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the broker is associated
				# with the con broker, then print the health and state broker node number
				for ($j = 0; $j < scalar(@health_conmap); $j++) {
					if ($health_conmap[$j] == $conhealth_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $health_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\n";
			}

			$formatoutput .= "UNC Health and State Brokers: "; 
			for ($i = 0; $i < scalar(@sorted_comb_unchealth_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@sorted_comb_unchealth_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$sorted_comb_unchealth_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@sorted_comb_unchealth_array) > 1);
			$formatoutput .= "\n";

			$formatoutput .= "Backup UNC Health and State Brokers: ("; 
			for ($i = 0; $i < scalar(@unchealth_backup_array); $i++) {
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$unchealth_backup_array[$i]";
			}
			$formatoutput .= ")";
			$formatoutput .= "\n";

			for ($i = 0; $i < scalar(@health_array); $i++) {
				$formatoutput .= "Health and State Broker: "; 
				$formatoutput .= "$health_array[$i]";
				$formatoutput .= "\n";
				$formatoutput .= "Associated UNC Health and State Brokers: "; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the unc broker is associated
				# with the broker, then print the unc broker node number
				for ($j = 0; $j < scalar(@unc_healthmap); $j++) {
					if ($unc_healthmap[$j] == $health_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $sorted_comb_unchealth_array[$j];
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\n";
			}
		} else {
			$formatoutput .= "Health and State Brokers\t"; 
			for ($i = 0; $i < scalar(@health_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@health_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$health_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@health_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup Health and State Brokers\t"; 
			for ($i = 0; $i < scalar(@health_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@health_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$health_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@health_backup_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Consolidating Health and State Brokers\t"; 
			for ($i = 0; $i < scalar(@conhealth_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conhealth_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conhealth_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conhealth_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup Consolidating Health and State Brokers\t"; 
			for ($i = 0; $i < scalar(@conhealth_backup_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@conhealth_backup_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$conhealth_backup_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@conhealth_backup_array) > 1);
			$formatoutput .= "\t";

			for ($i = 0; $i < scalar(@conhealth_array); $i++) {
				$formatoutput .= "Consolidating Health and State Broker\t"; 
				$formatoutput .= "$conhealth_array[$i]";
				$formatoutput .= "\t";
				$formatoutput .= "Associated Health and State Brokers\t"; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the broker is associated
				# with the con broker, then print the health and state broker node number
				for ($j = 0; $j < scalar(@health_conmap); $j++) {
					if ($health_conmap[$j] == $conhealth_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $health_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\t";
			}

			$formatoutput .= "UNC Health and State Brokers\t"; 
			for ($i = 0; $i < scalar(@sorted_comb_unchealth_array); $i++) {
				$formatoutput .= "(" if ($i == 0 && scalar(@sorted_comb_unchealth_array) > 1);
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$sorted_comb_unchealth_array[$i]";
			}
			$formatoutput .= ")" if (scalar(@sorted_comb_unchealth_array) > 1);
			$formatoutput .= "\t";

			$formatoutput .= "Backup UNC Health and State Brokers\t("; 
			for ($i = 0; $i < scalar(@unchealth_backup_array); $i++) {
				$formatoutput .= "," if ($i > 0);
				$formatoutput .= "$unchealth_backup_array[$i]";
			}
			$formatoutput .= ")";
			$formatoutput .= "\t";

			for ($i = 0; $i < scalar(@health_array); $i++) {
				$formatoutput .= "Health and State Broker\t"; 
				$formatoutput .= "$health_array[$i]";
				$formatoutput .= "\t";
				$formatoutput .= "Associated UNC Health and State Brokers\t"; 
				$string = ""; 
				$string_count = 0; 
				# if our mapping shows that the unc broker is associated
				# with the broker, then print the unc broker node number
				for ($j = 0; $j < scalar(@unc_healthmap); $j++) {
					if ($unc_healthmap[$j] == $health_array[$i] ) {
						$string .= "," if ($string_count > 0);
						$string .= $sorted_comb_unchealth_array[$j]; 
						$string_count++;
					}
				}
				$formatoutput .= "(" if ($string_count > 1);
				$formatoutput .= $string;
				$formatoutput .= ")" if ($string_count > 1);
				$formatoutput .= "\t";
			}
		}
			
	}

	return($formatoutput);

}

############################    MAIN SCRIPT  ##########################################
# ***********
# This is the main script where we will perform the configuration of the processes
# ***********
# create new Vili instance
$op = Vili::Functions->new( 
	opname => "seapilot_config",
        help_text => $seapilot_config_help_text,
        long_help_text => $seapilot_config_long_help_text,
	op_version => $seapilot_config_version,
	 );


# Standard args processed by all operations and workflows
# are -verbose, -list, -token and -version
# ***********
# add the nodes argument 
# but do not specify any default.  Require that it
# be passed in by the user
$op->add_args(
	arginfo => 'nodes=i',
	);

# add the event argument with a default
$op->add_args(
	arginfo => 'event',
	default => "0",
	);

# add the performance argument with a default
$op->add_args(
	arginfo => 'performance',
	default => "0",
	);

# add the health argument with a default
$op->add_args(
	arginfo => 'health',
	default => "0",
	);

# add the external argument with a default
$op->add_args(
	arginfo => 'external',
	default => "0",
	);

# call the getopts function to process all of the
# arguments that were received.  If arguments are
# missing an error message will be generated
# This function will also process "help", "long_help"
# and "version" right away.
# ****NOTE: The value for your arguments are stored in your instance****
# ****	data structure, e.g. $op->{verbose}****
# ***********
$op->getopts;

# At this point, all required arguments have been received
# now validate, as needed, your additional arguments
# validate the nodes value provided
if (! isvalid_integer($op->{nodes}) || ($op->{nodes} <= 0)) {
	my $errormsg = "Invalid nodes value specified: $op->{nodes} \n";
       	$op->op_die(
		output => $errormsg,
	);
}

# if no particular specifier was provided in the arguments, then provide information
# about all of the processes
if (!$op->{event} && ! $op->{performance} && ! $op->{health} && ! $op->{external} ) {
	$all = 1;
}

# if we want some verbose output - lets report
# all of our variable values
if ($op->{verbose}) {
	print "Opname is $op->{opname}\n";
	print "list is $op->{list}\n";
	print "token is $op->{token}\n";
	print "version is $op->{version}\n";
	print "nodes is $op->{nodes}\n";
	print "event is $op->{event}\n";
	print "performance is $op->{performance}\n";
	print "health is $op->{health}\n";
	print "external is $op->{external}\n";
	print "all is $all\n";
}

# ***********
# ****USE THE LOG FUNCTION, AS NECESSARY, TO PLACE INFORMATION IN THE LOG FILE****
# ****WHEN ENTERING LOG INFO, THE SEVERITY VALUES ARE DEFINED AS CONSTANTS****
# ****INFO, WARNING, ERROR****
# ***********
#$op->log(
#	severity => Vili::Functions::INFO,
#	message => "I think that this log function works.",
#	);

# let's initialize the return code variable
my $return_code = "";
my $key = "";
my $i = 0;
my $j = 0;

# Here are the "RULES" that this script was coded to:
# 1) if we should have one broker every x nodes, the minute
#	that you have a system with x+1 nodes then you need another broker
#	NOT once you have 2x nodes...
# 2) we are striving to keep the spread of brokers fair so that there
#	is not an abundance of processes on a few nodes

# for each broker start, check that we can start on that node with
# this number of nodes; otherwise adjust the starting node for the broker 
# NOTE: we don't adjust UNCPERFs because we they start on node 0 and as the number of nodes grows, we end 
# up adding one as the number of nodes is multiple of 10, a mulitple of 10 plus 1 up to a multiple of 10 plus 3.
# NOTE2: for the UNCHEALTHs we want to treat like the UNCPERFs but prefer them to start on nodes
# 4 and 5.  So, if we need to, we will adjust the start down
foreach $key (keys %starthash) {

	# don't change the start nodes for the UNCPERF brokers
	if (($key eq "UNCPERF1") || ($key eq "UNCPERF2") || ($key eq "UNCPERF3") || ($key eq "UNCPERF4")) {
		next;
	}
	# if the starting point is larger than the number of nodes
	if ($starthash{$key} >= $op->{nodes}) {

		# if number of nodes is less than our preferred
		# nodes then drop to nodes 0 and 1
		if ($key eq "UNCHEALTH1") {
			$starthash{$key} = 0;
			print("$key start is now $starthash{$key}\n") if ($op->{verbose});
			next;
		}

		# if number of nodes is less than our preferred
		# nodes then drop to nodes 0 and 1
		if ($key eq "UNCHEALTH2") {
			$starthash{$key} = 1;
			print("$key start is now $starthash{$key}\n") if ($op->{verbose});
			next;
		}
		# if number of nodes is less than our preferred
		# nodes then drop to nodes 0 and 1
		if ($key eq "UNCHEALTH3") {
			$starthash{$key} = 0;
			print("$key start is now $starthash{$key}\n") if ($op->{verbose});
			next;
		}
		# if number of nodes is less than our preferred
		# nodes then drop to nodes 0 and 1
		if ($key eq "UNCHEALTH4") {
			$starthash{$key} = 1;
			print("$key start is now $starthash{$key}\n") if ($op->{verbose});
			next;
		}

		# change to start at the remainder value of the start
		# divided by the number of nodes
		$starthash{$key} = ($starthash{$key} % $op->{nodes});
		print("$key start is now $starthash{$key}\n") if ($op->{verbose});
	}
}

# set the number of brokers to configure given the number of nodes
foreach $key (keys %spreadhash) {
	# force the value to be an integer, so no 2.5 for example.
	$numbrokerhash{$key} = int($op->{nodes}/$spreadhash{$key});

	# but, if the number of nodes is not evenly divided by
	# spread, then we need to add another one.
	# in other works if we have a broker every 4, on a 5 node system
	# then we need to have a second broker
	# or if we have one every 20 and we only have 5 nodes
	# then we need to be sure to have one
	if ($op->{nodes} % $spreadhash{$key} != 0) {
		$numbrokerhash{$key} = ($numbrokerhash{$key} + 1); 
	}
	print("Number of $key brokers we should have is $numbrokerhash{$key}\n") if ($op->{verbose});
}	

# NOTE: There certainly are things that could be done here to make one subroutine to do 
# the same configuration looping over each type of broker.  However, it was decided not to do
# that so that the code was more readable, and understandable.  Someone coming in to this
# code should be able to understand where to add their code.

# configure our event related brokers
if ($all || $op->{event}) {

	# configure event brokers
	# NOTE2: For all of these brokers we only loop to less than $nodes, because when we have 5 nodes
	# they are numbered 0 - 4... so our index of nodes must always be less than the node count
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{EVENT}; $j<$op->{nodes}; $i++,$j+=$spreadhash{EVENT}) {
	
		$event_array[$i] = $j;

		# so, if on our next loop we will drop out because we will pass the 
		# number of nodes but we haven't met the full count of brokers 
		# then let's throw in another one at the end.
		# NOTE: we haven't incremented $i yet, so we need to do that here
		if ((($j + $spreadhash{EVENT}) >= $op->{nodes}) && (($i + 1) < $numbrokerhash{EVENT})) {
			$event_array[$i+1] = ($op->{nodes} - 1);
		}
	}

	# configure event backup brokers, need to take special care of the last backup broker
	@event_backup_array = map $_ + $backupoffset{EVENT}, @event_array;
	$event_backup_array[-1] = ($op->{nodes} - 2) % $op->{nodes} if ($event_backup_array[-1] >= $op->{nodes});

	# configure our consolidating event brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{CONEVENT}; $j<$op->{nodes}; $i++,$j+=$spreadhash{CONEVENT}) {
	
		$conevent_array[$i] = $j;

		# so, if on our next loop we will drop out because we will pass the 
		# number of event brokers but we haven't met the full count of brokers 
		# then let's throw in another one at the end.
		# NOTE: we haven't incremented $i yet, so we need to do that here
		if ((($j + $spreadhash{CONEVENT}) >= $op->{nodes}) && (($i + 1) < $numbrokerhash{CONEVENT})) {
			$conevent_array[$i+1] = ($op->{nodes} - 1);
		}
	}

	# configure consolidating event backup brokers, need to take special care of the first backup broker
	@conevent_backup_array = map $_ + $backupoffset{CONEVENT}, @conevent_array;
	$conevent_backup_array[0] += $op->{nodes} if ($conevent_backup_array[0] < 0);

	# configure our UNC event brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{UNCEVENT}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCEVENT}) {

		$uncevent_array[$i] = $j;

		# NOTE: Even though we do not need this logic here.. let's keep it in case the spread
		# of UNC EVENT brokers changes in the future.
		# so, if on our next loop we will drop out because we will pass the 
		# number of event brokers but we haven't met the full count of brokers 
		# then let's throw in another one at the end.
		# NOTE: we haven't incremented $i yet, so we need to do that here
		if ((($j + $spreadhash{UNCEVENT}) >= $op->{nodes}) && (($i + 1) < $numbrokerhash{UNCEVENT})) {
			$uncevent_array[$i+1] = ($op->{nodes} - 1);
		}
	}

	# we don't configure UNC event brokers, since they are put on every node

	# configure the mapping of event brokers to consolidating event brokers
	# For every entry in the event broker aray we will have an entry in the mapping array
	# to tell us what con broker is associated with it.
	# $j holds the index into the consolidating brokers and $i is the index into the brokers
	$j = 0;
	for ($i=0; $i < scalar(@event_array); $i++) {
		$event_conmap[$i] = $conevent_array[$j];

		# if we have met the grouping of brokers to consolidating brokers then 
		# move to the next consolidating broker so the next association is correct
		if ((($i + 1) % $spreadhash{EVENT}) == 0) {
			$j++;
		}
	}

	# configure the mapping of unc event brokers to event brokers
	# For every entry in the unc event broker aray we will have an entry in the mapping array
	# to tell us what event broker is associated with it.
	# $j holds the index into the event brokers and $i is the index into the unc event brokers
	$j = 0;
	for ($i=0; $i < scalar(@uncevent_array); $i++) {
		$unc_eventmap[$i] = $event_array[$j];

		# if we have met the grouping of unc event brokers to event brokers then 
		# move to the next event broker so the next association is correct
		if ((($i + 1) % $spreadhash{UNCEVENTTOTAL}) == 0) {
			$j++;
		}
	}
}

# configure our performance related brokers
if ($all || $op->{performance}) {

	# configure our performance brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{PERF}; $j<$op->{nodes}; $i++,$j+=$spreadhash{PERF}) {
	
		$perf_array[$i] = $j;

		# so, if on our next loop we will drop out because we will pass the 
		# number of nodes but we haven't met the full count of brokers 
		# then let's throw in another one at the end.
		# NOTE: we haven't incremented $i yet, so we need to do that here
		if ((($j + $spreadhash{PERF}) >= $op->{nodes}) && (($i + 1) < $numbrokerhash{PERF})) {
			$perf_array[$i+1] = ($op->{nodes} - 1);
		}
	}

	# configure performance backup brokers, need to take special care of the first backup broker
	@perf_backup_array = map $_ + $backupoffset{PERF}, @perf_array;
	$perf_backup_array[0] = 0 if ($perf_backup_array[0] < 0);

	# configure our consolidating performance brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{CONPERF}; $j<$op->{nodes}; $i++,$j+=$spreadhash{CONPERF}) {

		$conperf_array[$i] = $j;

		# so, if on our next loop we will drop out because we will pass the 
		# number of performance brokers but we haven't met the full count of brokers 
		# then let's throw in another one at the end.
		# NOTE: we haven't incremented $i yet, so we need to do that here
		if ((($j + $spreadhash{CONPERF}) >= $op->{nodes}) && (($i + 1) < $numbrokerhash{CONPERF})) {
			$conperf_array[$i+1] = ($op->{nodes} - 1);
		}
	}

	# configure consolidating performance backup brokers, need to take special care of the first backup broker
	@conperf_backup_array = map $_ + $backupoffset{CONPERF}, @conperf_array;
	$conperf_backup_array[0] = $op->{nodes} - 1 if ($conperf_backup_array[0] < 0);

	# configure the first of our UNC performance brokers
	# NOTE: the rules are slightly different for these brokers, if there are 4 per 1 performance broker
	# we do not automatically jump up to 8 once we have 2 performance brokers.  These are added 1 by one
	# as the node count goes up over the number of nodes required for a performance broker
	# Therefore, we do not need the logic to see if we need to add another one
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{UNCPERF1}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCPERF1}) {
	
		$uncperf1_array[$i] = $j;

	}

	# configure the second of our UNC performance brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{UNCPERF2}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCPERF2}) {

		$uncperf2_array[$i] = $j;

	}

	# configure the third of our UNC performance brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{UNCPERF3}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCPERF3}) {

		$uncperf3_array[$i] = $j;

	}

	# configure the fourth of our UNC performance brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{UNCPERF4}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCPERF4}) {

		$uncperf4_array[$i] = $j;

	}

	# configure the mapping of performance brokers to consolidating performance brokers
	# For every entry in the performance broker aray we will have an entry in the mapping array
	# to tell us what con broker is associated with it.
	# $j holds the index into the consolidating brokers and $i is the index into the brokers
	$j = 0;
	for ($i=0; $i < scalar(@perf_array); $i++) {
		$perf_conmap[$i] = $conperf_array[$j];

		# if we have met the grouping of brokers to consolidating brokers then 
		# move to the next consolidating broker so the next association is correct
		if ((($i + 1) % $spreadhash{PERF}) == 0) {
			$j++;
		}
	}

	# configure the mapping of unc performance brokers to performance brokers
	# but first we need to combine into one array and sort the entries
	@comb_uncperf_array = @uncperf1_array;
	push(@comb_uncperf_array, @uncperf2_array);
	push(@comb_uncperf_array, @uncperf3_array);
	push(@comb_uncperf_array, @uncperf4_array);

	# sort the entries and store them in a sorted array
	@sorted_comb_uncperf_array = sort { $a <=> $b } @comb_uncperf_array;

	# For every entry in the unc performance broker aray we will have an entry in the mapping array
	# to tell us what performance broker is associated with it.
	# $j holds the index into the performance brokers and $i is the index into the unc performance brokers
	$j = 0;
	for ($i=0; $i < scalar(@sorted_comb_uncperf_array); $i++) {
		$unc_perfmap[$i] = $perf_array[$j];

		# if we have met the grouping of unc performance brokers to performance brokers then 
		# move to the next performance broker so the next association is correct
		if ((($i + 1) % $spreadhash{UNCPERFTOTAL}) == 0) {
			$j++;
		}
	}

	# configure backup unc performance brokers, need to take care of those brokers whose node is over $op->{nodes}
	my %uncperf_hash = ();
	for my $pid (@sorted_comb_uncperf_array) {
		$uncperf_hash{$pid} = 1;
	}
	my @potential_backup_nodes = grep !defined($uncperf_hash{$_}), (0 .. $op->{nodes}-1);
	if (scalar(@potential_backup_nodes) > 0) {
		@uncperf_backup_array = map $_ + $backupoffset{UNCPERF}, @sorted_comb_uncperf_array;
		for ($i=0; $i<scalar(@uncperf_backup_array); $i++) {
			if ($uncperf_backup_array[$i] >= $op->{nodes}) {
				my $idx = int(rand(scalar(@potential_backup_nodes)));
				$uncperf_backup_array[$i] = $potential_backup_nodes[$idx];
			}
		}
	} else {
		@uncperf_backup_array = ();
	}
}

# configure our health and state related brokers
if ($all || $op->{health}) {

	# configure our health and state brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{HEALTH}; $j<$op->{nodes}; $i++,$j+=$spreadhash{HEALTH}) {
	
		$health_array[$i] = $j;

		# so, if on our next loop we will drop out because we will pass the 
		# number of nodes but we haven't met the full count of brokers 
		# then let's throw in another one at the end.
		# NOTE: we haven't incremented $i yet, so we need to do that here
		if ((($j + $spreadhash{HEALTH}) >= $op->{nodes}) && (($i + 1) < $numbrokerhash{HEALTH})) {
			$health_array[$i+1] = ($op->{nodes} - 1);
		}
	}

	# configure health backup brokers, need to take special care of the last backup broker
	@health_backup_array = map $_ + $backupoffset{HEALTH}, @health_array;
        $health_backup_array[-1] = ($op->{nodes} - 2) % $op->{nodes} if ($health_backup_array[-1] >= $op->{nodes});
	$health_backup_array[-1] = 0 if ($health_backup_array[-1] == $health_array[-1]);

	# configure our consolidating health brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{CONHEALTH}; $j<$op->{nodes}; $i++,$j+=$spreadhash{CONHEALTH}) {
	
		$conhealth_array[$i] = $j;

		# so, if on our next loop we will drop out because we will pass the 
		# number of event brokers but we haven't met the full count of brokers 
		# then let's throw in another one at the end.
		# NOTE: we haven't incremented $i yet, so we need to do that here
		if ((($j + $spreadhash{CONHEALTH}) >= $op->{nodes}) && (($i + 1) < $numbrokerhash{CONHEALTH})) {
			$conhealth_array[$i+1] = ($op->{nodes} - 1);
		}
	}

	# configure consolidating health backup brokers, need to take special care of the last backup broker
	@conhealth_backup_array = map $_ + $backupoffset{CONHEALTH}, @conhealth_array;
        $conhealth_backup_array[0] = ($op->{nodes} - 2) if ($conhealth_backup_array[0] < 0);
        $conhealth_backup_array[0] = ($op->{nodes} - 1) if ($conhealth_backup_array[0] == $conhealth_array[0] || $conhealth_backup_array[0] < 0);
	$conhealth_backup_array[0]-- if ($conhealth_backup_array[0] == $conhealth_array[0] && $op->{nodes} > 1);

	# configure the first of our UNC health brokers
	# NOTE: the rules are slightly different for these brokers, if there are 2 per 1 health broker
	# we do not automatically jump up to 4 once we have 2 health brokers.  These are added 1 by one
	# as the node count goes up over the number of nodes required for a health broker
	# However, since we prefer to not have these brokers start on node 0, we need to check whether we
	# need to add another broker but don't have enough nodes to put it where we want.  
	# If we should be adding another broker but can't place it on our desired node then put it on the
	# third to last node
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{UNCHEALTH1}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCHEALTH1}) {

		$unchealth1_array[$i] = $j;

		# if node count is one over the spread for these brokers then we need to add this one
		# if we can't put the broker where we want, then let's put it
		# on a different node.
		my $overspread = ($op->{nodes} % $spreadhash{UNCHEALTH1});
		if ((($j + $spreadhash{UNCHEALTH1}) >= $op->{nodes}) && (($overspread >= 1) && ($overspread <= $starthash{UNCHEALTH1}))) {
			$unchealth1_array[$i+1] = ($op->{nodes} - 3);
		}

	}

	# configure the second of our UNC health brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{UNCHEALTH2}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCHEALTH2}) {
	
		$unchealth2_array[$i] = $j;

		# if we can't put the broker where we want, then let's put it
		# on the second to last node.
		my $overspread = ($op->{nodes} % $spreadhash{UNCHEALTH2});
		if ((($j + $spreadhash{UNCHEALTH2}) >= $op->{nodes}) && (($overspread >= 2) && ($overspread <= $starthash{UNCHEALTH2}))) {
			$unchealth2_array[$i+1] = ($op->{nodes} - 2);
		}

	}

	# configure the 3rd of our UNC health brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes	
	for ($i=0,$j=$starthash{UNCHEALTH3}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCHEALTH3}) {

		$unchealth3_array[$i] = $j;

		# if node count is one over the spread for these brokers then we need to add this one
		# if we can't put the broker where we want, then let's put it
		# on a different node.
		my $overspread = ($op->{nodes} % $spreadhash{UNCHEALTH3});
		if ((($j + $spreadhash{UNCHEALTH3}) >= $op->{nodes}) && (($overspread >= 1) && ($overspread <= $starthash{UNCHEALTH3}))) {
			$unchealth3_array[$i+1] = ($op->{nodes} - 4);
		}
	}

	
	# configure the 4th of our UNC health brokers
	# $i is an index for the # of brokers, and $j is an index for the # of nodes
	for ($i=0,$j=$starthash{UNCHEALTH4}; $j<$op->{nodes}; $i++,$j+=$spreadhash{UNCHEALTH4}) {

		$unchealth4_array[$i] = $j;

		# if node count is one over the spread for these brokers then we need to add this one
		# if we can't put the broker where we want, then let's put it
		# on a different node.
		my $overspread = ($op->{nodes} % $spreadhash{UNCHEALTH4});
		if ((($j + $spreadhash{UNCHEALTH4}) >= $op->{nodes}) && (($overspread >= 2) && ($overspread <= $starthash{UNCHEALTH4}))) {
			$unchealth4_array[$i+1] = ($op->{nodes} - 5);
		}

	}

	# configure the mapping of health and state brokers to consolidating health and state brokers
	# For every entry in the health and state broker aray we will have an entry in the mapping array
	# to tell us what con broker is associated with it.
	# $j holds the index into the consolidating brokers and $i is the index into the brokers
	$j = 0;
	for ($i=0; $i < scalar(@health_array); $i++) {
		$health_conmap[$i] = $conhealth_array[$j];

		# if we have met the grouping of brokers to consolidating brokers then 
		# move to the next consolidating broker so the next association is correct
		if ((($i + 1) % $spreadhash{HEALTH}) == 0) {
			$j++;
		}
	}

	# configure the mapping of unc health and state brokers to health and state brokers
	# but first we need to combine into one array and sort the entries
	@comb_unchealth_array = @unchealth1_array;
	push(@comb_unchealth_array, @unchealth2_array);
	push(@comb_unchealth_array, @unchealth3_array);
	push(@comb_unchealth_array, @unchealth4_array);	

	# sort the entries and store them in a sorted array
	@sorted_comb_unchealth_array = sort { $a <=> $b } @comb_unchealth_array;

	# For every entry in the unc health and state broker aray we will have an entry in the 
	# mapping array to tell us what health and state broker is associated with it.
	# $j holds the index into the health and state brokers and $i is the index into 
	# the unc health and state brokers
	$j = 0;
	for ($i=0; $i < scalar(@sorted_comb_unchealth_array); $i++) {
		$unc_healthmap[$i] = $health_array[$j];

		# if we have met the grouping of unc health and state brokers to health and state brokers then 
		# move to the next health and state broker so the next association is correct
		if ((($i + 1) % $spreadhash{UNCHEALTHTOTAL}) == 0) {
			$j++;
		}
	}

	# configure backup unc health brokers, need to take care of those brokers whose node is over $op->{nodes}
	my %unchealth_hash = ();
	for my $hid (@sorted_comb_unchealth_array) {
		$unchealth_hash{$hid} = 1;
	}
	my @potential_hbackup_nodes = grep !defined($unchealth_hash{$_}), (0 .. $op->{nodes}-1);
	if (scalar(@potential_hbackup_nodes) > 0) {
		@unchealth_backup_array = map $_ + $backupoffset{UNCHEALTH}, @sorted_comb_unchealth_array;
		for ($i=0; $i<scalar(@unchealth_backup_array); $i++) {
			if ($unchealth_backup_array[$i] >= $op->{nodes}) {
				my $idx = int(rand(scalar(@potential_hbackup_nodes)));
				$unchealth_backup_array[$i] = $potential_hbackup_nodes[$idx];
			}
		}
	} else {
		@unchealth_backup_array = ();
	}
}

$return_code = Vili::Functions::OK;

# now go format the output to be returned to the caller
my $name = $op->{opname};
my $formattedoutput = format_output();

$op->op_exit ( 
	retcode => $return_code,
	output => $formattedoutput,
	);

