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
from common import run_cmd, mod_file, err

def run():
    """ setup LDAP security """
    dbcfgs = json.loads(dbcfgs_json)

    db_root_user = dbcfgs['db_root_user']
    traf_home = os.environ['TRAF_HOME']
    traf_conf = os.environ['TRAF_CONF']
    sqenv_file = traf_home + '/sqenvcom.sh'
    traf_auth_config = '%s/.traf_authentication_config' % traf_conf
    traf_auth_template = '%s/sql/scripts/traf_authentication_config' % traf_home

    ldap_hostname = ''
    for host in dbcfgs['ldap_hosts'].split(','):
        ldap_hostname += 'LDAPHostName:%s\n' % host
    unique_identifier = ''
    for identifier in dbcfgs['ldap_identifiers'].split(';'):
        unique_identifier += 'UniqueIdentifier:%s\n' % identifier

    # set traf_authentication_config file
    change_items = {
        'LDAPHostName:.*': ldap_hostname.strip(),
        'LDAPPort:.*': 'LDAPPort:%s' % dbcfgs['ldap_port'],
        'UniqueIdentifier:.*': unique_identifier.strip(),
        'LDAPSSL:.*': 'LDAPSSL:%s' % dbcfgs['ldap_encrypt'],
        'TLS_CACERTFilename:.*': 'TLS_CACERTFilename:%s' % dbcfgs['ldap_certpath'],
        'LDAPSearchDN:.*': 'LDAPSearchDN:%s' % dbcfgs['ldap_user'],
        'LDAPSearchPwd:.*': 'LDAPSearchPwd:%s' % dbcfgs['ldap_pwd']
    }

    print 'Modify authentication config file'
    run_cmd('cp %s %s' % (traf_auth_template, traf_auth_config))
    mod_file(traf_auth_config, change_items)

    print 'Check LDAP Configuration file for errors'
    run_cmd('ldapconfigcheck -file %s' % traf_auth_config)

    print 'Verify that LDAP user %s exists' % db_root_user
    run_cmd('ldapcheck --verbose --username=%s' % db_root_user)
    #if not 'Authentication successful' in ldapcheck_result:
    #    err('Failed to access LDAP server with user %s' % db_root_user)

    print 'Modfiy sqenvcom.sh to turn on authentication'
    mod_file(sqenv_file, {'TRAFODION_ENABLE_AUTHENTICATION=.*$':'TRAFODION_ENABLE_AUTHENTICATION=YES'})

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
