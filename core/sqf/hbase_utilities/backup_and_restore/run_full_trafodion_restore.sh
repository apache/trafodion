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


#==========================================
# Restore all Trafodion tables
#==========================================
source ${0%/*}/backup_restore_functions.sh

usage()
{
  echo "The $0 script performs full offline restore of all Trafodion tables from"
  echo "a HDFS location"
  echo "The command to use the script is as follows:"
  echo "$0 -b backup_folder -b backup_dir -u trafodion_user -h hbase_user  -m mappers -n"
  echo "where"
  echo "-b  backup_folder"
  echo "     (Not Optional) HDFS path where all the Trafodion object are exported and"
  echo "      saved The HDFS path needs to have a format like hdfs://<host>:<port>/<folder>/..."
  echo "-u trafodion user"
  echo "     (Optional) The user under which Trafodion server runs. If not provided and"
  echo "     if -n option is not specified the user is asked whether the default"
  echo "     trafodion user 'trafodion' can be used or not. If the answer is yes then the"
  echo "      default trafodion user is used otherwise the script exits."
  echo "-h hbase user"
  echo "     (Optional) The user under which HBase server runs. If not provided the script"
  echo "     tries to compute it and if it does not succeed it considers using the default"
  echo "     hbase user 'hbase'. Unless the -n option is specified, the user is asked to"
  echo "      confirm the selection afterwards."
  echo "-d hdfs user"
  echo "     (Optional) The user under which HDFS server runs. If not provided the script"
  echo "     tries to compute it and if it does not succeed it considers using the default"
  echo "     HDFS user 'hdfs'. Unless the -n option is specified, the user is asked to"
  echo "      confirm the selection afterwards."
  echo "-m mappers"
  echo "     (Optional) Number of mappers. The default value is 2."
  echo "-n"
  echo "     (Optional) Non interactive mode. With this option the script does not prompt"
  echo "     the user to confirm the use of computed or default values when a parameter"
  echo "      like trafodion user, hbase user, hdfs user or backup path is not provided."
  echo " Example: $0  -b hdfs://<host>:<port>/<hdfs-path>  -m 4"
  exit 1
}

if [ $# -lt 2 ]; then
 usage
 exit 1
fi

hdfs_backup_location=
hbase_root_dir=
trafodion_user=
confirm=1
snapshot_name=

restore_run_id=${PWD}
tmp_dir=$restore_run_id/tmp
log_dir=$restore_run_id/logs
log_file=$log_dir/run_traf_restore_${date_str}.log
tmp_log_file=$tmp_dir/tmp_log_file.log
snapshot_list_file=${tmp_dir}/trafodion_snapshot_list_${date_str}.txt

# hbase shell scripts
hbase_restore_snapshots=${tmp_dir}/rstr_restore_snapshots.hbase
hbase_delete_snapshots=${tmp_dir}/rstr_delete_snapshots.hbase



while getopts :b:u:m:h:d:n arguments
do
  case $arguments in
  b)  hdfs_backup_location=$OPTARG;;
  m)  mappers=$OPTARG;;
  u)  trafodion_user=$OPTARG;;
  h)  hbase_user=$OPTARG;;
  d)  hdfs_user=$OPTARG;;
  n)  confirm=0;;
  *)  usage;;
  esac
done


echo "logging output to: ${log_file}"

#create tmp and log folders if they don't exist
create_tmp_and_log_folders

#validate the environmet and users --
validate_environment_and_users
if [[ $? -ne 0 ]]; then
  exit 1
fi

hbase_root_dir=$($(get_hbase_cmd) org.apache.hadoop.hbase.util.HBaseConfTool hbase.rootdir)
echo "hbase.rootdir= ${hbase_root_dir}"
if [[ -z "${hbase_root_dir}" ]]
then
   echo "***[ERROR]: Not able to determine hbase root dir. Please make sure all settings and classpath are correct"    | tee -a $log_file
   exit 1
fi

#verify that the hdfs backup location is valid and exists
verify_or_create_folder ${hdfs_backup_location} "nocreate"
ret_code=$?
if [[ $ret_code -ne 0  ]]; then
   exit $ret_code
fi


#get the hdfs uri
hdfs_uri=$(get_hdfs_uri)

#verify that the hbase home dir exist. ExportSnapshot fails if it does not exist
verify_or_create_folder "${hdfs_uri}/user/${hbase_user}" "create"
ret_code=$?
if [[ $ret_code -ne 0  ]] ; then
   exit $ret_code
fi


#copy the snapshot list from the hdfs backup location to local tmp folder
hbase_cmd="$(get_hadoop_cmd) fs -copyToLocal ${hdfs_backup_location}/trafodion_snapshot_list.txt ${snapshot_list_file}"
echo "${hbase_cmd}"  | tee -a $log_file
${hbase_cmd} 2>&1 | tee -a   $log_file
##check for erros
if [[ ${PIPESTATUS[0]} -ne 0 ]]
then
   echo "***[ERROR]: Error occurred while copying the file containing the list of trafodion snapshots from the hdfs backup location"   | tee -a $log_file
   exit 1;
fi
####verify numbers
var_backup_snapshot_count=$($(get_hadoop_cmd) fs -ls -d ${hdfs_backup_location}/TRAFODION.*.* | grep 'TRAFODION\..*\..*'  | wc -l)
if [[ ${PIPESTATUS[0]} -ne 0 ]]
then
   echo "***[ERROR]: Error occurred while copying the list of trafodion snapshots from the hdfs backup location ${hdfs_backup_location}"   | tee -a $log_file
   exit 1;
fi
var_snapshot_file_count=$(cat ${snapshot_list_file} | wc -l)
if [[ ${PIPESTATUS[0]} -ne 0 ]]
then
   echo "***[ERROR]: Error occurred while getting the content of ${snapshot_list_file} "      | tee -a $log_file
   exit 1;
fi
if [[  $var_snapshot_file_count -ne $var_backup_snapshot_count ]]
then
   echo "***[ERROR]: the number of snapshots in backup location ${hdfs_backup_location} " | tee -a $log_file
   echo "does not match the number of snapshots in snapshot list file."   | tee -a $log_file
   exit 1
fi

#########
>  $hbase_restore_snapshots
>  $hbase_delete_snapshots

#Check if there trafodion objects in the target hbase  and give error if there are
cat <<EOF > ${tmp_dir}/tmp_list_tgt_trafodion_tables.hbase
list 'TRAFODION\..*\..*'
exit
EOF
hbase_cmd="$(get_hbase_cmd) shell ${tmp_dir}/tmp_list_tgt_trafodion_tables.hbase"
echo "${hbase_cmd}" | tee -a $log_file
${hbase_cmd} | tee   ${tmp_dir}/tmp_tgt_trafodion_table_list.txt | tee -a $log_file
grep 'TRAFODION\..*\..*'  ${tmp_dir}/tmp_tgt_trafodion_table_list.txt  > /dev/null
if [ $? -eq 0 ]; then
    echo "****[ERROR]: The target HBase has Trafodion objects. "   | tee -a $log_file
    echo "Please remove all Trafodion objects and run the script again."  | tee -a $log_file
    exit 1;
fi

echo  "Start of Importing Snapshots"   | tee -a $log_file
#
while read line
do
  snapshot_name=$line

  # hbase scripts to create , delete ....
  echo "restore_snapshot	'${snapshot_name}'" >>	$hbase_restore_snapshots
  echo "delete_snapshot	'${snapshot_name}'" >>	$hbase_delete_snapshots
  echo  "Importing Snapshot: ${snapshot_name}"   | tee -a $log_file
  hbase_cmd="$(get_hbase_cmd)"
  hbase_cmd+="org.apache.hadoop.hbase.snapshot.ExportSnapshot"
  hbase_cmd+="-D hbase.rootdir=${hdfs_backup_location}/${snapshot_name}"
  hbase_cmd+="-snapshot ${snapshot_name}"
  hbase_cmd+="-copy-to ${hbase_root_dir}"
  echo "${hbase_cmd}" | tee -a  $log_file

  do_sudo ${hbase_user} "${hbase_cmd}" 2>&1 | tee -a  ${log_file}
  ##check for erros
  if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
      echo "****[ERROR]: Error encountered while importing snapshot ${snapshot_name}." | tee -a $log_file
      echo "More information can be found in the log file ${log_file}."
      exit 1;
  fi
done < ${snapshot_list_file}

echo "exit" >>	$hbase_restore_snapshots
echo "exit" >>  $hbase_delete_snapshots

## do restore the snapshots
echo  "Restoring Snapshots ..."   | tee -a $log_file
cat ${hbase_restore_snapshots} | tee -a $log_file
hbase_cmd="$(get_hbase_cmd)  shell ${hbase_restore_snapshots}"
echo "${hbase_cmd}"  | tee -a $log_file
${hbase_cmd}  2>&1  | tee $tmp_log_file | tee -a  ${log_file}
## check if there are errors. exit on error. need to verify the string to grep for
grep ERROR $tmp_log_file
if [[ $? -eq 0 ]]
then
    echo "****[ERROR]: Error encountered while restoring snapshots"   | tee -a $log_file
    echo "More information can be found in the log file ${log_file}."
    exit 1;
fi

## do delete the snapshots
echo  "Deleting Snapshots ..."   | tee -a $log_file
cat ${hbase_delete_snapshots} | tee -a $log_file
hbase_cmd="$(get_hbase_cmd) shell ${hbase_delete_snapshots}"
echo ${hbase_cmd}  | tee -a $log_file
${hbase_cmd}  2>&1   | tee $tmp_log_file | tee -a  ${log_file}
grep ERROR $tmp_log_file
if [ $? -eq 0 ]; then
    echo '****[ERROR]: Error encountered while deleting snapshots'    | tee -a $log_file
    exit 1;
fi

echo "restore complete"     | tee -a $log_file
exit 0
