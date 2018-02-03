#!/usr/bin/env python

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

### this script should be run on first node with trafodion user ###

import os
import sys
import json
from constants import DEF_HDFS_BIN, PARCEL_HBASE_LIB, PARCEL_HDFS_BIN
from common import err, run_cmd, cmd_output, run_cmd_as_user, get_sudo_prefix

def run():
    hdfs_bin = DEF_HDFS_BIN

    dbcfgs = json.loads(dbcfgs_json)
    distro = dbcfgs['distro']

    if 'CDH' in distro:
        parcel_lib = PARCEL_HBASE_LIB
        if os.path.exists(parcel_lib): hdfs_bin = PARCEL_HDFS_BIN
    elif 'APACHE' in distro:
        hdfs_bin = dbcfgs['hadoop_home'] + '/bin/hdfs'

    traf_loc = '/user/trafodion'
    traf_user = dbcfgs['traf_user']
    hdfs_user = dbcfgs['hdfs_user']
    hbase_user = dbcfgs['hbase_user']
    hbase_group = run_cmd_as_user(hdfs_user, '%s groups %s | cut -d" " -f3' % (hdfs_bin, hbase_user))

    run_cmd_as_user(hdfs_user, '%s dfsadmin -safemode wait' % hdfs_bin)
    run_cmd_as_user(hdfs_user, '%s dfs -mkdir -p %s/{trafodion_backups,bulkload,lobs} /hbase/archive' % (hdfs_bin, traf_loc))
    run_cmd_as_user(hdfs_user, '%s dfs -chown -R %s:%s /hbase/archive' % (hdfs_bin, hbase_user, hbase_user))
    run_cmd_as_user(hdfs_user, '%s dfs -chown -R %s:%s %s %s/{trafodion_backups,bulkload,lobs}' % (hdfs_bin, traf_user, traf_user, traf_loc, traf_loc))
    run_cmd_as_user(hdfs_user, '%s dfs -chmod 0755 %s' % (hdfs_bin, traf_loc))
    run_cmd_as_user(hdfs_user, '%s dfs -chmod 0750 %s/{trafodion_backups,bulkload,lobs}' % (hdfs_bin, traf_loc))
    run_cmd_as_user(hdfs_user, '%s dfs -chgrp %s %s/bulkload' % (hdfs_bin, hbase_group, traf_loc))
    run_cmd_as_user(hdfs_user, '%s dfs -setfacl -R -m user:%s:rwx /hbase/archive' % (hdfs_bin, traf_user))
    run_cmd_as_user(hdfs_user, '%s dfs -setfacl -R -m default:user:%s:rwx /hbase/archive' % (hdfs_bin, traf_user))
    run_cmd_as_user(hdfs_user, '%s dfs -setfacl -R -m mask::rwx /hbase/archive' % hdfs_bin)

    # Grant all privileges to the Trafodion principal in HBase
    if dbcfgs['secure_hadoop'] == 'Y':
        run_cmd('echo "grant \'%s\', \'RWXC\'" | %s su - %s -s /bin/bash -c "hbase shell" > /tmp/hbase_shell.out' % (traf_user, get_sudo_prefix(), hbase_user))
        has_err = cmd_output('grep -c ERROR /tmp/hbase_shell.out')
        if int(has_err):
            err('Failed to grant HBase privileges to %s' % traf_user)
        run_cmd('rm /tmp/hbase_shell.out')
# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
