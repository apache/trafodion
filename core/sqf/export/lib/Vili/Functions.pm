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

package Vili::Functions;

=head1 NAME

Vili::Functions - a module that encapsulates the functionsl to be used by Vili
operations and workflows

=head1 SYNOPSIS

    use Vili::Functions;

    my $wf = Vili::Functions->new(
        opname => "my_op_name",
        help_text => $my_help_text,
        long_help_text => $my_long_help_text,
        op_version => "1.0.0",
    );

    $wf->add_args(
        arginfo => 'hostname=s',
        default => "172.31.2.254",
    );

    $wf->getopts;

    my $has_changed = $wf->create_context();

    my $copy_name = $wf->get_backup_context();

    my $enum_string = $wf->get_enum_text(Vili::Functions::INFO);

    $wf->log(
        severity => Vili::Functions::INFO,
        message => "Operation my_op_name started.",
    );

    $wf->op_exit(
        retcode => Vili::Functions::FAILURE,
        output => $errormsg,
    );

    $wf->op_die(
        output => $errormsg,
    );

=head1 DESCRIPTION

A Perl class that represents a Vili operation or workflow.  It provides an
object oriented interface to the various functions required by scripts.
This class will ensure that all Vili operations and workflows use the
same mechanism to check script arguments, log messages, etc.

=head1 CLASS DATA

=head2 CONSTANTS

=over 4

=item * C<Vili::Functions::OK> = 0

=item * C<Vili::Functions::WARNING> = 1

=item * C<Vili::Functions::CRITICAL> = 2

=item * C<Vili::Functions::UNKNOWN> = 3

=item * C<Vili::Functions::FAILURE> = 4

=item * C<Vili::Functions::INFO> = 5

=item * C<Vili::Functions::WARN> = 6

=item * C<Vili::Functions::ERROR> = 7

=back

=head1 FUNCTIONS

=over 4

=item C<new()>

Returns a new object of the Vili::Functions class. This function requires the
name of the new operation or workflow, the help text for the new script, a long
help text, or man page, for the new script and the version of the script.  All
scripts will start with the following option string:

'verbose', 'mode=s', 'frequency=i', 'token', 'publication', 'list', 'context=s',
'help', 'long_help', 'version'

=item C<add_args()>

This function is called to add additional arguments to the new scripts option string.
The function requires the option string information for the argument, such as:
mode=s
frequency=i

This function also requires a default value for the additional argument.  If the
argument is not provided on the command line then it will be set to the default value.

=item C<getopts()>

This function is called to process the new scripts command line arguments.
It uses
the scripts options string to determine the arguments required and whether they
are missing or specified incorrectly.  If a problem has been found with the
arguments
that were passed in to the new script, this function will provide the problem
with the
input as well as provide the help text, like a usage line.  This function will
automatically process the 'help', 'long_help' and 'version' options.

=item C<create_context()>

This function is called to create the context file for a workflow.  This function
is not used by an operation.  This function will check to determine whether the
context file already exists, and if it does, then the context file is copied to
a backup context file so that it can be used to determine whether the state
of the
workflow has changed.  If the function finds that the context file does not exist
the path to the context file is created, if required, and a flag is returned
to indicate that there was no context file and therefore, there is a state change
since this workflow is just starting up. This function will return 1 if the
context file did not exist and there is a state change in the workflow.  Otherwise,
the function will return 0.

=item C<get_backup_context()>

This function encapsulates the naming scheme and location of the backup context file.
This function returns the name of the backup context file.

=item C<get_enum_text()>

This function translates the Perl class constants to a text representation.  The
function returns a string version of the constant that was passed in to the function.

=item C<log()>

This function places messages in a log file that is used by all operations and
workflows.  The function requires the severity of the message and the message
text itself.  The function will prepend the timestamp, the caller and the
severity to the message and then write it to the log file.

=item C<op_exit()>

This function is called to exit a script in an orderly manner.  The function requires
the return code to be returned to the caller and any output to be printed to STDOUT.

=item C<op_die()>

This function is called to kill a script that has run into a critical error.  This
function requires the message describing the error which will be printed to
STDOUT
for the user.  The function will also accept an option return code. If
no return code is provided, the default return code will be UNKNOWN.

=back

=head1 COPYRIGHT

(C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.

=cut

use strict;
use warnings;
use Params::Validate qw(:all);
use Data::Dumper;   # Useful for debugging
#use POSIX;
use Getopt::Long;
use File::Basename;
use File::Copy;
use File::Path qw( mkpath );
use File::Temp qw( :POSIX );

# use library to access Vili helper routines
use Vili::Helper ':all';

use constant
{
    OK                          => 0,
    WARNING                     => 1,
    CRITICAL                    => 2,
    UNKNOWN                     => 3,
    FAILURE			=> 4,
    INFO                        => 5,
    WARN                        => 6,
    ERROR                       => 7
};


# Return values and log severity
my %error_enum = (
    0                   => "OK",
    1         		=> "WARNING",
    2        		=> "CRITICAL",
    3 			=> "UNKNOWN",
    4 			=> "FAILURE",
    5		        => "INFO",
    6                	=> "WARN",
    7                	=> "ERROR",
);

my %DEFAULT = (
	mode => "baseline",
	context => "",
	vili => "Command line",
	frequency => 0,
	token => 0,
	publication => 0,
	list => 0,
	verbose => 0,
	version => 0,
	help => 0,
	long_help => 0,
	local => 0,
);

# Define some constants that are used by these functions
my $LOGFILE = "$ENV{MY_SQROOT}/seapilot/logs/Vili.log";

# Security requirement: all files created must show no permissions for "others"
# so change default umask from 022 to 027
umask 027;

##############################################################################
##############################################################################

#
#	This subroutine is called to create a new operation or workflow object
#
#	INPUT: 
#	opname - the name of the new object
#	help_text - the long usage information for this object
#	long_help_text - the man page for this object
#	op_version - the current version of this new object
#	
#	OUTPUT:
#	a function object is returned to the caller
#	
sub new
{
    my $class = shift;

    # lets get our SQROOT path, USER and the 
    # node id now since we will need those soon
    # and if there is a problem we can report it
    # right away.

    # get our instance name for use later.
    # if we cannot find it then exit now
    my $SQROOT = $ENV{'MY_SQROOT'};
    die ("The environment variable MY_SQROOT is empty.\n") unless (defined($SQROOT) && length($SQROOT)>0);

    # get our user name for use later.
    # if we cannot find it then exit now
    my $USER = $ENV{'USER'};
    die ("The environment variable USER is empty.\n") unless (defined($USER) && length($USER)>0);

    # call the helper function to get the node id 
    # that we are running on
    my $nodeid = GetNodeID();
    die("An Error occurred when obtaining the node id.\n") if  ($nodeid < 0);

    # define our starting option string, which will be used when we get our options
    my @opt_array = ('verbose', 'mode=s', 'frequency=i', 'token', 'publication', 'list', 'context=s', 'vili=s', 'help', 'long_help', 'version', 'local');

    # validate the arguments passed in to the constructor
    my %args = validate( @_,
	{
		opname		=> {
			type		=>	SCALAR,		# a scalar
			regex		=> 	qr/^\w+$/	# that is all word chars
		},
		help_text		=> {
			type		=>	SCALAR		# a scalar but can be mixed chars
		},
		long_help_text	=> {
			type		=>	SCALAR		# a scalar but can be mixed chars
		},
		op_version		=> {
			type		=>	SCALAR,		# a scalar but could be mixed chars
		},
	}
    );
		

    my $self = {};

    # take any values that have been passed in
    # also set initial values on the other variables that 
    # will be set later when getopts is called
    $self->{opname}  = $args{"opname"};
    $self->{help_text}  = $args{"help_text"};
    $self->{long_help_text}  = $args{"long_help_text"};
    $self->{op_version}  = $args{"op_version"};
    $self->{logfile} = $LOGFILE;

    # set the Default context value now that we have
    # the user name and the nodeid
    # along with the passed in workflow name
    # NOTE: This will not be used by operations.
    if (&is_cluster) {
        $DEFAULT{context} = "/home/$USER/context/$nodeid/$self->{opname}/automatic";
    } else {
        $DEFAULT{context} = "$SQROOT/seapilot/var/checks/context/$nodeid/$self->{opname}/automatic";
    }

    # set the context file backup file name to be used
    # when we copy the file in order to compare
    # we use the POSIX (tmpnam) which is the FILE::temp mktemp call 
    # in order to generate a valid temporary file name
    $self->{context_back} = tmpnam();

    # initialize the standard ops values
    foreach my $key (sort keys %DEFAULT) {
    	$self->{$key}  = $DEFAULT{$key};
    }

    # save the option array for the standard
    # options that will be used for GetOptions
    $self->{_opt_array} = [ @opt_array ];

    # Bless ourselves as an instance of the class
    bless($self, $class);

    return ($self);
}

#
#	This subroutine is called to add additional arguments/variables
#	to the option string, and to $self.  This is used by operations
#	and workflows to manage the additional arguments that the scripts require
#
#	INPUT: 
#	arginfo - will be argument name and type in getopts format
#      		such as "max=s" where max is the variable name and it is of type string
#	default - this will hold the initial default value when this argument is created
#	
#	OUTPUT:
#	no output - the provided argument info is added to the options string
#	 	and the argument is defined with the default value 
#	 	in $self for use by the operation or workflow
#
sub add_args {
    my $self = shift;

    my %args = validate( @_, {
	arginfo => 1,
	default => 0,
    });

    # now add the new argument info to our options array list 
    # that we will use for getopts
    push @{$self->{_opt_array}},$args{arginfo};

    # now extract the variable name from the argumentinfo so 
    # that we can use it to store the default value for now
    # That means we take what appears before the '=' sign as
    # the variable name
    my $varname = $args{arginfo};
    $varname =~ s/[=:].*$//;

    # if for some reason this variable is already defined
    # then we will not overwrite it
    # otherwise we initialize this variable with the default value
    if (!defined $self->{$varname}) {
	$self->{$varname} = $args{default};
    }
 
}

#
#	This subroutine is called to get the arguments that were
#	passed in to the operation or workflow.
#
#	INPUT: 
#	no input - the @ARGV is used
#	
#	OUTPUT:
#	no output - the provided arguments, once verified to be good,
#	are placed in $self for use by the operation or workflow
#
sub getopts {
    my $self = shift;

    # let's get the options that were passed in to the script
    # this call will check the options defined in the _opt_array
    # and will place the values in the variables in $self of the same name
    my $result = GetOptions ($self, @{$self->{_opt_array}});
    die($self->{help_text}) unless $result;

    # if mode argument provided is not what is expected, then generate an error
    # and display the usage
    if ($self->{mode} ne "interval" && $self->{mode} ne "baseline") {
	printf("Invalid mode specified: valid options are \"interval\" or \"baseline\"\n");	
    	my $output = $self->{help_text} . "\n";
    	die($output);
    }

    # if none of the output methods are selected, then set it to 'list'
    # because 'list' is the default
    if ($self->{token} == 0 && $self->{publication} == 0 && $self->{list} == 0) {
    	$self->{list} = 1;
    }

    # The context filename specified may not exist since we are just starting out.
    # However, the directory should so let's check to see that the directory actually exists
    if ($self->{context} ne $DEFAULT{context}) {
	my $contextdir = dirname($self->{context});
	if (! -e $contextdir) {
		printf("Invalid context file specified: directory $contextdir does not exist\n");	
    		die($self->{help_text});
  	}
    }

    # Now that we have validated the arguments that we know about 
    # lets process the options that we can dispatch right away
    # If help, then display the help text
    if ($self->{help}) {
	$self->op_exit (
        	retcode => Vili::Functions::OK,
        	output =>  $self->{help_text},
        );
    }

    # If long help, then display the long help text
    if ($self->{long_help}) {
	$self->op_exit (
        	retcode => Vili::Functions::OK,
        	output =>  $self->{long_help_text},
        );
    }

    # If version information is being requested, print it out
    if ($self->{version}) {
	$self->op_exit (
        	retcode => Vili::Functions::OK,
        	output =>  $self->{op_version},
        );
    }

}

#
#	This subroutine is called to place messages in the log
#	file.  This subroutine will prepend the timestamp and
#	the caller to the message as well as place the provided
#	severity in front of the message, as well.
#
#	INPUT: 
#	severity - the constant value of the severity that is associated
#		with this particular log entry
#	message - the message that will be placed in the log file
#	
#	OUTPUT:
#	The modified message will be placed in the log file
#
#	TO DO: Decide how to know Vili instance and then act on it
#
sub log {
    my $self = shift;

    my $caller = $0;

    # validate the options that have been passed in.
    my %args = validate( @_, 
	{
    		severity => {
                        type            =>      SCALAR,         # a scalar
                        callbacks       => {                    # that is one of our supported severity values
                                'what is the severity' =>
                                        sub {$_[0] eq INFO || $_[0] eq WARN || $_[0] eq ERROR},
                        },
        	},
		message => {
		        type            =>      SCALAR,         # a scalar
        	},
    });

    # Let's create the log directory if it does not exist
    my $log_dir = dirname($self->{logfile});
    if (!-e $log_dir) {
   	mkdir $log_dir || die("Vili Functions unable to create log directory.\n");
    } 

    # Open the log file
    open(LOG, ">>$self->{logfile}") || die("Vili Functions unable to open the log file");

    # Get the current time for log message
    my $Mark = localtime();

    # Now print it
    printf(LOG "%s: %s: %s: %s: %s\n", $Mark, $self->{vili}, $caller, $error_enum{$args{severity}}, $args{message});

    # Done. Now close it and exit
    close(LOG);
}

#
#	This subroutine is called to translate the constant
#	values to defined text values for the constants.
#
#	INPUT: 
#	constvalue - the constant value that should be translated
#	
#	OUTPUT:
#	The text value of constant is returned to the caller
#
sub get_enum_text {
    my $self = shift;

    # validate the options that have been passed in.
    my %args = validate( @_, 
	{
    		constvalue => {
                        type            =>      SCALAR,         # a scalar
                        callbacks       => {                    # that is one of our defined constant values
                                'what is the constant' =>
                                        sub {
$_[0] eq OK || $_[0] eq WARNING || $_[0] eq CRITICAL || $_[0] eq UNKNOWN || $_[0] eq INFO || $_[0] eq WARN || $_[0] eq ERROR},
                        },
        	},
    });

    my $ret = $error_enum{$args{constvalue}};

    return ($ret);

}

#
#	This subroutine is called to create the context file
#	and save a copy, as required
#
#	INPUT: 
#	none
#	
#	OUTPUT:
#	has_changed - a flag to indicate if the file needed
#		to be created, which means that the
#		state has changed
#	The context file will be created/truncated
#	 	and, if necessary, a backup of the older one
#	 	will be saved for comparison
#
sub create_context {
    my $self = shift;

    my $has_changed = 0;
    my $ret = "";

    # first check to see if the context file exists and is not empty CR6253
    if (!-s $self->{context}) {
        # remove empty file
        unlink( $self->{context} ) if ( -e $self->{context} );
        # it doesn't exist... do we need to create
        # the directory above it?
        my $context_dir = dirname($self->{context});
        if (!-e $context_dir) {

                # call mkpath and obtain error return
                $ret = mkpath($context_dir);
                if (!$ret) {
			my $errormsg = "Unable to create $context_dir directory:";
                    	$self->log(
                        	severity => Vili::Functions::ERROR,
                        	message => $errormsg,
                    	);
                    	$self->op_exit(
                        	retcode => Vili::Functions::FAILURE,
                        	output => $errormsg,
                    	);
                   
              }
        }

        # set $has_changed to true, because
        # the context file did not exist so we
        # created it
        # well, technically we might have created the directory
	# and when we first write to the file we 
	# will have created it
        $has_changed = 1;

    } else {

	# save copy of the context file
        $ret = copy($self->{context}, $self->{context_back});
        # copy returns 1 on success, 0 on failure
        if (!$ret ) {
               	my $errormsg = "Unable to copy $self->{context}";
               	$self->log(
                       	severity => Vili::Functions::ERROR,
                       	message => $errormsg,
               	);
               	$self->op_exit(
                       	retcode => Vili::Functions::FAILURE,
                       	output => $errormsg,
               	);
        }

    }

    return($has_changed);
}

#
#	This subroutine is used to exit a script is an orderly
#	manner.
#
#	INPUT: 
#	retcode - the exit code value to be returned
#	output - the message to be displayed when the script exits
#
#	OUTPUT:
#	output message with newline added is printed and the provided retcode is returned.
#
sub op_exit {
    my $self = shift;

    my %args = validate( @_, 
	{
		retcode => {
                        type            =>      SCALAR,         # a scalar
                        callbacks       => {                    # that is one of our supported return values
                                'what is the return code' =>
                        sub {$_[0] eq OK || $_[0] eq WARNING || $_[0] eq CRITICAL || $_[0] eq UNKNOWN || $_[0] eq FAILURE},
                        },
        	},
		output => {
		        type            =>      SCALAR,         # a scalar
        	},
    });

    $args{output} .= "\n";

    # just to be sure, if the backup context file still exists, then let's remove it
    if (-e $self->{context_back}) {
	unlink($self->{context_back});
    }

    print $args{output};
    exit $args{retcode};
}

#
#	This subroutine is used to kill a script that has run into
#	a critical error.  If no return code is provided, the default
#	return code will be UNKNOWN
#
#	INPUT: 
#	output - the message to be displayed when the script dies
#	retcode - the exit code value to be returned, or if not specified, UNKNOWN
#
#	OUTPUT:
#	output message with newline added is printed and the retcode is returned.
#
sub op_die {
    my $self = shift;

    my %args = validate( @_, 
	{
		output => {
		        type            =>      SCALAR,         # a scalar
        	},
		retcode => {
                        type            =>      SCALAR,         # a scalar
                        optional        =>      1,         	# that is optional
                        default         =>      UNKNOWN,        # that if not specified, should default to UNKNOWN
                        callbacks       => {                    # that should be one of our supported return values
                                'what is the return code' =>
                                        sub {$_[0] eq OK || $_[0] eq WARNING || $_[0] eq CRITICAL || $_[0] eq UNKNOWN},
                        },
        	},
    });

    $args{output} .= "\n\n";

    # just to be sure, if the backup context file still exists, then let's remove it
    if (-e $self->{context_back}) {
	unlink($self->{context_back});
    }

    print $args{output};
    exit $args{retcode};

}

##
## Get the name of the backup context file.
##
## Params: None
##
## Returns: name of backup context file
##
sub get_backup_context
{
    my $self = shift;

    return ($self->{context_back});
}

1;
