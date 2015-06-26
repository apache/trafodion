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
	$expected_flag=IsExpected($lines[0]);
	for ($i=0; $i<=$max; $i++)
	{
		if (($lines[$i] =~ /\;/) &&	(($lines[$i]=~/create mv /i) || ($lines[$i]=~/create materialized view /i)))
		{
			$mark_flag=IsMark($line[$i]);
			$new_mv_decl='';
			@words=split(/ /,$lines[$i]);
			$words_num=$#words;
			for ($j=0; $j<=$words_num; $j++)
			{
				if($words[$j] =~/as/i)
				{
					$new_mv_decl.="\n";
					if ($expected_flag==1)
					{
						$new_mv_decl.="+>";
					}
					if ($mark_flag==1)
					{
						$new_mv_decl.="--";
					}
					$new_mv_decl.=$words[$j]."\n";
					push(@new_lines,$new_mv_decl);
					$new_mv_decl='';
					if ($expected_flag==1)
					{
						$new_mv_decl.="+>";
					}
					if ($mark_flag==1)
					{
						$new_mv_decl.="--";
					}
				}
				else
				{
					$new_mv_decl.=$words[$j];
					$new_mv_decl.=' ';
				}
			}
			push(@new_lines,$new_mv_decl);
		}
		else
		{
			push(@new_lines,$lines[$i]);
		}
	}
	$file.='_test';
	open (INFO,">$file");
	print INFO @new_lines;
	close(INFO);
	$file = <STDIN>;
	chop $file;
	splice(@new_lines);
}

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
