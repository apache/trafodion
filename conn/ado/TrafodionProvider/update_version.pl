// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
#!/bin/perl
# usage: update_version.pl

use strict;

my ( $major, $minor, $release, $revision, $trafodion-ado, $infile )=@ARGV;
#my ($outfile);
#$version=`svn info  | grep Revision:`;
#@vlist = split / /, $version;
#$version = $vlist[1];
#chomp ( $version );
#$infile="./Trafodion.Data/Public/ProductVersion.cs";
my $outfile=$infile . ".update_version_temp";
my $version="${major}.${minor}.${release}";

#print "Updating Version in $infile...\n";
#print "\$infile=$infile\n";
#print "\$outfile=$outfile\n";
#print "\$version=$version\n";

open( INFILE, $infile ) or die "Error: Can't open $infile - $!";
open( OUTFILE, ">$outfile" ) or die "Error: Can't open $outfile - $!";

while ( <INFILE> ) {

    if ( /public const string FileVersion = | public const string VersionStr = / ) {
    
        #my @version_info = split /\./;
        #$version_info[ $#version_info ] = $version . "\"\;"; 
        s/\d+\.\d+\.\d+\.\d+/${version}.${revision}/;
    
        #print OUTFILE ( join ".", @version_info ) . "\n";
    		print OUTFILE;
    }
    elsif ( /public const string Vproc = / ) {
    
        #my @version_info = split /\[/;
        #$version_info[ $#version_info ] = $version . "])\"\;"; 
        
        #print OUTFILE ( join "[", @version_info ) . "\n";
        s/\d+\.\d+\.\d+(\D+)\d+\.\d+\.\d+(\D+)\d+/${version}${1}${trafodion-ado}${2}${revision}/;
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

