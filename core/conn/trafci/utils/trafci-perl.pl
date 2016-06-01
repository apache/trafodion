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
use lib '##TRAFCI_PERL_LIB_CLASSPATH##';

$script_file= $ARGV[0];
chomp $script_file;
 
if ( length(trim($script_file)) == 0 )
{
     print "Unknown number of arguments passed\n\n";
     exit;
}

# pass extra args to script file
@ARGV2 = @ARGV[1..($#ARGV+1)];

$status = `trafci.pl  $script_file @ARGV2`;
print $status;

sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}
