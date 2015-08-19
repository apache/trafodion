#!/bin/sh
#######################################################################
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
#######################################################################
#
# Help text
#
HELP="\n\
Usage: $0 [-mv | -q] [-i infile] [-o outfile] [-n name] [-h] \n\
\n\
   \t This shell script strips off the mxci parts of a log file with \n\
   \t MV descriptors.\n\n"

infile=
outfile=
name=
keyword=MV
type=0

#
# function to add in the Publish descriptor beginning XML
#

function begin_publish_old {

echo "<Publish TS='912111004606475327'>
   <MVName>
     $name
   </MVName>
   <Update op='Alter' hasIgnoreChanges='0'>
   </Update>"
return
}	# End of function begin_publish

function begin_publish {

echo "<Publish TS='0'>"
return
}	# End of function begin_publish

#
# function to add in the Publish descriptor beginning XML to outfile
#

function begin_publish_to_outfile_old {

echo "<Publish TS='912111004606475327'>
   <MVName>
     $name
   </MVName>
   <Update op='Alter' hasIgnoreChanges='0'>
   </Update>" > $outfile
return
}	# End of function begin_publish_to_outfile

function begin_publish_to_outfile {

echo "<Publish TS='0'>"  > $outfile
return
}	# End of function begin_publish_to_outfile

while [ $# -gt 0 ] ; do
  case $1 in
    -mv)     keyword=MV
             type=0
             ;;
    -q)      keyword=Query
             type=1
             ;;
    -i)      shift
             infile=$1
             ;;
    -o)      shift
             outfile=$1
             ;;
    -n)      shift
             name=$1
             ;;           
    **)      echo $HELP
             exit;
             ;;
  esac
  shift
done

case $type in
  0)

  if [ -z "$outfile" ]; then
    begin_publish
    echo $keyword
    awk "BEGIN    { p = 0 }
          /<$keyword>/   { p = 1 }
          { if (p > 0) print }
          /<\/$keyword>/ { p = 0 }" $infile
    echo "</Publish>"
  else
    begin_publish_to_outfile
    echo $keyword
    awk "BEGIN    { p = 0 }
          /<$keyword>/   { p = 1 }
          { if (p > 0) print }
          /<\/$keyword>/ { p = 0 }" $infile >>$outfile
    echo "<Update op='Refresh' TS='1'> </Update>" >>$outfile
    echo "</Publish>" >>$outfile
  fi
  ;;

 1)

  if [ -z "$outfile" ]; then
    echo $keyword
    awk "BEGIN    { p = 0 }
          /<$keyword>/   { p = 1 }
          { if (p > 0) print }
          /<\/$keyword>/ { p = 0 }" $infile
  else
    echo $keyword
    awk "BEGIN    { p = 0 }
          /<$keyword>/   { p = 1 }
          { if (p > 0) print }
          /<\/$keyword>/ { p = 0 }" $infile >>$outfile
  fi
  ;;

  *)

  echo "Unrecognized MV or Query type"
  ;;
esac
