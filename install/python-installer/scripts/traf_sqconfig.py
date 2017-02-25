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
from common import run_cmd, err

def run():
    dbcfgs = json.loads(dbcfgs_json)

    nodes = dbcfgs['node_list'].split(',')
    scratch_locs = dbcfgs['scratch_locs'].split(',')

    # this script is running by trafodion user, so get sqroot from env
    TRAF_HOME = os.environ['TRAF_HOME']
    if TRAF_HOME == '': err('TRAF_HOME var is empty')
    SQCONFIG_FILE = TRAF_HOME + '/sql/scripts/sqconfig'

    core, processor = run_cmd("lscpu|grep -E '(^CPU\(s\)|^Socket\(s\))'|awk '{print $2}'").split('\n')[:2]
    core = int(core)-1 if int(core) <= 256 else 255

    lines = ['begin node\n']
    if len(nodes) == 1:
        lines.append('_virtualnodes 2\n')
    else:
        for node_id, node in enumerate(nodes):
            line = 'node-id=%s;node-name=%s;cores=0-%d;processors=%s;roles=connection,aggregation,storage\n' % (node_id, node, core, processor)
            lines.append(line)

    lines.append('end node\n')
    lines.append('\n')
    lines.append('begin overflow\n')

    for scratch_loc in scratch_locs:
        line = 'hdd %s\n' % scratch_loc
        lines.append(line)

    lines.append('end overflow\n')

    with open(SQCONFIG_FILE, 'w') as f:
        f.writelines(lines)

    print 'sqconfig generated successfully!'

    run_cmd('sqgen')

    print 'sqgen ran successfully!'

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
