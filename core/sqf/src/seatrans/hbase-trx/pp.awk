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
#  The following example has been tested to work.
#  If the file f1 contains the following, run this script
#  as follows:    
#   awk -f pp.awk -v distro=HDP2.3 f1 > f2
#
#  contents of  file f1:
#   #ifdef HDP2.3
#   this is HDP2.3 specific code
#   #else
#   this is anything other than  HDP2.3 code
#   #endif
#
#   #ifndef HDP2.3
#   this is anything other than HDP2.3 code partof ifndef construct
#   #else
#   this is HDP2.3 code part of ifndef else construct
#   #endif
#
#   this is common code for all cases
#
#   #ifdef CDH5.4
#   this is CDH5.4 specific code
#   #endif
#
#   #ifdef CDH5.5 HDP2.3 APACHE1.1 CDH5.7
#   this is common CDH5.5 or HDP2.3 or APACHE1.1 or CDH5.7
#   #else
#   this is anything other than CDH5.5 or HDP2.3 or APACHE1.1 or CDH5.7 code
#   #endif

BEGIN{
printline=1      #print current line or not.
matchBegun=0     #match succeeded, keep printing
unmatchBegun=0   #opposite of  match, keep non-printing
ifdefpattern = "#ifdef"
endifpattern = "#endif"
elsepattern = "#else"
ifndefpattern = "#ifndef"
#distro passed in as argument
}
{
  #By default print everything
  printline =1

#########################
#  This section deals with resetting flags
########################
  #reset when #endif is encountered
  if(($0 ~ endifpattern) && (matchBegun == 1))
    {
      printline = 0
     matchBegun = 0
    }

  #reset when #endif is encountered
  if(($0 ~ endifpattern) && (unmatchBegun == 1))
    {
      printline = 0
     unmatchBegun = 0
    }
 
  #when #else is encountered and unmatchBegun, reset unmatchBegun
  #start print subsequent lines. 
  if(($0 ~ elsepattern) && (unmatchBegun == 1))
    {
      printline = 0
      unmatchBegun = 0
      matchBegun = 1
    }
  else
  {
    #when #else is encountered and matchBegun, reset matchBegun
    #start unprint subsequent lines by setting unmatchBegun 
    if(($0 ~ elsepattern) && (matchBegun == 1))
    {
      printline = 0
      unmatchBegun = 1
    }  
  }

##########################
# This section matches distro string and sets flags
##########################

  if($0 ~ ifdefpattern)
   {
     if( $2 ~ distro || $3 ~ distro || $4 ~ distro || $5 ~ distro || $6 ~ distro)
     {
       printline = 0
       matchBegun = 1
     }
     else
     {
      printline = 0
      unmatchBegun = 1
     }
   }
 if($0 ~ ifndefpattern)
  {
    if($2 ~ distro || $3 ~ distro || $4 ~ distro || $5 ~ distro || $6 ~ distro)
    {
      printline = 0
      unmatchBegun = 1
    }
    else
    {
     printline = 0
     matchBegun = 1
    }
  }
######################
# This section is final printing based on flags
######################

  #if unmatchBegun discard those lines 
  if(unmatchBegun == 1)
     printline = 0
 
  if(printline == 1)
	print $0;
}
END {
}
