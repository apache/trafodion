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
require "ctime.pl";
use sqnodes;
use sqpersist;
use POSIX;

# Process types.  Must match values defined by the monitor in msgdef.h.
my $ProcessType_Undefined = 0;
my $ProcessType_Generic   = 4;
my $ProcessType_SSMP      = 11;

my $gDebug = 0;

my $bVirtualNodes=0;

$gRoleEnumStorage     = "storage";
$gRoleEnumEdge        = "connection";
$gRoleEnumAggregation = "aggregation";

my $gdNumNodes=0;
my $gdZoneId=0;

my @gNodeIdToZoneIdIndex = ();

my @g_zonelist = ();

my $gdNumCpuCores = 1;

my $g_CCFormat = 2;

my $gbInitialLinesPrinted = 0;

my $gShellStarted=0;

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
my $BDR_ERROR = 70;


sub printScript {
    ($dWhich, @rest) = @_;

    if ($dWhich <= 1) {
        print SQS @rest;
    }

    if ($dWhich >= 1) {
        print SQW @rest;
    }
}

sub printIDTMScript {
    ($dWhich, @rest) = @_;
    print IDTM @rest;
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

sub printTMScript {
    ($dWhich, @rest) = @_;
    print TM @rest;
}

sub printTime {
    printScript(1, "# Trafodion Startup script generated @ ",&ctime(time),"\n");
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
        print ETC "MS_STREAMS_MIN=20000\n";
        print ETC "MS_STREAMS_MAX=20000\n";

    }
    # Cluster
    else {
        print ETC "MS_STREAMS_MIN=20000\n";
        print ETC "MS_STREAMS_MAX=20000\n";
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

    printScript(1, "\nshell <<eof \n");
    $gShellStarted=1;

    printScript(1, "\n");

    printScript(1, "! Start the monitor processes across the cluster\n");

    printScript(1, "startup\n");

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
    printScript(1, "set MY_NODES=\$MY_NODES\n");

    sqconfigdb::addDbClusterData( "SQ_MBTYPE", $ENV{'SQ_MBTYPE'});
    sqconfigdb::addDbClusterData( "TRAF_HOME", "$TRAF_HOME"); # comes out null

    genSQShellExit();

    $gbInitialLinesPrinted = 1;
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

sub genComponentTmWait {

    my $l_compname  = @_[0];
    my $l_duration  = @_[1];
    my $l_iteration = @_[2];

    printTMScript(1, "\n");
    printTMScript(1, "sqcheck -c $l_compname -d $l_duration -i $l_iteration -r\n");
    printTMScript(1, "sq_stat_$l_compname=\$\?\n");
    printTMScript(1, "if \(\[ \$sq_stat_$l_compname '!=' 0 \] && [ \$sq_stat_$l_compname '!=' 1 \]\); then\n");
    printTMScript(1, "\techo \"The $l_compname process(es) are Not Ready yet. Stopping further startup (if any).\"\n");
    printTMScript(1, "\texit 1\n");
    printTMScript(1, "else\n");
    printTMScript(1, "\techo \"The $l_compname process(es) are Ready.\"\n");
    printTMScript(1, "fi\n");
}


sub genIDTMSrv {
    if ($SQ_IDTMSRV > 0) {
        printIDTMScript(1, "#!/bin/sh\n");
        printIDTMScript(1, "# Trafodion config/utility file generated @ ",&ctime(time),"\n");
        
        printIDTMScript(1, "sqshell -c persist exec TMID\n");
        printIDTMScript(1, "exit $?\n");

        printScript(1, "\nidtmstart\n");
    }
}

sub genDTM {

    printTMScript(1, "#!/bin/sh\n");
    printTMScript(1, "# Trafodion config/utility file generated @ ",&ctime(time),"\n");
 
    printTMScript(1, "sqshell -a <<eof\n");
    printTMScript(1, "\n! Start DTM\n");
    printTMScript(1, "set DTM_RUN_MODE=2\n");
    printTMScript(1, "set SQ_AUDITSVC_READY=1\n");
    printTMScript(1, "set DTM_TLOG_PER_TM=1\n");
    printTMScript(1, "persist exec DTM\n");
    printTMScript(1, "delay 5\n");
    printTMScript(1, "exit\n");
    printTMScript(1, "eof\n");
    
    genComponentTmWait("dtm", 10, 60);
 
    printTMScript(1, "sqshell -a <<eof\n");
    printTMScript(1, "! Generate DTM Event 1\n");
    printTMScript(1, "event {DTM} 1\n");
    printTMScript(1, "exit\n");
    printTMScript(1, "eof\n");

    printTMScript(1, "\n");
    printTMScript(1, "echo \"Checking whether the transaction service is ready.\"\n");
    printTMScript(1, "sqr_stat=0\n");
    printTMScript(1, "while \[\[ \$sqr_stat == 0 \]\];\n");
    printTMScript(1, "do\n");
    printTMScript(1, "sqregck -r SQ_TXNSVC_READY -d 5 -i -1\n");
    printTMScript(1, "sqr_stat=\$\?\n");
    printTMScript(1, "done\n");
    printTMScript(1, "echo \"The Transaction Service is Ready.\"\n");

    printScript(1, "\ntmstart\n");
    printScript(1, "tm_ret=\$?\n");
    printScript(1, "if [ \$tm_ret != 0 ]; then\n");
    printScript(1, "exit \$tm_ret\n");
    printScript(1, "fi\n");
}

sub generateRMS {

    genSQShellExit();

    #Print out run RMS script
    printScript(1, "\necho \"SQ_START_RMS: \" \$SQ_START_RMS \n");
    printScript(1, "if [[ \$SQ_START_RMS == \"1\" ]]; then\n");
    printScript(1, "echo \"Starting RMS\"\n");
    
    #generate rmsstart and rmsstop scripts
    printRMSScript(0, "#!/bin/sh\n");
    printRMSScript(0, "# Trafodion config/utility file generated @ ",&ctime(time),"\n");
    printRMSScript(0, "\n# Start the RMS processes\n");
    printRMSScript(0, "sscpstart\n");
    printRMSScript(0, "ssmpstart\n");

    printRMSStopScript(0, "#!/bin/sh\n");
    printRMSStopScript(0, "# Trafodion config/utility file generated @ ",&ctime(time),"\n");
    printRMSStopScript(0, "\n# Stop the RMS processes\n");
    printRMSStopScript(0, "ssmpstop\n");
    printRMSStopScript(0, "sscpstop\n");
    printRMSStopScript(0, "\necho RMS Processes Stopped\n");

    #generate ssmpstart, ssmpstop, sscpstart, sscpstop scripts
    printRMSScript(1, "#!/bin/sh\n");
    printRMSScript(1, "# Trafodion config/utility file generated @ ",&ctime(time),"\n");
    printRMSScript(2, "\n# Start the SSMP processes\n");
    printRMSScript(2, "sqshell -c persist exec SSMP\n");
    printRMSScript(2, "exit \$?\n");
    printRMSScript(3, "\n# Start the SSCP processes\n");
    printRMSScript(3, "sqshell -c persist exec SSCP\n");
    printRMSScript(3, "exit \$?\n");

    printRMSStopScript(1, "#!/bin/sh\n");
    printRMSStopScript(1, "# Trafodion config/utility file generated @ ",&ctime(time),"\n");
    printRMSStopScript(2, "\n# Stop the SSMP processes\n");
    printRMSStopScript(2, "sqshell -c persist kill SSMP\n");
    printRMSStopScript(2, "exit \$?\n");
    printRMSStopScript(3, "\n# Stop the SSCP processes\n");
    printRMSStopScript(3, "sqshell -c persist kill SSCP\n");
    printRMSStopScript(3, "exit \$?\n");
   

    printRMSCheckScript(1, "-- Trafodion config/utility file generated @ ",&ctime(time),"\n");
    printRMSCheckScript(1, "prepare rms_check from select current_timestamp, \n");
    printRMSCheckScript(1, "cast('Node' as varchar(5)), \n");
    printRMSCheckScript(1, "cast(tokenstr('nodeId:', variable_info) as varchar(3)) node, \n");
    printRMSCheckScript(1, "cast(tokenstr('Status:', variable_info) as varchar(10)) status \n");
    printRMSCheckScript(1, "from table(statistics(null, ?));\n");

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

    while (<>) {
        next if (/^$/);
        next if (/^#/);
        if (/^_virtualnodes/) {
            @words=split(' ',$_);
            $gdNumNodes=@words[1];
            $bVirtualNodes=1;
            my $l_dNodeIndex = 0;

            print "Generating virtual configuration database, node-name=$g_HostName, virtual nodes count=$gdNumNodes\n";

            sqnodes::genVirtualConfigDb( $g_HostName, $gdNumNodes );
            for ($l_dNodeIndex = 0; $l_dNodeIndex < $gdNumNodes; $l_dNodeIndex++) {

                $gNodeIdToZoneIdIndex[$l_dNodeIndex] = $l_dNodeIndex;

                push(@g_EdgeNodes, $l_dNodeIndex);
            }
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
                {   # Valid configuration, generate sqconfig.db
                    $gdNumNodes = sqnodes::numNodes();

                    sqnodes::genConfigDb( );
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
                print "Exiting without generating sqconfig.db due to errors.\n";
                exit 1;
            }
        }
    }
}

sub processPersist {
    my $err = 0;
    while (<>) {
        if (/^begin persist/) {
        }
        elsif (/^end persist/) {
            if (sqpersist::validatePersist() != 0) {
                $err = 1;
            }
            if ($err != 0) {
                print "   Error: not a valid persist configuration statement.\n";
                print "Exiting without generating sqconfig.db due to errors.\n";
                exit 1;
            }
            return;
        }
        else {
            if (sqpersist::parseStmt() != 0) {
                $err = 1;
            }
        }
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

sub processFloatingIp {
    while (<>) {
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
#                        print "$lv_i : @g_EdgeNodes[$lv_i] \n";
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
#                        print "$lv_i : @g_EdgeNodes[$lv_i] \n";
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

sub printInitLinesAuxFiles {

    my $file_ptr  = @_[0];

    print $file_ptr "#!/bin/sh\n";
    print $file_ptr "# Trafodion config/utility file generated @ ", &ctime(time), "\n";
}

sub openFiles {

    open (SQS,">$coldscriptFileName")
        or die("unable to open $coldscriptFileName");

    open (IDTM,">$startIDTM")
        or die("unable to open $startIDTM");

    open (TM,">$startTM")
        or die("unable to open $startTM");

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

    sqconfigdb::openDb();
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
    print "Generated SQ Shell          file: $sqshell\n";
    print "Generated IDTM Startup      file: $startIDTM\n";
    print "Generated TM Startup        file: $startTM\n";
    print "Generated RMS Startup       file: $startRMS\n";
    print "Generated RMS Stop          file: $stopRMS\n";
    print "Generated RMS Check         file: $checkRMS\n";
    print "Generated SSMP Startup      file: $startSSMP\n";
    print "Generated SSMP Stop         file: $stopSSMP\n";
    print "Generated SSCP Startup      file: $startSSCP\n";
    print "Generated SSCP Stop         file: $stopSSCP\n";

    close(SQS);
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

    chmod 0700, $sqshell;

    chmod 0700, $startIDTM;
    chmod 0700, $startTM;

    chmod 0700, $startRMS;
    chmod 0700, $stopRMS;
    chmod 0700, $checkRMS;

    chmod 0700, $startSSMP;
    chmod 0700, $stopSSMP;

    chmod 0700, $startSSCP;
    chmod 0700, $stopSSCP;
}

sub doInit {

    $scriptFileName= shift(@ARGV);
    $g_HostName= shift(@ARGV);
    $g_FTFlag= shift(@ARGV);
    $g_PERFFlag= shift(@ARGV);
    # remaining args are input files


    $startIDTM="idtmstart";
    $startTM="tmstart";

    $startRMS="rmsstart";
    $startSSMP="ssmpstart";
    $startSSCP="sscpstart";
    $stopRMS="rmsstop";
    $stopSSMP="ssmpstop";
    $stopSSCP="sscpstop";
    $checkRMS="rmscheck.sql";


    $coldscriptFileName=sprintf("%s.cold", $scriptFileName);

    $sqshell = "sqshell";

    $gdNumCpuCores = `cat /proc/cpuinfo | grep "processor" | wc -l`;
#print "The number of cores is $gdNumCpuCores\n";

}


#
# Main
#

doInit();

openFiles;


while (<>) {
    if (/^begin node/) {
        processNodes;
        printInitialLines;
    }
    elsif (/^begin floating_ip/) {
        processFloatingIp;
    }
    elsif (/^begin persist/) {
        processPersist;
    }
    elsif (/^%/) {
        printSQShellCommand;
    }
    else {
        if (/^#/) {
        }
        elsif (/^\s*$/) {
        }
        else {
            print "invalid line:", $_;
            exit 1
        }
    }
}

#printZoneList;

genIDTMSrv();

genDTM();

generateRMS();


printScriptEndLines;

endGame;
