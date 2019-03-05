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

import os
import sys
import re
import json
from constants import DEF_HBASE_HOME, TRAF_SUDOER_FILE, TRAF_CFG_FILE, TRAF_CFG_DIR
from common import err, cmd_output, run_cmd, get_default_home

def run():
    dbcfgs = json.loads(dbcfgs_json)

    home_dir = get_default_home()
    if dbcfgs.has_key('home_dir'):
        home_dir = dbcfgs['home_dir']

    traf_user = dbcfgs['traf_user']
    traf_dirname = dbcfgs['traf_dirname']
    traf_home = '%s/%s/%s' % (home_dir, traf_user, traf_dirname)
    traf_var = dbcfgs['traf_var']
    traf_log = dbcfgs['traf_log']

    traf_ver = dbcfgs['traf_version']
    distro = dbcfgs['distro']
    traf_lib_path = traf_home + '/export/lib'
    scratch_locs = dbcfgs['scratch_locs'].split(',')

    SUDOER_CFG = """
## Allow trafodion id to run commands needed for backup and restore
%%%s ALL =(hbase) NOPASSWD: %s/bin/hbase"
""" % (traf_user, DEF_HBASE_HOME)

    ### kernel settings ###
    run_cmd('sysctl -w kernel.pid_max=65535 2>&1 > /dev/null')
    run_cmd('echo "kernel.pid_max=65535" >> /etc/sysctl.conf')

    ### copy trafodion bashrc ###
    bashrc_template = '%s/sysinstall/home/trafodion/.bashrc' % traf_home
    bashrc_file = '%s/%s/.bashrc' % (home_dir, traf_user)
    # backup orig bashrc
    if os.path.exists(bashrc_file):
        run_cmd('cp -f %s %s.bak' % (bashrc_file, bashrc_file))
    run_cmd('cp -f %s %s' % (bashrc_template, bashrc_file))
    run_cmd('chown -R %s:%s %s*' % (traf_user, traf_user, bashrc_file))
    # copy default config files
    run_cmd('cp -rf %s/conf/* %s/' % (traf_home, TRAF_CFG_DIR))
    run_cmd('chown -R %s:%s %s' % (traf_user, traf_user, TRAF_CFG_DIR))

    ### copy init script ###
    init_script = '%s/sysinstall/etc/init.d/trafodion' % traf_home
    if os.path.exists(init_script):
        run_cmd('cp -rf %s /etc/init.d/' % init_script)
        run_cmd('chkconfig --add trafodion')
        run_cmd('chkconfig --level 06 trafodion on')

    ### create and set permission for scratch file dir ###
    for loc in scratch_locs:
        # expand any shell variables
        locpath = cmd_output('source %s ; echo %s' % (TRAF_CFG_FILE,loc))
        if not os.path.exists(locpath):
            run_cmd('mkdir -p %s' % locpath)
            run_cmd('chown %s %s' % (traf_user,locpath))
    # var,log locations
    run_cmd('mkdir -p %s %s' % (traf_var,traf_log))
    run_cmd('chown %s %s %s' % (traf_user,traf_var,traf_log))

    ### copy jar files ###
    hbase_lib_path = dbcfgs['hbase_lib_path']
    if 'APACHE' in distro:
        distro += dbcfgs['hbase_ver']

    distro, v1, v2 = re.search(r'(\w+)-*(\d)\.(\d)', distro).groups()
    if distro == 'CDH':
        if v2 == '6': v2 = '5'
        if v2 == '8': v2 = '7'
    elif distro == 'HDP':
        if int(v2) > 3: v2 = '3'

    hbase_trx_jar = 'hbase-trx-%s%s_%s-%s.jar' % (distro.lower(), v1, v2, traf_ver)
    traf_hbase_trx_path = '%s/%s' % (traf_lib_path, hbase_trx_jar)
    hbase_trx_path = '%s/%s' % (hbase_lib_path, hbase_trx_jar)
    if not os.path.exists(traf_hbase_trx_path):
        err('Cannot find HBase trx jar \'%s\' for your Hadoop distribution' % hbase_trx_jar)

    # reinstall mode, check if existing trx jar doesn't match the new trx jar file
    if dbcfgs.has_key('reinstall') and dbcfgs['reinstall'].upper() == 'Y':
        if not os.path.exists(hbase_trx_path):
            err('The trx jar \'%s\' doesn\'t exist in hbase lib path, cannot do reinstall, please do regular install' % hbase_trx_jar)
    else:
        # remove old trx and trafodion-utility jar files
        run_cmd('rm -rf %s/{hbase-trx-*,trafodion-utility-*}' % hbase_lib_path)

        # copy new ones
        run_cmd('cp %s %s' % (traf_hbase_trx_path, hbase_lib_path))
        run_cmd('cp %s/trafodion-utility-* %s' % (traf_lib_path, hbase_lib_path))

    # set permission
    run_cmd('chmod +r %s/{hbase-trx-*,trafodion-utility-*}' % hbase_lib_path)

    if dbcfgs['dcs_ha'] == 'Y':
        # set trafodion sudoer file for specific cmds
        SUDOER_CFG += """
## Trafodion Floating IP commands
Cmnd_Alias IP = /sbin/ip
Cmnd_Alias ARP = /sbin/arping

## Allow Trafodion id to run commands needed to configure floating IP
%%%s ALL = NOPASSWD: IP, ARP
""" % traf_user

    ### write trafodion sudoer file ###
    with open(TRAF_SUDOER_FILE, 'w') as f:
        f.write(SUDOER_CFG)

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
