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

import sys
import time
import os
import json
from common import cmd_output, run_cmd, err

def run():
    """ start trafodion instance """
    dbcfgs = json.loads(dbcfgs_json)

    print 'Starting trafodion'
    traf_home = os.environ['TRAF_HOME']
    if os.path.exists('%s/sql/scripts/trafstart' % traf_home):
        run_cmd('trafstart')
    else:
        run_cmd('sqstart')

    # set a uniq file name
    tmp_file = '/tmp/initialize.out.' + str(int(time.time()))
    print 'Initialize trafodion'
    run_cmd('echo "initialize trafodion;" | sqlci > %s' % tmp_file)
    init_output = cmd_output('cat %s' % tmp_file)
    # error 1392, 1395
    if '1392' in init_output or '1395' in init_output:
        run_cmd('echo "get version of metadata;" | sqlci > %s' % tmp_file)
        meta_current = cmd_output('grep \'Metadata is current\' %s | wc -l' % tmp_file)
        if meta_current != "1":
            print 'Initialize trafodion, upgrade'
            run_cmd('echo "initialize trafodion, upgrade;" | sqlci > %s' % tmp_file)

        # update system library procedures and functions
        run_cmd('echo "initialize trafodion, upgrade library management;" | sqlci > %s' %tmp_file)
        library_output = cmd_output('cat %s' % tmp_file)
        if 'ERROR' in library_output:
           err('Failed to initialize trafodion, upgrade library management:\n %s' % library_output)

    # other errors
    elif 'ERROR' in init_output:
        err('Failed to initialize trafodion:\n %s' % init_output)


    run_cmd('rm -rf %s' % tmp_file)
    if dbcfgs['ldap_security'] == 'Y':
        run_cmd('echo "initialize authorization; alter user DB__ROOT set external name \\\"%s\\\";" | sqlci > %s' % (dbcfgs['db_root_user'], tmp_file))

        secure_output = cmd_output('cat %s' % tmp_file)
        if 'ERROR' in secure_output:
            err('Failed to setup security for trafodion:\n %s' % secure_output)

    run_cmd('rm -rf %s' % tmp_file)
    if os.path.exists('%s/sql/scripts/connstart' % traf_home):
        run_cmd('connstart')

    print 'Start trafodion successfully.'

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
