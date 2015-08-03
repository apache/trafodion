-- @@@ START COPYRIGHT @@@
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements.  See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership.  The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License.  You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied.  See the License for the
-- specific language governing permissions and limitations
-- under the License.
--
-- @@@ END COPYRIGHT @@@
#!/bin/sh

# Create an empty log file first
log=PLAN001.LOG
rm -f $log
touch $log

for i in `ls *_PLAN.LOG`
do
  qname=$(basename $i .LOG)
  plog=$i
  exp=`echo $qname | tr a-z A-Z`
  exp=$exp.EXP
  diff $exp $plog > tmp.dif 2>& 1
  if [ -s tmp.dif ]; then
    echo "" >> $log
    echo "Plan for $qname has changed:" >> $log
    cat tmp.dif >> $log
    rm -f tmp.diff
  fi
done
