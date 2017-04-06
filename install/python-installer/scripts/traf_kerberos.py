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

import sys
import re
import json
import socket
from common import run_cmd, cmd_output, err, get_sudo_prefix

def run():
    """ setup Kerberos security """
    dbcfgs = json.loads(dbcfgs_json)

    distro = dbcfgs['distro']
    admin_principal = dbcfgs['admin_principal']
    admin_passwd = dbcfgs['kdcadmin_pwd']
    kdc_server = dbcfgs['kdc_server']
    cluster_name = dbcfgs['cluster_name']
    # maxlife = dbcfgs['max_lifetime']
    # max_renewlife = dbcfgs['max_renew_lifetime']
    maxlife = '24hours'
    max_renewlife = '7days'
    kadmin_cmd = 'kadmin -p %s -w %s -s %s -q' % (admin_principal, admin_passwd, kdc_server)

    host_name = socket.getfqdn()
    traf_user = dbcfgs['traf_user']
    hdfs_user = 'hdfs'
    hbase_user = 'hbase'
    realm = re.match('.*@(.*)', admin_principal).groups()[0]
    traf_keytab_dir = '/etc/%s/keytab' % traf_user
    traf_keytab = '%s/%s.keytab' % (traf_keytab_dir, traf_user)
    traf_principal = '%s/%s@%s' % (traf_user, host_name, realm)
    hbase_principal = '%s/%s@%s' % (hbase_user, host_name, realm)

    ### setting start ###
    print 'Checking KDC server connection'
    run_cmd('%s listprincs' % kadmin_cmd)

    # create principals and keytabs for trafodion user
    principal_exists = cmd_output('%s listprincs | grep -c %s' % (kadmin_cmd, traf_principal))
    if int(principal_exists) == 0: # not exist
        run_cmd('%s \'addprinc -randkey %s\'' % (kadmin_cmd, traf_principal))
        # Adjust principal's maxlife and maxrenewlife
        run_cmd('%s \'modprinc -maxlife %s -maxrenewlife %s\' %s >/dev/null 2>&1' % (kadmin_cmd, maxlife, max_renewlife, traf_principal))

    run_cmd('mkdir -p %s' % traf_keytab_dir)

    # TODO: need skip add keytab if exist?
    print 'Create keytab file for trafodion user'
    run_cmd('%s \'ktadd -k %s %s\'' % (kadmin_cmd, traf_keytab, traf_principal))
    run_cmd('chown %s %s' % (traf_user, traf_keytab))
    run_cmd('chmod 400 %s' % traf_keytab)

    # create principals for hdfs/hbase user
    print 'Create principals for hdfs/hbase user'
    if 'CDH' in distro:
        hdfs_keytab = cmd_output('find /var/run/cloudera-scm-agent/process/ -name hdfs.keytab | head -n 1')
        hbase_keytab = cmd_output('find /var/run/cloudera-scm-agent/process/ -name hbase.keytab | head -n 1')
        hdfs_principal = '%s/%s@%s' % (hdfs_user, host_name, realm)
    elif 'HDP' in distro:
        hdfs_keytab = '/etc/security/keytabs/hdfs.headless.keytab'
        hbase_keytab = '/etc/security/keytabs/hbase.service.keytab'
        hdfs_principal = '%s-%s@%s' % (hdfs_user, cluster_name, realm)

    sudo_prefix = get_sudo_prefix()
    kinit_cmd_ptr = '%s su - %s -s /bin/bash -c "kinit -kt %s %s"'
    run_cmd(kinit_cmd_ptr % (sudo_prefix, hdfs_user, hdfs_keytab, hdfs_principal))
    run_cmd(kinit_cmd_ptr % (sudo_prefix, hbase_user, hbase_keytab, hbase_principal))

    print 'Done creating principals and keytabs'

    kinit_bashrc = """

# ---------------------------------------------------------------
# if needed obtain and cache the Kerberos ticket-granting ticket
# start automatic ticket renewal process
# ---------------------------------------------------------------
klist -s >/dev/null 2>&1
if [[ $? -eq 1 ]]; then
    kinit -kt %s %s >/dev/null 2>&1
fi

# ---------------------------------------------------------------
# Start trafodion kerberos ticket manager process
# ---------------------------------------------------------------
$TRAF_HOME/sql/scripts/krb5service start >/dev/null 2>&1
""" % (traf_keytab, traf_principal)

    traf_bashrc = '/home/%s/.bashrc' % traf_user
    with open(traf_bashrc, 'a') as f:
        f.write(kinit_bashrc)

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
