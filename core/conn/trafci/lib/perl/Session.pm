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

package Session;
use strict;
use Java;

my $java;
my $referenceCount = 0;

################################################################################
## the session object constructor
################################################################################
# Initialize the Java VM and increment the number of references.
sub doinit() {
   use Env qw(TRAFCI_PERL_JSERVER_PORT);
   my $jsrvr_port=$TRAFCI_PERL_JSERVER_PORT;
   $java = new Java(port => $jsrvr_port)     
   if (!$java);
   $referenceCount += 1;
}

# create an object reference to ci scripts and return to the caller
sub new {
	my $class = $_[0];
	doinit();
	my $sessObj= {
	   my $sess=>$java->create_object("org.trafodion.ci.ScriptsInterface")
	};
	bless $sessObj,$class;
	return $sessObj;
}

# connect to the database using the values passed 
sub connect{
   my $count;
   my $user;
   my $password;
   my $role;
   my $server;
   my $port;
   my $dsn;
   my $sessObj;
   my $sess;

   $sessObj=shift(@_);
   $sess=$sessObj->{my $sessi};
   
   $count = @_;
   if ($count >= 6 ) {
	     $user=shift(@_);
		 $password=shift(@_);
		 $role=shift(@_);
		 $server=shift(@_);
		 $port=shift(@_);
		 $dsn=shift(@_);

	     eval
         {
            $sess->call('openConnection',$user,$password,$role,$server,$port."#",$dsn);
         };
   }		
   else
   {
  		 $user=shift(@_);
		 $password=shift(@_);
		 $server=shift(@_);
		 $port=shift(@_);
		 $dsn=shift(@_);

	     eval
         {
            $sess->call('openConnection',$user,$password,$server,$port."#",$dsn);
         };
   }
   
  
   if($@)
   {
      print $@;
   }
}

# execute a query at a time and return the results but dont close the session yet 
 sub execute{
  my $sessObj=shift(@_);
  my $sess=$sessObj->{my $sessi};
  my $query=shift(@_);
   
    eval
    {
     	$Session::retVal=$sess->call('executeQuery',$query);
    };
    if($@)
    {
    	return get_exception($@);
    }
    return $Session::retVal->get_value();
 }

 # execute the script file and log the results in the log file 
 sub executeScript{
   my $sessObj=shift(@_);
   my $sess=$sessObj->{my $sessi};
   my $scriptFile=shift(@_);
   my $logFile=shift(@_);
   
    eval
    {
    	$sess->call('executeScript',$scriptFile,$logFile);
    	};
    if($@)
    {
        return get_exception($@);
    }
 }
 
 sub get_exception
  {
  my $exception=shift(@_);
  
  
  # check if there are any exceptions thrown from java
  if ($exception)
  {
   	$exception =~ s/^ERROR: //;     # Gets rid of 'ERROR: '
    $exception =~ s/at $0.*$//;     # Gets rid of 'croak' generated stuff
 	  
 	my $exception_object = $java->get_exception;
    if (!$exception_object) {
       return $exception;
 	}
 	return $exception_object->get_value();
  }
 }
 
 sub DESTROY { 
    $referenceCount -= 1;
 }
      
 sub disconnect{
       my $sessObj=shift(@_);
       my $sess=$sessObj->{my $sessi};
       eval
       {
       	$sess->call('disconnect');
    	};
       return  get_exception($@)
       if ($@);
 }
    
 END {
    eval {
     $java->static_call("java.lang.System", 'exit', 0);
     undef $java;
    };
   
    #dont throw any errors
 	if($@)
 	{
 	}
 }
      
 1;
