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

### this script should be run on local node ###

import sys
import json
from threading import Thread
from constants import SSHKEY_FILE
from common import ParseJson, Remote, run_cmd, err, get_sudo_prefix

def run(user, pwd):
    """ gen ssh key on local and copy to all nodes
        copy traf package file from local to all nodes
    """
    dbcfgs = json.loads(dbcfgs_json)
    hosts = dbcfgs['node_list'].split(',')
    traf_package = dbcfgs['traf_package']

    sudo_prefix = get_sudo_prefix()
    run_cmd('%s rm -rf %s*' % (sudo_prefix, SSHKEY_FILE))
    run_cmd('%s echo -e "y" | ssh-keygen -t rsa -N "" -f %s' % (sudo_prefix, SSHKEY_FILE))

    files = [SSHKEY_FILE, SSHKEY_FILE+'.pub', traf_package]

    remote_insts = [Remote(h, user=user, pwd=pwd) for h in hosts]
    threads = [Thread(target=r.copy, args=(files, '/tmp')) for r in remote_insts]
    for thread in threads: thread.start()
    for thread in threads: thread.join()
    for r in remote_insts:
        if r.rc != 0: err('Failed to copy files to %s' % r.host)

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')

try:
    pwd = sys.argv[2]
except IndexError:
    user = pwd = ''

try:
    user = sys.argv[3]
except IndexError:
    user = ''

run(user, pwd)
