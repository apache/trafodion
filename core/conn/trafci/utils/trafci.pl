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
#!/usr/bin/perl

use lib '##TRAFCI_PERL_LIB_CLASSPATH##';
use threads;

if(-e 'settings.pl')
{
    $jl_classpath = `perl settings.pl`;
}
else
{
    use Env qw(TRAFCI_PERL_JSERVER);
    $jl_classpath=$TRAFCI_PERL_JSERVER;
}

chomp $jl_classpath;

if (length(trim($jl_classpath)) == 0)
{
 print "Environment variable TRAFCI_PERL_JSERVER is not set\n";
 exit;
}

use Env qw(TRAFCI_PERL_JSERVER_PORT);
$jsrvr_port=$TRAFCI_PERL_JSERVER_PORT;
chomp $jsrvr_port;

if (length(trim($jsrvr_port)) == 0)
{
 print "Environment variable TRAFCI_PERL_JSERVER_PORT is not set\n";
 exit;
}
# ...

# ... execute the script file 

$script_file= $ARGV[0];
chomp $script_file;
#$script_file=trim($script_file);
 
if ( length(trim($script_file)) == 0 )
{
     print "Unknown number of arguments passed\n\n";
     exit;
}

# start the JavaServer thread
my $child_thread = threads->new(\&start_server);


# look for JavaServer error. It may still be
# initializing, lets wait and try again...
$output = eval{ new Java(port => $jsrvr_port) };
if($@){
    use Java;

    # This is the timeout to wait for the JavaServer
    $TIMEOUT = 15;
    
    $count = 0;
    $error = 1;
    do{
        sleep 1;
        $output = eval{ new Java(port => $jsrvr_port) };
        $error = $@;
        $count++;
    }while($count < $TIMEOUT && $error);

    if($error){
         print "ERROR: Script timed out waiting for perl JavaServer\n";
    }else{
         # Give the JVM some more time...
         sleep 2;
    }
}


# pass extra args to script file
@ARGV2 = @ARGV[1..($#ARGV+1)];

# run the script file
system $^X." ".$script_file." @ARGV2";


# start the java server in the background
# once the server is started in the background. server will process the client requests and aborted when doing System.exit
sub start_server {
    $classpath="##TRAFCI_PERL_CLASSPATH##$jl_classpath";
    $myarg='java -classpath '. "\"$classpath\"" . ' com.zzo.javaserver.JavaServer '. $jsrvr_port;
    system $myarg;
    0; }
    
sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}
