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
baseurl=http://%s:%s/
enabled=1
gpgcheck=0
"""

REPO_FILE = '/etc/yum.repos.d/traflocal.repo'

def run():
    """ install Trafodion dependencies """

    dbcfgs = json.loads(dbcfgs_json)

    if dbcfgs['offline_mode'] == 'Y':
        print 'Installing pdsh in offline mode ...'

        # setup temp local repo
        repo_content = LOCAL_REPO_PTR % (dbcfgs['repo_ip'], dbcfgs['repo_port'])
        with open(REPO_FILE, 'w') as f:
            f.write(repo_content)

        run_cmd('yum install -y --disablerepo=\* --enablerepo=traflocal pdsh-rcmd-ssh pdsh')
    else:
        pdsh_installed = cmd_output('rpm -qa|grep -c pdsh')
        if pdsh_installed == '0':
            release = platform.release()
            releasever, arch = re.search(r'el(\d).(\w+)', release).groups()

            if releasever == '7':
                pdsh_pkg = 'http://mirrors.neusoft.edu.cn/epel/7/%s/p/pdsh-2.31-1.el7.%s.rpm' % (arch, arch)
            elif releasever == '6':
                pdsh_pkg = 'http://mirrors.neusoft.edu.cn/epel/6/%s/pdsh-2.26-4.el6.%s.rpm' % (arch, arch)
            else:
                err('Unsupported Linux version')

            print 'Installing pdsh ...'
            run_cmd('yum install -y %s' % pdsh_pkg)

    package_list = [
        'apr',
        'apr-util',
        'expect',
        'gzip',
        'libiodbc-devel',
        'lzo',
        'lzop',
        'openldap-clients',
        'perl-DBD-SQLite',
        'perl-Params-Validate',
        'perl-Time-HiRes',
        'sqlite',
        'snappy',
        'unixODBC-devel',
        'unzip'
    ]

    all_pkg_list = run_cmd('rpm -qa')
    for pkg in package_list:
        if pkg in all_pkg_list:
            print 'Package %s had already been installed' % pkg
        else:
            print 'Installing %s ...' % pkg
            if dbcfgs['offline_mode'] == 'Y':
                run_cmd('yum install -y --disablerepo=\* --enablerepo=traflocal %s' % pkg)
            else:
                run_cmd('yum install -y %s' % pkg)

    # remove temp repo file
    if dbcfgs['offline_mode'] == 'Y':
        os.remove(REPO_FILE)

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
