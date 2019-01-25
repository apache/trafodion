#!/usr/bin/perl -w
#
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
#

use Exporter ();
package sqsystem;

sub isProd
{
   if ((isSingleNode()) || (-e "/home/dont_delete_me"))
     {# Dev
      return 0;  # false
     }
   else
     {# Prod  
      return 1;  # true
     }
}


sub isSingleNode
{
   my $TRAF_CONF = $ENV{'TRAF_CONF'};

   # Get the number of nodes in the instance
   my $TempList = `grep 'node-name=.[^A-Za-z].[0-9]*' $TRAF_CONF/sqconfig | grep -v "\#" | cut -d"=" -f3 | cut -d";" -f1 | sort -u`;
   if (!$TempList)
     {# Single-node development system
      return 1;  # true
     }
   else
     { # A cluster system (dev or prod)
      return 0;  # false
     }
}

# Below is to return true; this is required when this module is referenced via a "use" statement in another module
# (if we had variables defined and assigned in addition to functions, we would not need to include this implicit return)
1;
