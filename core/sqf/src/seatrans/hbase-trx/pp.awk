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
#  The following example has been tested to work. Any other
#  combination is not guranteed to work. 
#  If the file f1 contains the following, run this script
#  as follows:    
#   awk -f pp.awk -v distro=HDP2.3 f1 > f2
#   awk -f pp.awk -v distro=APACHE1.1 f1 > f2
#
#  contents of  file f1:
# #ifdef HDP2.3
# this is HDP2.3 specific code
# #else
# this is anything other than  [HDP2.3] code
# #endif
#
# common code 1
#
# #ifndef HDP2.3
# this is anything other than [HDP2.3] code partof ifndef construct
# #else
# this is HDP2.3 code part of ifndef else construct
# #endif
#
# common code 2
#
# #ifdef CDH5.4
# this is CDH5.4 specific code
# #endif
#
# common code 3
#
# #ifdef CDH5.5 HDP2.3 APACHE1.1 CDH5.7
# this is common CDH5.5 or HDP2.3 or APACHE1.1 or CDH5.7
# #else
# this is anything other than [CDH5.5] or [HDP2.3] or [APACHE1.1] or [CDH5.7] code
# #endif
#
# common code 4
#
# #ifdef CDH5.5 HDP2.3
# this is common CDH5.5 or HDP2.3
# #elseif APACHE1.1
# this is specific to APACHE1.1 part of elseif construct
# #elseif CDH5.7
# this is specific to CDH5.7 part of elseif construct
# #else
# this is anything other than [CDH5.5] or [HDP2.3] or [APACHE1.1] or [CDH5.7] code
# #endif
#
# common code 5
#
# #ifdef CDH5.5 HDP2.3
# this is common CDH5.5 or HDP2.3
# #elseif APACHE1.1
# this is specific to APACHE1.1 part of elseif construct
# #elseif CDH5.7
# this is specific to CDH5.7 part of elseif construct
# #endif
# 
# common code 6


BEGIN{
printline=1      #print current line or not.
matchBegun=0     #match succeeded, keep printing
unmatchBegun=0   #opposite of  match, keep non-printing
unmatchTillEndif=0 #specific to #elseif case.
ifdefpattern = "#ifdef"
endifpattern = "#endif"
elsepattern = "#else"
ifndefpattern = "#ifndef"
ifdefinedpattern = "#ifdefined"
elseifpattern = "#elseif"
#distro passed in as argument, can only be one tag set by makefile at a time.
}
{
  #By default print everything
  printline =1

#########################
#  This section deals with #else #elseif #endif tags
########################
  #reset when #endif is encountered
  if($1 == endifpattern)
    {
      printline = 0
      matchBegun = 0
      unmatchBegun = 0
      unmatchTillEndif = 0
    }

  # Handle #else in this section.
  #when #else is encountered and unmatchBegun, reset unmatchBegun
  #start print subsequent lines. 
  if(($1 == elsepattern) && (unmatchBegun == 1))
    {
      printline = 0
      unmatchBegun = 0
      matchBegun = 0
    }
  else
  {
    #when #else is encountered and matchBegun, reset matchBegun
    #start unprint subsequent lines by setting unmatchBegun 
    if(($1 == elsepattern) && (matchBegun == 1))
    {
      printline = 0
      unmatchBegun = 1
      matchBegun = 0
    }  
  }
  
  # Handle #elseif  in this section.
  #when #elseif is encountered and unmatchBegun, reset unmatchBegun
  #start print subsequent lines.
  if(($1 == elseifpattern) && ($0 ~ distro) && (unmatchBegun == 1))
    {
      printline = 0
      unmatchBegun = 0
      matchBegun = 1
    }
  else
  {
    #when #elseif is encountered and matchBegun, reset matchBegun
    #start unprint subsequent lines by setting unmatchBegun 
    if(($1 == elseifpattern) && (matchBegun == 1))
    {
      printline = 0
      unmatchBegun = 0   #setting to 1 or 0 does not matter. 
      matchBegun = 0
      unmatchTillEndif = 1 #setting to 1 handles subsequent #else
    }  
  }
  
##########################
# This section matches distro string and sets flags.
# This section deals with #ifdef and #ifndef tags only.
#
# printline is set 0 in all cases below to indicate not to print 
# current line. matchBegun and unmatchBegun toggles appropriately.
# matchBegun used for all lines that must be included.
# unmatchBegun used for all lines that must be excluded.
##########################

  if($1 == ifdefpattern)
   {
     if($0 ~ distro)
     {
       printline = 0
       matchBegun = 1
       unmatchBegun = 0
     }
     else
     {
      printline = 0
      unmatchBegun = 1
      matchBegun = 0
     }
   }
 if($1 == ifndefpattern)
  {
    if($0 ~ distro)
    {
      printline = 0
      unmatchBegun = 1
      matchBegun = 0
    }
    else
    {
     printline = 0
     matchBegun = 1
     unmatchBegun = 0
    }
  }
  if($0 ~ ifdefinedpattern)
  {
    if(($2 ~ distro) || ($4 ~ distro))
    {
      printline = 0
      matchBegun = 1
      unmatchBegun = 0
    }
    else
    {
      printline = 0
      unmatchBegun = 1
      matchBegun = 0
    }
  }

######################
# This section is final printing based on flags
######################

  #if unmatchBegun discard those lines 
  if(unmatchBegun == 1)
     printline = 0
     
  #if unmatchTillEndif discard those lines
  if(unmatchTillEndif == 1)
     printline = 0
     
  if(printline == 1)
	print $0;
}
END {
}
