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
#
# gdb macro to scan the seabed md's
#

define scan_md
  set $inx = -1
  set $maxmds = SB_Trans::Msg_Mgr::cv_md_table.iv_cap
  set $mdp = SB_Trans::Msg_Mgr::cv_md_table.ipp_table

  printf "\nSize of table: %u entries\n", $maxmds
  while (++$inx < $maxmds)
    if ($mdp[$inx].iv_inuse != 0)
      printf "md index :  0x%x\n", $inx
      x/s $mdp[$inx]->ip_stream
      p $mdp[$inx]->ip_where
    end
  end
  printf "\n"
end

document scan_md
scan_md

prints all the seabed md' that are in use
end
