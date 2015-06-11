#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2002-2015 Hewlett-Packard Development Company, L.P.
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
#######################################################################
#------------ Main ---------------------

$file = <STDIN>;
chop $file;
splice(@new_lines);
while ($file ne "EOF")
{
	open(INFO,"$file");
	@lines=<INFO>;
	close(INFO);
	$max = $#lines;
	$expected=IsExpected($lines[0]);
	for ($i=0; $i<=$max; $i++)
	{
		$flag=0;
		$do_nothing=0;
		push(@new_lines,$lines[$i]);
		$table_type=ScanLine($lines[$i]); #1-table, 2-mv, 0-otherwise
		if ($table_type!=0) # create table/mv has been found
		{
			if ($lines[$i] =~ /mvs allowed/i)
			{
				$do_nothing=1;
			}
			if (IsMark($lines[$i])==1)
			{
				$flag=1;
			}
			$TableName = GetTableName($lines[$i]); #assumption: create table <table's name> are always in the same row
			chomp $TableName;
			if ($lines[$i] !~ /\;/) # a special case of a table creation in one line in the script - don't run the do-until loop
			{
				do 
				{
					$i++;
					if ($lines[$i] =~ /mvs allowed/i)
					{
						$do_nothing=1;
					}
					push(@new_lines,$lines[$i]);
				}
				until ($lines[$i] =~ /\;/);
			}
			if ($do_nothing==0)
			{
				if ($expected==1) # we deal with expected file
				{
					$new_command='>>';
				}
				else
				{
					$new_command='';
				}
				if ($flag==1) #the table's creation is under remark (--)
				{
					$new_command='-- ';
					$flag=0;
				} 
				if ($table_type==1)
				{
					$new_command.="ALTER TABLE ";
				}
				else
				{	
					$new_command.="ALTER MV ";
				}
				$new_command.=$TableName." attribute all mvs allowed;\n";
				if ($expected==1) # push the -- sql operation comlete
				{
					push(@new_lines,$lines[$i+1]);
					push(@new_lines,$lines[$i+2]);
					$i+=2;					
				}
				push(@new_lines,$new_command);
			}
		}
	}
	open (INFO,">$file");
	print INFO @new_lines;
	close(INFO);
	$file = <STDIN>;
	chop $file;
	splice(@new_lines);
}

#---------------------------------------

sub IsExpected
{
	if ($_[0] =~ /^>>/)
	{
		return 1;
	}
	return 0;
}

sub IsMark
{
	if (($_[0] =~ /^\-\-*/)||($_[0] =~ /^>>\-\-/))
	{
		return 1;
	}
	return 0;
}

sub ScanLine
{
	if ($_[0] =~ /create table/i)
	{
		return 1;
	}
	if ($_[0] =~ /create mv /i)
	{
		return 2;
	}
	return 0;
}



sub GetTableName
{
	@command = split(/ /,$_[0]);
	
	$j=0;
	while (($command[$j] !~ /table/i) && ($command[$j] !~ /mv/i))
	{
		$j++;
	}
	@ret_val = split(/\(/,$command[$j+1]);
	@ret_val_fin = split(/\n/,$ret_val[0]);
	return $ret_val_fin[0];
}

