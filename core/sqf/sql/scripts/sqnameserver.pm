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
use Exporter ();

use sqconfigdb;

package sqnameserver;

# set g_debugFlag to 1 to print detailed debugging info
my $g_debugFlag = 0;

my @g_nodeNames;
my $g_ok;

my $errors = 0;
my $stmt;

# Display persist configuration statement if not already displayed.
sub displayStmt
{
    $errors++;
    if ($_[0] == 1)
    {
        print "For \"$stmt\":\n";
        # Set flag that statement has been displayed
        $_[0] = 0;
    }
}

sub parseComma {
    my ($s) =  @_;
    if ($s =~ /(,\s*)/) {
        $s =~ s:$1::;
        return (1, $s);
    } else {
        displayStmt($g_ok);
	print "   Error: Expecting ',', but saw $s\n"; #T
        return (0, '');
    }
}

sub parseEnd {
    my ($s) =  @_;
    if ($s =~ /^\s*?$/) {
        return 1;
    } else {
        displayStmt($g_ok);
        print "   Error: Expecting <eoln>, but saw $s\n"; #T
        return 0;
    }
}

sub parseEq {
    my ($s) =  @_;
    if ($s =~ /(=\s*)/) {
        $s =~ s:$1::;
        return (1, $s);
    } else {
        displayStmt($g_ok);
        print "   Error: Expecting '=', but saw $s\n"; #T
        return (0, '');
    }
}

sub parseStatement {
    my ($s) = @_;
    if ($g_debugFlag) {
        print "stmt: $s\n";
    }
    if ($s =~ /^#/) {
    } elsif ($s =~ /^\s*$/) {
    } elsif ($s =~ /(nodes)\s*/) {
        my $k = $1;
        $s =~ s:$k\s*::;
        my $eq;
        ($eq, $s) = parseEq($s);
        if ($eq) {
            while ($s =~ /([A-Za-z0-9.\-]+)(\s*,\s*)/) {
                my $nodeName = $1;
                $s =~ s:$nodeName$2::;
                push(@g_nodeNames, $nodeName);
            }
            if ($s =~ /([A-Za-z0-9.\-]+)/) {
                my $nodeName = $1;
                $s =~ s:$nodeName::;
                push(@g_nodeNames, $nodeName);
                parseEnd($s);
            } else {
                displayStmt($g_ok);
                print "   Error: Expecting <hostname> e.g. n054, but saw $s\n"; #T
            }
        }
    } else {
        displayStmt($g_ok);
        my $k = $s;
        if ($s =~ /^([A-Za-z_]+)/) {
           $k = $1;
        }
        print "   Error: Invalid keyword $k, expecting nodes\n"; #T
    }
}

sub validateNameserver
{
    if ($errors == 0) {
        sqconfigdb::delDbNameServerData();
        my $nodeName;
        foreach $nodeName (@g_nodeNames) {
            sqconfigdb::addDbNameServer( $nodeName );
        }
    }

    return $errors;
}

sub parseStmt
{
    $stmt = $_;
    chomp($stmt);
    $g_ok = 1;
    parseStatement($stmt);
    if ($errors != 0) { # Had errors
        return 1;
    }
}

# Below is to return true; this is required when this module is referenced via a "use" statement in another module
# (if we had variables defined and assigned in addition to functions, we would not need to include this implicit return)
1;
