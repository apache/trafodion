#!/bin/bash

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

default_trafodion_user="trafodion"
default_hbase_user="hbase"
default_hdfs_user="hdfs"
mappers=0
date_str="$(date '+%Y%m%d-%H%M')"

###############################################################################
#which_environment function
###############################################################################
which_environment()
{
  if [[ -e $TRAF_HOME/sql/scripts/sw_env.sh ]]; then
    # we are on a development system where install_local_hadoop has been executed
    return 1
  elif [[ -n "$(ls /usr/lib/hadoop/hadoop-*cdh*.jar 2>/dev/null)" || -n "$(ls /etc/init.d/ambari* 2>/dev/null)" ||  -d /opt/cloudera/parcels/CDH || -d /opt/mapr  ]]; then
    #cluster with Cloudera, Horton Works or Mapr distribution
    return 2
  else
    #cannot determine which environment we are using
    return 3
  fi
}

###############################################################################
#do_hadoop -- for dev environment only
###############################################################################
do_hadoop()
{
  which_environment
  local wenv=$?
  if [[  ${wenv} -eq 1 ]];  then
    . $TRAF_HOME/sql/scripts/sw_env.sh
    $TRAF_HOME/sql/local_hadoop/hadoop/bin/hadoop $*
  else
    #cluster
    return 1
  fi
}
###############################################################################
#get_hadoop_cmd
###############################################################################
get_hadoop_cmd()
{
  which_environment
  local wenv=$?
  if [[  ${wenv} -eq 1 ]];  then
    echo do_hadoop $*
  else
    echo hadoop $*
  fi
}

###############################################################################
#do_hbase -- for development environmnet only
###############################################################################
do_hbase()
{
  which_environment
  local wenv=$?
  if [[  ${wenv} -eq 1 ]];  then
    . $TRAF_HOME/sql/scripts/sw_env.sh
    $TRAF_HOME/sql/local_hadoop/hbase/bin/hbase $*
  else
    return 1
  fi
}
###############################################################################
#get_hbase_cmd
###############################################################################
get_hbase_cmd()
{
  which_environment
  local wenv=$?
  if [[  ${wenv} -eq 1 ]];  then
    echo do_hbase $*
  else
    echo hbase $*
  fi
}
###############################################################################
#do_sudo function --> run commands as sudo depending on environment
###############################################################################
do_sudo()
{
  sudo_user=$1
  cmd=$2
  sudo_options=$3
  which_environment
  which_env=$?
  if [[ $which_env -eq 1 ]];  then
   sudo_cmd=$cmd
  elif [[ $which_env -eq 2  ]]; then
   sudo_cmd="sudo $sudo_options -n -u $sudo_user $cmd"
  else
   echo "***[ERROR]: Did not find supported Hadoop distribution or $TRAF_HOME is not set" | tee -a  ${log_file}
   return 1
  fi
  #run the command
  $sudo_cmd
}
###############################################################################
#parse_srvr_user_name -- parse user name running a server process like hbase or hdfs ...
###############################################################################
parse_srvr_user_name()
{
  local myprocess_name=$1
  local pid_str=($(jps | grep $myprocess_name ))
  if [[ PIPESTATUS[0] -ne 0 || -z $pid_str ]] ; then
    echo "***[ERROR]: cannot parse the $myprocess_name pid."  | tee -a  ${log_file}
    return 1
  fi

  local myusername=($(ps -f --pid ${pid_str[0]} | grep ${pid_str[0]}))
  if [[ PIPESTATUS[0] -ne 0 || -z $pid_str ]] ; then
    echo "***[ERROR]: cannot parse the $myprocess_name user name."  | tee -a  ${log_file}
    return 1
  fi
  echo ${myusername[0]}  | tee -a  ${log_file}
  return 0
}
###############################################################################
#confirm_choice
###############################################################################
confirm_choice()
{
  if [[ $confirm -eq 0   ]] ; then
    return 0
  fi
  local msg=$1
  echo -n "$msg [ value (Y/N), default is N ]: "
  read answer

  if [[ -z $answer ]] ; then
     return 1
  else
     if [[ ${answer^} == "Y" ]]; then
       return 0
     else
       return 1
     fi
  fi
}
###############################################################################
#validate_srvr_user_name
###############################################################################
validate_srvr_user_name()
{
  verified_user_name=$1
  local my_process_name=$2
  local my_default_user=$3
  local service_name=$4
  local mytyp=
  local myuser=

  if [[ -z  $verified_user_name ]] ; then
    echo "WARNING: The user name running $service_name service was not provided by user." | tee -a  ${log_file}
    myuser=$(parse_srvr_user_name "$my_process_name" )
    if [[ $? -ne 0  || -z $myuser  ]]
    then
      echo "WARNING: The user name running $service_name service could not be computed." | tee -a  ${log_file}
      mytyp=default
      myuser=$my_default_user
    else
      echo "INFO: The user name running $service_name service was computed as: $myuser ." | tee -a  ${log_file}
      mytyp=computed
    fi
    confirm_choice "Would you like to use the $mytyp user name: ${myuser} ?"
    if [[ $? -ne 0 ]]
    then
      #echo "***[ERROR]: User $myuser was rejected."    | tee -a  ${log_file}
      return 1
     else
      verified_user_name=$myuser
    fi
  fi
  # now verify the id
  id $verified_user_name  2>&1 > /dev/null
  if [[ $? -ne 0  ]]
  then
    echo "***[ERROR]: user $verified_user_name is not a valid user."   | tee -a  ${log_file}
    return 1
  fi
  return 0
}

###############################################################################
#verify_or_create_folder
###############################################################################
verify_or_create_folder()
{
  local hsdf_loc="$1"
  local cr="$2"
  # check if it is valid HDFS path
  echo  "${hdfs_backup_location}" | egrep -i -e 'hdfs://.*:.*/.*'  2>&1 > /dev/null
  if [[ ${PIPESTATUS[1]} -ne 0 ]]
  then
    echo "***[ERROR]: "${hdfs_backup_location}"  is not a valid HDFS path."   | tee -a $log_file
    echo "***[ERROR]: Please provide a valid HDFS path as backup location."   | tee -a $log_file
    return 1
  fi
  #check if the hdfs location exists
  hbase_cmd="$(get_hadoop_cmd) fs -test -e ${hsdf_loc} "
  echo  "${hbase_cmd}" | tee -a $log_file
  do_sudo ${hbase_user} "${hbase_cmd}"   2>&1 | tee -a  ${log_file}
  if [[ ${cr} == "create" && ${PIPESTATUS[0]} -ne 0 ]] ; then
    hbase_cmd="$(get_hadoop_cmd) fs -mkdir -p ${hsdf_loc}"
    echo  "${hbase_cmd}" | tee -a $log_file
    do_sudo ${hdfs_user} "${hbase_cmd}"   2>&1 | tee -a  ${log_file}
    if [[ ${PIPESTATUS[0]} -ne 0 ]] ;  then
      echo "***[ERROR]: ${hsdf_loc} folder could not be created."   | tee -a $log_file
      return  1
    fi
    hbase_cmd="$(get_hadoop_cmd) fs -chown ${hbase_user}:${hbase_user}   ${hsdf_loc} "
    echo  "${hbase_cmd}" | tee -a ${log_file}
    do_sudo ${hdfs_user} "${hbase_cmd}"   2>&1 | tee -a  ${log_file}
    if [[ ${PIPESTATUS[0]} -ne 0 ]] ;  then
      echo "***[ERROR]: Could not change owner on ${hsdf_loc}."   | tee -a $log_file
      return 1
    fi
  fi

  #verify hbase user can write to folder
  if [[ ${cr} == "create" ]]; then
    hbase_cmd="$(get_hadoop_cmd) fs -touchz ${hsdf_loc}/tmp"
    echo  "${hbase_cmd}" | tee -a ${log_file}
    do_sudo ${hbase_user} "${hbase_cmd}"      2>&1 | tee -a  ${log_file}
    if [[ ${PIPESTATUS[0]} -ne 0 ]] ;  then
      echo "***[ERROR]: hbase user does not have privilege to write to the HDFS location ${hsdf_loc}."  | tee -a $log_file
      return 1
    fi
    hbase_cmd="$(get_hadoop_cmd) fs -rm -skipTrash $hsdf_loc/tmp"
    echo  "${hbase_cmd}" | tee -a ${log_file}
    do_sudo ${hbase_user} "$hbase_cmd"   | tee -a $log_file
  fi

  return 0
}

###############################################################################
#verify_trafodion_is_down
###############################################################################
verify_trafodion_is_down()
{
  local traf_user=$1
  ###
  echo "Checking if Trafodion processes running."  | tee -a $log_file
  ##
  process_count=$(do_sudo $traf_user "cstat -noheader" "-i" 2>/dev/null |wc -l)
  local ret_code=${PIPESTATUS[0]}
  which_environment
  local env1=$?
  # cstat seems to return 1 in the dev environment and not 0 even though it succeeds
  # in the cluster env it returns 0 on success
  if [[ ${env1} -eq 1 && ${ret_code} -eq 1 ]]; then
   #dev env
   ret_code=0
  fi

  if [[ ${ret_code} -ne 0 ]]
  then
   echo "***[ERROR]: Cannot verify whether trafodion is down or not."   | tee -a $log_file
   return  1
  fi
  if [[ $process_count -ne 0 ]]
  then
     echo "***[ERROR]: Trafodion needs to be shutdown completely before starting the full backup."    | tee -a $log_file
     return  1
  else
    echo "***[INFO]: Trafodion is down."  | tee -a $log_file
  fi
  return 0
}
###############################################################################
#validate_sudo_access
###############################################################################
validate_sudo_access()
{
   local usr=$1
   do_sudo $usr "echo 'aa'" &> /dev/null
   if [[  $? -ne 0 ]]
   then
     echo "***[ERROR]: Could not validate sudo access for ${usr}."     | tee -a ${log_file}
     return 1
   fi

   return 0
}
###############################################################################
#create_tmp_and_log_folders
###############################################################################
create_tmp_and_log_folders()
{
  echo "Checking if  ${log_dir} exists..."
  mkdir ${log_dir}   &> /dev/null
  if [[ ! -d "${log_dir}" ]]
  then
    echo "***[ERROR]: Cannot create ${log_dir} folder."
    return  1;
  fi
  mkdir $tmp_dir &> /dev/null
  if [[ ! -d "$tmp_dir" ]]
  then
    echo "***[ERROR]: Cannot create $tmp_dir folder."    | tee -a $log_file
    return 1;
  fi
  return 0
}


###############################################################################
#validate_environment_and_users
###############################################################################
validate_environment_and_users()
{

  #which environment are we using
  which_environment
  which_env=$?
  echo "environment: $which_env"  | tee -a $log_file

  if [[ $which_env -eq 1 ]]
  then
    #development environment
    trafodion_user=$USER
    hbase_user=$USER
    hdfs_user=$USER
  elif [[ $which_env -eq 2 ]]
  then
    # we are on a cluster with Cloudera, Horton Works or Mapr installed
    validate_srvr_user_name "$hbase_user" "HRegionServer" "$default_hbase_user" "HBase"
    if [[ $? -ne 0 ]]; then
      echo "***[ERROR]: HBase user could not be validated." | tee -a ${log_file}
      return 1
    fi
    hbase_user="$verified_user_name"
    echo "HBase user name: $hbase_user"    | tee -a ${log_file}

    validate_srvr_user_name "$hdfs_user" "DataNode" "$default_hdfs_user"  "HDFS"
    if [[ $? -ne 0 ]]; then
      echo "***[ERROR]: HDFS user could not be validated."  | tee -a ${log_file}
      return 1
    fi
    hdfs_user="$verified_user_name"
    echo "HDFS user name: $hdfs_user"  | tee -a ${log_file}

    validate_srvr_user_name "$trafodion_user" "Trafodion"  "$default_trafodion_user"  "Trafodion"
    if [[ $? -ne 0 ]] ; then
      echo "***[ERROR]: Trafodion user could not be validated." | tee -a ${log_file}
      return 1
    fi
    trafodion_user="$verified_user_name"
    echo "Trafodion user name: $trafodion_user"  | tee -a ${log_file}
    #verify sudo access
    validate_sudo_access $hbase_user
    if [[ $? -ne 0 ]]; then
      return 1
    fi
  else
    echo "***[ERROR]: Did not find supported Hadoop distribution or $TRAF_HOME is not set."
    return 1
  fi

  hbase_snapshot_enabled=$($(get_hbase_cmd) org.apache.hadoop.hbase.util.HBaseConfTool hbase.snapshot.enabled)
  echo "hbase.snapshot.enabled=${hbase_snapshot_enabled}"
  if [[ "$hbase_snapshot_enabled" != "true" ]]
  then
    echo "***[ERROR]: Snapshots do not seem to be enabled. Please enable snapshots and try again."   | tee -a $log_file
    return  1
  fi

  verify_trafodion_is_down ${trafodion_user}
  if [[ $? -ne 0 ]] ;  then
    return 1
  fi

  return 0
}
###############################################################################
#get_hdfs_uri
###############################################################################
get_hdfs_uri()
{
  local hbase_rtdir=$($(get_hbase_cmd) org.apache.hadoop.hbase.util.HBaseConfTool hbase.rootdir)
  local fs_defFs=$($(get_hbase_cmd) org.apache.hadoop.hbase.util.HBaseConfTool fs.defaultFS)
  echo  "${fs_defFs}" | egrep -i -e 'hdfs://.*' 2>&1 > /dev/null
  if [[ ${PIPESTATUS[1]} -ne 0 ]];  then
    echo ${hbase_rtdir} | sed "s/\(hdfs:\/\/.*:[0-9]*\).*/\1/i"
  else
    echo ${fs_defFs}
  fi
}
###############################################################################
#start_trafodion
###############################################################################
start_trafodion()
{
  local traf_user=$1
  ###
  echo "Starting Trafodion..."  | tee -a $log_file
  ##
  which_environment
  local env1=$?
  local sqstart_rc=1
  if [[ "$USER" -eq "$traf_user" ]]; then
    env1=1
  fi
  if [[ $env1 -eq 1 ]]; then
    $TRAF_HOME/sql/scripts/sqstart
    sqstart_rc=$?
  elif [[ $env1 -eq 2 ]]; then
    sudo -n -u $traf_user sh -c ". /home/trafodion/.bashrc; sqstart"  
    sqstart_rc=$?
  else
    sqstart_rc=1 
  fi

  if [[ $sqstart_rc -eq 0 ]]; then
   echo "Trafodion started successfully. Continuing ..." | tee -a $log_file
  else
   echo "Trafodion not started. Please start Trafodion at your convinience." | tee -a $log_file
  fi
  return 0
}
