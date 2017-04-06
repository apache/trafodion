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

import re
import json
import sys
import os
from common import run_cmd, cmd_output, err, Version, get_sudo_prefix
from constants import SSH_CONFIG_FILE

class Check(object):
    """ check system envs """

    def __init__(self, dbcfgs_json):
        self.dbcfgs = json.loads(dbcfgs_json)
        self.version = Version()

    def check_sudo(self):
        """ check sudo access """
        run_cmd('%s echo -n "check sudo access" > /dev/null 2>&1' % get_sudo_prefix())

    def check_ssh_pam(self):
        """ check if UsePAM is set to yes in sshd_config """
        if not cmd_output('grep "^UsePAM yes" %s' % SSH_CONFIG_FILE):
            err('\'UsePAM\' should be set to \'yes\' in %s' % SSH_CONFIG_FILE)

    def check_hbase_xml(self):
        """ check if hbase-site.xml file exists """
        hbase_xml_file = self.dbcfgs['hbase_xml_file']
        if not os.path.exists(hbase_xml_file):
            err('HBase xml file is not found')

    def check_java(self):
        """ check JDK version """
        jdk_path = self.dbcfgs['java_home']
        jdk_ver = cmd_output('%s/bin/javac -version' % jdk_path)
        try:
            jdk_ver, sub_ver = re.search(r'javac (\d\.\d).\d_(\d+)', jdk_ver).groups()
        except AttributeError:
            err('No JDK found')

        if self.dbcfgs['req_java8'] == 'Y': # only allow JDK1.8
            support_java = '1.8'
        else:
            support_java = self.version.get_version('java')

        if jdk_ver == '1.7' and int(sub_ver) < 65:
            err('Unsupported JDK1.7 version, sub version should be higher than 65')
        if jdk_ver not in support_java:
            err('Unsupported JDK version %s, supported version: %s' % (jdk_ver, support_java))

    #def check_scratch_loc(self):
    #    """ check if scratch file folder exists """
    #    scratch_locs = self.dbcfgs['scratch_locs'].split(',')
    #    for loc in scratch_locs:
    #        if not os.path.exists(loc):
    #            err('Scratch file location \'%s\' doesn\'t exist' % loc)

def run():
    PREFIX = 'check_'
    check = Check(dbcfgs_json)

    # call method
    [getattr(check, m)() for m in dir(check) if m.startswith(PREFIX)]

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
