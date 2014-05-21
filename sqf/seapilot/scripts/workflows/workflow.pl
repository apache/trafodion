#!/usr/bin/perl -w
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

my ($SQROOT, $SQROOTLIB);
BEGIN { 
   $SQROOT = $ENV{MY_SQROOT};
   die ("The environment MY_SQROOT is empty\n") 
      unless (defined($SQROOT) && length($SQROOT)>0);

   $SQROOTLIB = "$SQROOT/export/lib"; 
}
use lib $SQROOTLIB;

# definitions for type of health/state publication
my $BASELINE = 0;
my $INCREMENTAL = 1;
my $MANUAL = 2;

# here are some definitions that we need from the spstate.h file
my $SPSTATE_UNKNOWN = 1;
my $SPSTATE_UP = 3;
my $SPSTATE_DOWN = 4;

# here are some definitions that we need from the sphealth.h file
my $SPHEALTH_UNKNOWN = 1;
my $SPHEALTH_READY = 2;
my $SPHEALTH_NOTREADY = 3;
my $SPHEALTH_ABOVEWARNING = 6;
my $SPHEALTH_ABOVEERROR = 8;

package Workflow;

use 5.008000;
use strict;
use warnings;
use POSIX;
use List::Util qw/min max/;
use Data::Dumper;
use Params::Validate ':all';

use Vili::Functions ':all';
use Vili::Helper ':all';

my $TPA_PUBLISH = "$SQROOT/seapilot/scripts/TPA_Publish";

# labels for fields when printing list or token
my @labels = qw/ info_header publication_type check_interval_sec error error_text repeated logical_object_type logical_object_subtype logical_object_name logical_object_qual_1 logical_object_qual_2 logical_object_path physical_object_type physical_object_subtype physical_object_name current_state current_state_raw previous_state previous_state_raw state_change_ts_lct state_change_ts_utc current_health current_health_raw previous_health previous_health_raw health_change_ts_lct health_change_ts_utc /;

sub new
{
   my $class = shift;
   
   # validate the arguments passed in to the constructor
 
   my %args = validate( @_,
      {
         name => {
            type => SCALAR,
            regex => qr/^\w+$/	# that is all word chars
         },
         help_text =>{
            type => SCALAR
         },
         long_help_text =>{
            type => SCALAR
         },
         op => {
            type => SCALAR
         },
         routing_key => {
            type => SCALAR,
            regex => qr/^[\w\.]+$/	# that is all word chars
         },
         version =>{
            type => SCALAR            
         },
      }
   );

   my $self = {};
   $self->{name} = $args{name};
   $self->{op} = $args{op};
   $self->{params} = "";
   $self->{routing_key} = $args{routing_key};

   #$self->{standard_values} = {};
   #$self->{context_values} = {};
   #$self->{prev_context_values} = {};
  
   $self->{workflow} = Vili::Functions->new( 
      opname => $args{name},
      help_text => $args{help_text},
      long_help_text => $args{long_help_text},
      op_version => $args{version},
   );

   # Bless ourselves as an instance of the class
   bless($self, $class);

   return ($self);
}

sub get_context
{
   my($hashref, $output) =  @_;

    # get rid of newline... so it is just data
    chomp($output);

    # get rid of the token separators here, we don't
    # need them
    $output =~ s/://g;

    # first split data by tab delimeter since
    # that is how token output is delivered
    (my @output_fields) = split ("\t", $output);

    # put data in context hash
    my $index;
    for ($index = 0; $index < scalar(@output_fields); $index += 2) { 
	
	# and add them to the context hash
	# e.g. $hashref->{State} = "OK"
	$hashref->{$output_fields[$index]} = $output_fields[$index+1];
    }

}

sub get_prev_context
{
   my $self = shift;

   my($hash_ref, $copy_name) =  @_;

   my $ret = ();
   my $errormsg = ();
   my $severity = ();
   my $retcode = ();

    # if there is no backup context file then we have no
    # data to get -- so leave the hash in its intialized
    # state
    if (!-e $copy_name) {
    	return($errormsg, $severity, $retcode);
    }

    $ret = open(BAKCONTEXT, "<", $copy_name);
    # open returns 1 on success, 0 on failure
    if (!$ret ) {
        $errormsg = "Unable to open $copy_name: $!";
        $severity = Vili::Functions::ERROR;
        $retcode = Vili::Functions::FAILURE;

    	return($errormsg, $severity, $retcode);
    }

    # read the file contents into arrays so
    my @oldlines;

    while (<BAKCONTEXT>) {
        push(@oldlines, $_);
    }

    # now let's pull out the info from the backup 
    # context file so we can use it
    for (my $index = 0; $index < scalar(@oldlines); $index++) {
	my($token, $value) = split (":", $oldlines[$index]);
	
	# remove any trailing spaces
	chomp($token, $value);

	# remove the preceding spaces
	$value =~ s/^\s+//;

	$hash_ref->{$token} = $value;
    }

    $ret = close(BAKCONTEXT);
    if (!$ret) { 
	$errormsg = "Problems closing $copy_name: $!";
        $severity = Vili::Functions::ERROR;
        $retcode = Vili::Functions::FAILURE;
    }
    $ret = unlink $copy_name;
    if (!$ret) { 
	$errormsg = "Problems closing $copy_name: $!";
        $severity = Vili::Functions::ERROR;
        $retcode = Vili::Functions::FAILURE;
    }

    return($errormsg, $severity, $retcode);

}

sub compare_context
{
   my $self = shift;

   my($prev_hash_ref, $hash_ref) =  @_;

   my $errormsg = ();
   my $severity = ();
   my $retcode = ();

   my $has_changed = 0;

    # compare the two different raw states to see if any thing has changed
    # but first, to be safe, double check that they are both defined.
    if ( defined $hash_ref->{current_state} && defined $prev_hash_ref->{current_state}) {
	if ($prev_hash_ref->{current_state} != $hash_ref->{current_state} ){
                $has_changed = 1;
    	}	
    } else {	
	$errormsg = "Problems accessing workflow state value in context hash, context file may be corrupt";
        $severity = Vili::Functions::ERROR;
        $retcode = Vili::Functions::FAILURE;
    }	

    return($has_changed, $errormsg, $severity, $retcode);

}

sub write_context
{
   my $self = shift;

   # standard subroutine
   my($hashref, $contextname, $mode) =  @_;
   my $ret = "";  
   my $severity = ();  
   my $retcode = ();  
   my $errormsg = ();  

    # now any pre-existing context file has been saved 
    # so lets go create one or open existing and truncate 
    # and see if there are any differences
    $ret = open (CONTEXT,">", "$contextname");
    if (!$ret) { 
	
        $severity = Vili::Functions::ERROR;
        $retcode = Vili::Functions::FAILURE;

    	return($errormsg, $severity, $retcode);
    }

    # write the data from the context file
    # we use the token as our key in our 
    # context hash 
    foreach my $key (sort keys %$hashref ) {
	
	# write output to context file
	# e.g. State: OK
	printf(CONTEXT "%s: %s\n", $key, (defined $hashref->{$key} ? $hashref->{$key} : "\t"));
    }

    $ret = close(CONTEXT);
    if (!$ret) { 
	$errormsg = "Problems closing $contextname: $!";
        $severity = Vili::Functions::ERROR;
        $retcode = Vili::Functions::FAILURE;
    }

    return($errormsg, $severity, $retcode);
}

sub format_header
{
   my $self = shift;

   my($mode, $freq) =  @_;

   my $publication_type;
   my $output =  "";

    # using the value of mode and frequency, determine 
    # what the publication type is
    if ($mode eq "interval" ) {
    	$publication_type = $INCREMENTAL;
    } elsif (($mode eq "baseline") && ($freq == 0)) {
    	$publication_type = $MANUAL;
    } else {
    	$publication_type = $BASELINE;
    } 
    
    # based on the definition of the protobuf fields enter the data
    # add health header information
    $output .= $publication_type . "\t";
    $output .= $freq . "\t";
    # add spchs error info
    my $errorno = 0;
    $output .= $errorno . "\t";
    # NOTE: don't put on trailing tab because we
    # will do that when we assemble the publication
    my $errortext = "";
    $output .= $errortext ;   

    return($output);

}

sub format_data
{
   my $self = shift;

   my($std_values, $prev_hash_ref, $hash_ref) =  @_;

   my $output =  "";

   # add standard information for this workflow
   $output .= (defined($std_values->{logical_object_type}) ? $std_values->{logical_object_type} : "") . "\t";
   $output .= (defined($std_values->{logical_object_subtype}) ? $std_values->{logical_object_subtype} : "") . "\t";
   $output .= (defined($std_values->{logical_object_name}) ? $std_values->{logical_object_name} : "") . "\t";
   $output .= (defined($std_values->{logical_object_qual_1}) ? $std_values->{logical_object_qual_1} : "") . "\t";
   $output .= (defined($std_values->{logical_object_qual_2}) ? $std_values->{logical_object_qual_2} : "") . "\t";
   $output .= (defined($std_values->{logical_object_path}) ? $std_values->{logical_object_path} : "") . "\t";
   $output .= (defined($std_values->{physical_object_type}) ? $std_values->{physical_object_type} : "") . "\t";
   $output .= (defined($std_values->{physical_object_subtype}) ? $std_values->{physical_object_subtype} : "") . "\t";
   $output .= (defined($std_values->{physical_object_name}) ? $std_values->{physical_object_name} : "") . "\t";

   # now add in the specifics from this run and the past run
   $output .= (defined($hash_ref->{current_state}) ? $hash_ref->{current_state} : "") . "\t";
   $output .= (defined($hash_ref->{current_state_raw}) ? $hash_ref->{current_state_raw} : "") . "\t";
   $output .= (defined($prev_hash_ref->{current_state}) ? $prev_hash_ref->{current_state} : "") . "\t";
   $output .= (defined($prev_hash_ref->{current_state_raw}) ? $prev_hash_ref->{current_state_raw} : "") . "\t";
   $output .= (defined($hash_ref->{state_change_ts_lct}) ? $hash_ref->{state_change_ts_lct} : "") . "\t";
   $output .= (defined($hash_ref->{state_change_ts_utc}) ? $hash_ref->{state_change_ts_utc} : "") . "\t";

   $output .= (defined($hash_ref->{current_health}) ? $hash_ref->{current_health} : "") . "\t";
   $output .= (defined($hash_ref->{current_health_raw}) ? $hash_ref->{current_health_raw} : "") . "\t";
   $output .= (defined($prev_hash_ref->{current_health}) ? $prev_hash_ref->{current_health} : "") . "\t";
   $output .= (defined($prev_hash_ref->{current_health_raw}) ? $prev_hash_ref->{current_health_raw} : "") . "\t";
   # use state_changexxx for health_changexxx
   $output .= (defined($hash_ref->{state_change_ts_lct}) ? $hash_ref->{state_change_ts_lct} : "") . "\t";
   $output .= (defined($hash_ref->{state_change_ts_utc}) ? $hash_ref->{state_change_ts_utc} : "") . "\t";

   return($output);
}

sub print_token
{
   my $self = shift;

   my $pub_data = shift;

   (my @output_fields) = split ("\t", $pub_data);

   my $index = 0;
   foreach my $label (@labels) {
      printf("%s\t%s\t", $label, (defined($output_fields[$index]) ? $output_fields[$index] : " "));
      $index++;
   }
   printf("\n");
}

sub print_list
{
   my $self = shift;

   my $pub_data = shift;

   $pub_data = $pub_data . "\t";
   (my @output_fields) = split ("\t", $pub_data);

   my $index = 0;
   foreach my $label (@labels) {
      printf("%s: %s\n", $label, (defined($output_fields[$index]) ? $output_fields[$index] : " "));

      $index++;
   }
    printf("\n");
}

sub do_check
{
   my $self = shift;

   my ($errormsg,$severity, $retcode);
   
   my $wf = $self->{workflow};

   if (!-x $self->{op}) {
      $errormsg = "Unable to execute file: $self->{op}";
      
      $wf->log(
          severity => Vili::Functions::ERROR,
          message => $errormsg,
      );
      $wf->op_exit(
          retcode => Vili::Functions::FAILURE,
          output => $errormsg,
      );
   }

   my $curr_context = $self->{context_values};
   my $prev_context = $self->{prev_context_values};

   my $cmd = "$self->{op} $self->{params} -token -vili=$wf->{vili}";

   print "Executing $cmd\n" if ($wf->{verbose});
   chomp(my $op_output = `$cmd`);

   # Check for bad exit status
   # and set our current states given the return status
   my $posixval = WEXITSTATUS($?);
   if ($posixval != 0) {
	if ($posixval == Vili::Functions::WARNING) {
		$curr_context->{current_state} = $SPSTATE_UP; 
		$curr_context->{current_state_raw} = "UP";

		$curr_context->{current_health} = $SPHEALTH_ABOVEWARNING;
		$curr_context->{current_health_raw} = $wf->get_enum_text(
			constvalue => Vili::Functions::WARNING,
		);

	} elsif ($posixval == Vili::Functions::CRITICAL) {
		$curr_context->{current_state} = $SPSTATE_DOWN; 
		$curr_context->{current_state_raw} = "DOWN";

		$curr_context->{current_health} = $SPHEALTH_ABOVEERROR;
		$curr_context->{current_health_raw} = $wf->get_enum_text(
			constvalue => Vili::Functions::CRITICAL,
		);

	} elsif ($posixval == Vili::Functions::UNKNOWN) {
		$curr_context->{current_state} = $SPSTATE_UNKNOWN; 
		$curr_context->{current_state_raw} = "UNKNOWN";

		$curr_context->{current_health} = $SPHEALTH_UNKNOWN;
		$curr_context->{current_health_raw} = $wf->get_enum_text(
			constvalue => Vili::Functions::UNKNOWN,
		);

	} else {
		# If it is not one of the expected values, then generate an error message and exit
                $errormsg = "Error when executing $self->{op}: " . $posixval;
                $errormsg .= " -- $op_output" if ($op_output);

                $wf->log(
                        severity => Vili::Functions::ERROR,
                        message => $errormsg,
                );
                $wf->op_exit(
                        retcode => Vili::Functions::FAILURE,
                        output => $errormsg,
                );
	} 
   } else {
	$curr_context->{current_state} = $SPSTATE_UP; 
	$curr_context->{current_state_raw} = $wf->get_enum_text(
		constvalue => Vili::Functions::OK,
	);
	$curr_context->{current_health} = $SPHEALTH_READY;
	$curr_context->{current_health_raw} = $wf->get_enum_text(
		constvalue => Vili::Functions::OK,
	);
   }

   # now go get the time that we got this result
   # save the time of this workflow in case this is the time of our state change.
   ($curr_context->{workflow_ts_lct}, $curr_context->{workflow_ts_utc}) = &GetTimeStamp;

   # now go and create/truncate the context file
   # and save the old one, if it existed
   # then return a flag to indicate that we had to
   # create the context file which means that we
   # have a state change
   my $has_changed = $wf->create_context();
   if ($has_changed) {
      # now that we know we have a state change, then let's save the 
      # timestamp in the state change fields
      $curr_context->{state_change_ts_lct} = $curr_context->{workflow_ts_lct};
      $curr_context->{state_change_ts_utc} = $curr_context->{workflow_ts_utc};
  }

   # get name of backup context file
   my $copy_name = $wf->get_backup_context();

   # now let's go get the previous context info, if any
   # and put it in a hash to hold the previous context values
   ($errormsg, $severity, $retcode) = $self->get_prev_context($prev_context, $copy_name);

   # go put the data in the context hash
   $self->get_context($curr_context, $op_output);

   # if we had an error message returned, that means that
   # there was a problem with our get_prev_context call.  
   # Most likely, this has to do with the file handling of the backup context file
   # So report it and exit
   if (defined $errormsg && length($errormsg)) {
	$wf->log(
                severity => $severity,
                message => $errormsg,
        );
        $wf->op_exit(
                 retcode => $retcode,
                 output => $errormsg,
        );
   }


   # if we haven't already determined that we have a state change
   # and if we are doing mode interval, then we need to see
   # if something has changed in our output compared to 
   # last time before we determine that we need to send a publication. 
   if (!$has_changed && $wf->{mode} eq "interval") {
      ($has_changed, $errormsg, $severity, $retcode)  = $self->compare_context($prev_context, $curr_context);

      # did we have a problem getting the State value?  
      # if so then something is really wrong
      if (defined $errormsg && length($errormsg)) {
	 $wf->log(
               	severity => $severity,
               	message => $errormsg,
       	 );
       	 $wf->op_exit(
               	retcode => $retcode,
               	output => $errormsg,
       	 );
      }

      if ($has_changed) {
 	 # now that we know we have a state change, then let's save the 
	 # timestamp in the state change fields
	 $curr_context->{state_change_ts_lct} = $curr_context->{workflow_ts_lct};
	 $curr_context->{state_change_ts_utc} = $curr_context->{workflow_ts_utc};
      }
   }

   # TODO: handle flapping?  How, hold it in context file
   # but be sure not to compare that line??
   # We should maintain outcomes for the past 5 times, and if we are
   # switching in and out of OK, then set flapping and don't publish
   # We need to take these values out of backup context, remove oldest
   # put in our recent outcome as 5th, evaluate the outcomes and set flapping
   # also save our flapping state in context..

   # if we are running in interval mode then we should 
   # write out/update our context file, now that we have changed
   # all fields that we plan to
   if ($wf->{mode} eq "interval") {
      ($errormsg, $severity, $retcode) = 
            $self->write_context($curr_context, $wf->{context}, $wf->{mode});

      # if we had an error message returned, that means that
      # there was a problem with our write_context call.  
      # So report it and exit
      if (defined $errormsg && length($errormsg)) {
	 $wf->log(
                severity => $severity,
                message => $errormsg,
         );
         $wf->op_exit(
                 retcode => $retcode,
                 output => $errormsg,
         );
      }
   }

   # format the information for the health header
   my $mode = $wf->{mode};
   my $freq = $wf->{frequency};
   my $hdr_data = $self->format_header($mode, $freq);

   # now go get the data
   my $pub_data = $self->format_data($self->{standard_values},
                                     $prev_context, $curr_context);

   my $num_repeats = 1;
   $pub_data = "<info_header>\t" . $hdr_data. "\t" . $num_repeats . "\t" . $pub_data;

   if (($wf->{mode} eq "baseline" || $has_changed) && $wf->{publication}) {

	# if we want verbose output, then let's display what we are
	# sending to TPA_Publish
	if ($wf->{verbose}) {
		$wf->log(
			severity => Vili::Functions::INFO,
			message => "TPA_PUBLISH is $TPA_PUBLISH; opname is $wf->{opname}; routing key is $self->{routing_key}; data is $pub_data",
			);
	} 

	system($TPA_PUBLISH, $wf->{opname}, $self->{routing_key}, 
                             $curr_context->{workflow_ts_utc}, 
                             $curr_context->{workflow_ts_lct}, $pub_data);

	if (WIFEXITED($?) && (WEXITSTATUS($?) != 0)) {
       		$errormsg = "Error when executing $TPA_PUBLISH: " . $?;
       		$wf->log(
			severity => Vili::Functions::ERROR,
               		message => $errormsg,
       		);
       		$wf->op_exit(
               		retcode => Vili::Functions::FAILURE,
               		output => $errormsg,
       		);
	}
   }

   $self->print_token($pub_data) if ($wf->{token});
   $self->print_list($pub_data) if ($wf->{list});

}

sub exit
{
   my $self = shift;

   my $wf = $self->{workflow};

   # there is no output for our workflow
   # since we have already handled it
   my $formattedoutput = "";

   $wf->op_exit ( 
        retcode => Vili::Functions::OK,
	output => $formattedoutput,
	);
}

1;

