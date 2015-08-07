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

package Vili::Helper;

=head1 NAME

Vili::Helper - a module that contains helper functions to be used by
operations and workflows

=head1 SYNOPSIS

   # Constants OK, WARNING, CRITICAL, UNKNOWN, INFO, WARN and ERROR 
   # are exported by default
   use Vili::Functions;

   my $ret = isvalid_IP($wf->{hostname});

   my $ret = isvalid_integer($wf->{warning});

   my $nodeid = GetNodeId();

   my $ret = TPA_is_ready();

   my ($lct_usecs, $utc_secs) = GetTimeStamp();

   my $ret = is_cluster();

   set_timeout($seconds);

   my @nodearray = GetNodeArray();

   my $ret = check_tm();

=head1 DESCRIPTION

A Perl class that provides helper functions to be used by operations
and workflows.  Utility subroutines that will be used by all of these
scripts will be placed in this file.  Additional subroutines to validate
input, or commonly called routines should be placed here.  This will 
make it easier for the creation of new scripts and will ensure that all
scripts perform these functions the same way.

=back

=head1 FUNCTIONS

=over 4

=item C<isvalid_IP()>

Returns true if the value provided is a valid IP address.  Otherwise, 
false is returned.

=item C<isvalid_integer()>

Returns true if a valid integer is provided.  Otherwise, false is returned.

=item C<get_sqenv_vars()>

Loads all environment variables from sqevn.sh file

=item C<GetNodeId()>

Returns the node id of the current node.

=item C<TPA_is_ready()>

Returns 1 if the TPA_publish script is ready to be used, and a 0 if it is not ready.

=item C<GetTimeStamp>
Return the LCT and UTC in microsecond

=item C<is_cluster()>

Returns 1 if it runs on cluster, otherwise 0 if it runs on a workstation.

=item C<set_timeout()>

Set an alarm timeout.

=item C<check_tm()>

Returns 0: the instance is not up,
        1: the instance is up, and TM is enabled
        2: the instance is up, and TM is disabled, it should be a normal shutdown
        3: unknown errors


=item C<GetNodeArray()>

Returns an array with the nodes names of the nodes in the instance.  If unable to obtain this list of names it will die.

=back

=head1 COPYRIGHT

(C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.

=cut

use strict;
use warnings;

use base qw(Exporter);

use Params::Validate qw(:all);
use Data::Dumper;   # Useful for debugging
use POSIX;
use Getopt::Long;
use File::Basename;
use Time::HiRes qw(gettimeofday);

our (@ISA, %EXPORT_TAGS, @EXPORT, @EXPORT_OK);
#our ($SQROOT, $TPA_PUBLISH, $TS_PROG); # $MY_SQROOT-dependent variables

# global vars set in se_check_flow
our $gv_HOSTNAME = undef;
our $gv_LNID = undef;

%EXPORT_TAGS =
(
    'all'    => [qw(isvalid_IP isvalid_integer isvalid_domain GetNodeID SQ_is_ready TPA_is_ready is_cluster set_timeout check_tm GetTimeStamp GetNodeArray GetNodeName GetNodeStatus sudo_command get_HOSTNAME get_LNID get_sqenv_vars)],

    'defaults'    => [qw(isvalid_IP isvalid_integer isvalid_domain GetNodeID SQ_is_ready TPA_is_ready is_cluster set_timeout check_tm GetTimeStamp GetNodeArray GetNodeName GetNodeStatus sudo_command)],

    'consts'    => [qw(isvalid_IP isvalid_integer isvalid_domain GetNodeID SQ_is_ready TPA_is_ready is_cluster set_timeout check_tm GetTimeStamp GetNodeArray GetNodeName GetNodeStatus sudo_command)],
);

@EXPORT    = (@{$EXPORT_TAGS{'defaults'}});
@EXPORT_OK = (@{$EXPORT_TAGS{'all'}});

# return 1, true, if valid input
# return 0, false, if invalid input
my $SUCCESS = 1;
my $FAIL = 0;
my $PROBLEM = -1;

# Define some constants that are used by these functions
my $FIFO = "$ENV{MY_SQROOT}/seapilot/amqp-tpa";   # the fifo used by TPA_Publish

##############################################################################
##############################################################################

#
#	This subroutine is called to validate an IP address that was input
#
#	INPUT: 
#	ipaddress passed in to calling routine
#	
#	OUTPUT:
#	0 - indicates that the ip address is valid
#	1 - indicates that the ip address is NOT valid
#	
sub isvalid_IP
{
my($ip) =  @_;

        # empty string
        if (!defined($ip) or $ip eq '') {
                return $FAIL;
        }
        # string contains spaces
        if ($ip =~ /\s/) {
                return $FAIL;
        }
        # string begins with '.'
        if ($ip =~ /^\./) {
                return $FAIL;
        }
        # string ends with '.'
        if ($ip =~ /\.$/) {
                return $FAIL;
        }

        # now check the individual components of the string
        my(@components) = split(/\./,$ip);

        # string does not contain 4 components
        if (@components != 4) {
                return $FAIL;
        }

        my($comp);
        foreach $comp (@components) {
                # string has zero length component
                if (length($comp) == 0) {
                        return $FAIL;
                }

                # string has a non-digit in the component
                if ($comp !~ /^\d+$/ && $comp !~ /^0x[0-9A-Fa-f]+$/) {
                        return $FAIL;
                }

                # string has octal or hex value in the component
                if ($comp =~ /^0\d/ || $comp =~ /^0x/) {
                        return $FAIL;
                }

                # string has component out of range 0 to 255
                if ($comp < 0 || $comp > 255) {
                        return $FAIL;
                }
        }

    return($SUCCESS);
}

#
#	This subroutine is called to validate an IP address that was input
#
#	INPUT: 
#	ipaddress passed in to calling routine
#	
#	OUTPUT:
#	0 - indicates that the ip address is valid
#	1 - indicates that the ip address is NOT valid
#	
sub isvalid_integer
{
my($inputval) =  @_;

    # empty string
    if (!defined($inputval) or $inputval eq '') {
        return $FAIL;
    }

    # if the integer contains a '.', then
    # strip it off and check the rest of the characters
    if ($inputval =~ /\./) {
        my $temp = $inputval;
 	($inputval = $temp) =~ s/\.//;
    }

    # string has a non-digit in the component
    if ($inputval !~ /^\d+$/) {
        return $FAIL;
    }

    return($SUCCESS);
}

#
#    This subroutine is called to validate an domain name that was input
#
#    INPUT: 
#    domain name passed in to calling routine
#    
#    OUTPUT:
#    0 - indicates that the domain name is valid
#    1 - indicates that the domain name is NOT valid
#    
sub isvalid_domain {
    my ($str) = @_;
    if ( defined($str)
            # empty string
            and length($str) > 0
            # no longer than 255 chars
            and length($str) < 256
            # each label may contain up to 63 characters
            and $str =~ /^[a-zA-Z0-9][-a-zA-Z0-9]{0,62} (\.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})* \.?$/x
        ) {
            return 1;
    } else {
            return 0;
    }
}

#
# This subroutine will get all environment variables defined in sqenv.sh
#
sub get_sqenv_vars {

    my $MY_SQROOT = $ENV{'MY_SQROOT'};
    my $cmd = "cd $MY_SQROOT; . ./sqenv.sh; export; 2>/dev/null";
    my $cmdResult = `$cmd`;
    
    my @result = split /\n/, $cmdResult;

    foreach (@result) {
        if (/^export\s+(.+)="(.+)"/) {
            my ($var, $val) = ($1, $2);
	    next if(defined($ENV{$var})); 
	    $ENV{$var} = $val; 

        }
    } 
}    
 
#
# Return the physical node name
#
sub GetNodeName
{
    chomp (my $hostname = `hostname`);
    return $hostname;
}

#
#	This subroutine is called to obtain the node id
#	of the current node we are running on.  This value
#	is returned back to the caller.
#
#	INPUT: 
#	none
#	
#	OUTPUT:
#	nodeid - if things work well
#	if a problem occurs then die is called
#	
# `ps -ef | grep sp_proxy | grep -v grep | grep $USER` output
#sqdev15  16094 11046  0 15:40 ?        00:00:13 sp_proxy SQMON1.0 00002 00002 016094 $XDN2 172.31.0.39:59470 00010 00002 SPARE
# nid is in column 11 (0 offset 10)
# Per the standard SeaQuest process arguments format:
# sp_proxy        SQMON1.0    00002   00002  016094  $XDN2    172.31.0.39:59470  00010    00002  SPARE
# <program name>  "SQMON1.0"  <pnid>  <nid>  <pid>   <pname>  <port>             <ptype>  <zid>  "SPARE"
#

sub GetNodeID
{

    # get our SQROOT path
    my $MY_SQROOT = $ENV{'MY_SQROOT'};
    my $USER = $ENV{ 'USER' };

    die ("The environment variable MY_SQROOT is empty.\n") 
        unless (defined($MY_SQROOT) && length($MY_SQROOT)>0);
    die ("The environment variable USER is empty.\n") 
        unless (defined($USER) && length($USER)>0);

    set_HOSTNAME();
    my $psline = `ps -ef | grep sp_proxy | grep -v grep | grep $USER | head -1`;
    my @cols = split /\s+/, $psline;
    my $nodeID = undef;
    chomp( $nodeID = sprintf( "%d", $cols[10] )) if (defined($cols[10]));

    if (defined($nodeID) and length($nodeID) > 0) { 
        $gv_LNID = $nodeID;
        return ($nodeID); 
    } else {
        die ("The file $MY_SQROOT/tmp/cluster.conf does not exist.\n") 
            unless (-r "$MY_SQROOT/tmp/cluster.conf");
    
        $nodeID = `cat $MY_SQROOT/tmp/cluster.conf | cut -d: -f2,3 | sort -u | sed 's/\$/:/' | grep -m 1 '\:$gv_HOSTNAME\:' | cut -d: -f1`;
        chomp($nodeID);
    
        if ((WIFEXITED($?) && (WEXITSTATUS($?) != 0)))
        {
            die("ERROR: unable to find current Node ID in $MY_SQROOT/tmp/cluster.conf\n");
        }
        $gv_LNID = $nodeID;
        return ($nodeID);
    }
}

sub set_HOSTNAME() {
    chomp($gv_HOSTNAME = `/bin/hostname`);
    return 0;
}

sub get_HOSTNAME() {
    return $gv_HOSTNAME;
}

sub get_LNID() {
    return $gv_LNID;
}


## Verify that SQ environment is up
##
## Params: None
##
## Returns: 1 is ready, 0 is not ready
##
sub SQ_is_ready
{
    my $sqroot = $ENV{'MY_SQROOT'};
    my $cmd = "$sqroot/sql/scripts/sqcheck";
    `$cmd`;

    # sqcheck returning 0 means SQ is up
    return 1 if ((WIFEXITED($?) && (WEXITSTATUS($?) == 0)));
    return 0;
}

##
## Verify that the TPA_Publish fifo is ready for us to start
## calling workflows.  We do this by checking that the file
## descriptor is a valid named pipe and has write permission
##
## Params: None
##
## Returns: 0 - if the fifo fails one of these tests
##          1 - if the fifo passes all tests - fifo is ready
##
sub TPA_is_ready
{
    my $self = shift;

    # check that the file exists
    if ( ! -e $FIFO ) {
        return(0);
    }

    # check that the file is a named pipe
    if ( ! -p $FIFO ) {
        return(0);
    }

    # check that the file is has write permissions set
    if ( ! -w $FIFO ) {
        return(0);
    }

    return(1);
}

#
#  Check whether the script runs on a cluster
#
#  Input: None
#
#  Returns: 1 - if it runs on a cluster
#           0 - if it runs on a workstation
# 
sub is_cluster
{
    my $output = `type -a pdsh 2>/dev/null`;
    if (length($output) > 0) {
        return 1;
    } 
    return 0;
}

#
# Set workflow timeout

# Input: seconds to wait for an alarm

# Returns: none
#
sub set_timeout
{
    my $timeout = shift;

    $SIG{ALRM} = sub { die "$timeout seconds timeout.\n" };
    alarm($timeout);
}

# Check whether TM is OK by checking registry SQ_TXNSVC_READY,
# return values,
# 0: the instance is not up,
# 1: the instance is up, and TM is enabled
# 2: the instance is up, and TM is disabled, it should be a normal shutdown
# 3: unknown errors
sub check_tm
{
    `sqregck -f -q -r SQ_TXNSVC_READY`;
    if (WIFEXITED($?)) {
        my $rc = WEXITSTATUS($?);
        return $rc if ($rc eq "0" || $rc eq "2");

        return ($rc eq "1") ? 1 : 3;

    }
    else {
        return 3;
    }
}

#
#	This subroutine is called to obtain the list of nodes
#	that are part of the instance.  An array of the nodes 
#	is returned back to the caller.
#
#	INPUT: 
#	none
#	
#	OUTPUT:
#	nodearray - an array of nodes, if things work well
#	if a problem occurs then die is called
#	
sub GetNodeArray
{
my $startsqshellcmd = "sql/scripts/sqshell -c";
my $sqshellcmd = "node info";
my @nodearray = ();

    # get our SQROOT path
    my $MY_SQROOT = $ENV{'MY_SQROOT'};

    # check that we are currently in an instance
    die ("The environment variable MY_SQROOT is empty.\n") unless (defined($MY_SQROOT) && length($MY_SQROOT)>0);

    # if it's on a workstation, the only node is 'localhost'
    if (!&is_cluster()) {
        push (@nodearray,"localhost");
        return (@nodearray);
    }

    # get list of nodes using the correct command 
    # if the instance is not up then we will get a non-zero output
    my $nodeoutput = `$MY_SQROOT/$startsqshellcmd $sqshellcmd`;
    if ($? != 0) {
    	die ("$MY_SQROOT/$startsqshellcmd returned an error, command output : \n$nodeoutput\n");
    }
	
    # now massage output to obtain an array of nodes
    # we expect the output to look like this
    # Processing cluster.conf on local host n001
    # [$Z050BU2] Shell/shell Version 1.0.1 Release 1.2.0 (Build release [14733], date 05Feb12)
    # [$Z050BU2] %node info
    # [$Z050BU2] Logical Nodes    = 7
    # [$Z050BU2] Physical Nodes   = 7
    # [$Z050BU2] Spare Nodes      = 0
    # [$Z050BU2] Available Spares = 0
    # [$Z050BU2] NID Type        State    Processors   #Procs
    # [$Z050BU2]     PNID        State        #Cores  MemFree SwapFree CacheFree Name
    # [$Z050BU2] --- ----------- -------- ---------- -------- -------- --------- --------
    # [$Z050BU2] 000 Backend     Up                2       57
    # [$Z050BU2]     000         Up                8 40434788 47999992  47272556 n004
    # [$Z050BU2] 001 Backend     Up                2       51
    # [$Z050BU2]     001         Up                8 40895884 47999992  47423368 n005
    # [$Z050BU2] 002 Backend     Up                2       49
    # [$Z050BU2]     002         Up                8 40212820 47999992  47366860 n006
    # [$Z050BU2] 003 Backend     Up                2       51
    # [$Z050BU2]     003         Up                8 40071368 47999992  46594532 n007
    # [$Z050BU2] 004 Backend     Up                2       54
    # [$Z050BU2]     004         Up                8 40313340 47999992  46504032 n008
    # [$Z050BU2] 005 Frontend    Up                2       38
    # [$Z050BU2]     005         Up                8 42488384 47999992  47990288 n001
    # [$Z050BU2] 006 Frontend    Up                2       35
    # [$Z050BU2]     006         Up                8 40917684 47999992  47730040 n002
    # [$Z050BU2] %quit
    # 
    # and we want to end up with just the node names
    # 
    # We also will not check for UP or DOWN because
    # we want the list of all nodes.
    # In addition, we will not get spare nodes using this code
    # which is OK because we currently do not plan to run health
    # checks on spare nodes.
    my $next = 0; # flag that indicates we want to look at next line
    for my $line (split("\n", $nodeoutput) ) {

	# if the next flag is set then this is a line that 
	# contains our node name
	# Node name may have ":xyz" on the end of the name, so
	# we will have to get rid of that!!
	if ($next) {
		my (@fields) = split(" ",$line);

		# check for Spare node, because we do
		# not want to run our health checks on a spare
		# Correction Per mail from Viral on 2/10/12 --
		# Spare would be in 3rd column:
		# [$Z000DTQ] 002 Any         Up                2       31
		# [$Z000DTQ]     002         Up                8 45574224 47999992  47966220 n056
		# [$Z000DTQ]     003         Spare             8 45777200 47999992  48370576 n048

		if ($fields[2] !~ /Spare/) {
			my $nextnode = $fields[7];
			chomp($nextnode);
			$nextnode =~ s/:(.*)$//;
			push(@nodearray,$nextnode);
		}

		# clear the next flag since we retrieved our node name
		$next = 0;
	}

	# if line matches any names to identify the nodes then
	# set the flag to look at the next line
	if (($line =~ /Edge/) || ($line =~ /Excluded/) || ($line =~ /Aggregation/) || ($line =~ /Storage/) || ($line =~ /Backend/) || ($line =~ /Frontend/) || ($line =~ /Any/)) {
		# set the next flag so that we retrieve our node name
		$next = 1;
	}

   }

   return (@nodearray);
}

sub GetNodeStatus {
    my $sqshellcmd = $ENV{'MY_SQROOT'} . "/sql/scripts/sqshell -c";
    my $output = '';
    my %nodestate = ();

    $output = `$sqshellcmd node info`;
    if ((WIFEXITED($?) && (WEXITSTATUS($?) != 0))) {
    	die ("$sqshellcmd returned an error, command output : \n$output\n");
    }

    my $next = 0; # flag that indicates we want to look at next line
    for my $line (split("\n", $output)) {
	if ($next) {
		my (@fields) = split(" ",$line);
                # CR 6252 -- robustness
                # if line:
                #
                # [$Z020P5R]     000         Up                8  
                # 2100836 94330844   9320092 n037
                #
                # $fields[7] would be n037
                if (defined($fields[7])) {
		    $fields[7] =~ s/:(.*)$//;
                    $nodestate{$fields[7]} = $fields[2];
                }
		$next = 0;
	}

	# if line matches any names to identify the nodes then
	# set the flag to look at the next line
	if (($line =~ /Edge/) || ($line =~ /Excluded/) || ($line =~ /Aggregation/) || ($line =~ /Storage/) || ($line =~ /Backend/) || ($line =~ /Frontend/) || ($line =~ /Any/)) {
		# set the next flag so that we retrieve our node name
		$next = 1;
	}
    }
    chomp(my $hst = `hostname`);
    $nodestate{'localhost'} = $nodestate{$hst};

    return \%nodestate;
}

# Get the current timestamp, return the LCT and UTC
sub GetTimeStamp
{
   my ($start_sec, $start_usec) = gettimeofday;
   my $utc_usecs = $start_sec * 1000000  + $start_usec;

   my $offset = strftime("%z", localtime());

   $offset =~ s/0//g;
   $offset = 0 if ($offset eq "+"); # zero timezone
   
   $offset *= 3600 * 1000000; # convert hour to microsecond

   my $lct_usecs = $utc_usecs + $offset;

   return ($lct_usecs, $utc_usecs);
}

# sudo wrapper
sub sudo_command {
    my $cmd = shift;
    my $output = '';

    if (&is_cluster()) {
        $output = `sudo $cmd`;
    }
    return wantarray ? (split /\n/, $output) : $output;
}

1;
