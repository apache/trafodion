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
import json
from common import cmd_output, run_cmd, err

def run():
    """ start trafodion instance """
    dbcfgs = json.loads(dbcfgs_json)

    print 'Starting trafodion'
    run_cmd('sqstart')

    tmp_file = '/tmp/initialize.out'
    if dbcfgs.has_key('upgrade') and dbcfgs['upgrade'].upper() == 'Y':
        print 'Initialize trafodion upgrade'
        run_cmd('echo "initialize trafodion, upgrade;" | sqlci > %s' % tmp_file)
        init_output = cmd_output('cat %s' % tmp_file)
        if 'ERROR' in init_output:
            err('Failed to upgrade initialize trafodion:\n %s' % init_output)
    else:
        print 'Initialize trafodion'
        run_cmd('echo "initialize trafodion;" | sqlci > %s' % tmp_file)
        init_output = cmd_output('cat %s' % tmp_file)
        # skip error 1392
        # ERROR[1392] Trafodion is already initialized on this system. No action is needed.
        if 'ERROR' in init_output and not '1392' in init_output:
            err('Failed to initialize trafodion:\n %s' % init_output)

    if dbcfgs['ldap_security'] == 'Y':
        run_cmd('echo "initialize authorization; alter user DB__ROOT set external name \"%s\";" | sqlci > %s' % (dbcfgs['db_root_user'], tmp_file))
        if dbcfgs.has_key('db_admin_user'):
            run_cmd('echo "alter user DB__ADMIN set external name \"%s\";" | sqlci >> %s' % (dbcfgs['db_admin_user'], tmp_file))

        secure_output = cmd_output('cat %s' % tmp_file)
        if 'ERROR' in secure_output:
            err('Failed to setup security for trafodion:\n %s' % secure_output)

    run_cmd('rm %s' % tmp_file)
    print 'Start trafodion successfully.'

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
