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
import os
import sys
import json
import platform
from common import run_cmd, cmd_output, err

LOCAL_REPO_PTR = """
[traflocal]
name=trafodion local repo
baseurl=http://%s:%s/
enabled=1
gpgcheck=0
"""

REPO_FILE = '/etc/yum.repos.d/traflocal.repo'
EPEL_FILE = '/etc/yum.repos.d/epel.repo'

def run():
    """ install Trafodion dependencies """

    dbcfgs = json.loads(dbcfgs_json)

    node_list = dbcfgs['node_list'].split(',')

    offline = True if dbcfgs['offline_mode'] == 'Y' else False

    if offline:
        repo_content = LOCAL_REPO_PTR % (dbcfgs['repo_ip'], dbcfgs['repo_http_port'])
        with open(REPO_FILE, 'w') as f:
            f.write(repo_content)

    if not offline and not os.path.exists(EPEL_FILE):
        run_cmd('yum install -y epel-release')

    package_list = [
        'apr',
        'apr-util',
        'expect',
        'gzip',
        'libiodbc-devel',
        'lzo',
        'lzop',
        'pdsh', # epel
        'perl-DBD-SQLite',
        'perl-Params-Validate',
        'perl-Time-HiRes',
        'protobuf', # epel
        'sqlite',
        'snappy',
        'unixODBC-devel',
        'unzip'
    ]

    if dbcfgs['ldap_security'].upper() == 'Y':
        package_list += ['openldap-clients']

    all_pkg_list = run_cmd('rpm -qa')
    for pkg in package_list:
        if pkg in all_pkg_list:
            print 'Package %s had already been installed' % pkg
        else:
            print 'Installing %s ...' % pkg
            if offline:
                run_cmd('yum install -y --disablerepo=\* --enablerepo=traflocal %s' % pkg)
            else:
                run_cmd('yum install -y %s' % pkg)

    # pdsh should not exist on single node
    if len(node_list) == 1:
        cmd_output('yum remove -y pdsh')

    # remove temp repo file
    if offline: os.remove(REPO_FILE)

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
