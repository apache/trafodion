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
#files that should be changed manually - TestMV071,502,503,519
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
		if (ScanLine($lines[$i])==1) # create mv has been found
		{
			$title=GetTableName($lines[$i]);
			$start=$i;
			if (IsMark($lines[$i])==1) # checks wether the mv's creation is under --
			{
				$flag=1;
			}
			$exist=0;
			$dec_exist=0;
			$as_line=-1;
			$attr_line=-1;
			$initialized_on_line=-1;
			while ($lines[$i] !~ /\;/) 
			{
				if (($lines[$i]=~/initialized on/i)||($lines[$i]=~/initialize on/i))
				{
					$exist=1;
					$initialized_on_line=$i;
				}
				if (($lines[$i]=~/as/i)&&(($lines[$i]=~/select/i)||($lines[$i+1]=~/select/i))&&($as_line==-1))
				{
					$as_line=$i;
				}
				if (($lines[$i]=~/mvattribute/i)||($lines[$i]=~/store by/i))
				{
					$attr_line=$i;
				}
				$i++;
			}
			$finish=$i;
			if ($exist==1)
			{
				for ($j=$start; $j<=$finish; $j++)
				{
					if ($expected==1)
					{
						$new_command='+>';
					}
					else
					{
						$new_command='';
					}
					if ($flag==1)
					{
						$new_command.="--";
					}
					$new_command.="initialized on refresh\n";

					if (($initialized_on_line>-1)&&($j==$initialized_on_line))
					{
						push(@new_lines,$new_command);
					} 
					else
					{
					
						if (($initialized_on_line==-1)&&($j==$as_line))
						{	
							push(@new_lines,$new_command);
							push(@new_lines,$lines[$j]);
						}
						else 
						{
							push(@new_lines,$lines[$j]);
						}
					}
				} #for 
			} #if exist==1
			else
			{
				for ($j=$start; $j<=$finish; $j++)
				{
					push(@new_lines,$lines[$j]);
				}
			}
			$new_command="refresh ".$title.";\n";
			push(@new_lines,$new_command);
		} # end of 'create mv has been found'
		else 
		{	#no MV relevant
			push(@new_lines,$lines[$i]);
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
	if (($_[0] =~ /create mv /i)||($_[0] =~ /create materialized view/i))
	{
		return 1;
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

