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

# For example, if you want to update the number to 2.1.0
# Please run it as :
# perl update_version.pl 2.1.0

my @version = split(/\./, $ARGV[0]);

my $major = @version[0];
my $minor = @version[1];
my $sp = @version[2];

my $revision = 0;

unless ( $major =~ /^\d+$/ && $minor =~ /^\d+$/ && $sp =~ /^\d+$/ && $revision =~ /^\d+$/ ) {
    print "Error: Invalid version on input\n";
    exit 1;

}


my @resource_files = ("TranslationDll/TranslationDll.rc",
    "drvr35adm/drvr35adm.rc",
    "drvr35msg/DrvMsg35.rc",
    "drvr35/TCPIPV4/TCPIPV4.RC",
    "drvr35/TCPIPV6/TCPIPV6.RC",
    "drvr35/drvr35.rc",
    "Drvr35Res/Drvr35Res.rc",
    "../Install/SetCertificateDirReg/SetCertificateDirReg/SetCertificateDirReg.rc",
    "drvr35/cdatasource.cpp",
    "drvr35adm/drvr35adm.h"
    );

$outfile=$infile . "\.update_version_temp";

print "Updating Version to $major.$minor.$sp\n";

sub update_file {
    my $infile = $_[0];
    print  "Update " , $infile, "\n";
    my $outfile = $infile + '.tmp';
    open( INFILE, $infile ) or die "Error: Can't open $infile - $!";
    open( OUTFILE, ">$outfile" ) or die "Error: Can't open $outfile - $!";

    while ( <INFILE> ) {
        if ( /FILEVERSION|PRODUCTVERSION/ ) {
            s/(\d+),(\d+),(\d+),(\d+)/$major,$minor,$sp,$revision/;
            print OUTFILE;
        }
        elsif( /"ProductVersion|FileVersion"/ ) {
            s/, "(\d+), (\d+), (\d+), (\d+)"/, "$major,$minor,$sp,$revision"/;
            s/, "(\d+)\.(\d+)\.(\d+)\.(\d+)"/, "$major.$minor.$sp.$revision"/;
            print OUTFILE;
        }
        elsif( /SOFTWARE\\\\ODBC\\\\ODBCINST.INI\\\\TRAF ODBC /) {
            s/(\d+)\.(\d+)/$major.$minor/;
            print OUTFILE;
        }
        elsif( /DRIVER_NAME\[\] = "TRAF ODBC / ) {
            s/(\d+)\.(\d+)/$major.$minor/;
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
}

foreach $file (@resource_files) {
    update_file $file
}

# update the version in build_os.bat
sub update_build_os {
    my $infile = $_[0];
    print  "Update " , $infile, "\n";
    my $outfile = $infile + '.tmp';
    open( INFILE, $infile ) or die "Error: Can't open $infile - $!";
    open( OUTFILE, ">$outfile" ) or die "Error: Can't open $outfile - $!";

    while ( <INFILE> ) {
        if (/TFODBC64-/) {
            s/(\d+)\.(\d+)\.(\d+)/$major.$minor.$sp/;
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
}

update_build_os "build_os.bat";

# update the version in inno setup script
sub update_innosetup_script {
    my $infile = $_[0];
    print  "Update " , $infile, "\n";
    my $outfile = $infile + '.tmp';
    open( INFILE, $infile ) or die "Error: Can't open $infile - $!";
    open( OUTFILE, ">$outfile" ) or die "Error: Can't open $outfile - $!";

    while ( <INFILE> ) {
        # update MyAppname
        if ( /MyAppName "Trafodion ODBC64/ ) {
            s/(\d+)\.(\d+)/$major.$minor/;
            print OUTFILE;
        }
        # update MyAppVersion
        elsif ( /MyAppVersion "/) {
            s/(\d+)\.(\d+)\.(\d+)/$major.$minor.$sp/;
            print OUTFILE;
        }
        # update the MyDriverName
        elsif ( /MyDriverName "TRAF ODBC / ) {
            s/(\d+)\.(\d+)/$major.$minor/;
            print OUTFILE;
        }
        # update DefaultDirName
        elsif ( /TRAF ODBC / ) {
            s/(\d+)\.(\d+)/$major.$minor/;
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
}
update_innosetup_script "../Install/win64_installer/installer.iss";

exit 0;
