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
################################################################################
##  Include the library path here
################################################################################
use lib '##TRAFCI_PERL_LIB_CLASSPATH##';
use Session;

################################################################################
##  create a new session
################################################################################
$sess = Session->new();

################################################################################
##  connect to the database
################################################################################
$sess->connect("USERNAME",'PASSWORD',"HOSTNAME","PORTNUMBER","DSNAME");

################################################################################
##  collect args 
################################################################################
my $numArgs;
my $filter;

$numArgs = $#ARGV + 1;

$filter = "*";
if($numArgs > 0){
    $filter = $ARGV[0];
}

################################################################################
## Execute sample queries
################################################################################
my $retval;

$retval=$sess->execute("set schema TRAFODION.CI_SAMPLE");
$retval=$sess->execute("set markup csv");
$retval=$sess->execute("get tables");

if($retval =~ /java.lang.NullPointerException/){
    exit(1);
}

my @tables = split(/\n/, $retval);      # split table names


# format output queries

format_lines_per_page STDOUT 999999;    # disables reprinting of the header

format STDOUT_TOP =
TableName                            Row Count
------------------------------------ ---------------
.

format STDOUT =
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< @<<<<<<<<<<<<<<
$tables[$i],                 $retval
.


# execute count on all tables

my $output_counter = 0;
for($i=0;$i<@tables;$i++)
{
    $tables[$i] = trim($tables[$i]);
    if($tables[$i] ne '')
    {
        $retval = trim($sess->execute("select count(*) from ".$tables[$i]));
        
        if($retval !~ /\*\*\* ERROR/)
        {
            $output_counter++;
            # test if table name is too big for format
            if(length($tables[$i])>36){
                print $tables[$i]."\n";
                # print out whitespace
                for($z=0; $z<37; $z++){
                    print " ";
                }
                print $retval."\n";
            }else{
                write;
            }
        }
    }
}

if($output_counter == 0){
    print "No tables found.\n";
}

################################################################################
### disconnect the session
################################################################################
$sess->disconnect();



# trim whitespace before and after the string
# also removes \r\n (carriage-return)
sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    $string =~ s/\r|\n//g;
    return $string;
}
