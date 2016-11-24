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

### this script should be run on all nodes with sudo user ###

import sys
import json
import socket
from common import MODCFG_FILE, ParseJson, ParseXML, err, run_cmd

def run():
    dbcfgs = json.loads(dbcfgs_json)
    if 'APACHE' in dbcfgs['distro']:
        modcfgs = ParseJson(MODCFG_FILE).load()
        MOD_CFGS = modcfgs['MOD_CFGS']

        hdfs_xml_file = dbcfgs['hdfs_xml_file']
        hbase_xml_file = dbcfgs['hbase_xml_file']

        hbasexml = ParseXML(hbase_xml_file)
        for key, value in MOD_CFGS['hbase-site'].items():
            hbasexml.add_property(key, value)
        hbasexml.write_xml()

        hdfsxml = ParseXML(hdfs_xml_file)
        for key, value in MOD_CFGS['hdfs-site'].items():
            hdfsxml.add_property(key, value)
        hdfsxml.write_xml()

        print 'Apache Hadoop modification completed'
        first_node = dbcfgs['first_rsnode']
        local_host = socket.gethostname()
        if first_node in local_host:
            hadoop_home = dbcfgs['hadoop_home']
            hbase_home = dbcfgs['hbase_home']
            # stop
            run_cmd(hbase_home + '/bin/stop-hbase.sh')
            run_cmd(hadoop_home + '/sbin/stop-dfs.sh')
            # start
            run_cmd(hadoop_home + '/sbin/start-dfs.sh')
            run_cmd(hbase_home + '/bin/start-hbase.sh')

            print 'Apache Hadoop restart completed'
    else:
        print 'no apache distribution found, skipping'

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
