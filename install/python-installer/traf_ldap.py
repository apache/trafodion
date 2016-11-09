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

### this script should be run on all nodes with trafodion user ###

import os
import sys
import json
from common import run_cmd, mod_file, err, TMP_DIR

def run():
    """ setup LDAP security """
    dbcfgs = json.loads(dbcfgs_json)

    DB_ROOT_USER = dbcfgs['db_root_user']
    SQ_ROOT = os.environ['MY_SQROOT']
    SQENV_FILE = SQ_ROOT + '/sqenvcom.sh'
    TRAF_AUTH_CONFIG = '%s/sql/scripts/.traf_authentication_config' % SQ_ROOT
    TRAF_AUTH_TEMPLATE = '%s/traf_authentication_conf.template' % TMP_DIR

    # set traf_authentication_config file
    change_items = {
        '{{ ldap_hosts }}': dbcfgs['ldap_hosts'],
        '{{ ldap_port }}': dbcfgs['ldap_port'],
        '{{ ldap_identifiers }}': dbcfgs['ldap_identifiers'],
        '{{ ldap_encrypt }}': dbcfgs['ldap_encrypt'],
        '{{ ldap_certpath }}': dbcfgs['ldap_certpath'],
        '{{ ldap_user }}': dbcfgs['ldap_user'],
        '{{ ldap_pwd }}': dbcfgs['ldap_pwd']
    }

    print 'Modify authentication config file'
    run_cmd('cp %s %s' % (TRAF_AUTH_TEMPLATE, TRAF_AUTH_CONFIG))
    mod_file(TRAF_AUTH_CONFIG, change_items)


    print 'Check LDAP Configuration file for errors'
    run_cmd('ldapconfigcheck -file %s' % TRAF_AUTH_CONFIG)

    print 'Verify that LDAP user %s exists' % DB_ROOT_USER
    run_cmd('ldapcheck --verbose --username=%s' % DB_ROOT_USER)
    #if not 'Authentication successful' in ldapcheck_result:
    #    err('Failed to access LDAP server with user %s' % DB_ROOT_USER)

    print 'Modfiy sqenvcom.sh to turn on authentication'
    mod_file(SQENV_FILE, {'TRAFODION_ENABLE_AUTHENTICATION=NO':'TRAFODION_ENABLE_AUTHENTICATION=YES'})

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
