#!/bin/perl
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
# usage: update_version.pl inputfile.rc major minor sp revision

use strict;

my ( $infile, $major, $minor, $sp, $revision, $outfile ) = @ARGV;

unless ( $infile =~ /\.rc$/i ) {

    print "Error: Invalid file on input\n";
    exit 1;
}

unless ( $major =~ /^\d+$/ && $minor =~ /^\d+$/ && $sp =~ /^\d+$/ && $revision =~ /^\d+$/ ) {

    print "Error: Invalid version on input\n";
    exit 1;

}

$outfile=$infile . "\.update_version_temp";

print "Updating Version in $infile...\n";
#print "\$infile=$infile\n";
#print "\$outfile=$outfile\n";
#print "\$version=$version\n";

open( INFILE, $infile ) or die "Error: Can't open $infile - $!";
open( OUTFILE, ">$outfile" ) or die "Error: Can't open $outfile - $!";

while ( <INFILE> ) {

    if ( /FILEVERSION|PRODUCTVERSION/ ) {
   
	s/(\d+),(\d+),(\d+),(\d+)/$major,$minor,$sp,$revision/;

        print OUTFILE;
    
    }
    elsif( /"FileVersion"|"ProductVersion"/ ){
	s/, "(\d+), (\d+), (\d+), (\d+)"/, "$major,$minor,$sp,$revision"/;
        print OUTFILE;
    }
    else {
    
        print OUTFILE;
        
    }

}

close( INFILE ) or warn "Warning: Can't close $infile - $!";
close( OUTFILE ) or warn "Warning: Can't close $outfile - $!";

unless ( rename $outfile, $infile ) {

    print "Error: Updating Version for $infile failed.\n";
    exit 1;
}

exit 0;

