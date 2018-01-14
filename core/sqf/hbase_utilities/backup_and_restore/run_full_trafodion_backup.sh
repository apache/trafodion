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
# Backup all Trafodion tables
#==========================================

source ${0%/*}/backup_restore_functions.sh

usage()
{
  echo "The $0 script performs full offline backup of all Trafodion tables"
  echo "and copies the backup files to an HDFS location"
  echo "The command to use the script is as follows:"
  echo "$0 -b backup_folder -b backup_dir -u trafodion_user -h hbase_user -d hdfs_user -m mappers -l 10 -n -o"
  echo "where"
  echo "-b backup_folder"
  echo "     (Optional) HDFS path where all the Trafodion object are exported and saved"
  echo "     The HDFS path needs to have a format like hdfs://<host>:<port>/<folder>/..."
  echo "     If the path is not provided the script generates a path with a format like"
  echo "     hdfs://<host>:<port>/trafodion-backups/backup_<timestamp> and unless -n"
  echo "     is specified the user is asked whether to confirm the use of the generated"
  echo "     path."
  echo "-u trafodion user"
  echo "     (Optional) The user under which Trafodion server runs. If not provided and"
  echo "     if -n option is not specified the user is asked whether the default"
  echo "     trafodion user 'trafodion' can be used or not. If the answer is yes then the"
  echo "      default trafodion user is used otherwise the script exits."
  echo "-h hbase user"
  echo "     (Optional) The user under which HBase server runs. If not provided the script"
  echo "     tries to compute it and if it does not succeed it considers using the default"
  echo "     hbase user 'hbase'. Unless the -n option is specified, the user is asked to"
  echo "     confirm the selection afterwards."
  echo "-d hdfs user"
  echo "     (Optional) The user under which HDFS server runs. If not provided the script"
  echo "     tries to compute it and if it does not succeed it considers using the default"
  echo "     HDFS user 'hdfs'. Unless the -n option is specified, the user is asked to"
  echo "     confirm the selection afterwards."
  echo "-m mappers"
  echo "     (Optional) Number of mappers. If unspecified each snapshot will use a number suitable for its size."
  echo "-n"
  echo "     (Optional) Non interactive mode. With this option the script does not prompt"
  echo "     the user to confirm the use of computed or default values when a parameter"
  echo "     like trafodion user, hbase user, hdfs user or backup path is not provided."
  echo "-o"
  echo "     (Optional) offline. With this option trafodion will not be restarted after"
  echo "     snapshots are taken."
  echo "-l"
  echo "     (Optional) Snapshot size limit in MB above which map reduce is used for copy. Snapshots with size below this value will be copied using HDFS FileUtil.copy. Default value is 100 MB. FileUtil.copy is invoked through a class provided by Trafodion. Use 0 for this option to use HBase' ExportSnaphot class instead."
  echo " Example: $0  -b hdfs://<host>:<port>/<hdfs-path>  -m 4"
  exit 1
}

hdfs_backup_location=
snap_suffix=SNAP
snapshot_name=

backup_run_dir=${PWD}
tmp_dir=$backup_run_dir/tmp
log_dir=$backup_run_dir/logs
log_file=${log_dir}/run_traf_backup_${date_str}.log
tmp_log_file=${tmp_dir}/tmp_log.log

#hbase shell scripts
hbase_create_snapshots=$tmp_dir/bckp_create_snapshots_${date_str}.hbase
hbase_delete_snapshots=$tmp_dir/bckp_delete_snapshots_${date_str}.hbase
hbase_disable_tables=$tmp_dir/bckp_disable_tables_${date_str}.hbase
hbase_drop_tables=$tmp_dir/bckp_drop_tables_${date_str}.hbase
hbase_trafodion_table_list=${tmp_dir}/bckp_trafodion_table_list_${date_str}.hbase

trafodion_table_list_file=$tmp_dir/bckp_trafodion_table_list_${date_str}.txt
tmp_trafodion_table_list_file=${tmp_dir}/bckp_tmp_trafodion_table_list_${date_str}.txt
trafodion_snapshot_list_file=trafodion_snapshot_list.txt
trafodion_snapshot_list_path=$tmp_dir/${trafodion_snapshot_list_file}



#
trafodion_user=
hbase_user=
hdfs_user=
confirm=1
stay_offline=0
mr_limit=100

while getopts b:u:m:h:d:l:no arguments
do
  case $arguments in
  b)  hdfs_backup_location=$OPTARG;;
  m)  mappers=$OPTARG;;
  u)  trafodion_user=$OPTARG;;
  h)  hbase_user=$OPTARG;;
  d)  hdfs_user=$OPTARG;;
  l)  mr_limit=$OPTARG;;
  o)  stay_offline=1;;
  n)  confirm=0;;
  *)  usage;;
  esac
done

#check the HBase compatiblity if TrafExportSnapshot is able to be used
java org.trafodion.utility.backuprestore.TrafExportSnapshot -test
if [[ $? -ne 0 ]]; then
  echo 'not able to use TrafExportSnapshot'
  mr_limit=0
else
  echo 'able to use TrafExportSnapshot'
fi

echo "logging output to: ${log_file}"

#create tmp and log folders if they don't exist
create_tmp_and_log_folders
if [[ $? -ne 0 ]]; then
  exit 1
fi

#validate the environmet and users --
validate_environment_and_users
if [[ $? -ne 0 ]]; then
  exit 1
fi

#get the hdfs uri
hdfs_uri=$(get_hdfs_uri)
echo "hdfs_uri: ${hdfs_uri}"
echo "hdfs_backup_location:${hdfs_backup_location}"
# if hdfs backup location is empty generate one
if [[ -z "$hdfs_backup_location" ]] ; then
  new_path=${hdfs_uri}/user/trafodion/trafodion_backups/backup_${date_str}
  confirm_choice "Would you like to use this path as the backup folder: ${new_path} ?"
  if [[ $? -ne 0 ]]; then
    echo "***[ERROR]: New path ${new_path} could not be validated."    | tee -a  ${log_file}
    exit 1
   else
     hdfs_backup_location=${new_path}
  fi
fi

echo "hdfs_backup_location  ${hdfs_backup_location}"

#verify that the hdfs backup location is valid and exists. create it if not there
verify_or_create_folder ${hdfs_backup_location} "create"
ret_code=$?
if [[ $ret_code -ne 0  ]]; then
   exit $ret_code
fi

#verify backup folder has no contents
var_count=$($(get_hadoop_cmd) fs -ls ${hdfs_backup_location} | wc -l)
if [  $var_count -ne 0 ]; then
   echo "***[ERROR]: The HDFS backup location ${hdfs_backup_location} is not empty. "  | tee -a $log_file
   echo "***[ERROR]: Please provide an empty HDFS location for the backup."  | tee -a $log_file
   exit 1
fi

echo "Backup Folder: ${hdfs_backup_location} ." | tee -a $log_file

#verify that the hbase home dir exist. ExportSnapshot fails if it does not exist
verify_or_create_folder "${hdfs_uri}/user/${hbase_user}" "create"
ret_code=$?
if [[ $ret_code -ne 0  ]] ; then
   exit $ret_code
fi

###get list of trafodion tables
echo "hbase_trafodion_table_list: ${hbase_trafodion_table_list}" | tee -a $log_file
cat <<EOF > ${hbase_trafodion_table_list}
list  'TRAFODION\..*\..*'
exit
EOF
if [[ ! -f "${hbase_trafodion_table_list}" ]]; then
 echo "***[ERROR]: Cannot create the ${hbase_trafodion_table_list} file. Exiting ..."   | tee -a $log_file
 exit 1;
fi

echo "Getting list of Trafodion tables from HBase "  | tee -a  $log_file
$(get_hbase_cmd) shell ${hbase_trafodion_table_list} | tee  ${tmp_trafodion_table_list_file}   | tee -a  $log_file
echo "grep for trafodion tables"      | tee -a  $log_file
#filter out anything other than Trafodion tables from the list
grep 'TRAFODION.*'   ${tmp_trafodion_table_list_file} | tee  ${trafodion_table_list_file}    | tee -a  $log_file


###Generate HBase shell scripts
> $hbase_create_snapshots
> $hbase_delete_snapshots
> $hbase_disable_tables
> $hbase_drop_tables
> ${trafodion_snapshot_list_path}
#
while read line
do
  table_name=$line
  snapshot_name=${table_name}_${snap_suffix}_${date_str}

  echo "Generating hbase scripts for table: ${table_name}"
  # hbase shell scripts - create snapshot
  echo "snapshot	'${table_name}'	, '${snapshot_name}'" >>	$hbase_create_snapshots
  # hbase shell script - delete snapshot
  echo "delete_snapshot	'${snapshot_name}'" >>	$hbase_delete_snapshots
  # hbase shell script - disable table
  echo "disable	'${table_name}'" >> $hbase_disable_tables
  # hbase shell script - drop table
  echo "drop	'${table_name}'" >> $hbase_drop_tables
  # make a list of the snapshots.
  echo ${snapshot_name} >> ${trafodion_snapshot_list_path}
done < ${trafodion_table_list_file}

echo "exit" >>  $hbase_create_snapshots
echo "exit" >>  $hbase_disable_tables
echo "exit" >>  $hbase_drop_tables
echo "exit" >>  $hbase_delete_snapshots


## Do create the snapshots
echo "Creating hbase snapshots ..."  | tee  -a  ${log_file}
cat ${hbase_create_snapshots}  | tee  -a  ${log_file}
hbase_cmd="$(get_hbase_cmd) shell ${hbase_create_snapshots}"
echo "${hbase_cmd}"  | tee  -a  ${log_file}
${hbase_cmd}   2>&1 |  tee $tmp_log_file | tee -a  ${log_file}
## check if there are errors. exit on error.
grep ERROR $tmp_log_file
if  [[ $? -eq 0 ]]; then
    echo "***[ERROR]: Error encountered while creating snapshots"     | tee -a $log_file
    echo "For more information please check the logs at ${log_file}. "     | tee -a $log_file
    exit 1;
fi

## Restart Trafodion
if [[ ${stay_offline} -ne 1 ]]; then
 start_trafodion $trafodion_user 
else
 echo "Trafodion can now be restarted at your convinience. Backup script does not need for it to be offline anymore."
fi

while read line
do
  snapshot_name=$line
  echo "********************************************************************"
  echo "Exporting ${snapshot_name} ..."
  if [[ ${mr_limit} -eq 0 ]]; then
     hbase_cmd="$(get_hbase_cmd) org.apache.hadoop.hbase.snapshot.ExportSnapshot"
  else 
     hbase_cmd="$(get_hbase_cmd) org.trafodion.utility.backuprestore.TrafExportSnapshot"
  fi
  hbase_cmd+=" -snapshot ${snapshot_name}"
  hbase_cmd+=" -copy-to ${hdfs_backup_location}/${snapshot_name}"
  hbase_cmd+=" -mappers $mappers"
  if [[ ${mr_limit} -ne 0 ]]; then
     hbase_cmd+=" -mr-lowlimit-mb ${mr_limit}"
  fi
  echo "${hbase_cmd}" | tee -a ${log_file}

  do_sudo ${hbase_user} "${hbase_cmd}" 2>&1 | tee -a  ${log_file}
  ##check for errors
  if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
      echo "***[ERROR]: Error while exporting snapshot: ${snapshot_name}."   | tee -a $log_file
      echo "For more information please check the logs at ${log_file}."    | tee -a $log_file
      exit 1;
  fi
done < ${trafodion_snapshot_list_path}

#copy the generated snapshot list to the hdfs backup location
#first copy the list to /tmp because in some cases hadoop fs -copyFromLocal gives error
#message when using the local path -- permission issue
cp ${trafodion_snapshot_list_path} /tmp/${trafodion_snapshot_list_file}

hbase_cmd="$(get_hadoop_cmd) fs -copyFromLocal /tmp/${trafodion_snapshot_list_file}  ${hdfs_backup_location}"
echo "${hbase_cmd}"   | tee -a  ${log_file}
do_sudo  ${hbase_user}  "${hbase_cmd}"  2>&1   | tee -a  ${log_file}
if [[ ${PIPESTATUS[0]} -ne 0 ]] ; then
   echo "***[ERROR]: Error occurred while copying the ${trafodion_snapshot_list_path} file to hdfs backup location ${hdfs_backup_location}."   | tee -a $log_file
   echo "For more information please check the logs at ${log_file}. "   | tee -a $log_file
   exit 1;
fi

## do delete the snapshots after export is done
echo "Deleting hbase snapshots ..."  | tee  -a  ${log_file}
cat ${hbase_delete_snapshots}  | tee  -a  ${log_file}
echo "$(get_hbase_cmd) shell ${hbase_delete_snapshots} "   | tee -a  ${log_file}
$(get_hbase_cmd) shell ${hbase_delete_snapshots}   2>&1 | tee  $tmp_log_file | tee -a  ${log_file}
## check if there are errors. exit on error.
grep ERROR $tmp_log_file
if [ $? -eq 0 ]; then
    echo "***[ERROR]: An Error occurred while deleting snapshots"
    echo "They can be deleted manually using the ${hbase_delete_snapshots} script after the backup is done."
    #exit 1;
fi

###Verifications
#echo "cat ./${trafodion_table_list_file} | wc -l"
#echo "$(get_hadoop_cmd) fs -ls $hdfs_backup_location | grep -v ./${trafodion_snapshot_list_path}"
#echo "$(get_hadoop_cmd) fs -cat $hdfs_backup_location/${trafodion_snapshot_list_path} | wc -l"
traf_table_count=$(cat ${trafodion_table_list_file} | wc -l )
echo    "traf_table_count: ${traf_table_count} " | tee -a $log_file

hbase_cmd="$(get_hadoop_cmd) fs -ls -d ${hdfs_backup_location}/TRAFODION.*.*"
var_backup_snapshot_count=$(do_sudo ${hbase_user}  "$hbase_cmd" | grep 'TRAFODION.\.*\.*' | wc -l)

echo "var_backup_snapshot_count: ${var_backup_snapshot_count}" | tee -a $log_file

hbase_cmd="$(get_hadoop_cmd) fs -cat ${hdfs_backup_location}/${trafodion_snapshot_list_file}"
var_snapshot_file_count=$(do_sudo ${hbase_user} "${hbase_cmd} " | wc -l)
echo "var_snapshot_file_count: ${var_snapshot_file_count} "  | tee -a $log_file
if [[  ${traf_table_count} -ne ${var_backup_snapshot_count} ]]
then
   echo "***[ERROR]: the number of snapshots in backup location ${hdfs_backup_location} does not match the number of trafodion tables."  | tee -a $log_file
   exit 1
fi

if [[  ${traf_table_count} -ne ${var_snapshot_file_count} ]]
then
   echo "***[ERROR]: the number of snapshots listed in ${hdfs_backup_location}/${trafodion_snapshot_list_file} does not match the number of trafodion tables."  | tee -a $log_file
   exit 1
fi


echo "Backup complete."  | tee -a $log_file
exit 0
