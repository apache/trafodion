#!/bin/bash
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

function setup_my_nodes {
    export MY_NODES=`trafconf -wname`
    export NODE_LIST=`trafconf -name`
}

function setup_sqpdsh {
    if [ -e $SQ_PDSH ]; then
        if [ -z "$MY_NODES" ]; then
            export SQPDSHA="$SQ_PDSH -a"
        else
            setup_my_nodes
            export SQPDSHA="$PDSH $MY_NODES $PDSH_SSH_CMD"
        fi
    else
        export SQPDSHA="eval"
    fi
}

# head of the monitor.map files in the cluster
function cmaph {
    l_nl=1
    if [ $# != 0 ]; then
	l_nl=$1
    fi
    setup_sqpdsh
    eval '$SQPDSHA "head --quiet --lines $l_nl $TRAF_VAR/monitor.map.[0-9]*.*" 2>/dev/null | sort -k1'
}

# tail of the monitor.map.[0-9]*.* files in the cluster
function cmapt {
    l_nl=1
    if [ $# != 0 ]; then
	l_nl=$1
    fi
    setup_sqpdsh
    eval '$SQPDSHA "tail --quiet --lines $l_nl $TRAF_VAR/monitor.map.[0-9]*.*" 2>/dev/null | sort -k1'
}

#grep the monitor map file(s) on a node for the given string
function ngrepmm {
    l_lookup=$*
    grep -h "$l_lookup" $TRAF_VAR/monitor.map.[0-9]*.*
}

#grep the monitor map file(s) on a node for the given string and then filters anything other than tdm_arkesp 
function ngrepmms {
    l_lookup=$*
    grep -h "$l_lookup" $TRAF_VAR/monitor.map.[0-9]*.* | grep tdm_arkesp
}

# lookup the monitor.map.[0-9]*.* files in the cluster
function cmapl {
    if [ $# == 0 ]; then
	echo "Enter a string to lookup in all the monitor.map.[0-9]*.* files"
	return 1;
    fi
    l_lookup=$*
    setup_sqpdsh
    eval '$SQPDSHA "ngrepmm $l_lookup" 2>/dev/null'
}

function cmaplc {

    cmapl $* | wc -l
}

function cmaplbc {

    cmapl $* | grep BEGIN | wc -l
}

function cmaplec {

    cmapl $* | grep END | wc -l
}

function cmaplbec {
    l_lookup=$*

    lv_all=`cmaplc $*`
    lv_begin=`cmaplbc $*`
    lv_end=`cmaplec $*`

    echo "$l_lookup,$lv_all,$lv_begin,$lv_end"
    
}

# ===== BEGIN ======= lookup the monitor.map.[0-9]*.* files for ESPs 
function cmapls {
    if [ $# == 0 ]; then
	echo "Enter a string to lookup in all the monitor.map.[0-9]*.* files"
	return 1;
    fi
    l_lookup=$*
    setup_sqpdsh
    eval '$SQPDSHA "ngrepmms $l_lookup" 2>/dev/null'
}

function cmaplsc {

    cmapls $* | wc -l
}

function cmaplsbc {

    cmapls $* | grep BEGIN | wc -l
}

function cmaplsec {

    cmapls $* | grep END | wc -l
}

function cmaplsbec {
    l_lookup=$*

    lv_all=`cmaplsc $*`
    lv_begin=`cmaplsbc $*`
    lv_end=`cmaplsec $*`

    echo "$l_lookup,$lv_all,$lv_begin,$lv_end"
    
}
# =======  END ======= lookup the monitor.map.[0-9]*.* files for ESPs

# Count the number of processes started (in the lifetime of this SQ env) for a particular program string - defaults to arkesp
function cmappc {
    l_nl=arkesp
    if [ $# != 0 ]; then
	l_nl=$1
    fi
    setup_sqpdsh
    eval '$SQPDSHA "grep -h $l_nl $TRAF_VAR/monitor.map.[0-9]*.* | grep BEGIN | wc -l" 2>/dev/null | sort -nk2'
}


#### Begin MD5 related functions
function sqmd5b {
    setup_sqpdsh
    eval '$SQPDSHA "cd $TRAF_HOME/export/bin${SQ_MBTYPE}; md5sum $1 2>/dev/null" 2>/dev/null | cut -f2 -d: | sort -u'
}

function sqmd5ba {
    setup_sqpdsh
    eval '$SQPDSHA "cd $TRAF_HOME/export/bin${SQ_MBTYPE}; md5sum $1 2>/dev/null" 2>/dev/null | sort'
}

function sqmd5l {
    setup_sqpdsh
    eval '$SQPDSHA "cd $TRAF_HOME/export/lib${SQ_MBTYPE}; md5sum $1 2>/dev/null" 2>/dev/null | cut -f2 -d: | sort -u'
}

function sqmd5la {
    setup_sqpdsh
    eval '$SQPDSHA "cd $TRAF_HOME/export/lib${SQ_MBTYPE}; md5sum $1 2>/dev/null" 2>/dev/null | sort -u'
}

#### End MD5 related functions

function cmapc {
    setup_sqpdsh
    eval '$SQPDSHA "grep $1 $TRAF_VAR/monitor.map* | grep $2 | wc -l" '
}

function pdsh_counter {
    if [ -e $SQ_PDSH ]; then
	awk 'BEGIN {cnt=0} {cnt=cnt+$2} END {print cnt;}'
    else
	awk 'BEGIN {cnt=0} {cnt=cnt+$1} END {print cnt;}'
    fi
}

function tnbprog {
    if [ $# != 0 ]; then
        lv_prog=$1
    else
        lv_prog=arkesp
    fi
    cmapc BEGIN $lv_prog | pdsh_counter
}

function tneprog {
    if [ $# != 0 ]; then
        lv_prog=$1
    else
        lv_prog=arkesp
    fi
    cmapc END $lv_prog | pdsh_counter
}

function espstats {
   lv_ncurr=`nesp`
   lv_nbegin=`tnbprog tdm_arkesp`
   lv_nend=`tneprog tdm_arkesp`
   echo "`date`; ESPs running: $lv_ncurr; Total started: $lv_nbegin; Total exitted: $lv_nend;"
}

function sqlcistats {
   lv_ncurr=`nsqlci`
   lv_nbegin=`tnbprog sqlci`
   lv_nend=`tneprog sqlci`
   echo "`date`; SQLCIs running: $lv_ncurr; Total started: $lv_nbegin; Total exitted: $lv_nend;"
}

function cmpstats {
   lv_ncurr=`ncmp`
   lv_nbegin=`tnbprog tdm_arkcmp`
   lv_nend=`tneprog tdm_arkcmp`
   echo "`date`; CMPs running: $lv_ncurr; Total started: $lv_nbegin; Total exitted: $lv_nend;"
}

function mxosstats {
   lv_ncurr=`nmxos`
   lv_nbegin=`tnbprog mxosrvr`
   lv_nend=`tneprog mxosrvr`
   echo "`date`; MXOSRVRs running: $lv_ncurr; Total started: $lv_nbegin; Total exitted: $lv_nend;"
}

function nprocpn {
    setup_sqpdsh
    eval '$SQPDSHA "pstat | grep $1 | wc -l" | sort -nk2'
}

#unsorted nprocn
function nprocpn_us {
    setup_sqpdsh
    eval '$SQPDSHA "pstat | grep $1 | wc -l"'
}

function ncmp {
    nprocpn_us tdm_arkcmp | pdsh_counter
}

function nesp {
    nprocpn_us tdm_arkesp | pdsh_counter
}

function nsqlci {
    nprocpn_us sqlci | pdsh_counter
}

function nmxos {
    nprocpn_us mxosrvr | pdsh_counter
}

function ndbm {
    setup_sqpdsh
    eval '$SQPDSHA "df -h | grep database" 2>/dev/null | wc -l'
}

function chkReturnCodeExit {
    if [[ $1 != 0 ]]; then
	echo "$2 returned error $1, exitting..."
	exit $1;
    else
	echo "$2 executed successfully."
    fi
}

function chkReturnCode {
    if [[ $1 != 0 ]]; then
	echo "$2 returned error $1..."
	return $1;
    else
	echo "$2 executed successfully."
	return 0
    fi
}

# $1: program/utility/script to run
# $2: (optional): retry count (default 0)
# $3: (optional): sleep for this many seconds
# 
function run_util {
    echo "--------------------------------------"
    lv_cmd=$*
    echo "executing: $1"
    $1
    lv_stat=$?
    if [ ! -z $3 ]; then
	declare -i lv_retries=0
	while [ $lv_retries -lt $2 ]; do
	    let lv_retries=($lv_retries+1)
	    chkReturnCode ${lv_stat} $1
	    if [ $? != 0 ]; then
		if [ $lv_retries -lt $2 ]; then
		    echo "retrying in $3 seconds"
		    sleep $3
		    $1
		    lv_stat=$?
		else
		    exit ${lv_stat}
		fi
	    else
		return 0
	    fi
	done
    else 
	chkReturnCodeExit ${lv_stat} $1
    fi
    echo "--------------------------------------"
}
# check the startup log and sort the interesting even chronologically
function sqchksl {
    setup_sqpdsh
    eval '$SQPDSHA "cd $TRAF_LOG; grep Executing mon_startup.log 2>/dev/null" 2>/dev/null | sort -k4 -k5'
}

function sqchkopt {
    setup_sqpdsh
    eval '$SQPDSHA "df -h /opt 2>/dev/null" 2>/dev/null | grep -v Avail | sort -rnk1'
}

function sqchkvm {
    vmstat -a | head -2
    setup_sqpdsh
    eval '$SQPDSHA "vmstat -a" | grep -v swpd | grep -v memory | sort -rnk5'
}

function sqlscore {
    if [ $# == 0 ]; then
	echo "Enter a string. Exiting.";
	return 1;
    fi

    lv_string=$1

    setup_sqpdsh
    eval '$SQPDSHA "cd /local/cores/$UID 2>/dev/null; ls -al 2>/dev/null | grep core | grep \"${lv_string}\" | cut -f 2 -d: | cut -b 4- "'
}

function sqrmcore {
    if [ $# == 0 ]; then
	echo "Enter a string. Exiting.";
	return 1;
    fi

    lv_string=$1

    setup_sqpdsh
    eval '$SQPDSHA "cd /local/cores/$UID 2>/dev/null; ls -al 2>/dev/null | grep core | grep \"${lv_string}\" | cut -f 2 -d: | cut -b 4- | xargs rm"'
}


function sqsecheck {
    sediskstatus | grep DOWN
}

function sqchkmpi {
    pdsh $MY_NODES "cd $TRAF_LOG; egrep -i '(mpi bug|ibv_create)' *.log" 2>/dev/null
}

#### Log Collection functions
function LogHeader {
    echo >> $lv_file_name
    echo "================= $1 =================" >> $lv_file_name
}

function LogPstack {
    LogHeader $1
    sqpstack $2 >> $lv_file_name
}

function collect_cstat {
    lv_file_name='cstat.out'
    LogHeader "CSTAT"
    cstat | sort -k13 >> $lv_file_name
}

function collect_cfindcore {
    lv_file_name='cfindcore.out'
    LogHeader "CFINDCORE"
    cfindcore >> $lv_file_name
}

function collect_cmaph {
    lv_file_name='cmaph.out'
    
    LogHeader "Output of the cmaph function:"
    cmaph 16 >> $lv_file_name
}

function collect_cmapt {
    lv_file_name='cmapt.out'
    
    LogHeader "Output of the cmapt function:"
    cmapt 16 >> $lv_file_name
}


function collect_sqchkvm {
    lv_file_name='sqchkvm.out'
    
    LogHeader "SQCHKVM"
    sqchkvm >> $lv_file_name
}

# Works on a per node basis. 
function sqsavelogs {

    lv_logs_collection_dir=$1
    lv_linux_collection_dir=$2
    lv_copy_to_dir=${lv_logs_collection_dir}/logs
    
    lv_node=`uname -n`
    
    sqcollectmonmemlog 2>/dev/null

    cp -p $TRAF_LOG/master_exec*.log ${lv_copy_to_dir}
    cp -p $TRAF_LOG/mon.*.log ${lv_copy_to_dir}
    cp -p $TRAF_LOG/monmem.*.log ${lv_copy_to_dir}
    cp -p $TRAF_LOG/pstart*.log ${lv_copy_to_dir}
    cp -p $TRAF_LOG/smstats.*.log ${lv_copy_to_dir}
    cp -p $TRAF_LOG/sqmo*.log ${lv_copy_to_dir}
    cp -p $TRAF_LOG/trafodion.*log* ${lv_copy_to_dir}
    cp -p $TRAF_LOG/tm*.log ${lv_copy_to_dir}
    cp -p $TRAF_LOG/wdt.*.log ${lv_copy_to_dir}

    cp -p $TRAF_VAR/monitor.map.[0-9]*.* ${lv_copy_to_dir}
    cp -p $TRAF_VAR/monitor.trace* ${lv_copy_to_dir}

    lv_stdout_dir_name=${lv_copy_to_dir}/stdout_${lv_node}
    mkdir -p ${lv_stdout_dir_name}
    cp -p $TRAF_LOG/startup.log ${lv_copy_to_dir}/startup.${lv_node}.log
    cp -p $TRAF_LOG/stdout_* ${lv_stdout_dir_name}

    lv_config_dir_name=${lv_copy_to_dir}/sqconfig_db
    cp -p $TRAF_VAR/sqconfig.db ${lv_config_dir_name}/${lv_node}_sqconfig.db

    sqsave_linux_info ${lv_linux_collection_dir}/linux

}

# drives the collection and compression of the logs/stdout files
# (for a particular node). 
function sqsavelogs_compress {

    pushd . >/dev/null

    lv_node_collection_dir_name=$1
    lv_move_to_dir=$2
    
    lv_node=`uname -n`

    cd $TRAF_HOME

    mkdir -p ${lv_node_collection_dir_name}/logs/sqconfig_db
    sqsavelogs ${TRAF_HOME}/${lv_node_collection_dir_name} ${lv_move_to_dir}

    lv_tar_file_name=${lv_node}_${lv_node_collection_dir_name}.tgz
    cd ${lv_node_collection_dir_name}
    tar czf ${lv_tar_file_name} logs

    # Move the tar file to the central collection location
    mv ${lv_tar_file_name} ${lv_move_to_dir}

    cd ..
    rm -rf ${lv_node_collection_dir_name}

    popd >/dev/null
}

function sqsave_linux_info {

    lv_node=`uname -n`
    
    dmesg > ${1}/dmesg.${lv_node}.log

    df > ${1}/df.${lv_node}.log

    ps -ef > ${1}/ps_ef.${lv_node}.log

    last reboot > ${1}/last_reboot.${lv_node}.log
    
}

function sqcollectmetrics {

    mkdir -p ~/logs/metrics
    pushd . >/dev/null
    cd ~/logs/metrics

    lv_curr_time=`date +%Y%m%d_%H%M`;
    lv_dir_name=sqmetrics.$lv_curr_time

    mkdir $lv_dir_name
    cd $lv_dir_name
    
    sqstate sbtest4-metrics -seltype tse > sbtest4_metrics.out

    echo "Output in the directory: $PWD/$lv_outfile"

    popd >/dev/null

}

# Wrapper script to collect logs and pstacks (via sqcollectstacks)    
function sqcollectlogs {
    
    declare -i lv_all
    declare -i lv_compress
    declare -i lv_zip
    declare -i lv_delete_directory_after_zip
    declare -i lv_help
    
    let lv_all=0
    let lv_compress=0
    let lv_zip=0
    let lv_delete_directory_after_zip=0
    let lv_help=0

    # The line below (OPTIND=1) is needed to use the 'getopts' routine to work in a function
    OPTIND=1
    while getopts "acdzh" lv_arg $1 $2 $3 $4 $5 $6 $7 $8 $9
      do
      case $lv_arg in 
	  a)
	      lv_all=1
	      ;;

	  c)
	      lv_compress=1
	      ;;

	  d)
	      lv_delete_directory_after_zip=1
	      ;;
	  z)
	      lv_zip=1
	      ;;
	  h)
	      lv_help=1
	      echo
	      echo "Usage: sqcollectlogs {-a | -d | -z | -h}"
	      echo
	      echo "-a Collect all (logs and pstacks). Default is logs only"
	      echo "-c Compress (tar-zip) the logs/stdout files on a per-node basis and move the tar-zip files to the"
	      echo "   collection node. To extract these files, execute the xtract_logs program that can be found"
	      echo "   in the logs collection directory."
	      echo "   On a cluster, recommend executing xtract_logs on the head node."
	      echo "-d After zipping, delete the directory where the logs were collected."
	      echo "   Note: This option (-d) only applies if the -z option is also used."
	      echo "-z tar/zip up the directory (containing the collected logs/pstacks). Default is not to tar/zip"
	      echo "-h Help "
	      echo
	      return 0
              ;;
      esac
    done

    mkdir -p ~/logs
    pushd . > /dev/null
    cd ~/logs
    
    lv_curr_time=`date +%Y%m%d_%H%M`
    lv_dir_name=sqinfo.$lv_curr_time
    
    mkdir $lv_dir_name
    cd $lv_dir_name
    lv_node_local_dir_name=$lv_dir_name

    echo "Collection in progress..."
    
    collect_cstat
    espstats > esp_stats.out
    mxosstats > mxos_stats.out
    cmpstats > cmp_stats.out

    collect_cmapt
    collect_cmaph
    
    collect_sqchkvm
    
    collect_cfindcore


#   sqgetsem > sqgetsem.out

#   begin configuration info
    mkdir config
    cd config
    sqinfo > sqinfo.out 2>/dev/null
    sqvers > sqvers.out 2>/dev/null
    sqvers -u > sqvers_u.out 2>/dev/null
    sqid > sqid.out 2>/dev/null

    cp -p $TRAF_HOME/sqenv.sh .
    cp -p $TRAF_HOME/sqenvcom.sh .
    cp -p $TRAF_VAR/ms.env .
    cp -p $TRAF_HOME/sql/scripts/gomon.cold .
    cp -p $TRAF_HOME/sql/scripts/gomon.warm .
    cp -p $TRAF_CONF/sqconfig .
    cp -p $TRAF_VAR/mon.env . 2>/dev/null
    cp -p $TRAF_VAR/shell.env . 2>/dev/null
    cd ..
#   end configuration info

    mkdir -p logs/sqconfig_db
    mkdir -p linux
    setup_sqpdsh
    
    if [[ $lv_compress == 0 ]]; then
	eval '$SQPDSHA "sqsavelogs $PWD $PWD" 2>/dev/null'
    else
	eval '$SQPDSHA "sqsavelogs_compress $lv_node_local_dir_name $PWD" 2>/dev/null'

        # Generate an extractor for the .tgz files created in the above step (sqsavelogs_compress)
	lv_xtract_script=xtract_logs
	echo "#!/bin/bash" > ${lv_xtract_script}
	echo "" >> ${lv_xtract_script}
	echo "lv_files=\`ls *.tgz\`"  >> ${lv_xtract_script}
	echo "for lv_fn in \$lv_files; do tar xzf \$lv_fn; done"  >> ${lv_xtract_script}
	chmod +x ${lv_xtract_script}
    fi

    eval '$SQPDSHA "netstat -aep 2>/dev/null" | sort'  > netstat.out
    eval '$SQPDSHA "uptime 2>/dev/null" | sort'  > uptime.out

    lv_pstacks=""
    if [[ $lv_all == 1 ]]; then
	# run sqcollectstacks and collect the output in the current directory.
	# (sqcollectstacks can be run separately - where it creates a new directory for the pstack logs)
	sqcollectstacks here   
	lv_pstacks="and pstacks"
    fi

    echo "Logs $lv_pstacks collected in $PWD"
    if [[ $lv_zip == 1 ]]; then
	echo "Creating a tar/zip file..."
	cd ..
	tar chzf ${lv_dir_name}.tgz ${lv_dir_name}
	echo "Created the .tgz file: $PWD/${lv_dir_name}.tgz"
	if [[ $lv_delete_directory_after_zip == 1 ]]; then
	    echo "Deleting the directory $PWD/${lv_dir_name}"
	    rm -rf ${lv_dir_name}
	fi
    fi
    popd > /dev/null
}

# Collects pstack of processes for the specified program. It launches
# driver processes per node (and does all of them in parallel).
function sqnpstack {
    prog=$1
    lv_dirname=$2
    lv_node_name=`uname -n`
    lv_fname=${lv_dirname}/${lv_node_name}_${prog}.pstacks
    w=`whoami`
    
#    pids=`$ssh ps -ef|grep -v 'ps -ef'|grep -vw $0|grep -w $w|grep -w $prog|grep -vw mpirun|grep -vw grep|awk '{ print $2 }'`
    pids=`pstat | grep -w $prog| grep -vw mpirun |grep -vw grep|awk '{ print $2 }'`
    for pid in $pids; do
        echo "====== PID: $pid" >> ${lv_fname}
        echo "ps -lfLp $pid" >> ${lv_fname}
        ps -lfLp $pid >> ${lv_fname}
        echo "--" >> ${lv_fname}
        echo pstack $pid >> ${lv_fname}
        pstack $pid >> ${lv_fname}
        echo "--" >> ${lv_fname}
    done

}

# Collects Monitor's inmemory log and dumps it to a file
function sqcollectmonmemlog {
    monpid_x=`ipcs -m | grep '0x1234' | grep $USER | head -1 | cut -c7-11`
    if [ $monpid_x ]; then
      monpid=`printf "%d" 0x$monpid_x`
      nodename=`uname -n`
      monmemlog $monpid nowait > $TRAF_LOG/monmem.${nodename}.${monpid}.log
    fi
}

# Collects pstacks of some programs (dp2, monitor, mxosrvr, arkesp, arkcmp, dtm)
#  the 'sqnpstacks' function
function sqcollectstacks {

    pushd . > /dev/null
    if [ -z $1 ]; then
	mkdir -p ~/logs/sqpstacks
	cd ~/logs/sqpstacks
	
	lv_curr_time=`date +%Y%m%d_%H%M`
	lv_dir_name=pstack.$lv_curr_time
    
	mkdir $lv_dir_name
	cd $lv_dir_name
    else
	mkdir -p sqpstacks
	cd sqpstacks
    fi
    
    setup_sqpdsh
    mkdir -p dp2 monitor mxosrvr tdm_arkcmp tdm_arkesp tm
    echo "Collecting dp2 pstacks"
    eval '$SQPDSHA "sqnpstack dp2 $PWD/dp2" 2>/dev/null &'
    echo "Collecting monitor pstacks"
    eval '$SQPDSHA "sqnpstack monitor $PWD/monitor" 2>/dev/null &' 
    echo "Collecting mxosrvr pstacks"
    eval '$SQPDSHA "sqnpstack mxosrvr $PWD/mxosrvr" 2>/dev/null &' 
    echo "Collecting tdm_arkcmp pstacks"
    eval '$SQPDSHA "sqnpstack tdm_arkcmp $PWD/tdm_arkcmp" 2>/dev/null &' 
    echo "Collecting tdm_arkesp pstacks"
    eval '$SQPDSHA "sqnpstack tdm_arkesp $PWD/tdm_arkesp" 2>/dev/null &' 
    echo "Collecting tm pstacks"
    eval '$SQPDSHA "sqnpstack tm $PWD/tm" 2>/dev/null &' 
    wait

    if [ -z $1 ]; then
	echo "Pstacks collected in $PWD"
    fi
    popd > /dev/null

}

# Get the value of the sb disconnect semaphore
function sqgetsem {

    setup_sqpdsh
    eval '$SQPDSHA "sqsemval" | sort -rnk5'
    
}

# Increase the value of the sb disconnect semaphore
function sqpostsem {

    declare -i lv_semval_threshold
    let lv_semval_threshold=10

    if [ $# != 0 ]; then
	if [ $1 '>' 40 ]; then
	    let lv_semval_threshold=40
	else
	    let lv_semval_threshold=$1
	fi
    fi

    echo "Incrementing the value of semaphore on nodes where its value is < $lv_semval_threshold"

    setup_sqpdsh
    eval '$SQPDSHA "sqsempost $1"'
    
}

function sqgdb_doit {

    OPTIND=1

    prog=$1
    my_script=$2
    host=$3
    lv_dir_name=$4
    
    if [ -e $SQ_PDSH ]; then
	ssh="ssh $host"
    fi
    
    pids=`$ssh ps -C $prog -o uid,pid |grep $UID | awk '{ print $2 }'`
    echo "PIDs: $pids"
    my_script_basename=`basename ${my_script}`
    
    declare -i lv_count
    let lv_count=0
    for pid in $pids; do
	echo "execute: $ssh gdb -p $pid --batch -x $my_script > $lv_dir_name/${host}_${pid}.${my_script_basename}.out "
	$ssh gdb -p $pid --batch -x $my_script > $lv_dir_name/${host}_${pid}.${my_script_basename}.out &
	let ++lv_count
	if [ $lv_count == 8 ]; then
	    let lv_count=0
	    wait
	fi
    done
    wait
}

function sqgdb_help {

    echo
    echo "Usage: sq_gdb {-p <program file> | -c <gdb command file> | -h}"
    echo
    echo "-p <program file> Program whose processes that gdb should attach to"
    echo "-c <gdb command file> File containing gdb commands to be executed"
    echo "-h Help "
    echo

}

function sq_gdb_main {

    declare -i lv_proc
    declare -i lv_script

    let lv_proc=0
    let lv_script=0

    # The line below (OPTIND=1) is needed to use the 'getopts' routine to work in a function
    OPTIND=1
    while getopts "p:c:h" lv_arg
      do
      case $lv_arg in 
	  p)
	      lv_proc=1
	      lv_proc_str=$OPTARG
	      ;;
	  c)
	      lv_script=1
	      lv_script_str=$OPTARG
	      if [[ ! -f $lv_script_str ]]; then
		  echo "gdb script $lv_script_str not found"
		  return 1
	      fi
	      ;;
	  h)
	      lv_help=1
	      sqgdb_help
	      return 0
              ;;
          *)
	      sqgdb_help;
	      return 0
              ;;
      esac
    done

    if ( [ $lv_proc == 0 ] || [ $lv_script == 0 ] ); then
	sqgdb_help
	return 0;
    fi

    OPTIND=1
    echo "Program: $lv_proc_str Gdb commmand file: $lv_script_str"
    w=`whoami`
    lv_curr_time=`date +%Y%m%d_%H%M`
    lv_dir_name=${PWD}/sq_gdb_${lv_proc_str}_${lv_curr_time}
    mkdir $lv_dir_name
    
    if [ -e $SQ_PDSH ]; then
	hosts=`$SQ_PDSH $MY_NODES hostname|awk '{ print $2 }'|sort`
    else
	hosts=`hostname`
    fi

    echo "Execute gdb commands on: $hosts"
    for host in $hosts; do
#	echo "sqgdb_doit $lv_proc_str $lv_script_str $host $lv_dir_name"
	sqgdb_doit ${lv_proc_str} ${lv_script_str} ${host} ${lv_dir_name} &
	pids="$pids $!"
    done

    echo "Data collection in progress (output files in the directory ${lv_dir_name} )"
    wait 
    
    echo
    echo "Done ... Output files in the directory ${lv_dir_name}"
    echo

}

function sqgdb_help {

    echo
    echo "Execute gdb (across all the nodes of the SQ environment) on the processes"
    echo "of the specified program with the gdb commands in the specified command file"
    echo ""
    echo "Usage: sq_gdb {-p <program file> | -c <gdb command file> | -h}"
    echo
    echo "-p <program file> Program whose processes that gdb should attach to"
    echo "-c <gdb command file> File containing gdb commands to be executed"
    echo "-h Help "
    echo
    echo "sq_gdb will output the results in a sub-directory under the current directory"
    echo "and print that location once its done."
    echo

}

#The following allows the above functions to be used from some other shell scripts spawned from this shell
export -f setup_my_nodes
export -f setup_sqpdsh
export -f cmaph
export -f cmapt
export -f ngrepmm
export -f ngrepmms
export -f cmapl
export -f cmaplc
export -f cmaplbc
export -f cmaplec
export -f cmaplbec

# for ESPs
export -f cmapls
export -f cmaplsc
export -f cmaplsbc
export -f cmaplsec
export -f cmaplsbec

export -f cmappc

export -f nprocpn
export -f nprocpn_us
export -f nesp
export -f ncmp
export -f nsqlci
export -f ndbm

export -f sqchkmpi
export -f sqsecheck

export -f cmapc
export -f pdsh_counter
export -f tnbprog
export -f tneprog
export -f espstats
export -f sqlcistats
export -f cmpstats
export -f mxosstats

export -f sqmd5b
export -f sqmd5l

export -f sqchkvm
export -f sqchksl
export -f sqchkopt

export -f LogHeader
export -f LogPstack
export -f collect_cstat
export -f collect_cmapt
export -f collect_sqchkvm
export -f sqsavelogs
export -f sqsavelogs_compress
export -f sqcollectlogs
export -f sqsave_linux_info
export -f sqcollectmetrics
export -f sqnpstack
export -f sqcollectmonmemlog
export -f sqcollectstacks
export -f sqgetsem
export -f sqpostsem
export -f sqgdb_doit
export -f sq_gdb_main

export -f chkReturnCodeExit
export -f chkReturnCode
export -f run_util

# A front end to sq_gdb_main (as sq_gdb_main is a function, this function executes sq_gdb_main in a fresh 
# bash context and that allows background tasks spawned by sq_gdb_main to be managed).
function sq_gdb {

    export MY_CURRENT_DIR=$PWD
    bash -c "cd $TRAF_HOME; . $TRAF_HOME/sqenv.sh; cd $MY_CURRENT_DIR ; sq_gdb_main $* ; "
    lv_retcode=$?
    unset MY_CURRENT_DIR
    return $lv_retcode

}

export -f sq_gdb

# functions to check space
function sqchkdsk {
    sediskstatus | grep -v UP
}
function dbspace {
    pdsh -a df -h | grep database | sort -rnk5
}
function tmpspace {
    pdsh -a df -h /tmp | grep -v Used | sort -rnk5
}

# functions to cd to a particular directory
function cds {
    cd $TRAF_HOME/sql/scripts
}
function cdw {
    cd $TRAF_HOME
}
function cdl {
    cd $TRAF_LOG
}
function cdb {
    cd $TRAF_HOME/export/bin${SQ_MBTYPE}
}
function cdi {
    cd $TRAF_HOME/export/lib${SQ_MBTYPE}
}
function cdt {
    cd $MPI_TMPDIR
}
function cdc {
    cd $TRAF_CONF
}
function cdj {
    cd $TRAF_HOME/../sql/src/main/java/org/trafodion/sql
}
function cdci {
    cd $TRAF_HOME/../conn/trafci/src/main/java/org/trafodion/ci
}
function cdt4 {
    cd $TRAF_HOME/../conn/jdbcT4/src/main/java/org/trafodion/jdbc/t4
}
# ls variants
function lst {
ls -lsrt $*
}
function ll {
ls -al $*
}

#### Convenience functions to run SeaBase regressions - Begin
function run_sb_regr {

    pushd . 
    mkdir -p $TRAF_HOME/rundir
    cd $TRAF_HOME/sql
    ln -sf $TRAF_HOME/../sql/regress regress
    ln -sf $TRAF_HOME/../sql/sqludr sqludr

    cd $TRAF_HOME/sql/regress/tools
    . ./setuplnxenv
    
    cd $scriptsdir
    tools/runallsb $*  
    popd
}

# runs a specific seabase SQL regression
# run it like: 
# run_sb_test core 001 019
function run_sb_test {

    pushd . 
    mkdir -p $TRAF_HOME/rundir
    cd $TRAF_HOME/sql
    ln -sf $TRAF_HOME/../sql/regress regress
    ln -sf $TRAF_HOME/../sql/sqludr sqludr

    cd $TRAF_HOME/sql/regress/tools
    . ./setuplnxenv
    
    if [ -d $scriptsdir/$1 ]; then
	cd $scriptsdir/$1
	mkdir -p $TRAF_HOME/rundir/$1
	shift
	../tools/runregr -sb $*  
    else
	echo "Test suite $1 does not exist"
    fi
    popd

}

function check_sb_regr {
    pushd . > /dev/null
    cd $TRAF_HOME/rundir
    find . -name runregr-sb.log | xargs grep FAIL
    popd > /dev/null
}

function status_sb_regr {
    pushd . > /dev/null
    cd $TRAF_HOME/rundir
    find . -name runregr-sb.log | xargs tail -n 1 
    popd > /dev/null
}

function remove_duplicates_in_path {
    echo "$1" | awk -F: '{for(i=1;i<=NF;i++) if(!($i in arr)){arr[$i];printf s$i;s=":"}}'
}

export -f run_sb_regr
export -f run_sb_test
export -f check_sb_regr
export -f status_sb_regr
#### Convenience functions to run SeaBase regressions - End

export -f sqchkdsk
export -f dbspace
export -f tmpspace

export -f cds
export -f cdw
export -f cdl
export -f cdb
export -f cdi
export -f cdt
export -f cdc

export -f lst
export -f ll

export PATH=$PATH:~/bin
