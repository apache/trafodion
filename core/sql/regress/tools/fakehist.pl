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
# This Perl script is used to create SQL commands
# to create a fake histogram for an existing column in a table.
# input: table name, column name, histogram data file name

$opt = -1;

foreach $arg(@ARGV) {
#   printf("%s ", $arg);

   if ( $arg =~ /\-t/ ) {
      $opt = 0;
      next;
   }
   if ( $arg =~ /\-c/ ) {
      $opt = 1;
      next;
   }
   if ( $arg =~ /\-f/ ) {
      $opt = 2;
      next;
   }

    if ( $opt == 0 ) {
      #printf("-t %s, ", $arg);
      $opt_t=$arg;
      $opt = -1;
      next;
    }

    if ( $opt == 1 ) {
      #printf("-c %s, ", $arg);
      $opt_c=$arg;
      $opt = -1;
      next;
    }

    if ( $opt == 2 ) {
      #$printf("-f %s, ", $arg);
      $opt_f=$arg;
      $opt = -1;
      next;
    } else {
      #printf("%s, ", $arg);
    }
}

if ( $opt_t eq "" || $opt_c eq "" || $opt_f eq "" ) {
   die "
  Usage: perl fakehist.pl -t table -c column -f file 

    table: a fully qualified table name (no . character in the name)
    column: a column name
    file: the name of a histogram data file with content in following format:

    -- numeric data types:
    No.   Rows    UEC     Boundary
    0     0       0       (1)
    1     652173  652173  (652173)
    2     652173  652173  (1304346)

    -- ascii data types:
    No.   Rows    UEC     Boundary
    0     0       0       (''abc'')
    1     652173  652173  (''abc'')
    2     654170  653220  (''def'')

  Examples:

    perl fakehist -t \"cat.sch.t\" -c a -f \"t.a.fakehist\"
    perl fakehist -t c.s.t -c a -f data 
\n";
}

chop($sysname=`uname`);
if ( $sysname eq "Linux" ) {
  $syscat="HP_SYSTEM_CATALOG";
} elsif ( $sysname eq "Windows_NT" ) {
  $syscat="HP_SYSTEM_CATALOG";
} elsif ( $sysname eq "NONSTOP_KERNEL" ) {
  chop($sysName=uc(`uname -n`));
  $syscat="NONSTOP_SQLMX_".$sysName;
} else {
  die "Can only run on Linux, Windows, or NSK.\n";
}

@nameparts= split(/\./, $opt_t);
if ( scalar(@nameparts) != 3 ) {
  die "The table name must be fully qualified (catalog.schema.table).\n";
}
$catName=uc(@nameparts[0]);
$schName=uc(@nameparts[1]);
$tabName=uc(@nameparts[2]);
$colName=uc($opt_c);
$fileName=$opt_f;

# start to create input for mxci
printf("control query default POS 'OFF';\n");
printf("control query default INSERT_VSBB 'OFF';\n");
printf("set schema %s.%s;\n", $catName, $schName);

printf("update statistics for table %s on (%s) clear;\n", $tabName, $colName);
printf("update statistics for table %s on (%s);\n", $tabName, $colName);

printf(
"-- Store catalog ID of catalog CAT in the CATID table. --\ 
insert into FAKE_CATID \
  select distinct cat_uid \
    from %s.SYSTEM_SCHEMA.CATSYS \
  where cat_name = '%s';\n", $syscat, $catName); 
printf(
"-- Store schema ID of schema SCH in the FAKE_SCHID table. --\
insert into FAKE_SCHID \
  select distinct schema_uid \
    from %s.SYSTEM_SCHEMA.SCHEMATA, FAKE_CATID \
  where cat_uid = cat_id \
    and schema_name = '%s';\n", $syscat, $schName);
printf(
"-- The following prepared queries are used repeatedly  -- \
-- for updating histograms and histogram intervals.  --\n");
printf(
"-- Clear out and store in FAKE_TABID table.   --\
prepare clearTABID from delete from FAKE_TABID; \
prepare insertTabID from \
  insert into FAKE_TABID \
    select object_uid \
      from HP_DEFINITION_SCHEMA.OBJECTS, FAKE_SCHID \
     where     object_type = 'BT' \
           and schema_uid = sch_id
           and object_name = ?a;\n");
printf(
"-- Lock down the table in the OBJECTS metadata for the --\
-- duration of the transaction to protect the schema.  --\
prepare lockTabId from \
select 'FUNKY_OPT_UNIQUE', object_uid, object_name \
  from HP_DEFINITION_SCHEMA.OBJECTS \
 where object_uid = (select tab_id from FAKE_TABID) \
   for serializable access in exclusive mode ; \
-- Clear out FAKE_HISTID table.  --\
prepare clearHISTID from delete from FAKE_HISTID;   \
-- Clear out HISTGTMP table.  --\
prepare clearHistGrpTemp from delete from FAKE_HISTGTMP; \
-- Clear out FAKE_COLNUM table.  --
prepare clearCOLNUM from delete from FAKE_COLNUM; \
-- Store column number of a specified column in FAKE_COLNUM table. --\
prepare insertColNum from \
  insert into FAKE_COLNUM \
    select column_number \
      from HP_DEFINITION_SCHEMA.COLS, FAKE_TABID \
     where     object_uid = tab_id \
           and column_name = ?a;\n");   
printf(
"-- Store histogram id for column of interest in the FAKE_HISTID table. --\
prepare insertHistID from \
  insert into FAKE_HISTID \
    select histogram_id \
      from %s.%s.HISTOGRAMS, FAKE_TABID, FAKE_COLNUM \
     where     table_uid = tab_id \
           and column_number = col_num \
           and colcount      = 1;\n", $catName, $schName);
printf(
"-- Update histogram for column of interest  --\
-- to reflect the newly inserted intervals. --\
prepare updateHistogram from \
  update %s.%s.HISTOGRAMS \
    set    interval_count = ?a, \
           rowcount       = ?b, \
           total_uec      = ?c, \
           stats_time     = ?d, \
           low_value      = translate(cast(?e as varchar(500)) using iso88591ToUcs2), \
           high_value     = translate(cast(?f as varchar(500)) using iso88591ToUcs2) \
   where     table_uid     = (select tab_id from FAKE_TABID)  \
         and column_number = (select col_num from FAKE_COLNUM) \
         and colcount      = 1 \
         and histogram_id  = (select hist_id from FAKE_HISTID);\n", $catName, $schName);
printf(
"-- Clear all histogram intervals for a table of interest. --\
prepare clearIntervals from \
  delete from %s.%s.HISTOGRAM_INTERVALS \
     where     table_uid    = (select tab_id from FAKE_TABID) \
           and histogram_id = (select hist_id from FAKE_HISTID);\n", $catName, $schName);		   
printf(
"-- Insert a specified interval into the histogram intervals table. --\
prepare insertInterval from \
  insert into %s.%s.HISTOGRAM_INTERVALS \
     values ( (select tab_id from FAKE_TABID),   -- Table ID of current table \
              (select hist_id from FAKE_HISTID), -- Histogram ID of specified column \
              ?a,                           -- Interval number \
              ?b,                           -- Interval row count \
              ?c,                           -- Interval unique entry count (UEC) \
              translate(cast(?d as varchar(500)) using iso88591ToUcs2), \
              0, 0, 0, 0, 0, _ucs2' ', _ucs2' ' \
             );\n", $catName, $schName);
printf(
"-- Display general histogram for column of interest. --\
prepare displayHist from \
   select stats_time, \
          interval_count, \
          rowcount, \
          total_uec, \
          substring(translate(low_value using Ucs2Toiso88591),1,40) as \"Low Value\", \
          substring(translate(high_value using Ucs2Toiso88591),1,40) as \"High Value\", \
          'FUNKY_OPT_UNIQUE', \
          'Table/Histogram ID', \
          table_uid, \
          histogram_id \
      from %s.%s.HISTOGRAMS \
     where     table_uid    = (select tab_id from FAKE_TABID)  \
           and histogram_id = (select hist_id from FAKE_HISTID);\n", $catName, $schName);
printf(
"prepare displayIntervals from \
   select interval_number, \
          interval_rowcount,  \
          interval_uec, \
          substring(translate(interval_boundary using Ucs2Toiso88591),1,40) as \"Interval Boundary\", \
          'FUNKY_OPT_UNIQUE', \
          'Table/Histogram ID', \
          table_uid, \
          histogram_id \
      from %s.%s.HISTOGRAM_INTERVALS \
     where     table_uid    = (select tab_id from FAKE_TABID)   \
           and histogram_id = (select hist_id from FAKE_HISTID) \
     order by interval_number;\n", $catName, $schName);
printf(
"--  Fake statistics for a table. -- \
begin work; \
-- Clear out old table ID and store table         -- \
execute clearTABID; \
set param ?a %s; \
execute insertTabId;\n", $tabName);
printf(
"-- Lock down table for duration of update         -- \
execute lockTabId;\n");
printf(
"-- Clear out and store column number --\
execute clearColNum; \
set param ?a %s; \
execute insertColNum;\n", $colName);
printf(
"-- Clear out and store histogram ID --\
execute clearHistId; \
execute insertHistId; \
-- Delete all histogram intervals for the column --\
execute clearIntervals;\n"); 

# read histogram data from a file and 
# prepare scripts to create histogram intervals
open(FILE, $fileName);
$intCount=0;$rowCount=0;$uecCount=0;
$lowValue="";$highValue="";
$j=0;

while (defined($line=<FILE>)) {
#print "<", $line;
  $line =~ s/^\s*(.*)/$1/;
#print ">", $line;
  @tokens = split(/ +/, $line, 4); # split up to 4 fields to avoid getting 
                                   # partial char-typed boundary values because
                                   # of embedded space characters
  if (@tokens[0] =~ /^\d+$/) {
    $intCount=@tokens[0];
    if ($intCount==0) { 
      $lowValue=@tokens[3]; 
      chomp($lowValue);
    }
    $rowCount+=@tokens[1];
    $uecCount+=@tokens[2];
    $highValue=@tokens[3];
    chomp($highValue);

    @intervals[$j] =  @tokens[0];
    @rowcounts[$j] =  @tokens[1];
    @uecs[$j]      =  @tokens[2];
    @boundaries[$j] = $highValue;


    $j++;

    if ( $j == 100 ) {
       insertTheseRows($j);
       $j=0;
    }
  }
}
      
insertTheseRows($j);

printf("
update histogram_intervals set  \
table_uid = (select tab_id from FAKE_TABID),    \
histogram_id = (select hist_id from FAKE_HISTID) \ 
where table_uid = 0;\n
");

close(FILE);

$timeStamp="2006-05-04 00:00:00.0";
printf(
"-- Update histogram for column to                                    --\
-- reflect the newly inserted intervals.                                --\
set param ?a %d;                     -- Interval count \
set param ?b %d;               -- Total row count \
set param ?c %d;               -- Total unique entry count (UEC) \
set param ?d  '%s';               -- Histogram timestamp \
set param ?e  '%s';               -- Low Value \
set param ?f  '%s';               -- High Value \
execute updateHistogram;\n", $intCount, $rowCount, $uecCount, $timeStamp, $lowValue, $highValue);

printf(
"-- Display histogram information to verify that it was faked properly --\
execute displayHist;\
commit work;\
delete from FAKE_CATID;\
delete from FAKE_SCHID;\
delete from FAKE_TABID;\
delete from FAKE_COLNUM;\
delete from FAKE_HISTID;\n");


# insert $n rows into the interval table
sub insertTheseRows {
  my($n) = @_;

  printf("insert into %s.%s.HISTOGRAM_INTERVALS values \n", $catName, $schName);

  for ($i=0; $i<$n; $i++) {
      #        tabid hid int  rc  uec  hb
      printf("(0, 0,  %d,  %d,  %d,  \
translate(cast('%s' as varchar(500)) using iso88591ToUcs2), \
0, 0, 0, 0, 0, _ucs2' ', _ucs2' ' \
)",
             @intervals[$i], @rowcounts[$i], @uecs[$i], @boundaries[$i]
            );

      if ( $i < $n-1 ) {
         printf(",\n");
      }
  }
  printf(";\n");
}
