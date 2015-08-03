#!/usr/bin/perl
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
use Encode;

$output_file="unixmsg.h";
$data_file="tdm.tmp";

#use external function iconv to convert file
#`iconv -f UTF-16 -t ASCII tdm_odbcdrvmsg.mc`
#`ls`

system("iconv -f UTF-16 -t ASCII tdm_odbcdrvmsg.mc > $data_file");
system("dos2unix $data_file");

open(OUTFILE,">$output_file") || die("Could not open file $output_file!");
open(INFILE,"<$data_file") || die("Could not open file $data_file!");
my $test = 0;

$max_id = 0;

# determine maximum message id!
while(<INFILE>)
{
        my($line) = $_;
        chomp($line);
        @temp=split(/\=/,$line);
        if (($temp[0] =~  m/^MessageID/))
        {
                if ($temp[1] > $max_id)
		{$max_id = $temp[1]}
	}
}
close(INFILE);

print ("MAX_MSG_ID: $max_id\n");

# create initial structre for array
print OUTFILE "\#include \"mxomsg.h\"\n\#define DWORD long\n\n";

print OUTFILE "struct msg\n\{\n\tchar sever\[10\]\;\n\tshort id\;\n\tchar text\[250\]\;\n\} msgarray\[\] \= \{\n";


# reopen file for reading
open(INFILE,"<$data_file") || die("Could not open file $data_file!");

$current_line = 1;
# now read the valeus from the input file
while(<INFILE>)
{
	my($line) = $_;	
#	Encode::from_to($line, "UTF16-BE", "ASCII");

	#my($line) = decode("UTF16-BE", $_);
	chomp($line);
	$line =~ tr/[a-z]/[A-Z]/;
	@temp=split(/\=/,$line);
	if (($temp[0] =~  m/^MESSAGEID/))
	{
		$test = 1;
		$messageid = $temp[1];
	}
        elsif (($temp[0] =~  m/^SEVERITY/))
                {($severity) = $temp[1]}
	elsif (($temp[0] =~  m/^FACILITY/)) 
		{$fac = $temp[1]}
	elsif (($temp[0] =~  m/^SYMBOLICNAME/))
        	{$symname = $temp[1]}
        elsif (($temp[0] =~  m/^LANGUAGE/))
        	{$lang = $temp[1]}
	elsif (($test == 1) && ( $line !~  m/^\./ ))
	{
		$text = $temp[0];
		$text =~ tr/\%/0/;
	}

	if ( $line =~  m/^\./ )
	{	
		for ($count=$current_line; $count<$messageid; $count++)
		{
			$current_line++;
			print OUTFILE "\t\"\",0,\"\",\n";
		}
		print OUTFILE "\t\"$severity\", $symname, \"$text\",\/\/ $messageid\n";
		$current_line++;
		$test = 0;
	}
	
}

# now to close out the header file
print OUTFILE "\};\n";

close(OUTFILE);

