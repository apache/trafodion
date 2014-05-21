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
use English;
use POSIX;
use threads;
use threads::shared;
use File::Basename;

# The PERL5LIB env variable wasn't available to us, so we dynamically add
# $MY_SQROOT/export/lib to @INC here.
my $SQROOTLIB;
BEGIN { $SQROOTLIB = $ENV{'MY_SQROOT'} . "/export/lib"; }

use lib $SQROOTLIB;

# use library to access Vili helper routines
use Vili::Helper ':all';

use Vili::NoDriftTimer;

# This is the log file name
# for all of the harnesses that run on this node
my $LOGFILE = "/seapilot/logs/Vili.log";

# global variable that holds our instance name
my $SQROOT = ();

# define the number of seconds in one day for comparison
my $SECONDS_IN_ONE_DAY = 24 * 60 * 60;

# initialize my global variables
# add this variable to turn on/off extra output, as needed
my $verbose = 0;

# initialize the sleep interval value to 0
# this is our sleep value in seconds
my $sleep_value = ();
my %harness_config = ();
my $start_time = ();
my $mode = ();
my $INTERVALMODE = "interval";
my $BASELINEMODE = "baseline";
my $exec_type = ();
my $ALL = "all";
my $ONE = "one";

# initialize the array that will hold the list of nodes
# in this instance
my @nodes = ();

# initialize the Vili name that we will include in the
# log file
# we will append the sleep interval to the name to get 
# the individual vili name
my $viliname = "Vili";

# This is the time we sleep between checking to see if TPA_Publish is
# ready for us to start calling the workflows
# Also set the max number of loops we will perform
my $TPA_sleep = 10;
my $TPA_max_cycles = 10;

# Time interval used to check whether SQ environment is up
# Sleep 60*60 seconds at most
my $SQ_sleep = 60;
my $SQ_max_cycles = 60;

# **********  Subroutines  ***************
# This subroutine will execute the program requested
# on the nodes requested
#   Input - program to be run, string containing the list
#		of nodes on which to run the program
#   Output - output from run
#
# This subroutine is not used any more, just kept for reference
sub runprog {
my ($program, $node) = @_;

my $errormsg;
my $cmd;

    print "entering runprog for $program on node $node\n" if ($verbose); 

    # generate command to run program on this node
    # be sure to pass in our arguments, too
    # and always request the output be a publication

    # first, prepend the instance to the name of the program
    # that we are going to run so that it runs in our instance
    my $fullprogram = $SQROOT . $program;

    if ($node eq "localhost") {
        $cmd = "$fullprogram -mode=$mode -frequency=$sleep_value -vili $viliname -publication >> /dev/null";
    } else {
        $cmd = "/usr/bin/ssh -o ConnectTimeout=5 $node $fullprogram -mode=$mode -frequency=$sleep_value -vili $viliname -publication >> /dev/null";
    }

    print "$node:$program: about to run $cmd\n" if ($verbose); 

    # run the command and check the error return
    chomp(my $output = `$cmd`);
    if (WIFEXITED($?) && (WEXITSTATUS($?) != 0)) {

    	print "$node:$program: problem running $cmd: $?\n" if ($verbose); 
    } else {
	print "$node:$program: $cmd ran fine!: $output\n" if ($verbose); 
    }

}

# This subroutine will fork a child process to execute
# the program requested # on the nodes requested
#   Input - program to be run, string containing the list
#               of nodes on which to run the program
#   Output - output from run
#
sub start_workflow {
    my ($workflow, $node) = @_;
    my $errormsg = "";

    print "entering start_workflow for $workflow on node $node\n" if ($verbose);
    my $pid = fork();
    if (!defined($pid)) {
        $errormsg = "Unable to start a workflow. Fork() error: $!\n";
        logmsg($errormsg);
        die($errormsg);
    }

    if ($pid == 0) {    # Child process 
        my $progname = $SQROOT . $workflow;
        my @params = ("-mode=$mode", "-frequency=$sleep_value", "-vili=$viliname", "-publication");

        if ($node ne "localhost") {
                $progname = "/usr/bin/ssh";
                unshift @params, ("-n", "-o", "ConnectTimeout=5", $node, $SQROOT.$workflow);
        }

        open STDOUT, "> /dev/null";
        exec($progname, @params);
        exit(0);
    }
}

# This subroutine will kill all timeout workflows on all nodes
sub kill_timeout_workflows {
    my $vili_filter = "$viliname $mode $sleep_value";
    my $killer = $SQROOT . "/seapilot/scripts/vili_cleanup.pl";

    my $cmd;
    foreach my $node (@nodes) {
         if ($node eq "localhost") {
            $cmd = "$killer $vili_filter";
         } else {
            $cmd = "/usr/bin/ssh -o ConnectTimeout=5 $node $killer $vili_filter";
         } 
         chomp(my $output = `$cmd`);
         if (WIFEXITED($?) && (WEXITSTATUS($?) != 0)) {
    	     logmsg("Failed to run $killer on node $node.\n");
         } else {
             logmsg($output) if (length($output) > 0);
         }
    }
}

# This subroutine will obtain the configuration for
# the harness
# NOTE: we only call this script if the file exists so
# there is no need to check again here!
#
#   Input - configuration file
#   Output - none
#
sub get_info_from_config {
my ($config_file) = @_;

my $errormsg = "";

    # open the configuration file
    my $ret = open(CONFIG, "<", $config_file);
    # open returns 1 on success, 0 on failure
    if (!$ret ) {
        $errormsg = "Unable to open $config_file: $!";

    	logmsg($errormsg);
        die($errormsg);
    }

    # read the file contents into the harness_config hash 
    while (<CONFIG>) {
	# first, though, let's skip any comment lines or blank lines
	if ((/^#/) || (/^\s+/)) {
		next;
	# set our sleep interval
	} elsif (/^sleep_interval/) {
		(my $label, $sleep_value) = split(":", $_);

		# remove trailing and preceding whitespace
		chomp($sleep_value);
		$sleep_value =~ s/^\s+//;

		if ($sleep_value !~ /^\d+$/ || $sleep_value <= 0) {
        	   $errormsg = "Invalid sleep interval provided in $config_file: $sleep_value";
    		   logmsg($errormsg);
        	   die($errormsg);
		} 

                # The sleep value in config is measured in minutes, convert it to seconds
                $sleep_value *= 60;
	# set our start time
	} elsif (/^start_time/) {
		# only split off the first ":" because the rest
	 	# are associated with the time value itself
	 	# so we only want 2 fields
		(my $label, my $start_value) = split(":", $_, 2);

		# remove trailing and preceding whitespace
		chomp($start_value);
		$start_value =~ s/^\s+//;

		if ($start_value =~ /^\d?\d:\d?\d:\d?\d$/) {
			# get start time value
			$start_time = $start_value;
        		$errormsg = "Harness start time provided in $config_file: $start_value";
    			logmsg($errormsg);
		} else { 
        		$errormsg = "Invalid start time provided in $config_file: $start_value";
    			logmsg($errormsg);
        		die($errormsg);
		} 
	# set our mode
	} elsif (/^mode/) {
		# obtain the mode configuration value
	 	# we only want 2 fields
		(my $label, my $mode_value) = split(":", $_);

		# remove trailing and preceding whitespace
		chomp($mode_value);
		$mode_value =~ s/^\s+//;

		if ($mode_value eq $INTERVALMODE || $mode_value eq $BASELINEMODE) {
			# get mode value
			$mode = $mode_value;
        		$errormsg = "Harness mode provided in $config_file: $mode_value";
    			logmsg($errormsg);
		} else { 
        		$errormsg = "Invalid mode provided in $config_file: $mode_value";
    			logmsg($errormsg);
        		die($errormsg);
		} 
	# set our execution type; are these scripts being executed on 
	# all nodes in the instance or on one node in the instance?
	} elsif (/^exec_type/) {
		# obtain the exec_type configuration value
	 	# we only want 2 fields
		(my $label, my $exec_value) = split(":", $_);

		# remove trailing and preceding whitespace
		chomp($exec_value);
		$exec_value =~ s/^\s+//;

		if ($exec_value eq $ALL || $exec_value eq $ONE) {
			# get exec_type value
			$exec_type = $exec_value;
        		$errormsg = "Execution type provided in $config_file: $exec_value";
    			logmsg($errormsg);
		} else { 
        		$errormsg = "Invalid execution type provided in $config_file: $exec_value";
    			logmsg($errormsg);
        		die($errormsg);
		} 
	} else {
	
		# FOR NOW: we do not pass in nodes
		# We pass in exec_type which is either "all" or "one"
		# which means we either run on all nodes in instance or just one
#		(my $executable, my $node_list) = split (":", $_);
		my $executable = $_;

		# remove trailing spaces
		chomp($executable);
		#chomp($node_list);

        	# remove any preceding spaces
        	#$node_list =~ s/^\s+//;
	
		# FOR NOW: don't fill in the node list info
		$harness_config{$executable} = "";
		#$harness_config{$executable} = $node_list;
    	}
    }

    # check to make sure that the values that are required
    # have been provided, else exit

    # the config file absolutely has to contain the mode
    # so check that it is defined before we leave the configuration 
    # subroutine.  If not, then generate an error
    if (!defined ($mode)) {
        $errormsg = "Fatal error: mode is not defined in $config_file";

    	logmsg($errormsg);
        die($errormsg);
    }

    # the config file absolutely has to contain the execution type
    # so check that it is defined before we leave the configuration 
    # subroutine.  If not, then generate an error
    if (!defined ($exec_type)) {
        $errormsg = "Fatal error: execution type is not defined in $config_file";

    	logmsg($errormsg);
        die($errormsg);
    }

    # the config file absolutely has to contain the sleep interval
    # so check that it is defined before we leave the configuration 
    # subroutine.  If not, then generate an error
    if (!defined ($sleep_value)) {
        $errormsg = "Fatal error: sleep_interval is not defined in $config_file";

    	logmsg($errormsg);
        die($errormsg);
    } else {

	# sleep interval was defined, so let's complete our vili name
	$viliname = $viliname . $sleep_value;
    }

    # close up the configuration file
    $ret = close(CONFIG);
    if (!$ret) {
        $errormsg = "Problems closing $config_file: $!";

    	logmsg($errormsg);
        die($errormsg);
    }

}

# This subroutine will log all harness messages to
# the log file
#
#   Input - log message
#   Output - none
#
sub logmsg {
my ($msg) = @_;

my $errormsg = "";

    # Let's create the log directory if it does not exist
    my $logmsg_dir = dirname($LOGFILE);
    if (!-e $logmsg_dir) {
        mkdir $logmsg_dir || die("Harness unable to create log directory.\n");
    }

    # Open the log file
    open(LOG, ">>$LOGFILE") || die("Harness unable to open the log file");

    # Get the current time for log message
    my $Mark = localtime();

    # Now print it using the name of this vili to identify where the message
    # is coming from
    printf(LOG "%s: %s: %s\n", $Mark, $viliname, $msg);

    # Done. Now close it and exit
    close(LOG);
}

# This subroutine will convert a start time
# to seconds
#
#   Input - start_time
#   Output - start_time in seconds
#
sub convert_to_seconds {
    my $ts = shift;

    my ($hh, $mm, $ss) = split /:/, $ts;

    # note: leading zero here will not make the value treated as an octal number
    my $ret = $hh * 3600 + $mm * 60 + $ss;

    if ($ret >= $SECONDS_IN_ONE_DAY) {
	my $errormsg = "Invalid start time provided in $LOGFILE: $ts\n";

    	logmsg($errormsg);
	die ($errormsg);
    }

    return $ret;
}

# This subroutine will sleep until the provided start time
#
#   Input - log message
#   Output - none
#
sub sleep_until_starttime {

    # get the number of seconds of start time
    my $start_secs = &convert_to_seconds($start_time);

    # now get the current time in seconds
    my $cur_time = (split /\s+/, localtime(time))[3];
    my $cur_secs = &convert_to_seconds($cur_time);

    # now determine how long to sleep until it is time to start
    if ($cur_secs <= $start_secs) {
	my $count = $start_secs - $cur_secs;

    	my $errormsg = "Sleeping $count seconds before starting harness \n" if ($verbose);
	logmsg($errormsg);
	sleep ($count);
    } else {
	my $count = ($SECONDS_IN_ONE_DAY - ($cur_secs - $start_secs));

    	my $errormsg = "Sleeping $count seconds before starting harness \n" if ($verbose);
	logmsg($errormsg);
	sleep ($count);
    }
}

############ ***MAIN***** ######################
my $errormsg = "";

    # Security requirement: all files created must show no permissions for "others"
    # so change default umask from 022 to 027
    umask 027;

    # obtain our instance path and verify that it is correct
    # before moving forward
    $SQROOT = $ENV{'MY_SQROOT'};
    die ("The environment MY_SQROOT is empty\n") unless (defined($SQROOT) && length($SQROOT)>0);

    # obtain log file name to use by combining our instance
    # path with the base name of the log file
    $LOGFILE = $SQROOT . $LOGFILE;

    # if no arguments passed in to script then we have a problem
    if (scalar(@ARGV) != 1) {
    	$errormsg = "Configuration file not passed as an argument to $0\n";
    	logmsg($errormsg);
    	die($errormsg);
    }

    # get our configuration file, if passed in
    my $config_file = $ARGV[0]; 

    # Config file passed in fully qualified from Seapilot
    if (!-e $config_file) { 
    	$errormsg = "Configuration file $config_file does not exist.\n";
    	logmsg($errormsg);
    	die($errormsg);
    }

    # get more env variables from sqenv.sh
    &get_sqenv_vars();
    
    # go get the configuration and save it in a hash file
    get_info_from_config($config_file);

    # get basename of config file; we will use that as an
    # indicator in our logging text
    $viliname = "Vili";
    my ($configbase, $directories, $suffix) = fileparse($config_file);
    $viliname = $configbase;

    # NOTE: FOR NOW use the exec_type value in the config file to
    # specify whether the scripts will be run on all nodes in the
    # instance or whether the scripts will be run on one node in the
    # instance.
    # 
    # TODO: Modify sqgen to configure the nodes and the config file
    # go get the nodes in this configuration.  Or, come up with another
    # way to identify the nodes to execute the scripts on, as necessary.
    if ($exec_type eq $ONE) {
    	$nodes[0] = "localhost";
    } else {
        # call Helper.pm routine to obtain array of nodes in instance
    	@nodes = GetNodeArray();
    }

    # if a start time was provided then
    # determine how much time to sleep, if any, before starting harness
    # otherwise, go ahead and start the harness now.
    if (defined $start_time) {
    	sleep_until_starttime();
    }

    # For baseline mode, we wait an hour for startup
    sleep(3600) if ($mode eq $BASELINEMODE);

    my $iteration = 0;
    while (!TPA_is_ready() && $iteration <= $TPA_max_cycles ) {

	sleep($TPA_sleep);
	$iteration++;
    }

    # if after all of that time the TPA is still not ready
    # then we will have to die.
    # NOTE: we will die and proxy will restart us up again
    # a maximum of 7 times
    if (!TPA_is_ready()) {
	$errormsg = "$0:TPA_Publish is not ready.";
    	logmsg($errormsg);
    	die($errormsg);
    }

    # log when the harness started
    logmsg("Harness running every $sleep_value seconds has been started.");

    my $count= 0;

    $SIG{CHLD} = sub {
        local ($!, $?);
        while (waitpid(-1, WNOHANG) > 0) {}
    };

    # init timer
    my $timer = Vili::NoDriftTimer->new($sleep_value);

    # now loop forever, sleeping the configured
    # sleep interval between runs
    for (;;) {
        #$timer->reset;
    
        my $node_status = {};
        $node_status = GetNodeStatus() if (scalar(keys%harness_config) > 0);

	# now go call each program on each node specified
    	foreach my $workflow (sort keys %harness_config) {

    		foreach my $node (@nodes) {
                        
			# Don't run a $workflow on the node that is NOT in "up" state 
		        next unless ($node_status->{$node} =~ /up/i); 	
			print "*** executing $workflow on node $node ***\n" if ($verbose);

                        &start_workflow($workflow, $node);

    		} # for each node running a workflow
    	} # for each workflow defined

    	print "run $count complete !\n" if ($verbose);
    	$count ++;
	
    	print "sleeping for $sleep_value seconds !\n" if ($verbose);

        $timer->sleep;
        &kill_timeout_workflows;
    }

    print "Harness is exiting; how could the harness have gotten here?\n" if ($verbose);
