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

package Operation;

use 5.008000;
use strict;
use warnings;
use POSIX;
use Params::Validate ':all';

# The PERL5LIB env variable isn't available to us, so we dynamically add
# $MY_SQROOT/export/lib to @INC here.
my $SQROOTLIB;
BEGIN { $SQROOTLIB = $ENV{'MY_SQROOT'} . "/export/lib"; }

use lib $SQROOTLIB;

# use library to access Vili functions
use Vili::Functions ':all';

# use library to access Vili helper routines
use Vili::Helper ':all';

sub new
{
   my $class = shift;

    my %args = validate( @_,
      {
         name => {
            type => SCALAR,
            regex => qr/^\w+$/  # that is all word chars
         },
         script => 0,
         help_text =>{
            type => SCALAR
         },
         long_help_text =>{
            type => SCALAR
         },
         version =>{
            type => SCALAR
         },
      }
   );

   my $self = {};
   $self->{name} = $args{name};
   $self->{script} = $args{script};
   $self->{params} = "";

   $self->{code} = 0;
   $self->{output} = "";

   $self->{operation} = Vili::Functions->new(
	opname => $self->{name},
        help_text => $args{help_text},
        long_help_text => $args{long_help_text},
	op_version => $args{version},
   );

   bless $self, $class;

   return $self;

}

sub format_output
{
   my $self = shift;
   my($output) =  @_;
   my $formatoutput =  "";

   # default output for operation: name & status
    my $status = '';
    if($self->{code} == Vili::Functions::OK){
        $status = 'OK';
    }elsif($self->{code} == Vili::Functions::WARNING){
        $status = 'WARNING';
    }elsif($self->{code} == Vili::Functions::CRITICAL){
        $status = 'CRITICAL';
    }elsif($self->{code} == Vili::Functions::UNKNOWN){
        $status = 'UNKNOWN';
    }

    my $op = $self->{operation};
    if ($op->{list}) {
        $formatoutput = "Name: $self->{name}\n";
        $formatoutput .= "Status: $status\n";
    } else {
        $formatoutput = "Name\t$self->{name}\t";
        $formatoutput .= "Status\t$status\t";
    }
    if ($op->{token}) {
        $formatoutput .= "\n";
    }


   return $formatoutput;

}

sub do_check
{
   my $self = shift;

   my $op = $self->{operation};

   my $errormsg;

   if (!-x $self->{script}) {
      $errormsg = "Unable to execute file: $self->{script}";

      $op->log(
          severity => Vili::Functions::ERROR,
          message => $errormsg,
      );
      $op->op_exit(
          retcode => Vili::Functions::FAILURE,
          output => $errormsg,
      );
   }

   # Now perform the work of the operation
   my $cmd = "$self->{script} $self->{params}";
   chomp(my $output = `$cmd 2>&1`);

   # Check for bad exit status
   my $return_code;
   my $posixval = WEXITSTATUS($?);
   if ($posixval != 0) {
      if ($posixval == Vili::Functions::WARNING) {
                $return_code = Vili::Functions::WARNING;
      } elsif ($posixval == Vili::Functions::CRITICAL) {
                $return_code = Vili::Functions::CRITICAL;
      } elsif ($posixval == Vili::Functions::UNKNOWN) {
                $return_code = Vili::Functions::UNKNOWN;
      } else {

         # If it is not one of the expected values, then log an error, generate an error message and exit
         my $errormsg = "Unable to execute $self->{script}: " . $posixval;
         $op->log(
            severity => Vili::Functions::ERROR,
            message => $errormsg,
         );
         $op->op_exit(
            retcode => Vili::Functions::FAILURE,
            output => $errormsg,
         );
      }
   } else {
        $return_code = Vili::Functions::OK;
   }

   $self->{code} = $return_code;
   $self->{output} = $self->format_output($output);
}

sub exit
{
   my $self = shift;
   my $op = $self->{operation};

   $op->op_exit (
        retcode => $self->{code},
        output => $self->{output},
    );
}
