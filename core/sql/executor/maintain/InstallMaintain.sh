#!/bin/ksh
##################################################################
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
##################################################################
# InstallMaintain.sh - Maintain Initialization Script.
#
# This script will generate Maintain initialization commands, redirect
# them to an OSS .txt obey file, and execute them via mxci.
#
# Maintain initialization only needs to be performed on the Neotable
# master segment.
#
#
##################################################################
#
# This section initializes constants and globals for the script
# It should not contain any script code
#
##################################################################

export scriptName=`/bin/basename $0`
export scriptParams=$1

# Update these variables for each release
export scriptRevDate='11/18/2009'

# Must be in current directory tree in case PATH not set
. ./install/build_globals             # variables used in all scripts
. ./install/build_printGlobals 

#=================================================================
# FUNCTION: initializeMaintain
#    Initialize Maintain
#=================================================================
function initializeMaintain
{

    echo "" > ${obeyFileI1}

    case ${MSUT} in

     (N02.04)
       print "\nExecuting function initializeMaintain" | tee -a ${logFile}
       print "\nINITIALIZE MAINTAIN;" | tee -a ${logFile}
       print -r "INITIALIZE MAINTAIN;"  >> ${obeyFileI1}
       ;;

     (N02.93)
       print "\nExecuting function initializeMaintain" | tee -a ${logFile}
       print "\nINITIALIZE MAINTAIN;" | tee -a ${logFile}
       print -r "INITIALIZE MAINTAIN;"  >> ${obeyFileI1}
       ;;

    (N02.05)
       print "\nExecuting function initializeMaintain" | tee -a ${logFile}
       print "\nINITIALIZE MAINTAIN;" | tee -a ${logFile}
       print -r "INITIALIZE MAINTAIN;"  >> ${obeyFileI1}
       ;;

     esac
		
     executeInMxci
     ret=$?

     if [[ ${ret} = 0 ]] ; then
        print "\nMaintain objects were successfully initialized."  | tee -a ${logFile}
        print "\n---- END OF INITIALIZE MAINTAIN -----" | tee -a ${logFile}
        result=0
     else
        print -r "\nERROR(s) were encountered while initializing Maintain objects." | tee -a ${logFile}
        print -r "  Please check the log file ${logFile} and resolve the errors "  | tee -a ${logFile}
        print "  before initializing Maintain.\n"  | tee -a ${logFile}
        exit 1
     fi
  
     print "\nMaintain Initialization completed" | tee -a ${logFile}
}
#=================================================================
# FUNCTION: executeInMxci
#    Execute the DDL/DML obey file in SQL/MX mxci
# 
#=================================================================
function executeInMxci
{
  ### Execute DDL/DML obey file in SQL/MX mxci
  print "\nExecuting Maintain Obey File in MXCI..." | tee -a ${logFile}
  print "obey ${obeyFileI1}; exit;" | ${SQLMXBINPATH}/${MXCI} | tee -a ${logFile}

  ### If an ERROR occurs from Maintain initialization.
  #
  #           ERROR[1002] Catalog does not exist
  #
  # Only Warn for the following errors:
  #
  #           ERROR[1003] Schema does not exist
  #           ERROR[1004] Table does not exist
  #           ERROR[1008] User name does not exist
  #           ERROR[1035] Catalog already exists and 
  #           ERROR[1022] Schema already exists  and
  #           ERROR[1030] and [1071] Object could not be accessed
  #           ERROR[1031] Object could not be dropped.
  #           ERROR[1075] The catalog must be empty. It contains at least one schema <schema-name>
  #           ERROR[1134] A concurrent utility or DDL operation is being performed on object,
  #                       its parent, or one of its dependencies. That operation must complete  
  #                       before the requested operation can run.
  #           ERROR[25401] Catalog <catalog name> has not been registered on node <node name>

  catschErr=$(${EGREP} -e "1002" ${logFile})
  if [[ ${catschErr} != "" ]]; then
    print -r "Catalog ${MCAT} does not exist." | tee -a ${logFile}
    warnFlag=2
  fi

  catschErr=$(${EGREP} -e "1035" ${logFile})
  if [[ ${catschErr} != "" ]]; then
    print -r "NOTE: Catalog ${MCAT} already exist(s).  Not an error." | tee -a ${logFile}
    warnFlag=1
  fi

  catschErr=$(${EGREP} -e "1022" ${logFile})
  if [[ ${catschErr} != "" ]]; then
    print -r "NOTE: Schema ${MSCH} already exist(s).  Not an error." | tee -a ${logFile}
    print -r "Please see ${logFile} log file for details." | tee -a ${logFile}
    warnFlag=1
  fi

  tbltableErr=$(${EGREP} -e "1004|1031" ${logFile})
  if [[ ${tbltableErr} != "" ]]; then
    print -r "NOTE: Some tables and/or tables do not exist and could not be dropped.  Not an error." | tee -a ${logFile}
    print -r "Please see ${logFile} log file for details." | tee -a ${logFile}
    warnFlag=1
  fi
  
  tbltableErr=$(${EGREP} -e "1030|1071" ${logFile})
  if [[ ${tbltableErr} != "" ]]; then
    print -r "NOTE: Some objects could not be accessed during upgrade." | tee -a ${logFile}
    print -r "Please see ${logFile} log file for details." | tee -a ${logFile}
    warnFlag=1
  fi

  tbltableErr=$(${EGREP} -e "1003|1069" ${logFile})
  if [[ ${tbltableErr} != "" ]]; then
    print -r "NOTE: Schema does not exist and could not be dropped.  Not an error." | tee -a ${logFile}
    warnFlag=1
  fi
  
  tbltableErr=$(${EGREP} -e "1075" ${logFile})
  if [[ ${tbltableErr} != "" ]]; then
    print -r "NOTE: A non-Maintain schema exists in the Maintain catalog.  Not an error." | tee -a ${logFile}
    print -r "      Installation continuing..." | tee -a ${logFile}
    warnFlag=1
  fi

  tableErr=$(${EGREP} -e "4082|8222" ${logFile})
  if [[ ${tableErr} != "" ]]; then
    print -r "NOTE: Table does not exist.  Not an error." | tee -a ${logFile}
    print -r "Please see ${logFile} log file for details." | tee -a ${logFile}
    warnFlag=1
  fi
  
  tableErr=$(${EGREP} -e "1008" ${logFile})
  if [[ ${tableErr} != "" ]]; then
    print -r "User name does not exist, so could not grant select privilege on the table." | tee -a ${logFile}
    print -r "Please see ${logFile} log file for details." | tee -a ${logFile}
    warnFlag=1
  fi
  
  tableErr=$(${EGREP} -e "1134" ${logFile})
  if [[ ${tableErr} != "" ]]; then
    print -r | tee -a ${logFile}
    print -r "*** A concurrent operation or utility is running on this object." | tee -a ${logFile}
    print -r "    This operation must complete before the current operation can complete." | tee -a ${logFile}
    print -r "    Continuing to create other database objects..." | tee -a ${logFile}
    print -r | tee -a ${logFile}
    warnFlag=1
  fi
  
  tableErr=$(${EGREP} -e "25401" ${logFile})
  if [[ ${tableErr} != "" ]]; then
    print -r | tee -a ${logFile}
    print -r "*** Catalog has not been registered on a remote segment" | tee -a ${logFile}
    print -r "    This catalog must be registered before the current operation can run." | tee -a ${logFile}
    print -r "    Please make sure this catalog is registered before rerunning this script." | tee -a ${logFile}
    print -r | tee -a ${logFile}
    warnFlag=1
  fi
    
  #Append obeyFileI1 to tempobeyFileI1 to be kept for debugging
  cat ${obeyFileI1} >> ${tempobeyFileI1}
  echo "" > ${obeyFileI1}

  if [[ ${warnFlag} -eq "1" ]]; then
   print "\nA WARNING/ERROR occurred during the initialization of maintain" | tee -a ${logFile}
   return 1
  fi

  print "Done Executing Maintain Obey File In MXCI" | tee -a ${logFile}

  return 0
}

# execute the initializeMaintain function

initializeMaintain
ret=$?

if [[ ${ret} = 1 ]] ; then
  exit 1
else
  exit 0
fi

#=================================================================
# Script end
#=================================================================
exit ${exitcode}
