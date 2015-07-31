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
##
##  Include the library path here
##
package XMLHandler;
use lib '##TRAFCI_PERL_LIB_CLASSPATH##';
use XML::Parser::PerlSAX;
use Session;

##
##  create a new session
##
$sess = Session->new();

##
##  connect to the database
##
$sess->connect("USERNAME",'PASSWORD',"HOSTNAME","PORTNUMBER","DSNAME");


###########################################
## Execute a SELECT statement in XML markup.
## Parse the XML output into a 2D array
###########################################

$retval=$sess->execute("set schema TRAFODION.CI_SAMPLE");
$retval=$sess->execute("set markup XML");

my $selQuery;
$selQuery  = "select * from job";
$retval=$sess->execute($selQuery);

my $current_element;
my $print;
my @arr_2d;
my @row_glob;
my $rowNo;
my $colNo;

##
## Parse the XML output 
##
my $handler = XMLHandler->new();
my $parser = XML::Parser::PerlSAX->new(Handler => $handler);
initialize();
$parser->parse($retval);
print "\n--------------------------------------------\n";
print "The results of the SELECT statement         \n";
print "      Query: " . $selQuery . "\n";
print "--------------------------------------------\n";

#### print 
printResultSet();


##############################################
##
## Display the number of records returned
## by the previous statement. Parse the XML 
## output into an array and display the count
##
#############################################

$retval=$sess->execute("show reccount");

##
## Parse the XML output 
##
initialize();
$parser->parse($retval);


####Print 
print "\n----------------------------------------------------\n";
print "Total count of rows returned by the SELECT statement \n";
print "-----------------------------------------------------\n";

printResultSet();

##############################################
##
## Execute a SELECT statement on an invalid
## table and check the LASTERROR.
##
#############################################

$invalid_query = "select * from TRAFODION.CI_SAMPLE.TESTTAB";
$retval=$sess->execute($invalid_query);
$retval=$sess->execute("SHOW LASTERROR");

##
## Parse the XML output 
##
initialize();
$parser->parse($retval);


####Print 
print "\n---------------------------------------------------\n";
print "Error number returned by the previous query          \n";
print "Query : " . $invalid_query . "\n";
print "-----------------------------------------------------\n";

printResultSet();

## Initialises the global variables ##
sub initialize {
     $current_element='' ;
     $print = 0;
     $rowNo = -1;
     $colNo = 0;
     @arr_2d = ();
     
}

sub new {
    my $type = shift;
    return bless {}, $type;
}

sub start_element {
    my ($self, $element) = @_;

    $current_element = $element->{Name};
    if ($current_element eq 'row') {
        my @row;
    }

    if (($current_element eq 'Name') ||
       ($current_element eq 'RECCOUNT') ||
       ($current_element eq 'LASTERROR') ||
       ($current_element eq 'row'))
    {
        $rowNo++;
        push @{ $arr_2d[$rowNo] }, [@row];
        $print = 1;
    }
}

sub end_element {
    my ($self, $element) = @_;

    if (($element->{Name} eq 'row') ||
       ($element->{Name} eq 'Name') ||
       ($element->{Name} eq 'LASTERROR') ||
       ($element->{Name} eq 'RECCOUNT') )
    {
        $print = 0;
    }
}

sub characters {
    my ($self, $characters) = @_;
    my $text = $characters->{Data};
    $text =~ s/^\s*//;
    $text =~ s/\s*$//;

    if ($print == 1) {
        
        if ($text ne '') {
            push (@{ $arr_2d[$rowNo] }, $text);
            
        }
    }
    return '' unless $text;
}

sub printResultSet {
    for $i ( 0 .. $#arr_2d ) {
            $aref = $arr_2d[$i];
            $n = @$aref - 1;
            for $j ( 1 .. $n ) {
                print "$arr_2d[$i][$j]  ";
            }
            print "\n";
        }
        undef (@arr_2d);
        
}

################################################################################
### disconnect the session
################################################################################
$sess->disconnect();



