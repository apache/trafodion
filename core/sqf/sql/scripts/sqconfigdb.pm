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

use strict;
use warnings;
use DBI;
use Exporter ();

package sqconfigdb;

# Database handle
my $DBH = 0;

sub addDbKeyName {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $key = $_[0];

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

    my $name = $_[0];

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

    my $key = $_[0];
    my $dataValue = $_[1];

    addDbKeyName($key);

    my $insDbClusterDataStmt
        = $DBH->prepare("insert or replace into monRegClusterData (dataValue, keyId) select ?, k.keyId FROM monRegKeyName k where k.keyName = ?");

    $insDbClusterDataStmt->bind_param(1, $dataValue);
    $insDbClusterDataStmt->bind_param(2, $key);

    $insDbClusterDataStmt->execute;
}

sub addDbNameServer {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $nodeName    = $_[0];

    my $insDbNameServerStmt = $DBH->prepare("insert into monRegNameServer values (?, ?)");

    $insDbNameServerStmt->bind_param(1, $nodeName);
    $insDbNameServerStmt->bind_param(2, $nodeName);

    $insDbNameServerStmt->execute;
}

sub delDbNameServerData {
    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $delDbNameServerStmt = $DBH->prepare("delete from monRegNameServer");
    $delDbNameServerStmt->execute;
}

#sub addDbProcData {
#
#    if (not defined $DBH) {
#        # Database not available
#        return;
#    }
#
#    my $procName = $_[0];
#    my $key = $_[1];
#    my $dataValue = $_[2];
#
#    addDbKeyName($key);
#    addDbProcName($procName);
#
#    my $insDbProcDataStmt
#        = $DBH->prepare("insert or replace into monRegProcData (dataValue, procId, keyId ) select ?, p.procId, (SELECT k.keyId FROM monRegKeyName k WHERE k.keyName = ?) FROM monRegProcName p WHERE p.procName = ?");
#
#    $insDbProcDataStmt->bind_param(1, $dataValue);
#    $insDbProcDataStmt->bind_param(2, $key);
#    $insDbProcDataStmt->bind_param(3, $procName);
#
#    $insDbProcDataStmt->execute;
#}

sub addDbProcDef {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $procType    = $_[0];
    my $procName    = $_[1];
    my $procNid     = $_[2];
    my $procProg    = $_[3];
    my $procStdout  = $_[4];
    my $procArgs    = $_[5];

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

    my $procName    = $_[0];
    my $zone        = $_[1];
    my $reqTm       = $_[2];

    my $insDbPersistStmt = $DBH->prepare("insert into persist values (?, ?, ?)");

    $insDbPersistStmt->bind_param(1, $procName);
    $insDbPersistStmt->bind_param(2, $zone);
    $insDbPersistStmt->bind_param(3, $reqTm);

    $insDbPersistStmt->execute;
}

sub addDbPersistData {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $keyName   = $_[0];
    my $valueName = $_[1];

    my $insDbPersistStmt = $DBH->prepare("insert into monRegPersistData values (?, ?)");

    $insDbPersistStmt->bind_param(1, $keyName);
    $insDbPersistStmt->bind_param(2, $valueName);

    $insDbPersistStmt->execute;
}

sub delDbPersistData {
    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $delDbPersistStmt = $DBH->prepare("delete from monRegPersistData");
    $delDbPersistStmt->execute;
}

# Physical node table
sub addDbPNode {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $insDbPNodeStmt = $DBH->prepare("insert into pnode values (?, ?, ?, ?)");

    my $nodeId          = $_[0];
    my $nodeName        = $_[1];
    my $firstExcCore    = $_[2];
    my $lastExcCore     = $_[3];

    $insDbPNodeStmt->bind_param(1, $nodeId);
    $insDbPNodeStmt->bind_param(2, $nodeName);
    $insDbPNodeStmt->bind_param(3, $firstExcCore);
    $insDbPNodeStmt->bind_param(4, $lastExcCore);

    $insDbPNodeStmt->execute;
}

# Logical node table
sub addDbLNode {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $insDbLNodeStmt = $DBH->prepare("insert into lnode values (?, ?, ?, ?, ? , ?)");

    my $lNodeId       = $_[0];
    my $pNodeId       = $_[1];
    my $numProcessors = $_[2];
    my $roleSet       = $_[3];
    my $firstCore     = $_[4];
    my $lastCore      = $_[5];

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

    my $insDbSpareStmt = $DBH->prepare("insert into snode values (?, ?, ?, ?, ?)");

    my $pNodeId       = $_[0];
    my $nodeName      = $_[1];
    my $firstCore     = $_[2];
    my $lastCore      = $_[3];
    my $sparedpNid    = $_[4];

    $insDbSpareStmt->bind_param(1, $pNodeId);
    $insDbSpareStmt->bind_param(2, $nodeName);
    $insDbSpareStmt->bind_param(3, $firstCore);
    $insDbSpareStmt->bind_param(4, $lastCore);
    $insDbSpareStmt->bind_param(5, $sparedpNid);

    $insDbSpareStmt->execute;
}

sub addDbUniqStr {

    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $lv_nid      = $_[0];
    my $lv_id       = $_[1];
    my $lv_str      = $_[2];

    my $insDbUniqStrStmt = $DBH->prepare("insert or replace into monRegUniqueStrings values ( ?, ?, ?)");

    $insDbUniqStrStmt->bind_param(1, $lv_nid);
    $insDbUniqStrStmt->bind_param(2, $lv_id);
    $insDbUniqStrStmt->bind_param(3, $lv_str);

    $insDbUniqStrStmt->execute;
}

sub listNodes {
    if (not defined $DBH) {
        # Database not available
        return;
    }

    my $insDbListStmt = $DBH->prepare("select p.pNid, l.lNid, p.nodeName, l.firstCore, l.lastCore, l.processors, l.roles from pnode p, lnode l where p.pNid = l.pNid");

    $insDbListStmt->execute;
    my @rows;
    my @row;
    while (@row = $insDbListStmt->fetchrow_array()) {
        push (@rows, @row);
    }
    return @rows;
}

sub openDb {
#    my $dbargs = {AutoCommit => 1,
#                  PrintError => 1};
    my $dbargs = {AutoCommit => 1,
                  RaiseError => 1,
                  PrintError => 0,
                  ShowErrorStatement => 1};
    $DBH = DBI->connect("dbi:SQLite:dbname=$ENV{'TRAF_VAR'}/sqconfig.db","","",$dbargs);
#   Disable database synchronization (fsync) because it slows down writes
#   too much.
    $DBH->do("PRAGMA synchronous = OFF");
}

1;
