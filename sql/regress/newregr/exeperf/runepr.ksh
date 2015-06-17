#! /bin/sh
#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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
#######################################################################

###########################################################################
#                                RunEPR                                   #
#                                                                         #
# run the executor performance regressions from a NT box on a nsk machine #
###########################################################################

Standalone=batman
exePerfBaseLocation=//americas/cacfile/Ssqlshare/Executor/Perf
exePerfScriptDir=/usr/nskport/regress/exeperf
exePerfRunDir=/usr/nskport/regsrl/regress/exeperf
archiveLocation="server"
userName=super.super
password=me

#. $TMP/BuildParameterFile.txt
processed_options="FALSE"
ftpScripts="TRUE"
runTest="TRUE"
archive="FALSE"

IpAddress=
BuildLabel=

while [ "$processed_options" = "FALSE" ];do
  case "$1" in
  
  -system)
    shift
    Standalone=$1
    shift
    ;;
  -ipaddr)
    shift
    IpAddress=$1
    shift
    ;;
  -scriptsdir)
    shift
    exePerfScriptsDir=$1
    shift
    ;;
  -rundir)
    shift
    exePerfRunDir=$1
    shift
    ;;
  -archive)
    shift
    archive="TRUE"
    runTest="FALSE"
    ;;
  -archivelocation)
    shift
    archiveLocation=$1
    shift
    ;;
  -buildlabel)
    shift
    BuildLabel=$1
    shift
    ;;
  -username)
    shift
    username=$1
    shift
    ;;
  -password)
    shift
    password=$1
    shift
    ;;
  -ftponly)
    shift
    runTest="FALSE"
    ftpScripts="TRUE"
    ;;
  -runonly)
    shift
    ftpScripts="FALSE"
    runTest="TRUE"
    ;;
  *)
    processed_options="TRUE"
    ;;
  esac
done

#change standalone name to all lower case
Standalone=$(echo $Standalone|tr "[A-Z]" "[a-z]")

#convert system name to ipaddress if available
#this to ensure that the script works on systems 
#where DNS settings are not done.

if [ "$IpAddress" = "" ]; then
case "$Standalone" in

maya) 
  IpAddress="16.107.109.55"
  ;;
batman)
  IpAddress="16.107.109.165"
  ;;
robin)
  IpAddress="16.107.109.175"
  ;;
*)
  #in case we dont have ip address, this should
  #still work
  IpAddress="$Standalone.caclab.cac.cpqcorp.net"
  ;;
esac
fi

#----------------------------------------------------------------------
#remove old telnet script
rm -f pre-FtpSetup.epr

#create telnet script for doing pre-ftp setup
echo "$IpAddress" > pre-FtpSetup.epr
echo "wait \"Enter Choice>\"" >> pre-FtpSetup.epr
echo "send \"tacl\\m\"" >> pre-FtpSetup.epr
echo "wait \"TACL 1>\"" >> pre-FtpSetup.epr
echo "send \"logon $username,$password\\m\"" >> pre-FtpSetup.epr
echo "wait \"1>\"" >> pre-FtpSetup.epr
echo "send \"osh\\m\"" >> pre-FtpSetup.epr
echo "wait \":\"" >> pre-FtpSetup.epr
echo "send \"mkdir -p $exePerfScriptsDir\\m\"" >> pre-FtpSetup.epr
echo "wait \":\"" >> pre-FtpSetup.epr
echo "send \"mkdir -p $exePerfRunDir\\m\"" >> pre-FtpSetup.epr
echo "wait \":\"" >> pre-FtpSetup.epr
echo "send \"exit\\m\"" >> pre-FtpSetup.epr
echo "wait \"2>\"" >> pre-FtpSetup.epr
echo "send \"logoff\\m\"" >> pre-FtpSetup.epr

#doing this to get around problem in tst10 (the telnet scripting tool)
#create scripts to generate numbers and do comparison and update test 
#history.
#I am only doing this because there seems to be some problem in telnet
#scripting that causes the scripting tool to hang waiting for user input
#----------------------------------------------------------------------
#remove previous scripts for running the tests from oss
rm -f runtests.ksh

#echo "gtacl -c \"password me2too\"" > runtests.ksh
#echo "./perfgenerate.ksh $BuildLabel" >> runtests.ksh
#echo "./perfcompare.ksh -useExpectedFile -expected -simple $BuildLabel" >> runtests.ksh
#echo "./updateCPRHistory.ksh $BuildLabel" >> runtests.ksh
#echo "mkdir -p /usr/nskport/regsrl/regress/exeperf" >> runtests.ksh
#echo "cp runregr-mx.log /usr/nskport/regsrl/regress/exeperf" >> runtests.ksh
#echo "gtacl -c \"password me\"" >> runtests.ksh

#----------------------------------------------------------------------
#remove old ftp script
rm -f ftpput.epr

#create ftp script to put system specific baseline numbers and defs
echo "open $IpAddress" > ftpput.epr
echo "$username" >> ftpput.epr
echo "$password" >> ftpput.epr
echo "quote oss" >> ftpput.epr
echo "cd $exePerfRunDir" >> ftpput.epr
echo "lcd $exePerfBaseLocation/$Standalone/baseline" >> ftpput.epr
echo "put perfbaseline.dat" >> ftpput.epr
echo "cd ../tools" >> ftpput.epr
echo "lcd $exePerfBaseLocation/$Standalone" >> ftpput.epr
echo "put exeperfdefs" >> ftpput.epr
echo "quit" >> ftpput.epr

#----------------------------------------------------------------------
#remove previous telnet script
rm -f runtest.epr

#create telnet script to run tests and compare results
echo "$IpAddress" > runtest.epr
echo "wait \"Enter Choice>\"" >> runtest.epr
echo "send \"tacl\\m\"" >> runtest.epr
echo "wait \"TACL 1>\"" >> runtest.epr
echo "send \"logon $username,$password\\m\"" >> runtest.epr
echo "wait \"1>\"" >> runtest.epr
echo "send \"osh /cpu 1/\\m\"" >> runtest.epr
echo "wait \":\"" >> runtest.epr
echo "send \"cd $exePerfRunDir\\m\"" >> runtest.epr
echo "wait \"$exePerfRunDir:\"" >> runtest.epr
echo "send \"./runregr\\m\"" >> runtest.epr
echo "wait \"$exePerfRunDir:\"" >> runtest.epr
echo "send \"exit\\m\"" >> runtest.epr
echo "wait \"2>\"" >> runtest.epr
echo "send \"logoff\\m\"" >> runtest.epr

#----------------------------------------------------------------------
#remove old ftp script
rm -f ftpget.epr

#create ftp script to get results from nsk system
echo "open $IpAddress" > ftpget.epr
echo "$username" >> ftpget.epr
echo "$password" >> ftpget.epr
echo "asci" >> ftpget.epr
echo "quote oss" >> ftpget.epr
echo "cd $exePerfRunDir" >> ftpget.epr
echo "get runregr-mx.log" >> ftpget.epr
echo "quit" >> ftpget.epr

#----------------------------------------------------------------------
#Do the ftping and running

if [ "$ftpScripts" = "TRUE" ]
then
  #do the pre-ftp setup
  tst10 /r:pre-FtpSetup.epr /o:pre-FtpSetup.log
  
  #ftp the scripts and expected files
  ftp -i -v -s:ftpput.epr
fi

if [ "$runTest" = "TRUE" ]
then
  #run the telnet script
  tst10 /r:runtest.epr /o:runtest.log

  #get the generated results
  ftp -i -v -s:ftpget.epr
fi

#If the label is to be archived, call the script to archive the label
if [ "$archive" = "TRUE" ]
then
  if [ "$archiveLocation" = "server" ]
  then
    archiveLocation=""
  else
    archiveLocation="-archivelocation $archiveLocation"
  fi
  
  #----------------------------------------------------------------------
  #remove old run archive script
  rm -f runarch.epr

  #create telnet script to run archiveOnly operation
  echo "$IpAddress" > runarch.epr
  echo "wait \"Enter Choice>\"" >> runarch.epr
  echo "send \"tacl\\m\"" >> runarch.epr
  echo "wait \"TACL 1>\"" >> runarch.epr
  echo "send \"logon $username,$password\\m\"" >> runarch.epr
  echo "wait \"1>\"" >> runarch.epr
  echo "send \"osh /cpu 1/\\m\"" >> runarch.epr
  echo "wait \":\"" >> runarch.epr
  echo "send \"cd $exePerfRunDir\\m\"" >> runarch.epr
  echo "wait \"$exePerfRunDir:\"" >> runarch.epr
  echo "send \"./runregr -archiveOnly\\m\"" >> runarch.epr
  echo "wait \"$exePerfRunDir:\"" >> runarch.epr
  echo "send \"exit\\m\"" >> runarch.epr
  echo "wait \"2>\"" >> runarch.epr
  echo "send \"logoff\\m\"" >> runarch.epr

  #remove old ftp script
  rm -f ftpgetarch.epr
  
  #create ftp script to get results from nsk system
  echo "open $IpAddress" > ftpgetarch.epr
  echo "$username" >> ftpgetarch.epr
  echo "$password" >> ftpgetarch.epr
  echo "asci" >> ftpgetarch.epr
  echo "quote oss" >> ftpgetarch.epr
  echo "cd $exePerfRunDir" >> ftpgetarch.epr
  echo "get perfbaseline.dat" >> ftpgetarch.epr
  echo "quit" >> ftpgetarch.epr

  #run script on nsk to produce new baseline numbers
  tst10 /r:runarch.epr /o:runarch.log

  #get the new baseline numbers and archive them
  ftp -i -v -s:ftpgetarch.epr

  #save to the archive location and make it new base line
  mkdir -p $exePerfBaseLocation/$Standalone/$BuildLabel
  cp perfbaseline.dat $exePerfBaseLocation/$Standalone/$BuildLabel
  rm -f $exePerfBaseLocation/$Standalone/baseline/*
  cp -f $exePerfBaseLocation/$Standalone/$BuildLabel/* $exePerfBaseLocation/$Standalone/baseline/.
fi
