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

## This script should be run on all nodes with trafodion user ##

import sys
import json
from common import run_cmd, err

def run():
    dbcfgs = json.loads(dbcfgs_json)

    TRAF_DIR = '%s-%s' % (dbcfgs['traf_basename'], dbcfgs['traf_version'])

    # untar traf package
    TRAF_PACKAGE_FILE = '/tmp/' + dbcfgs['traf_package'].split('/')[-1]
    run_cmd('mkdir -p %s' % TRAF_DIR)
    run_cmd('tar xf %s -C %s' % (TRAF_PACKAGE_FILE, TRAF_DIR))
    run_cmd('rm -rf %s' % TRAF_PACKAGE_FILE)

    print 'Trafodion package extracted successfully!'

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
