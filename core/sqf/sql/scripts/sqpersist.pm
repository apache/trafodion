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

package sqpersist;

# set g_debugFlag to 1 to print detailed debugging info
my $g_debugFlag = 0;

my %g_keys;
my %g_prefixSeen;
my $g_ok;
my $g_opts;
my $g_prefix;
my $g_prefixSave;
my $g_processName;
my $g_processType;
my $g_programName;
my $g_programArgs;
my $g_requiresDtm;
my $g_stdout;
my $g_persistRetries;
my $g_persistZones;
my @g_dbList;

my $errors = 0;
my $stmt;

sub checkPrefix {
    my ($prefix) =  @_;
    if ($prefix ne $g_prefix) {
        validatePrefix();
    }
    $g_prefix = $prefix;
}

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

sub getKeyList {
    my $keyList = '';
    my $k;
    foreach $k (keys %g_keys) {
        if ($keyList eq '') {
            $keyList = $k;
        } else {
            $keyList = $keyList . ',' . $k;
        }
    }
    return $keyList;
}

sub validKey {
    my ($k) =  @_;
    if ($g_keys{$k}) {
        return 1;
    } else {
        displayStmt($g_ok);
        my $keyList = getKeyList();
        print "   Error: unknown key: $k. valid keys are: $keyList\n"; #T
        return 0;
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

sub parseNid {
    my ($s) =  @_;
    if ($s =~ /(%nid\+)/) {
        my $r = $1;
        $s =~ s:$1::;
        return (1, $r);
    } elsif ($s =~ /(%nid)/) {
        my $r = $1;
        $s =~ s:$1::;
        return (1, $r);
    } elsif ($s =~ /(^\s*)/) {
        my $r = $1;
        $s =~ s:$1::;
        return (1, $r);
    } else {
        displayStmt($g_ok);
        print "   Error: Expecting { %nid | %nid+ }, but saw $s\n"; #T
        return (0, '');
    }
}

sub parseZid {
    my ($s) =  @_;
    if ($s =~ /(%zid\+)/) {
        my $r = $1;
        $s =~ s:$1::;
        return (1, $r);
    } elsif ($s =~ /(%zid)/) {
        my $r = $1;
        $s =~ s:$1::;
        return (1, $r);
    } else {
        displayStmt($g_ok);
        print "   Error: Expecting { %zid | %zid+ }, but saw $s\n"; #T
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
    } elsif ($s =~ /^(PERSIST_PROCESS_KEYS)\s*/) {
        my $k = $1;
        $s =~ s:$k\s*::;
        my $eq;
        ($eq, $s) = parseEq($s);
        if ($eq) {
            my $value;
            while ($s =~ /([A-Z]+)(\s*,\s*)/) {
                my $key = $1;
                $g_keys{$key} = $key;
                $value = $value . $key . ',';
                $s =~ s:$key$2::;
            }
            if ($s =~ /([A-Z]+)/) {
                my $key = $1;
                $g_keys{$key} = $key;
                $s =~ s:$key::;
                $value = $value . $key;
                push(@g_dbList, $k, $value);
                parseEnd($s);
            } else {
                displayStmt($g_ok);
                print "   Error: Expecting <key> e.g. DTM, but saw $s\n"; #T
            }
        }
    } elsif ($s =~ /^([A-Z]+)(_PROCESS_NAME)\s*/) {
        my $prefix = $1;
        my $k = $2;
        checkPrefix($prefix);
        $s =~ s:$prefix$k\s*::;
        if (validKey($prefix)) {
            my $eq;
            ($eq, $s) = parseEq($s);
            if ($eq) {
                if ($s =~ /(\$)([A-Z]+[0-9]*)/) {
                    $g_processName = $1 . $2;
                    $s =~ s:\$$2::;
                    my ($res, $r) = parseNid($s);
                    if ($res == 1) {
                        $g_processName = $g_processName . $r;
                        $g_opts |= 0x1;
                        $s =~ s:$r::;
                        $s =~ s:\+::;
                    }
                    push(@g_dbList, $g_prefix . $k, $g_processName);
                    if ($res == 1) {
                        parseEnd($s);
                    }
                } else {
                    displayStmt($g_ok);
                    print "   Error: Expecting <process-name> e.g. \$TM[%nid[+]], but saw $s\n"; #T
                }
            }
        }
    } elsif ($s =~ /(^[A-Z]+)(_PROCESS_TYPE)\s*/) {
        my $prefix = $1;
        my $k = $2;
        checkPrefix($prefix);
        $s =~ s:$prefix$2\s*::;
        if (validKey($prefix)) {
            my $eq;
            ($eq, $s) = parseEq($s);
            if ($eq) {
                if ($s =~ /(DTM|PERSIST|PSD|SSMP|TMID|WDG|TNS)/) {
                    $g_processType = $1;
                    $s =~ s:$1::;
                    $g_opts |= 0x2;
                    push(@g_dbList, $g_prefix . $k, $g_processType);
                    parseEnd($s);
                } else {
                    displayStmt($g_ok);
                    print "   Error: Expecting { DTM | PERSIST |PSD | SSMP | TMID | WDG | TNS }, but saw $s\n"; #T
                }
            }
        }
    } elsif ($s =~ /(^[A-Z]+)(_PROGRAM_NAME)\s*/) {
        my $prefix = $1;
        my $k = $2;
        checkPrefix($prefix);
        $s =~ s:$prefix$2\s*::;
        if (validKey($prefix)) {
            my $eq;
            ($eq, $s) = parseEq($s);
            if ($eq) {
                if ($s =~ /([a-zA-Z0-9_]+)/) {
                    $g_programName = $1;
                    $s =~ s:$1::;
                    $g_opts |= 0x4;
                    push(@g_dbList, $g_prefix . $k, $g_programName);
                    parseEnd($s);
                } else {
                    displayStmt($g_ok);
                    print "   Error: Expecting <program-name> e.g. tm, but saw $s\n"; #T
                }
            }
        }
    } elsif ($s =~ /(^[A-Z]+)(_PROGRAM_ARGS)\s*/) {
        my $prefix = $1;
        my $k = $2;
        checkPrefix($prefix);
        $s =~ s:$prefix$2\s*::;
        if (validKey($prefix)) {
            my $eq;
            ($eq, $s) = parseEq($s);
            if ($eq) {   # ([a-zA-Z0-9_]+)
                if ($s =~ /([a-zA-Z0-9\!-\/\:-\@\[-\`\{-\~\h10]*)/) {
                    $g_programArgs = $1;
                    $s =~ s:$1::;
                    $g_opts |= 0x80;
                    push(@g_dbList, $g_prefix . $k, $g_programArgs);
                    parseEnd($s);
                } else {
                    displayStmt($g_ok);
                    print "   Error: Expecting <args> e.g. -t 1, but saw $s\n"; #T
                }
            }
        }
    } elsif ($s =~ /(^[A-Z]+)(_REQUIRES_DTM)\s*/) {
        my $prefix = $1;
        my $k = $2;
        checkPrefix($prefix);
        $s =~ s:$prefix$2\s*::;
        if (validKey($prefix)) {
            my $eq;
            ($eq, $s) = parseEq($s);
            if ($eq) {
                if ($s =~ /(Y|N)/) {
                    $g_requiresDtm = $1;
                    $s =~ s:$1::;
                    $g_opts |= 0x8;
                    push(@g_dbList, $g_prefix . $k, $g_requiresDtm);
                    parseEnd($s);
                } else {
                    displayStmt($g_ok);
                    print "   Error: Expecting { Y | N }, but saw $s\n"; #T
                }
            }
        }
    } elsif ($s =~ /(^[A-Z]+)(_STDOUT)\s*/) {
        my $prefix = $1;
        my $k = $2;
        checkPrefix($prefix);
        $s =~ s:$prefix$2\s*::;
        if (validKey($prefix)) {
            my $eq;
            ($eq, $s) = parseEq($s);
            if ($eq) {
                if ($s =~ /([a-zA-Z0-9_\/]+)/) {
                    $g_stdout = $1;
                    $s =~ s:$1::;
                    if ($s =~ /(%nid\+)/) {
                        $g_stdout = $g_stdout . $1;
                        $s =~ s:$1::;
                        $s =~ s:\+::;
                    }
                    elsif ($s =~ /(%nid)/) {
                        $g_stdout = $g_stdout . $1;
                        $s =~ s:$1::;
                    }
                    $g_opts |= 0x10;
                    push(@g_dbList, $g_prefix . $k, $g_stdout);
                    parseEnd($s);
                } else {
                    displayStmt($g_ok);
                    print "   Error: Expecting <stdout> e.g. stdout_TM[%nid[+]], but saw $s\n"; #T
                }
            }
        }
    } elsif ($s =~ /(^[A-Z]+)(_PERSIST_RETRIES)\s*/) {
        my $prefix = $1;
        my $k = $2;
        checkPrefix($prefix);
        $s =~ s:$prefix$2\s*::;
        if (validKey($prefix)) {
            my $eq;
            ($eq, $s) = parseEq($s);
            if ($eq) {
                if ($s =~ /(\d+)\s*/) {
                    $g_persistRetries = $1;
                    $s =~ s:$1\s*::;
                    if ($s =~ /,\s*/) {
                        $s =~ s:,\s*::;
                        if ($s =~ /(\d+)\s*/) {
                            $g_persistRetries = $g_persistRetries . ',' . $1;
                            $s =~ s:$1\s*::;
                            $g_opts |= 0x20;
                            push(@g_dbList, $g_prefix . $k, $g_persistRetries);
                            parseEnd($s);
                        } else {
                            displayStmt($g_ok);
                            print "   Error: Expecting <secs> e.g. 30, but saw $s\n"; #T
                        }
                    } else {
                        displayStmt($g_ok);
                        print "   Error: Expecting ',', but saw $s\n"; #T
                    }
                } else {
                    displayStmt($g_ok);
                    print "   Error: Expecting <retries> e.g. 2, but saw $s\n"; #T
                }
            }
        }
    } elsif ($s =~ /(^[A-Z]+)(_PERSIST_ZONES)\s*/) {
        my $prefix = $1;
        my $k = $2;
        checkPrefix($prefix);
        $s =~ s:$prefix$2\s*::;
        if (validKey($prefix)) {
            my $eq;
            ($eq, $s) = parseEq($s);
            if ($eq) {
                my ($res, $r) = parseZid($s);
                if ($res == 1) {
                    $g_persistZones = $r;
                    $s =~ s:$r::;
                    $s =~ s:\+::;
                    $g_opts |= 0x40;
                }
                push(@g_dbList, $g_prefix . $k, $g_persistZones);
                if ($res == 1) {
                    parseEnd($s);
                }
            }
        }
    } else {
        displayStmt($g_ok);
        my $k = $s;
        if ($s =~ /^([A-Z_]+)/) {
           $k = $1;
        }
        print "   Error: Invalid keyword $k, expecting {PERSIST_PROCESS_KEYS|<prefix>_PROCESS_NAME|<prefix>_PROCESS_TYPE|<prefix>_PROGRAM_NAME|<prefix>_REQUIRES_DTM|<prefix>_STDOUT|<prefix>_PERSIST_RETRIES|<prefix>_PERSIST_ZONES}\n"; #T
    }
}

sub resetVars
{
    $g_opts = 0;
    $g_prefix = '';
    $g_processName = '';
    $g_processType = '';
    $g_programName = '';
    $g_programArgs = '';
    $g_requiresDtm = 0;
    $g_stdout = '';
    $g_persistRetries = '';
    $g_persistZones = '';
}

sub validatePersist
{
    validatePrefix();
    for my $key ($sqpersist::g_keys) {
        if ($key eq '') {
        } elsif (!$g_prefixSeen{$key}) {
            displayStmt($g_ok);
            print "  Error: $key has no entries, but is listed in PERSIST_PROCESS_KEYS\n" #T
        }
    }

    if ($errors == 0) {
        sqconfigdb::delDbPersistData();
        my $kv;
        my $k = '';
        my $v;
        foreach $kv (@g_dbList) {
            if ($k eq '') {
                $k = $kv;
            } else {
                $v = $kv;
                sqconfigdb::addDbPersistData( $k, $v );
                $k = '';
            }
        }
    }

    return $errors;
}

sub validatePrefix
{
    if ($g_prefix ne '') {
        # mark prefix seen
        $g_prefixSeen{$g_prefix} = 1;
        if (($g_opts & 0x1) == 0) {
            displayStmt($g_ok);
            my $str = "_PROCESS_NAME";
            print "   Error: missing $g_prefix$str\n"; #T
        }
        if (($g_opts & 0x2) == 0) {
            displayStmt($g_ok);
            my $str = "_PROCESS_TYPE";
            print "   Error: missing $g_prefix$str\n"; #T
        }
        if (($g_opts & 0x4) == 0) {
            displayStmt($g_ok);
            my $str = "_PROGRAM_NAME";
            print "   Error: missing $g_prefix$str\n"; #T
        }
        if (($g_opts & 0x8) == 0) {
            displayStmt($g_ok);
            my $str = "_REQUIRES_DTM";
            print "   Error: missing $g_prefix$str\n"; #T
        }
        if (($g_opts & 0x10) == 0) {
            displayStmt($g_ok);
            my $str = "_STDOUT";
            print "   Error: missing $g_prefix$str\n"; #T
        }
        if (($g_opts & 0x20) == 0) {
            displayStmt($g_ok);
            my $str = "_PERSIST_RETRIES";
            print "   Error: missing $g_prefix$str\n"; #T
        }
        if (($g_opts & 0x40) == 0) {
            displayStmt($g_ok);
            my $str = "_PERSIST_ZONES";
            print "   Error: missing $g_prefix$str\n"; #T
        }
        if (($g_opts & 0x80) == 0) {
            displayStmt($g_ok);
            my $str = "_PROGRAM_ARGS";
            print "   Error: missing $g_prefix$str\n"; #T
        }
        resetVars();
    }
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
