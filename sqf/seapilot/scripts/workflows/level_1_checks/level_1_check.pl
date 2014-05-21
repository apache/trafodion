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

# definitions from the spstate.h file
my $SPSTATE_NULL = 0;
my $SPSTATE_UNKNOWN = 1;
my $SPSTATE_NONEXISTENT = 2;
my $SPSTATE_UP = 3;
my $SPSTATE_DOWN = 4;
my $SPSTATE_MAINTENANCE = 5;

# definitions from the sphealth.h file
my $SPHEALTH_NULL = 0;
my $SPHEALTH_UNKNOWN = 1;
my $SPHEALTH_READY = 2;
my $SPHEALTH_NOTREADY = 3;
my $SPHEALTH_DEGRADED = 4;
my $SPHEALTH_FLAPPING = 5;
my $SPHEALTH_ABOVEWARNING = 6;
my $SPHEALTH_BELOWWARNING = 7;
my $SPHEALTH_ABOVEERROR = 8;
my $SPHEALTH_BELOWERROR = 9;
my $SPHEALTH_NOTPREFERREDNODE = 10;
my $SPHEALTH_NOTPREFERREDPATH = 11;

# definitions for scores
my $SPSCORE_NULL = -1;
my $SPSCORE_GREEN = 0;
my $SPSCORE_YELLOW = 1;
my $SPSCORE_RED = 2;
my $SPSCORE_UNKNOWN = 3;

   
# here is the program to call to send the publication
my $TPA_PUBLISH = "$SQROOT/seapilot/scripts/TPA_Publish";

# here is the base name of the program to call
# to massage the timestamps the way we need them for SeaPilot
my $SSP_TIMESTAMP = "$SQROOT/seapilot/scripts/ssp_providerTimestamp";


package Level_1_check;

use strict;
use 5.008000;
use warnings;
use POSIX;
use List::Util qw/min max/;
use Data::Dumper;
use Params::Validate ':all';

use Vili::Functions ':all';
use Vili::Helper ':all';

our @ALLLAYERS = ('accesslayer', 'oslayer', 'databaselayer', 
                  'serverlayer', 'foundationlayer', 'storagelayer');
                  
our $help_text = "%PROGRAM% is a workflow that will performance the %LAYER% checks for the instance
This operation accepts the following arguments:
        -verbose - gives debug information (optional)
        -mode {interval | baseline} - defines how the workflow reports health/state information; interval indicates that only changes since the last run should be reported and baseline indicates that the health/state should always be reported; baseline is the default (optional)
        -list - indicates that output should be provided in human readable format; this is the default value (optional)
        -token - indicates that output should be provided in an easily parsible format (optional)
        -publication - indicates that the output should be provided in a publication; if neither list, token or pbulication is reported then the default of list is used (optional)
        -context=FILE - provides the fully qualified path of the file where the workflow should place its context file.  If the context argument is omitted, then /home/{squserid}/context/{nodeid}/%PROGRAM%/automatic is used as the default value. (optional)
        -config-file=FILE - provides the fully qualified path of the config file. If the context argument is ommited, then $SQROOT/seapilot/export/conf/level_1/%LAYER%.cfg is used as the default value. (optional)
        -version - returns the version information of the operation (optional)
        -help - returns the long usage information for the operation (optional)
        -long_help - returns the man page for the operation (optional) 
	";

our $long_help_text = "%PROGRAM%

Usage: %PROGRAM%  -verbose -mode {interval | baseline} -list -token - publication -frequency=VALUE -context=FILE -version -help -long_help -config-file=FILE

This operation accepts the following arguments:
        -verbose - gives debug information (optional)
        -mode {interval | baseline} - defines how the workflow reports health/state information; interval indicates that only changes since the last run should be reported and baseline indicates that the health/state should always be reported; baseline is the default (optional)
        -list - indicates that output should be provided in human readable format; this is the default value (optional)
        -token - indicates that output should be provided in an easily parsible format (optional)
        -publication - indicates that the output should be provided in a publication; if neither list, token or pbulication is reported then the default of list is used (optional)
        -context=FILE - provides the fully qualified path of the file where the workflow should place its context file.  If the context argument is omitted, then /home/{squserid}/context/{nodeid}/%PROGRAM%/automatic is used as the default value. (optional)
        -config-file=FILE - provides the fully qualified path of the config file. If the context argument is ommited, then $SQROOT/seapilot/export/conf/level_1/%LAYER%.cfg is used as the default value. (optional)
        -version - returns the version information of the operation (optional)
        -help - returns the long usage information for the operation (optional)
        -long_help - returns the man page for the operation (optional)


This workflow is used to determine the current state of %LAYER% of the instance on the current node.
The workflow will return the results of the check if baseline mode has been selected or if interval mode has been selected and a change in state has been determined.  
If there is a problem executing the workflow then it will
return a non-zero value.  If the workflow succeeds then the
workflow will return a successful exit(0).  ";

# labels for fields when printing list or token
# ****CHANGE THESE FIELDS AS APPROPRIATE FOR YOUR WORKFLOW ****
our @labels = ("info_header"
             , "publication_type"
             , "check_interval_sec"
             , "error" 
             , "error_text" 
             , "layer_name" 
             , "layer_current_score" 
             , "layer_previous_score" 
             , "layer_score_change_ts_lct"
             , "layer_score_change_ts_utc"
             , "repeated");

our @sub_labels = ("subject_name"
                 , "subject_current_score"
                 , "subject_previous_score"
                 , "subject_score_change_ts_lct"
                 , "subject_score_change_ts_utc");

# ################### sub new ###########################
sub new {
   my $class = shift;
   
   # validate the arguments passed in to the constructor
   my %args = validate( @_,
      { 
        layer        => {
                          type  => SCALAR,    # a scalar
                          regex => qr/^\w+$/  # that is all word chars
                        },
        opname       => {
                          type  => SCALAR,    # a scalar
                          regex => qr/^\w+$/  # that is all word chars
                        },
        routing_key  => {
                          type  => SCALAR,       # a scalar
                          regex => qr/[\w_\.]+/  # that is: word chars, '_' and '.'
                        }
      }
   );
   
   my $self = {};
   
   $self->{layer} = $args{layer};
   
   die "Invalid layer name: $self->{layer} \n"
      unless (grep(/$self->{layer}/, @ALLLAYERS));

   
   $self->{opname} = $args{opname};
   $self->{routing_key} = $args{routing_key};
   
   $self->{curr_context} = {
       "Name"                      => $self->{opname},
       "Error"                     => 0,
       "State"                     => "OK",
       "layer_name"                => $self->{layer},
       "layer_current_score"       => undef,
       "layer_score_change_ts_lct" => undef,
       "layer_score_change_ts_utc" => undef,
       "workflow_ts_lct" 	   => undef,
       "workflow_ts_utc" 	   => undef,
       "workflow_error" 	   => undef,
       "subjects"                  => {},
   };

   $self->{prev_context} = {
       "Name"                      => $self->{opname},
       "Error"                     => 0,
       "State"                     => "OK",
       "layer_name"                => $self->{layer},
       "layer_current_score"       => undef,
       "layer_score_change_ts_lct" => undef,
       "layer_score_change_ts_utc" => undef,    
       "workflow_ts_lct" 	   => undef,
       "workflow_ts_utc" 	   => undef,
       "workflow_error" 	   => undef,
       "subjects"                   => {},
   };
  
   $help_text =~ s/%PROGRAM%/$args{opname}/g;
   $help_text =~ s/%LAYER%/$args{layer}/g;
   
   $long_help_text =~ s/%PROGRAM%/$args{opname}/g;
   $long_help_text =~ s/%LAYER%/$args{layer}/g;
   
   $self->{workflow} = Vili::Functions->new( 
      opname => $self->{opname},
      help_text => $help_text,
      long_help_text => $long_help_text,
      op_version => "1.0.0",
   );

   $self->{workflow}->add_args(
      arginfo => 'config-file=s',
      default => "$SQROOT/seapilot/export/conf/level_1/$self->{layer}.cfg",
   );

   # Bless ourselves as an instance of the class
   bless($self, $class);

   return ($self);
}

###############Subroutines######################
# This sub calculate the score from a health state
sub get_score
{
   my $sphealth = shift;

   return $SPSCORE_UNKNOWN unless (isvalid_integer($sphealth));
   
   return $SPSCORE_GREEN if ($sphealth == $SPHEALTH_READY);              # 2
                         
                         
   return $SPSCORE_YELLOW if ($sphealth == $SPHEALTH_DEGRADED            # 4
                          || $sphealth == $SPHEALTH_FLAPPING             # 5
                          || $sphealth == $SPHEALTH_ABOVEWARNING         # 6
                          || $sphealth == $SPHEALTH_BELOWWARNING         # 7 
                          || $sphealth == $SPHEALTH_NOTPREFERREDNODE     # 10
                          || $sphealth == $SPHEALTH_NOTPREFERREDPATH);   # 11
                          
   return $SPSCORE_RED    if ($sphealth == $SPHEALTH_NOTREADY            # 3
                          || $sphealth == $SPHEALTH_ABOVEERROR           # 8
                          || $sphealth == $SPHEALTH_BELOWERROR);         # 9

   return $SPSCORE_UNKNOWN;   # $SPHEALTH_NULL = 0;
                             # $SPHEALTH_UNKNOWN = 1;
}

#
# This sub obtains the information that was 
# set from our last run, which is in our 
# backup copy. An then deletes the backup copy.
# ****YOU MAY NOT NEED TO CHANGE THIS SUBROUTINE AT ALL ****
#
# Input: reference to hash structure to hold info, backup context file name
#
# Output: severity of problem if problem found, return code value for problem, if found, and 
#		the actual error message itself
#
sub get_prev_context
{
   my ($hash_ref, $copy_name) =  @_;

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

   my $in_subject = 0;
   my $ref;
   while (<BAKCONTEXT>) {
      chomp;
      my ($token, $value) = split /:/;

      # remove the preceding spaces
      $token =~ s/^\s+//; $token =~ s/\s+$//;
      $value =~ s/^\s+//; $value =~ s/\s+$//;

      if ($token eq "subject_name") {
         $in_subject = 1;
         $hash_ref->{subjects}{$value} = {};
         $ref = $hash_ref->{subjects}{$value};
         $ref->{subject_name} = $value;

      }
      elsif ($in_subject == 1) {
         $ref->{$token} = $value;
      }
      else {
         $hash_ref->{$token} = $value;
      }
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

#
# This sub is called to take all of the context info
# that is in the context hash and write it out to
# the context file
# ****YOU MAY NOT NEED TO CHANGE THIS SUBROUTINE AT ALL ****
#
# Input: hash reference containing context info, context file name, 
#			mode used for workflow
#
# Output: severity of problem if problem found, return code value for problem, if found, and 
#		the actual error message itself
#
sub write_context
{
   my($hashref, $contextname) =  @_;
   my $ret = "";  
   my $severity = ();  
   my $retcode = ();  
   my $errormsg = ();  

   # now any pre-existing context file has been saved 
   # so lets go create one or open existing and truncate 
   # and see if there are any differences
   $ret = open (CONTEXT,">", "$contextname");
   if (!$ret) { 
	$errormsg = "Unable to open $contextname: $!";
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
      printf(CONTEXT "%s: %s\n", $key, (defined $hashref->{$key} ? $hashref->{$key} : "")) unless ($key eq "subjects");
   }
   
   foreach my $subject (sort keys %{$hashref->{subjects}}) {
      my $ref = $hashref->{subjects}->{$subject};
      foreach my $key (("subject_name", "subject_current_score", 
                      "subject_score_change_ts_lct","subject_score_change_ts_utc")) {
          printf(CONTEXT "%s: %s\n", $key, (defined $ref->{$key} ? $ref->{$key} : ""));
      }
   }

   $ret = close(CONTEXT);
   if (!$ret) { 
	$errormsg = "Problems closing $contextname: $!";
        $severity = Vili::Functions::ERROR;
        $retcode = Vili::Functions::FAILURE;
   }

   return($errormsg, $severity, $retcode);
}

#
#
# Input: previous contents of context file, current contents of context file
#        subject name, and token
# Output: a flag indicating whether a state change has occurred
#
sub compare_context
{
   my($prev_hash_ref, $hash_ref, $sub_name, $token) =  @_;

   my $has_changed = 0;

   if (defined $hash_ref->{subjects}->{$sub_name}->{$token}) {
      if (!defined $prev_hash_ref->{subjects}->{$sub_name}->{$token}) {
         $has_changed = 1;
      }
      elsif ($prev_hash_ref->{subjects}->{$sub_name}->{$token} 
            != $hash_ref->{subjects}->{$sub_name}->{$token}) {
         $has_changed = 1;
      }
    } 
    
    return $has_changed;
}

#
# This sub obtains the subjects to check for the layer 
# also, there will be the script name and some fixed paramters for that check
#
# Input: reference to hash structure to hold subject info, config file name
#
# Output: severity of problem if problem found, return code value for problem, if found, and 
#		the actual error message itself
#
sub get_subjects
{
   my($hash_ref, $cfg_name) =  @_;

   my $errormsg = ();
   my $severity = ();
   my $retcode = ();

   if (!-f $cfg_name) {
      $errormsg = "Config file does not exist: $cfg_name.";
      $severity = Vili::Functions::ERROR;
      $retcode = Vili::Functions::FAILURE;
      
      return($errormsg, $severity, $retcode);
   }
   
   my $ret = open(CFGFILE, "<", $cfg_name);
   # open returns 1 on success, 0 on failure
   if (!$ret ) {
      $errormsg = "Unable to open $cfg_name: $!";
      $severity = Vili::Functions::ERROR;
      $retcode = Vili::Functions::FAILURE;

      return($errormsg, $severity, $retcode);
   }

   while (<CFGFILE>) {
      chomp;
      next if /^\s*$/;
      next if /^#/;
      
      my ($name, $value) = split /:/;

      # remove the preceding spaces
      $name =~ s/^\s+//; $name =~ s/\s+$//;
      $value =~ s/^\s+//; $value =~ s/\s+$//;

      $hash_ref->{$name} = $value;
   }

   $ret = close(CFGFILE);
   if (!$ret) { 
      $errormsg = "Problems closing $cfg_name: $!";
      $severity = Vili::Functions::ERROR;
      $retcode = Vili::Functions::FAILURE;
   }
   
   return($errormsg, $severity, $retcode);
}

#
# This sub formats the health header information
# The publication fiels are tab separated.
#
# Input: previous contents of context file, current contents of context file, 
#		the current mode, and the current frequency
#
# Output: string of data that is ready to be passed to TPA
#
sub FormatHdr
{
    my ($self, $mode, $freq) =  @_;

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
    # errorno
    $output .= $self->{curr_context}->{Error} . "\t"; 
    # error_text
    $output .= $self->{curr_context}->{State};

    return($output);

}

#
# This sub formats our context data, previous context
# data and some standard values into a publication
# The publication fiels are tab separated.
#
# Input: previous contents of context file, 
#        current contents of context file, 
#	 flag to show whether containing subject data
#
# Output: string of data that is ready to be passed to TPA
#
sub FormatData
{
   my($prev_hash_ref, $hash_ref, $need_data) =  @_;

   $need_data = 1; # for HPDM, don't set null value, temp solution

   my $output =  "";

   $output .= $hash_ref->{layer_name} . "\t";
   $output .= (defined($hash_ref->{layer_current_score}) ? 
               $hash_ref->{layer_current_score} : ""). "\t";
   $output .= (defined($prev_hash_ref->{layer_current_score}) ? 
               $prev_hash_ref->{layer_current_score} : "") . "\t";
   $output .= (defined($hash_ref->{layer_score_change_ts_lct}) ? 
               $hash_ref->{layer_score_change_ts_lct} : "") . "\t";
   $output .= (defined($hash_ref->{layer_score_change_ts_utc}) ? 
               $hash_ref->{layer_score_change_ts_utc} : "") . "\t";

   my $repeated = scalar(keys %{$hash_ref->{subjects}});

   if ($repeated > 0)
   {
      $output .= $repeated . "\t";
   
      foreach my $subject (sort keys %{$hash_ref->{subjects}}) {

         my ($ref, $prev_ref) = (undef, undef);
         if($need_data) {
            $ref = $hash_ref->{subjects}->{$subject};
            $prev_ref = $prev_hash_ref->{subjects}->{$subject};
         }
         $output .= $subject . "\t";
         
         $output .= (defined($ref->{subject_current_score}) ? $ref->{subject_current_score} : "") . "\t";
         $output .= (defined($prev_ref->{subject_current_score}) ? $prev_ref->{subject_current_score} : "") . "\t";
 
         $output .= (defined($ref->{subject_score_change_ts_lct}) ? $ref->{subject_score_change_ts_lct} : "") . "\t";
         $output .= (defined($ref->{subject_score_change_ts_utc}) ? $ref->{subject_score_change_ts_utc} : "") . "\t";
      }
   }

   return($output);
}

#
# This sub is called to print the fomatted data in
# a token value output format
#
# Input: formatted data
#
# Output: none
#
sub print_token
{
   my($pub_data) =  @_;

   my @output_fields = split /\t/, $pub_data . " ";

   my $index = 0;
   foreach my $label (@labels) {
      print "$label\t"
            , (defined($output_fields[$index]) ? $output_fields[$index] : "")
            , "\t";

      $index++;
   }

   while ($index < $#output_fields) {
      foreach my $sub_label (@sub_labels) {
         print "$sub_label\t"
            , (defined($output_fields[$index]) ? $output_fields[$index] : "")
            , "\t";

         $index++;
      }
   }
   print "\n";

}

#
# This sub is called to print the fomatted data in
# a list output format
#
# Input: formatted data
#
# Output: none
#
sub print_list
{
   my $pub_data = shift;

   my @output_fields = split /\t/, $pub_data . " ";

   my $index = 0;
   foreach my $label (@labels) {
      print "$label : "
            , (defined($output_fields[$index]) ? $output_fields[$index] : "")
            , "\n";

      $index++;
   }

   while ($index < $#output_fields) {
      foreach my $sub_label (@sub_labels) {
         print "$sub_label : "
            , (defined($output_fields[$index]) ? $output_fields[$index] : "")
            , "\n";

         $index++;
      }
   }
   print "\n";
}

# this sub is called to perform the check
sub do_check {
   my $self = shift;

   my $wf = $self->{workflow};

   $wf->getopts;

   print "Opname is $wf->{opname}\n"
       , "list is $wf->{list}\n"
       , "token is $wf->{token}\n"
       , "publication is $wf->{publication}\n"
       , "mode is $wf->{mode}\n"
       , "frequency is $wf->{frequency}\n"
       , "context is $wf->{context}\n"
       , "version is $wf->{version}\n"
       , "config-file is $wf->{'config-file'}\n"
       , "\n"
      if ($wf->{verbose});
   
   ($self->{curr_context}->{workflow_ts_lct}, 
    $self->{curr_context}->{workflow_ts_utc}) = &GetTimeStamp;

   my $has_changed = $wf->create_context();

   # get name of backup context file
   my $copy_name = $wf->get_backup_context();
   
   my ($errormsg, $severity, $retcode) = 
      get_prev_context($self->{prev_context}, $copy_name);

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
   
   my %subjects = ();
   ($errormsg, $severity, $retcode)  = 
      get_subjects(\%subjects, $wf->{"config-file"});

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

   my $max_score = $SPSCORE_NULL;

   foreach my $sub_name (sort keys %subjects)
   {

      my $max_sub_score;

      my $refsubject = {};
      $self->{curr_context}->{subjects}{$sub_name} = $refsubject;

      $refsubject->{subject_name} = $sub_name;

      $subjects{$sub_name} = $SQROOT . $subjects{$sub_name};
      my ($cmd) = split /\s/, $subjects{$sub_name};

      if (!-x $cmd) {
         $wf->log(
            severity =>  Vili::Functions::ERROR,
            message => "Unable to execute file: $cmd"
         );
         # set score to UNKNOWN if workflow cannot be found
         $max_sub_score = $SPSCORE_UNKNOWN;
      }
      else {
         my $params = " -mode=$wf->{mode} -frequency=$wf->{frequency}";
         $params .= " -publication" if ($wf->{publication});
         $params .= " -token -vili=$wf->{vili}";
   
         chomp (my $output = `$subjects{$sub_name} $params 2>/dev/null`);
   
         my $posixval = WEXITSTATUS($?);
         if ($posixval != 0) {
            if ($posixval == Vili::Functions::WARNING) {
               $self->{curr_context}->{Error} = Vili::Functions::WARNING;
               $self->{curr_context}->{State} = $wf->get_enum_text(
                        constvalue => Vili::Functions::WARNING);
            } elsif ($posixval == Vili::Functions::CRITICAL) {
               $self->{curr_context}->{Error} = Vili::Functions::CRITICAL;
	       $self->{curr_context}->{State} = $wf->get_enum_text(
                        constvalue => Vili::Functions::CRITICAL); 
            } elsif ($posixval == Vili::Functions::UNKNOWN) {
               $self->{curr_context}->{Error} = Vili::Functions::UNKNOWN;
               $self->{curr_context}->{State} = $wf->get_enum_text(
                        constvalue => Vili::Functions::UNKNOWN); 
            } else {
               $self->{curr_context}->{Error} = Vili::Functions::ERROR;
               $self->{curr_context}->{State} = $wf->get_enum_text(
                        constvalue => Vili::Functions::ERROR); 
            }

            $errormsg = "Error when executing $cmd: " . $posixval;
            $wf->log(
               severity => Vili::Functions::WARN,
               message => $errormsg,
            );

            # set score to UNKNOWN if workflow return non-zero
            $max_sub_score = $SPSCORE_UNKNOWN;
         } 
         else { # $posixval == 0
            $self->{curr_context}->{Error} = Vili::Functions::OK;
            $self->{curr_context}->{State} = $wf->get_enum_text(
                        constvalue => Vili::Functions::OK); 
                        my @tokens = split /\t/, $output;
                       
            $max_sub_score = $SPSCORE_NULL;
            while (@tokens > 1) {
               my $key = shift @tokens;
               my $val = shift @tokens;

               if ($key eq "current_health") {             
                  $max_sub_score = max($max_sub_score, get_score($val));
               }
            }
         }
      }
      
      if ($max_sub_score == $SPSCORE_NULL) {
         my $prev_context = $self->{prev_context};
         if (defined($prev_context->{subjects}{$sub_name}->{subject_current_score}))
         {
            $max_sub_score = 
               $prev_context->{subjects}{$sub_name}->{subject_current_score};

         }
         else {
            $max_sub_score = $SPSCORE_GREEN; # the first time layer_check run, 
                                             # but subject_check has run for several time
                                             # not a leagal state
         }
      }
      $refsubject->{subject_current_score} = $max_sub_score;
   
      my $sub_changed = compare_context($self->{prev_context}, 
                            $self->{curr_context}, 
                            $sub_name, "subject_current_score" );

      if ($sub_changed) {
         $has_changed = 1;
         $refsubject->{subject_score_change_ts_lct} = 
            $self->{curr_context}->{workflow_ts_lct};
         $refsubject->{subject_score_change_ts_utc} = 
            $self->{curr_context}->{workflow_ts_utc};
      }
      else {
         $refsubject->{subject_score_change_ts_lct} = 
            $self->{prev_context}->{subjects}->{$sub_name}->{subject_score_change_ts_lct};
          
         $refsubject->{subject_score_change_ts_utc} = 
            $self->{prev_context}->{subjects}->{$sub_name}->{subject_score_change_ts_utc};
      }
   
      # analysis the output to calculate the score of the subject
      $max_score = max($max_score, $max_sub_score);
   }

   $max_score = $SPSCORE_UNKNOWN if ($max_score == $SPSCORE_NULL);
  
   $self->{curr_context}->{layer_current_score} = $max_score;
          
   if ($has_changed) {
      $self->{curr_context}->{layer_score_change_ts_lct} = 
                 $self->{curr_context}->{workflow_ts_lct};
      $self->{curr_context}->{layer_score_change_ts_utc} = 
                 $self->{curr_context}->{workflow_ts_utc};

   }
   else {

      $self->{curr_context}->{layer_score_change_ts_lct} = 
         $self->{prev_context}->{layer_score_change_ts_lct};
      $self->{curr_context}->{layer_score_change_ts_utc} = 
         $self->{prev_context}->{layer_score_change_ts_utc};
   }

   if ($wf->{mode} eq "interval") {
      # write out our context file if anything changed
      ($errormsg, $severity, $retcode) = 
         write_context($self->{curr_context}, $wf->{context});

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
 
   # if we need to send any output
   # then go format the output found in the context file

   # format the information for the health header
   my $mode = $wf->{mode};
   my $freq = $wf->{frequency};
   my $hdr_data = $self->FormatHdr($mode, $freq);

   # now go get the data for the publication
   my $pub_data = FormatData($self->{prev_context}, $self->{curr_context}, 
                             ($wf->{mode} eq "baseline") || $has_changed);

   $pub_data = "<info_header>\t" . $hdr_data . "\t" . $pub_data;

   if ($wf->{publication}) {
      system($TPA_PUBLISH, $self->{opname}, $self->{routing_key}, 
             $self->{curr_context}->{workflow_ts_utc}, 
             $self->{curr_context}->{workflow_ts_lct}, 
             $pub_data);

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

   print_token($pub_data) if ($wf->{token});

   print_list($pub_data) if ($wf->{list});

   my $formattedoutput = "";
   
   $wf->op_exit ( 
      retcode => Vili::Functions::OK,
	  output => $formattedoutput,
   );
}

# end of package level_1_check

1;
