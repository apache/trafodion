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
# gdb user defined commands for use with the SE component.  Some scripts
# might not function if the corresponding symbols aren't available in
# the current stack trace.
#
# This script sources in all the available SE scripts.  By using multiple
# script files, it's easier to edit individual files and should there be
# a naming collision with other gdb commands only a subset of files can
# be sourced in.  If you decide to add new commands please also add the
# corresponding document commands to help the user.
#
source se_actt.gdb
source se_cache.gdb
source se_fcb.gdb
source se_lab.gdb
source se_lnb.gdb
source se_ocb.gdb
source se_pool.gdb
source se_rcb.gdb
source se_sscb.gdb
source se_thread.gdb
