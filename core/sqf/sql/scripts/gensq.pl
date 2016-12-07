#!/usr/bin/perl
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
use sqnodes;
use POSIX;
use DBI;

# Process types.  Must match values defined by the monitor in msgdef.h.
my $ProcessType_Undefined = 0;
my $ProcessType_Generic   = 4;
my $ProcessType_SPX       = 10;
my $ProcessType_SSMP      = 11;
my $ProcessType_SMS       = 13;

my $gDebug = 0;

my $gv_mount_entry_count=0;

my $bVirtualNodes=0;
my $g_storage_engine_dir="";
my $g_mirror_storage_engine_dir="";

$gRoleEnumStorage     = "storage";
$gRoleEnumEdge        = "connection";
$gRoleEnumAggregation = "aggregation";

my $g_nextStorageNodeIndex = 0;
my $g_nextStorageNode = 0;
my $g_currentTSEIndex = 0;

my @g_storageNodes = ();
my @g_edgeNodes = ();
my @g_AggregationNodes = ();
my @g_ssdOverflow = ();
my @g_hddOverflow = ();


my $g_dbsize = "dbsize";

my $gdNumNodes=0;
my $gdNumPNodes=0;
my $gdZoneId=0;

my @g_nodelist = ();
my @gNodeIdToNameIndex = ();
my %gNodeNameToIdIndex;
my @gNodeIdToZoneIdIndex = ();

my @g_zonelist = ();
my %gZoneNameToIdIndex;

# This to help with getting the next AT name
my @ase_list = ();
my @g_alphabets = (A..Z);
my $g_char1 = 0;
my $g_char2 = 0;

my $g_ASE_TSE_Index = 1;
my $gNumASEProcess  = 0;

my $g_TMASE_TM_Index = 0;
my $gNumTMASEProcess = 0;

my $gNumATExtents    = 0;
my $gMinATExtents    = 0;
if ((! -e "/etc/hptc-release") && (! -e "/etc/cm-release")){
   $gMinATExtents    = 1550;  #NOT a CLUSTER
}
else{
   $gMinATExtents    = 15500; #CLUSTER
}
my $gNumATFiles      = 0;
my $gMinATFiles      = 10;
my $gTX_Capacity     = 0;
my $gBO_PER_TSE      = 1;
my $gMAX_TX_Capacity = 40;
# Must enable this code if _DISABLE_BEGINTRANS defined in the TSE
#my $gMAX_BeginTX_Disable = 80;
my $gMAX_BO_PER_TSE  = 3;

my $BOOL_BO_PER_TSE_SET = 0;

my $g_dRMID  = 1;

my $gdNumCpuCores = 1;

my $g_CCFormat = 2;

my $gbInitialLinesPrinted = 0;
my $gbOverflowLinesPrinted = 0;

my $gbLunmgrOn; # cleandb scripts must be generated differently if true

my $gShellStarted=0;

my $gEncSectionProcessed = 0;

my $gProxyNodePort = 0;

my $gFloatingExternalIp = "";
my $gFloatingNodeId = -1;
my $gFloatingFailoverNodeId = -1;


my $TRAF_HOME = $ENV{'TRAF_HOME'};
my $HOME = $ENV{'HOME'};
my $MPI_TMPDIR = $ENV{'MPI_TMPDIR'};
my $SQ_SEAMONSTER = $ENV{'SQ_SEAMONSTER'};
my $SQ_TRANS_SOCK = $ENV{'SQ_TRANS_SOCK'};
my $SQ_DTM_PERSISTENT_PROCESS = $ENV{'SQ_DTM_PERSISTENT_PROCESS'};
my $SQ_IDTMSRV = $ENV{'SQ_IDTMSRV'};

# define the error values that are being returned
my $CONFIG_ERROR = 5;
my $STORAGE_ERROR = 40;
my $BACKUP_STORAGE_ERROR = 50;
my $BACKUP_NODEID_ERROR = 60;
my $BDR_ERROR = 70;


# Database handle
my $DBH = 0;

# instance type
my $instanceType = "";

sub getTime {
    return strftime("%a %b %d %H:%M:%S %Y\n", localtime(time));
}

sub printScript {
    ($dWhich, @rest) = @_;

    if ($dWhich <= 1) {
	print SQS @rest;
    }

    if ($dWhich >= 1) {
	print SQW @rest;
    }
}

sub printRMSScript {
    ($dWhich, @rest) = @_;
    if ($dWhich == 1) {
       print SSMP @rest;
       print SSCP @rest;
    }
    if ($dWhich == 2) {
       print SSMP @rest;
    }
    if ($dWhich == 3) {
       print SSCP @rest;
    }
    if ($dWhich == 0) {
       print RMS @rest;
    }
}

sub printRMSStopScript{
    ($dWhich, @rest) = @_;
    if ($dWhich == 1) {
       print SSMPS @rest;
       print SSCPS @rest;
    }
    if ($dWhich == 2) {
       print SSMPS @rest;
    }
    if ($dWhich == 3) {
       print SSCPS @rest;
    }
    if ($dWhich == 0) {
       print RMSS @rest;
    }
}

sub printRMSCheckScript{
    ($dWhich, @rest) = @_;
    if ($dWhich == 1) {
       print RMSC @rest;
    }
}

sub printTime {
    printScript(1, "# Trafodion Startup script generated @ ",getTime(),"\n");
}

sub validate_config_script {
}

sub printInitialLines {

    # So we don't re-print the initial lines
    if ($gbInitialLinesPrinted) {
	return;
    }

    printScript(1, "#!/bin/sh \n");
    printTime;

    $smenv = "$ENV{'SQETC_DIR'}/seamonster.env";
    if ( -e $smenv ) {
      print "\nThe SeaMonster environment variable file $smenv exists.\n";
      print "The file will not be re-generated.\n\n";
    } else {
      print "\nGenerating SeaMonster environment variable file: $smenv\n\n";
      #Create SeaMonster environment variable file
      open (ETC,">>$smenv")
          or die("unable to open $smenv");
      if ($bVirtualNodes == 1) {
          print ETC "SM_VIRTUALNODE=1\n";
      }
      if (!$ENV{'SHARED_HARDWARE'} || $ENV{SHARED_HARDWARE} eq 'YES') {
          print ETC "SM_PIPEDEPTH=6\n";
          print ETC "SM_LOWATER=3\n";
          print ETC "SM_BUFFSIZE=102400\n";
          print ETC "SM_THRESHOLD_NBYTES=51200\n";
      }
      close(ETC);
    }

    $msenv = "$ENV{'SQETC_DIR'}/ms.env";
    $mirroringoff_string = "TSE_MIRRORING_OFF=1\n";
    $acttenable_string = "TSE_ACTT_ENABLE_THRESHOLDS=0\n";
    $acttdiskio_string = "TSE_ACTT_DISKIO_THRESHOLD=20000000\n";
    $acttcachec_string = "TSE_ACTT_CACHEC_THRESHOLD=20000000\n";
    $acttaccessed_string = "TSE_ACTT_ACCESSED_THRESHOLD=20000000\n";
    $acttinterval_string = "TSE_ACTT_THRESHOLD_INTERVAL=1\n";
    $acttpublishevent_string = "TSE_ACTT_PUBLISH_EVENT=0\n";
    $acttpublishlog_string = "TSE_ACTT_PUBLISH_LOG=0\n";
    $acttpublishstd_string = "TSE_ACTT_PUBLISH_STDOUT=0\n";
    $fcbenable_string = "TSE_FCB_ENABLE_THRESHOLDS=0\n";
    $fcbdiskio_string = "TSE_FCB_DISKIO_THRESHOLD=20000000\n";
    $fcbcachec_string = "TSE_FCB_CACHEC_THRESHOLD=20000000\n";
    $fcbaccessed_string = "TSE_FCB_ACCESSED_THRESHOLD=20000000\n";
    $fcbinterval_string = "TSE_FCB_THRESHOLD_INTERVAL=1\n";
    $fcbpublishevent_string = "TSE_FCB_PUBLISH_EVENT=0\n";
    $fcbpublishlog_string = "TSE_FCB_PUBLISH_LOG=0\n";
    $fcbpublishstd_string = "TSE_FCB_PUBLISH_STDOUT=0\n";

    open (ETC,">>$msenv")
	or die("unable to open $msenv");

    if ($SQ_TRANS_SOCK == 1) {
	print ETC "SQ_TRANS_SOCK=1\n";
    }
    else {
	print ETC "SQ_TRANS_SOCK=0\n";
    }

    if ($bVirtualNodes == 1) {
        $virtualnode_string = "SQ_VIRTUAL_NODES=$gdNumNodes\n";
        $virtualnid_string = "SQ_VIRTUAL_NID=0\n";
        printScript(1, "export $virtualnode_string");
        printScript(1, "export $virtualnid_string");

        print ETC "$virtualnode_string";
        print ETC "$virtualnid_string";
           # Allow specific mirroring ON override for virtual node
        print ETC "$mirroringoff_string" if (!$ENV{'TSE_MIRRORING'});
        print ETC "MS_STREAMS_MIN=20000\n";
        print ETC "MS_STREAMS_MAX=20000\n";
        print ETC "$acttenable_string";
        print ETC "$acttdiskio_string";
        print ETC "$acttcachec_string";
        print ETC "$acttaccessed_string";
        print ETC "$acttinterval_string";
        print ETC "$acttpublishevent_string";
        print ETC "$acttpublishlog_string";
        print ETC "$acttpublishstd_string";
        print ETC "$fcbenable_string";
        print ETC "$fcbdiskio_string";
        print ETC "$fcbcachec_string";
        print ETC "$fcbaccessed_string";
        print ETC "$fcbinterval_string";
        print ETC "$fcbpublishevent_string";
        print ETC "$fcbpublishlog_string";
        print ETC "$fcbpublishstd_string";

        # As of v1.12, sqgen doesn't recreate ms.env if it already exists,
        # so users will see issues switching between mirroring ON and mirroring
        # OFF, because the TSE treats mirroring on as default.  The variable in
        # ms.env is an override to turn mirroring OFF, we will remove that if
        # the user intends to turn mirroring on now
        if (length($ENV{'TSE_MIRRORING'}) && $ENV{'TSE_MIRRORING'} == 1)
        {
            @msenv_contents = <ETC>;

            # This holds contents of ms.env, excluding all TSE_MIRRORING_OFF=1
            @grep_res = grep(!/$mirroringoff_string/, @msenv_contents);
            $grep_res_num = grep(/$mirroringoff_string/, @msenv_contents);

            # Only rewrite it out if we found occurrences
            if ($grep_res_num > 0) {
                print ETC join("", @grep_res);
            }
        }
    }
    # Cluster
    else {
        print ETC "$mirroringoff_string"
            if (length($ENV{'TSE_MIRRORING'}) && $ENV{'TSE_MIRRORING'} == 0);
        print ETC "MS_STREAMS_MIN=20000\n";
        print ETC "MS_STREAMS_MAX=20000\n";
        print ETC "$acttenable_string";
        print ETC "$acttdiskio_string";
        print ETC "$acttcachec_string";
        print ETC "$acttaccessed_string";
        print ETC "$acttinterval_string";
        print ETC "$acttpublishevent_string";
        print ETC "$acttpublishlog_string";
        print ETC "$acttpublishstd_string";
        print ETC "$fcbenable_string";
        print ETC "$fcbdiskio_string";
        print ETC "$fcbcachec_string";
        print ETC "$fcbaccessed_string";
        print ETC "$fcbinterval_string";
        print ETC "$fcbpublishevent_string";
        print ETC "$fcbpublishlog_string";
        print ETC "$fcbpublishstd_string";
        $hugePages=`cat /proc/sys/vm/nr_hugepages`;
        if ($hugePages != 0) {
           if ($ENV{SHARED_HARDWARE} eq 'YES') {
              print ETC "SQ_RMS_ENABLE_HUGEPAGES=0\n"; }
           else {
              print ETC "SQ_RMS_ENABLE_HUGEPAGES=1\n"; }
        }

        else {
            print ETC "SQ_RMS_ENABLE_HUGEPAGES=0\n";
        }
    }

    print ETC "CLASSPATH=$ENV{'CLASSPATH'}:\n";
    close (ETC);

    if ($gbLunmgrOn == 1) {
        printScript(1, "\n\$SQ_PDSH \$MY_NODES \"rm -rf \$TRAF_HOME/logs/lunmgr.log.LOCKDIR\"\n");
    }

    printScript(1, "\nshell <<eof \n");
    $gShellStarted=1;

    printScript(1, "\n");

    printScript(1, "! Start the monitor processes across the cluster\n");

    printScript(0, "startup\n");
    printScript(2, "warmstart\n");

    genSQShellExit();

    printScript(1, "\nsqcheckmon\n");
    printScript(1, "let lv_checkmon_ret=\$\?\n");
    printScript(1, "if [ \$lv_checkmon_ret '==' 0 ]; then\n");
    printScript(1, "   echo \"Continuing with the Startup...\"\n");
    printScript(1, "   echo\n");
    printScript(1, "else\n");
    printScript(1, "   echo \"Aborting startup.\"\n");
    printScript(1, "   more $TRAF_HOME/logs/sqcheckmon.log\n");
    printScript(1, "   exit 1\n");
    printScript(1, "fi\n");


    genSQShellStart();

    if ($bVirtualNodes == 0) {
	printScript(1, "\nset CLUSTERNAME=\$CLUSTERNAME\n");
    }
    printScript(1, "\nset SQ_MBTYPE=$ENV{'SQ_MBTYPE'}\n");
    printScript(0, "\nset MY_NODES=\$MY_NODES\n");

    addDbClusterData( "SQ_MBTYPE", $ENV{'SQ_MBTYPE'});
    addDbClusterData( "TRAF_HOME", "$TRAF_HOME"); # comes out null

    $gbInitialLinesPrinted = 1;
}

sub printOverflowLines {
    if($gbOverflowLinesPrinted) {
	return;
    }

    $msenv = "$ENV{'SQETC_DIR'}/ms.env";

    open (ETC,">>$msenv")
        or die("unable to open $msenv");

    if(@g_ssdOverflow) {
	$ssdDir = join(':',@g_ssdOverflow);
	print ETC "STFS_SSD_LOCATION=$ssdDir\n";
    }

    if(@g_hddOverflow) {
	$hddDir = join(':',@g_hddOverflow);
	print ETC "STFS_HDD_LOCATION=$hddDir\n";
    }
    close(ETC);

    $gbOverflowLinesPrinted = 1;
}

sub printScriptEndLines {

    printScript(1, "\n");
    printScript(1, "exit 0\n");
}

sub genSQShellExit {

    if ($gShellStarted == 1) {
	printScript(1, "\n");
	printScript(1, "exit\n");
	printScript(1, "eof\n");

	$gShellStarted = 0;
    }
}

sub genSQShellStart {

    if ($gShellStarted == 0) {
	printScript(1, "\n");
	printScript(1, "sqshell -a <<eof\n");

	$gShellStarted = 1;
    }
}

sub genRegWait {

    my $l_registry  = @_[0];
    my $l_svcname   = @_[1];
    my $l_duration  = @_[2];
    my $l_iteration = @_[3];

    printScript(1, "\n");
    printScript(1, "sqregck -r $l_registry -d $l_duration -i $l_iteration\n");
    printScript(1, "sqr_stat=\$\?\n");
    printScript(1, "if \[\[ \$sqr_stat == 0 \]\]; then\n");
    printScript(1, "\techo \"The $l_svcname is Not Ready yet. Stopping further startup (if any).\"\n");
    printScript(1, "\texit 1\n");
    printScript(1, "else\n");
    printScript(1, "\techo \"The $l_svcname is Ready.\"\n");
    printScript(1, "fi\n");
}

sub genComponentWait {

    my $l_compname  = @_[0];
    my $l_duration  = @_[1];
    my $l_iteration = @_[2];

    printScript(1, "\n");
    printScript(1, "sqcheck -c $l_compname -d $l_duration -i $l_iteration -r\n");
    printScript(1, "sq_stat_$l_compname=\$\?\n");
    printScript(1, "if \(\[ \$sq_stat_$l_compname '!=' 0 \] && [ \$sq_stat_$l_compname '!=' 1 \]\); then\n");
    printScript(1, "\techo \"The $l_compname process(es) are Not Ready yet. Stopping further startup (if any).\"\n");
    printScript(1, "\texit 1\n");
    printScript(1, "else\n");
    printScript(1, "\techo \"The $l_compname process(es) are Ready.\"\n");
    printScript(1, "fi\n");
}



sub genIdTmSrv {
    if ($SQ_IDTMSRV > 0) {
        my $l_pn = "";
        for ($i=0; $i < $gdNumNodes; $i++) {
            $l_pn = $l_pn . $i;
            if ($i + 1 < $gdNumNodes) {
                $l_pn = $l_pn . ",";
            }
        }
        printScript(1, "\n");
        printScript(1, "\n! Start TSID\n");
        for ($i=0; $i < $SQ_IDTMSRV; $i++) {
            printScript(1, "set {process \\\$TSID$i } PERSIST_RETRIES=2,30\n");
            addDbProcData('$TSID'."$i", "PERSIST_RETRIES", "2,30");
            printScript(1, "set {process \\\$TSID$i } PERSIST_ZONES=$l_pn\n");
            addDbProcData('$TSID'."$i", "PERSIST_ZONES", "$l_pn");
            printScript(1, "exec {nowait, name \\\$TSID$i, nid 0, out stdout_idtmsrv_$i} idtmsrv\n");
        }
        printScript(1, "delay 1\n");
    }
}

sub genDTM {

    genSQShellStart();
    printScript(1, "\n! Start DTM\n");
    printScript(1, "set DTM_RUN_MODE=2\n");
    printScript(1, "set SQ_AUDITSVC_READY=1\n");
    printScript(1, "set DTM_TLOG_PER_TM=1\n");
    addDbClusterData("DTM_RUN_MODE", "2");
    addDbClusterData("SQ_AUDITSVC_READY", "1");
    addDbClusterData("DTM_TLOG_PER_TM", "1");
    genSQShellExit();

    for ($i=0; $i < $gdNumNodes; $i++) {
        printScript(1, "\ntmp_node_status=`mktemp -t`\n");
        printScript(1, "sqshell -c zone nid $i > \$tmp_node_status\n");
        printScript(1, "let node_up_value=`grep 'Up' \$tmp_node_status | wc -l`\n");
        printScript(1, "if [[ \$node_up_value == 1 ]]; then\n");

        genSQShellStart();
         if ($SQ_DTM_PERSISTENT_PROCESS == 1) {
            printScript(1, "set {process \\\$TM$i } PERSIST_RETRIES=2,30\n");
            addDbProcData('$TM'."$i", "PERSIST_RETRIES", "2,30");
            printScript(1, "set {process \\\$TM$i } PERSIST_ZONES=$i\n");
            addDbProcData('$TM'."$i", "PERSIST_ZONES", "$i");
        }
        printScript(1, "set {process \\\$TM$i} TMASE=TLOG$i\n");
        addDbProcData('$TM'."$i", "TMASE", "TLOG$i");
        printScript(1, "exec {type dtm, nowait, name \\\$TM$i, nid $i, out stdout_dtm_$i} tm");
        if ($i == 0 || $i == ($gdNumNodes-1)) {
            printScript(1, "\ndelay 5");
        }
        genSQShellExit();

        printScript(1, "\nfi\n");
        printScript(1, "\nrm -f \$tmp_node_status\n");
    }

    genComponentWait("dtm", 10, 60);
    genSQShellStart();

    printScript(1, "! Generate DTM Event 1\n");
    printScript(1, "event {DTM} 1\n");

    genSQShellExit();
#    genRegWait("SQ_TXNSVC_READY", "Transaction Service", 5, 40);
    printScript(1, "\n");
    printScript(1, "echo \"Checking whether the transaction service is ready.\"\n");
    printScript(1, "sqr_stat=0\n");
    printScript(1, "while \[\[ \$sqr_stat == 0 \]\];\n");
    printScript(1, "do\n");
    printScript(1, "sqregck -r SQ_TXNSVC_READY -d 5 -i -1\n");
    printScript(1, "sqr_stat=\$\?\n");
    printScript(1, "done\n");
    printScript(1, "echo \"The Transaction Service is Ready.\"\n");
}
sub genLOBConfig {

    # Generate sqconfig.db config for LOB.
    # This allows the process startup daemon (pstartd)
    # to start it up after a node failure.
    for ($i=0; $i < $gdNumNodes; $i++) {
	my $l_progname="mxlobsrvr";
	my $l_procargs="";
	my $l_procname="\$ZLOBSRV$i";
	my $l_procname_config = sprintf('$ZLOBSRV%d', $i);
	my $l_stdout="stdout_\$ZLOBSRV_$i";
	addDbProcData($l_procname_config, "PERSIST_RETRIES", "10,60");
	addDbProcData($l_procname_config, "PERSIST_ZONES", $i);
	addDbPersistProc($l_procname_config, $i, 1);
	addDbProcDef( $ProcessType_Generic, $l_procname_config, $i, $l_progname, $l_stdout, $l_procargs);
    }

}

sub genSSMPCommand {

    my $l_nid                  = @_[0];
    my $l_process_name_prefix  = @_[1];
    my $l_program_name         = @_[2];
    my $l_retries              = @_[3];

    my $l_string =  sprintf("\tset {process \\\$%s%03d } PERSIST_RETRIES=$l_retries,60\n",
			    $l_process_name_prefix,
			    $l_nid);
    printRMSScript(2, $l_string);

    my $l_stopString = sprintf("kill {abort} \\\$%s%03d\n", $l_process_name_prefix, $l_nid);
    printRMSStopScript(2, $l_stopString);

    my $l_string =  sprintf("\tset {process \\\$%s%03d } PERSIST_ZONES=$l_nid,$l_nid \n",
			    $l_process_name_prefix,
			    $l_nid);
    printRMSScript(2, $l_string);

    my $l_string =  sprintf("\texec {type ssmp, nowait, nid $l_nid, name \\\$%s%03d, out stdout_%s%03d } %s\n",
			    $l_process_name_prefix,
			    $l_nid,
			    $l_process_name_prefix,
			    $l_nid,
			    $l_program_name
			    );
    printRMSScript(2, $l_string);

    my $l_procname = sprintf('$%s%03d', $l_process_name_prefix, $l_nid);
    my $l_stdout = sprintf('stdout_%s%03d', $l_process_name_prefix, $l_nid);
    addDbPersistProc("$l_procname", $l_nid, 1);
    addDbProcDef( $ProcessType_SSMP, $l_procname, $l_nid, $l_program_name,
                  $l_stdout, "" );
}

sub genSSCPCommand {

    my $l_nid                  = @_[0];
    my $l_process_name_prefix  = @_[1];
    my $l_program_name         = @_[2];
    my $l_retries              = @_[3];

    my $l_string =  sprintf("\tset {process \\\$%s%03d } PERSIST_RETRIES=$l_retries,60\n",
			    $l_process_name_prefix,
			    $l_nid);
    printRMSScript(3, $l_string);

    my $l_stopString = sprintf("kill {abort} \\\$%s%03d\n", $l_process_name_prefix, $l_nid);
    printRMSStopScript(3, $l_stopString);

    my $l_string =  sprintf("\tset {process \\\$%s%03d } PERSIST_ZONES=$l_nid,$l_nid \n",
			    $l_process_name_prefix,
			    $l_nid);
    printRMSScript(3, $l_string);

    my $l_string =  sprintf("\texec {nowait, nid $l_nid, name \\\$%s%03d, out stdout_%s%03d } %s\n",
			    $l_process_name_prefix,
			    $l_nid,
			    $l_process_name_prefix,
			    $l_nid,
			    $l_program_name
			    );
    printRMSScript(3, $l_string);

    my $l_procname = sprintf('$%s%03d', $l_process_name_prefix, $l_nid);
    my $l_stdout = sprintf('stdout_%s%03d', $l_process_name_prefix, $l_nid);
    addDbPersistProc("$l_procname", $l_nid, 1);
    addDbProcDef( $ProcessType_Generic, $l_procname, $l_nid, $l_program_name,
                  $l_stdout, "" );
}

sub generateRMS {

    $RMS_Retries = 10;
    genSQShellExit();

    #Print out run RMS script
    printScript(1, "\necho \"SQ_START_RMS: \" \$SQ_START_RMS \n");
    printScript(1, "if [[ \$SQ_START_RMS == \"1\" ]]; then\n");
    printScript(1, "echo \"Starting RMS\"\n");
    
    #generate rmsstart and rmsstop scripts
    printRMSScript(0, "#!/bin/sh\n");
    printRMSScript(0, "# SQ config/utility file generated @ ",getTime(),"\n");
    printRMSScript(0, "\n# Start the RMS processes\n");
    printRMSScript(0, "sscpstart\n");
    printRMSScript(0, "ssmpstart\n");

    printRMSStopScript(0, "#!/bin/sh\n");
    printRMSStopScript(0, "# SQ config/utility file generated @ ",getTime(),"\n");
    printRMSStopScript(0, "\n# Stop the RMS processes\n");
    printRMSStopScript(0, "ssmpstop\n");
    printRMSStopScript(0, "sscpstop\n");

    #generate ssmpstart, ssmpstop, sscpstart, sscpstop scripts
    printRMSScript(1, "#!/bin/sh\n");
    printRMSScript(1, "# SQ config/utility file generated @ ",getTime(),"\n");
    printRMSScript(2, "\n# Start the SSMP processes\n");
    printRMSScript(3, "\n# Start the SSCP processes\n");

    printRMSStopScript(1, "#!/bin/sh\n");
    printRMSStopScript(1, "# SQ config/utility file generated @ ",getTime(),"\n");
    printRMSStopScript(1, "sqshell -a << eof\n"); 
    printRMSStopScript(2, "\n!Stop the SSMP processes\n");
    printRMSStopScript(3, "\n!Stop the SSCP processes\n");

    printRMSCheckScript(1, "-- SQ config/utility file generated @ ",getTime(),"\n");
    printRMSCheckScript(1, "prepare rms_check from select current_timestamp, \n");
    printRMSCheckScript(1, "cast('Node' as varchar(5)), \n");
    printRMSCheckScript(1, "cast(tokenstr('nodeId:', variable_info) as varchar(3)) node, \n");
    printRMSCheckScript(1, "cast(tokenstr('Status:', variable_info) as varchar(10)) status \n");
    printRMSCheckScript(1, "from table(statistics(null, ?));\n");

    for ($i=0; $i < $gdNumNodes; $i++) {

        my $l_string =  sprintf("execute rms_check using 'RMS_CHECK=%d' ;\n", $i);
        printRMSCheckScript(1, $l_string);
        my $l_string =  sprintf("\ntmp_node_status=`mktemp -t`\n");
        printRMSScript(1, $l_string);
        my $l_string =  sprintf("sqshell -c zone nid %d > \$tmp_node_status\n", $i);
        printRMSScript(1, $l_string);
        my $l_string =  sprintf("let node_up_value=`grep 'Up' \$tmp_node_status | wc -l`\n");
        printRMSScript(1, $l_string);
        my $l_string =  sprintf("if [[ \$node_up_value == 1 ]]; then\n");
        printRMSScript(1, $l_string);

        printRMSScript(1, "\tsqshell -a << eof\n");
        genSSCPCommand($i, "ZSC", "mxsscp", $RMS_Retries);
        genSSMPCommand($i, "ZSM", "mxssmp", $RMS_Retries);
        printRMSScript(1, "\texit\neof\n");

        my $l_string =  sprintf("fi\n");
        printRMSScript(1, $l_string);
        my $l_string =  sprintf("rm -f \$tmp_node_status\n");
        printRMSScript(1, $l_string);
    }

    printRMSStopScript(1, "delay 1\n");
    printRMSStopScript(1, "exit\neof\n");
    printRMSStopScript(1, "echo RMS Processes Stopped\n");
   
    printRMSScript(1, "\n");

    printScript(1, "rmsstart\n");

    #printScript(1, "\n! delay 5\n");
	
    printScript(1, "\necho \"Started RMS\"\n");
    printScript(1, "fi\n");
}

sub printSQShellCommand {
    printScript(1, substr($_,1));
}


sub processNodes {
    my $bNodeSpecified = 0;

    while (<SRC>) {
        next if (/^#/);
	if (/^_virtualnodes/) {
	    @words=split(' ',$_);
	    $gdNumNodes=@words[1];
	    $bVirtualNodes=1;
	    my $l_dNodeIndex = 0;
	    for ($l_dNodeIndex = 0; $l_dNodeIndex < $gdNumNodes; $l_dNodeIndex++) {
		print SQC "$l_dNodeIndex:$l_dNodeIndex:";
                print SQC "$g_HostName:";
		print SQC "0:0:";
		print SQC "$gRoleEnumEdge,$gRoleEnumAggregation,$gRoleEnumStorage\n";
		$gNodeIdToZoneIdIndex[$l_dNodeIndex] = $l_dNodeIndex;

		push(@g_EdgeNodes, $l_dNodeIndex);
	    }
#           open (SHELLENV, ">shell.env")
#               or die ("unable to open shell.env");
#
#           print SHELLENV "MON_TRACE_ENABLE=1 \n";
#           print SHELLENV "MON_TRACE_EVLOG_MSG=1 \n";
#           print SHELLENV "MON_TRACE_INIT=1 \n";
#           print SHELLENV "MON_TRACE_REQUEST=1 \n";
#           print SHELLENV "MON_TRACE_PROCESS=1 \n";
#           close (SHELLENV);
#
	}
	elsif (/^end node/) {

	    # Just for the time being - this should be an error
	    if (($bNodeSpecified == 0) &&
		($bVirtualNodes == 0)) {
		$gdNumNodes = 1;
	    }

            if ($bVirtualNodes == 0)
            {
                if (sqnodes::validateConfig() == 0)
                {	  # Valid configuration, generate cluster.conf
                    $gdNumNodes = sqnodes::numNodes();

                    sqnodes::genConfig( *SQC );
                }

		my $lv_numEdgeNodes = sqnodes::getNumberOfConnNodes();
		my $lv_node_index = 0;
		for ($i=1; $i <= $lv_numEdgeNodes; $i++) {
		    $lv_node_index = sqnodes::getConnNode($i);
		    push(@g_EdgeNodes, $lv_node_index);
		}

		for ($i=0; $i < $gdNumNodes; $i++) {
		    push(@g_BackupTSENode, $i);
		}

            }

	    return;
	}
	else {
		if (sqnodes::parseStmt() == 0) {
		    $bNodeSpecified = 1;
		    $gdZoneId++;
		}
		else {
		    print "   Error: not a valid node configuration statement.\n";
		    print "Exiting without generating cluster.conf due to errors.\n";
		    exit 1;
		}

	}
    }
}

## Process the enclosures section, which lumps node hostnames together based
## on whether or not they are in the same physical C7000 chassis.
sub processEnclosures
{
    while (<SRC>)
    {
        next if (/^#/);
        next if ($bVirtualNodes); # Just read through this section and return
        return if (/^end enclosure/);

        chomp($_);

        sqnodes::parseStmt;
    }
}

sub printNodeIdToName {

    if (!$gDebug) {
	return;
    }

    my $i = 0;

    print "Number of nodes: ", $#gNodeIdToNameIndex + 1, "\n";
    print "gdNumNodes = $gdNumNodes\n";

    for ($i = 0; $i <= $#gNodeIdToNameIndex; $i++) {
	print "gNodeIdToNameIndex[$i]=", $gNodeIdToNameIndex[$i], "\n";
    }
}

sub printZoneList {

    if (!$gDebug) {
	return;
    }

    my $i = 0;

    print "Number of nodes: ", $#g_zonelist + 1, "\n";
    print "Current Zone ID = $gdZoneId\n";

    for ($i = 0; $i <= $#g_zonelist; $i++) {
	print "g_zonelist[$i]=", $g_zonelist[$i], "\n";
    }
}

sub processRoleNodeMap {
    my $bNodeSpecified = 0;

    while (<SRC>) {
	if (/^storage/) {
	    if ($bVirtualNodes == 1) {
		@g_storageNodes=split(' ',$_);
	    }
#	    print "Edge Node IDs       : @g_EdgeNodes .. Total: ", $#g_EdgeNodes + 1, "\n";
#	    print "Storage Node IDs    : @g_storageNodes .. Total: ", $#g_storageNodes + 1, "\n";
#	    print "Aggregation Node IDs: @g_AggregationNodes .. Total: ", $#g_AggregationNodes + 1, "\n";
	}
	elsif (/^edge/) {
	}
	elsif (/^end role_node_map/) {
	    return;
	}
    }
}

sub processOverflow {
    while (<SRC>) {
	if(/^ssd/) {
	    @ssdLine = split(' ',$_);
	    if(@ssdLine[1]) {
	        push(@g_ssdOverflow, @ssdLine[1]);
	    }
	}
	elsif(/^hdd/) {
	    @hddLine = split(' ',$_);
	    if(@hddLine[1]) {
		push(@g_hddOverflow, @hddLine[1]);
	    }
	}
	elsif(/^end overflow/) {
	    return;
	}
    }
}

sub processFloatingIp {
    while (<SRC>) {
        if (/^process/) {
           @this_line = split(/;/, $_);
	   if($#this_line >= 2) {
             @external_ip = split(/=/,@this_line[2]);
#            print "external_ip @external_ip\n";
             if (($#external_ip >= 1) && (@external_ip[0] eq "external-ip")) {
                $gFloatingExternalIp = @external_ip[1];
                $gFloatingExternalIp =~ s/\s+$//; # remove trailing spaces, including new-line characters
#                print "Floating External IP  $gFloatingExternalIp\n";
             }
           }
        }
	elsif(/^floating_ip_node_id/) {
	    @this_line = split(' ',$_);
	    if($#this_line > 0) {
		$gFloatingNodeId=@this_line[1];
		# Validate the node id
		if (($gFloatingNodeId < 0) || ($gFloatingNodeId >= $gdNumNodes)) {
		    print "Error: Invalid Floating IP Node Id provided. Please check your config file.\n";
		    print "Exiting..\n";
		    exit $BDR_ERROR;
		}
                $lv_bEdgeNodeFound = 0;
		for ($lv_i = 0; $lv_i < $#g_EdgeNodes + 1 ; $lv_i++) {
		    if (@g_EdgeNodes[$lv_i] == $gFloatingNodeId) {
			$lv_bEdgeNodeFound = 1;
#			print "$lv_i : @g_EdgeNodes[$lv_i] \n";
			break;
		    }
		}
		if ($lv_bEdgeNodeFound == 0) {
		    print "Error: Floating IP Node Id : $gFloatingNodeId is NOT an edge node. Please check your config file.\n";
		    print "Exiting..\n";
		    exit $BDR_ERROR;
		}
	    }
	}
	elsif(/^floating_ip_failover_node_id/) {
	    @this_line = split(' ',$_);
	    if($#this_line > 0) {
		$gFloatingFailoverNodeId=@this_line[1];
		# print "Floating IP Failover Node Id = $gFloatingFailoverNodeId \n";
		# Validate the node id
		if (($gFloatingFailoverNodeId < 0) || ($gFloatingFailoverNodeId >= $gdNumNodes)) {
		    print "Error: Invalid Floating IP Failover Node Id provided. Please check your config file.\n";
		    print "Exiting..\n";
		    exit $BDR_ERROR;
		}
                $lv_bEdgeNodeFound = 0;
		for ($lv_i = 0; $lv_i < $#g_EdgeNodes + 1 ; $lv_i++) {
		    if (@g_EdgeNodes[$lv_i] == $gFloatingFailoverNodeId) {
			$lv_bEdgeNodeFound = 1;
#			print "$lv_i : @g_EdgeNodes[$lv_i] \n";
			break;
	            }
		}
		if ($lv_bEdgeNodeFound == 0) {
		    print "Error: Floating IP  Failover Node Id : $gFloatingFailoverNodeId is NOT an edge node. Please check your config file.\n";
		    print "Exiting..\n";
		    exit $BDR_ERROR;
                }
            }
        }
        elsif(/^end floating_ip/) {
#           printf "Floating Node Id : $gFloatingNodeId \n";
           if ($gFloatingNodeId == -1) {
               print "Error: floating_ip_node_id not provided. Please check your config file.\n";
               print "Exiting..\n";
               exit $BDR_ERROR;
           }
           if ($gFloatingExternalIp eq "") {
	       print "Error: bdr_ip_address is not provided, Please check your config file.\n";
	       print "Exiting..\n";
               exit $BDR_ERROR;
           }
           return;
	}
    }
}

sub processInstanceType {
    while( <SRC> ) {
        if(/^type/) {
            chomp($_);
            @this_line = split(' ', $_);
            if($this_line[1]) {
            $instanceType = $this_line[1];
            }
        }
        elsif(/^end instance/) {
            if($instanceType ne 'user' && $instanceType ne 'management') {
            $instanceType = "";
            }
            return;
        }
    }
}

sub getEdgeNode1 {
   if ($bVirtualNodes == 1) {
      return 0;
   }
   else
   {
       sqnodes::getConnNode(1);
   }
}

sub getEdgeNode2 {
   if ($bVirtualNodes == 1) {
      if ($gdNumNodes > 1) {
         return 1;
      }
      else {
         return 0;
      }
   }
   else
   {
       sqnodes::getConnNode(2);
   }
}

sub printInitLinesAuxFiles {

    my $file_ptr  = @_[0];

    print $file_ptr "#!/bin/sh\n";
    print $file_ptr "# SQ config/utility file generated @ ", getTime(), "\n";
}

sub openFiles {

    open (SRC,"<$infile")
	or die("unable to open $infile");

    open (SQS,">$coldscriptFileName")
	or die("unable to open $coldscriptFileName");

    open (SQW,">$warmscriptFileName")
	or die("unable to open $warmscriptFileName");

    open (SQC,">$clusterconfFileName")
	or die("unable to open $clusterconfFileName");

    open (RMS,">$startRMS")
	or die("unable to open $startRMS");

    open (RMSS,">$stopRMS")
	or die("unable to open $stopRMS");

    open (RMSC,">$checkRMS")
	or die("unable to open $checkRMS");

    open (SSMP,">$startSSMP")
	or die("unable to open $startSSMP");

    open (SSMPS,">$stopSSMP")
	or die("unable to open $stopSSMP");

    open (SSCP,">$startSSCP")
	or die("unable to open $startSSCP");

    open (SSCPS,">$stopSSCP")
	or die("unable to open $stopSSCP");

#    my $dbargs = {AutoCommit => 1,
#                  PrintError => 1};
    my $dbargs = {AutoCommit => 1,
                  RaiseError => 1,
                  PrintError => 0,
                  ShowErrorStatement => 1};
    $DBH = DBI->connect("dbi:SQLite:dbname=sqconfig.db","","",$dbargs);
#   Disable database synchronization (fsync) because it slows down writes
#   too much.
    $DBH->do("PRAGMA synchronous = OFF");
}

sub addDbKeyName {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $key = @_[0];

    my $insDbKeyStmt
        = $DBH->prepare("insert into monRegKeyName (keyName) values ( ? );");

    $insDbKeyStmt->bind_param(1, $key);

#    local $insDbKeyStmt->{PrintError} = 0;

#    unless ($insDbKeyStmt->execute) {
        # Ignore error 19 "constraint violated" on monRegKeyName table
#        if ( $insDbKeyStmt->err != 19) {
#            print "addDbKeyName got error code: ",
#            $insDbKeyStmt->err, ", msg: ",
#            $insDbKeyStmt->errstr, "\n";
#        }

    eval {
        ($insDbKeyStmt->execute)
    };
    if ($@) {
#        print "In eval error handling code for addDbKeyName\n";
        # Ignore error 19 "constraint violated" on monRegKeyName table
        if ( $insDbKeyStmt->err != 19) {
            print "addDbKeyName got error code: ",
            $insDbKeyStmt->err, ", msg: ",
            $insDbKeyStmt->errstr, "\n";
        }
    }

}

sub addDbProcName {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $name = @_[0];

    my $insDbProcNameStmt
        = $DBH->prepare("insert into monRegProcName (procName) values ( ? );");

    $insDbProcNameStmt->bind_param(1, $name);

    eval {
        $insDbProcNameStmt->execute;
    };
    if ($@) {
#        print "In eval error handling code for addDbProcName\n";
        # Ignore error 19 "constraint violated" on monRegKeyName table
        if ( $insDbProcNameStmt->err != 19) {
            print "addDbProcName got error code: ",
            $insDbProcNameStmt->err, ", msg: ",
            $insDbProcNameStmt->errstr, "\n";
        }
    }
}

sub addDbClusterData {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $key = @_[0];
    my $dataValue = @_[1];

    addDbKeyName($key);

    my $insDbClusterDataStmt
        = $DBH->prepare("insert or replace into monRegClusterData (dataValue, keyId) select ?, k.keyId FROM monRegKeyName k where k.keyName = ?");

    $insDbClusterDataStmt->bind_param(1, $dataValue);
    $insDbClusterDataStmt->bind_param(2, $key);

    $insDbClusterDataStmt->execute;
}

sub addDbProcData {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $procName = @_[0];
    my $key = @_[1];
    my $dataValue = @_[2];

    addDbKeyName($key);
    addDbProcName($procName);

    my $insDbProcDataStmt
        = $DBH->prepare("insert or replace into monRegProcData (dataValue, procId, keyId ) select ?, p.procId, (SELECT k.keyId FROM monRegKeyName k WHERE k.keyName = ?) FROM monRegProcName p WHERE p.procName = ?");

    $insDbProcDataStmt->bind_param(1, $dataValue);
    $insDbProcDataStmt->bind_param(2, $key);
    $insDbProcDataStmt->bind_param(3, $procName);

    $insDbProcDataStmt->execute;
}

sub addDbProcDef {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $procType    = @_[0];
    my $procName    = @_[1];
    my $procNid     = @_[2];
    my $procProg    = @_[3];
    my $procStdout  = @_[4];
    my $procArgs    = @_[5];

    my $insDbProcDefStmt
        = $DBH->prepare("insert or replace into procs values ( ?, ?, ?, ?, ?, ? )");
    $insDbProcDefStmt->bind_param(1, $procType);
    $insDbProcDefStmt->bind_param(2, $procName);
    $insDbProcDefStmt->bind_param(3, $procNid);
    $insDbProcDefStmt->bind_param(4, $procProg);
    $insDbProcDefStmt->bind_param(5, $procStdout);
    $insDbProcDefStmt->bind_param(6, $procArgs);

    $insDbProcDefStmt->execute;
}

sub addDbPersistProc {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $procName    = @_[0];
    my $zone        = @_[1];
    my $reqTm       = @_[2];

    my $insDbPersistStmt = $DBH->prepare("insert into persist values (?, ?, ?)");

    $insDbPersistStmt->bind_param(1, $procName);
    $insDbPersistStmt->bind_param(2, $zone);
    $insDbPersistStmt->bind_param(3, $reqTm);

    $insDbPersistStmt->execute;
}

# Physical node table
sub addDbPNode {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $insDbPNodeStmt = $DBH->prepare("insert into pnode values (?, ?)");

    my $nodeId  = @_[0];
    my $nodeName    = @_[1];

    $insDbPNodeStmt->bind_param(1, $nodeId);
    $insDbPNodeStmt->bind_param(2, $nodeName);

    $insDbPNodeStmt->execute;
}

# Logical node table
sub addDbLNode {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $insDbLNodeStmt = $DBH->prepare("insert into lnode values (?, ?, ?, ?, ? , ?)");

    my $lNodeId       = @_[0];
    my $pNodeId       = @_[1];
    my $numProcessors = @_[2];
    my $roleSet       = @_[3];
    my $firstCore     = @_[4];
    my $lastCore      = @_[5];

    $insDbLNodeStmt->bind_param(1, $lNodeId);
    $insDbLNodeStmt->bind_param(2, $pNodeId);
    $insDbLNodeStmt->bind_param(3, $numProcessors);
    $insDbLNodeStmt->bind_param(4, $roleSet);
    $insDbLNodeStmt->bind_param(5, $firstCore);
    $insDbLNodeStmt->bind_param(6, $lastCore);

    $insDbLNodeStmt->execute;
}

# Logical node table
sub addDbSpare {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $insDbSpareStmt = $DBH->prepare("insert into spare values (?, ?)");

    my $pNodeId       = @_[0];
    my $spareId       = @_[1];

    $insDbSpareStmt->bind_param(1, $pNodeId);
    $insDbSpareStmt->bind_param(2, $spareId);

    $insDbSpareStmt->execute;
}

sub endGame {

    open (SQSH,">$sqshell")
	or die("unable to open $sqshell");
    printInitLinesAuxFiles (SQSH);

    if ($bVirtualNodes == 1) {
	print SQSH "export SQ_VIRTUAL_NODES=$gdNumNodes\n";
	print SQSH "export SQ_VIRTUAL_NID=0\n";
    }
    print SQSH "\nshell \$1 \$2 \$3 \$4 \$5 \$6 \$7 \$8 \$9\n";


    print "\n";
    print "Generated SQ startup script file: $coldscriptFileName\n";
    print "Generated SQ startup script file: $warmscriptFileName\n";
    print "Generated SQ cluster config file: $clusterconfFileName\n";
    print "Generated SQ Shell          file: $sqshell\n";
    print "Generated RMS Startup       file: $startRMS\n";
    print "Generated RMS Stop          file: $stopRMS\n";
    print "Generated RMS Check         file: $checkRMS\n";
    print "Generated SSMP Startup      file: $startSSMP\n";
    print "Generated SSMP Stop         file: $stopSSMP\n";
    print "Generated SSCP Startup      file: $startSSCP\n";
    print "Generated SSCP Stop         file: $stopSSCP\n";

    close(SRC);
    close(SQS);
    close(SQW);
    close(SQC);
    close(SQSH);

    close(DBZ);

    close(RMS);
    close(RMSS);
    close(RMSC);

    close(SSMP);
    close(SSMPS);

    close(SSCP);
    close(SSCPS);


    chmod 0700, $coldscriptFileName;
    chmod 0700, $warmscriptFileName;

    chmod 0700, $sqshell;
    chmod 0700, $g_dbsize;

    chmod 0700, $startRMS;
    chmod 0700, $stopRMS;
    chmod 0700, $checkRMS;

    chmod 0700, $startSSMP;
    chmod 0700, $stopSSMP;

    chmod 0700, $startSSCP;
    chmod 0700, $stopSSCP;
}

sub doInit {

    $infile=@ARGV[0];
    $scriptFileName=@ARGV[1];
    $clusterconfFileName=@ARGV[2];
    $g_HostName=$ARGV[3];
    $g_FTFlag=$ARGV[4];
    $g_PERFFlag=$ARGV[5];


    $startRMS="rmsstart";
    $startSSMP="ssmpstart";
    $startSSCP="sscpstart";
    $stopRMS="rmsstop";
    $stopSSMP="ssmpstop";
    $stopSSCP="sscpstop";
    $checkRMS="rmscheck.sql";


    $coldscriptFileName=sprintf("%s.cold", $scriptFileName);
    $warmscriptFileName=sprintf("%s.warm", $scriptFileName);

    $sqshell = "sqshell";

    $gdNumCpuCores = `cat /proc/cpuinfo | grep "processor" | wc -l`;
#print "The number of cores is $gdNumCpuCores\n";

    print "Note: Using cluster.conf format type $g_CCFormat.\n";


}


#
# Main
#

doInit();

openFiles;


while (<SRC>) {
    if (/^begin node/) {
	processNodes;
	printInitialLines;
    }
    elsif (/^begin enclosure/) {
        processEnclosures;
        $gEncSectionProcessed = 1;
    }
    elsif (/^begin tmase/) {
        validate_config_script();

    }
    elsif (/^begin role_node_map/) {
	processRoleNodeMap;
    }
    elsif (/^begin overflow/) {
	processOverflow;
	printOverflowLines;
    }
    elsif (/^begin floating_ip/) {
        processFloatingIp;
    }
    elsif (/^%/) {
	printSQShellCommand;
    }
}

#printZoneList;


    genIdTmSrv();
    genLOBConfig();
    genDTM();

    generateRMS();


printScriptEndLines;

endGame;
