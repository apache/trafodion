#!/usr/bin/perl
#
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
#

use strict;
use warnings;

use sqconfigdb;

my $bGen=1;
my $bVirtualNodes=0;
my $infile;
my $nodesAdded=0;
my $nodesRemoved=0;

sub doInit {
    $infile=$ARGV[0];
}

sub openFiles {
    open(SRC, "<$infile")
        or die("unable to open $infile");

    open(TMP, ">/tmp/zregen")
        or die("unable to open /tmp/zregen");

    sqconfigdb::openDb();
}

sub printScript {
    my (@rest) = @_;

    print TMP @rest;
}

sub printSQShellCommand {
    printScript($_);
}

sub processFloatingIp {
    printScript($_);
    while (<SRC>) {
        printScript($_);
        if (/^end floating_ip/) {
           return;
        }
    }
}

sub processNodes {
    printScript($_);
    while (<SRC>) {
        if (/^#/) {
            printScript($_);
        } elsif (/^_virtualnodes/) {
            $bVirtualNodes=1;
            $bGen=0;
            printScript($_);
            print "Warning - virtual nodes cannot be regenerated - sqconfig NOT changed.\n";
        }
        elsif (/^end node/) {
            if (!$bVirtualNodes) {
                my @row = sqconfigdb::listNodes();
                for (my $i = 0; $i <= $#row; $i+=7) {
                    $nodesAdded++;
                    my $pnid = $row[$i+0];
                    my $lnid = $row[$i+1];
                    my $nodeName = $row[$i+2];
                    my $firstCore = $row[$i+3];
                    my $lastCore = $row[$i+4];
                    my $processors = $row[$i+5];
                    my $rolesData = $row[$i+6];
                    my $roles = '';
                    if ($rolesData & 0x001) {
                        if ($roles ne '') {
                            $roles = $roles . ",";
                        }
                        $roles = $roles . "connection";
                    }
                    if ($rolesData & 0x002) {
                        if ($roles ne '') {
                            $roles = $roles . ",";
                        }
                        $roles = $roles . "aggregation";
                    }
                    if ($rolesData & 0x004) {
                        if ($roles ne '') {
                            $roles = $roles . ",";
                        }
                        $roles = $roles . "storage";
                    }
                    my $str = "node-id=$pnid;node-name=$nodeName;cores=$firstCore-$lastCore;processors=$processors;roles=$roles\n";
                    printScript($str);
                }
            }
            printScript($_);
            return;
        }
        else {
            if (/^\s*node-id.*/) {
                $nodesRemoved++;
            }
            # don't print this
        }
    }
}

sub processPersist {
    printScript($_);
    while (<SRC>) {
        printScript($_);
        if (/^end persist/) {
            return;
        }
    }
}

sub endGame {
    close(SRC);
    close(TMP);

    if ($bGen) {
        open(TMP, "</tmp/zregen")
            or die("unable to open /tmp/zregen");
        open(SRC, ">$infile")
            or die("unable to open $infile");
        while (<TMP>) {
            print SRC $_;
        }
        close(SRC);
        close(TMP);

        print "Re-Generated (removed nodes=$nodesRemoved, added nodes=$nodesAdded) configuration file: $infile\n";
    }
}

#
# Main
#

doInit();

openFiles;


while (<SRC>) {
    if (/^begin node/) {
        processNodes();
    }
    elsif (/^begin floating_ip/) {
        processFloatingIp();
    }
    elsif (/^begin persist/) {
        processPersist();
    }
    else {
        printScript($_);
    }
}

endGame();
