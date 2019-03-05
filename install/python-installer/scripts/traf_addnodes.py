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

### this script should be run on new nodes with sudo user ###

import os
import sys
import re
import json
import socket
from constants import DEF_HBASE_HOME, TRAF_SUDOER_FILE, TRAF_CFG_FILE, TRAF_CFG_DIR
from common import err, cmd_output, run_cmd, get_default_home, mod_file, write_file

def run():
    dbcfgs = json.loads(dbcfgs_json)

    if not os.path.exists(dbcfgs['hbase_lib_path']):
        err('Cannot find HBase lib folder')
    if not os.path.exists(dbcfgs['java_home']):
        err('Cannot find Java, please set the JAVA_HOME on the new nodes to: %s' % dbcfgs['java_home'])

    home_dir = get_default_home()
    if dbcfgs.has_key('home_dir'):
        home_dir = dbcfgs['home_dir']

    traf_user = dbcfgs['traf_user']
    traf_home = dbcfgs['traf_home']
    traf_user_dir = '%s/%s' % (home_dir, traf_user)

    traf_ver = dbcfgs['traf_version']
#    scratch_locs = dbcfgs['scratch_locs'].split(',')

    SUDOER_CFG = """
## Allow trafodion id to run commands needed for backup and restore
%%%s ALL =(hbase) NOPASSWD: %s/bin/hbase"
""" % (traf_user, DEF_HBASE_HOME)

    ### add trafodion user ###
    # create trafodion user and group
    if cmd_output('getent passwd %s' % traf_user):
        print 'user [%s] exists' % traf_user
        # trafodion user exists, set actual trafodion group
        traf_group = cmd_output('id -ng %s' % traf_user)
    else:
        # default trafodion group
        traf_group = traf_user
        if not cmd_output('getent group %s' % traf_group):
            run_cmd('groupadd %s' % traf_group)
        traf_shadow = dbcfgs['traf_shadow']
        print 'Adding user [%s]' % traf_user
        run_cmd('useradd --shell /bin/bash -m %s -g %s --home %s --password "%s"' % (traf_user, traf_group, traf_user_dir, traf_shadow))
        print 'Added user [%s]' % traf_user

    if not os.path.exists(traf_user_dir):
        run_cmd('mkdir -p %s' % traf_user_dir)
        run_cmd('chmod 700 %s' % traf_user_dir)

    ### untar the copied trafoion binaries ###
    TRAF_PKG_FILE = '/tmp/traf_bin.tar.gz'
    run_cmd('mkdir -p %s' % traf_home)
    run_cmd('mkdir -p %s' % TRAF_CFG_DIR)
    run_cmd('tar xf %s -C %s' % (TRAF_PKG_FILE, traf_home))
    run_cmd('cp -rf %s/conf/* %s/' % (traf_home, TRAF_CFG_DIR))

    run_cmd('mv -f /tmp/trafodion_config %s' % TRAF_CFG_FILE)
    run_cmd('cp -rf /tmp/.ssh %s/..' % traf_home)
    run_cmd('mv -f /tmp/hbase-trx-* %s' % dbcfgs['hbase_lib_path'])
    run_cmd('mv -f /tmp/trafodion-utility-* %s' % dbcfgs['hbase_lib_path'])

    ### copy trafodion bashrc ###
    bashrc_template = '%s/sysinstall/home/trafodion/.bashrc' % traf_home
    bashrc_file = '%s/%s/.bashrc' % (home_dir, traf_user)
    # backup orig bashrc
    if os.path.exists(bashrc_file):
        run_cmd('cp -f %s %s.bak' % (bashrc_file, bashrc_file))
    run_cmd('cp -f %s %s' % (bashrc_template, bashrc_file))

    # set permission
    run_cmd('chmod 700 %s/../.ssh' % traf_home)
    cmd_output('chmod 600 %s/../.ssh/{id_rsa,config,authorized_keys}' % traf_home)
    run_cmd('chmod 777 %s' % TRAF_CFG_FILE)
    run_cmd('chown -R %s:%s %s' % (traf_user, traf_group, TRAF_CFG_DIR))
    run_cmd('chmod +r %s/{hbase-trx-*,trafodion-utility-*}' % dbcfgs['hbase_lib_path'])
    run_cmd('chown -R %s:%s %s' % (traf_user, traf_group, traf_user_dir))

    ### modify CLUSTERNAME ###
    mod_file(TRAF_CFG_FILE, {'CLUSTERNAME=.*': 'CLUSTERNAME=%s' % socket.gethostname()})

    ### kernel settings ###
    run_cmd('echo "kernel.pid_max=65535" >> /etc/sysctl.conf')
    run_cmd('echo "kernel.msgmnb=65536" >> /etc/sysctl.conf')
    run_cmd('echo "kernel.msgmax=65536" >> /etc/sysctl.conf')
    run_cmd('/sbin/sysctl -p /etc/sysctl.conf 2>&1 > /dev/null')

    ### copy init script ###
    init_script = '%s/sysinstall/etc/init.d/trafodion' % traf_home
    if os.path.exists(init_script):
        run_cmd('cp -rf %s /etc/init.d/' % init_script)
        run_cmd('chkconfig --add trafodion')
        run_cmd('chkconfig --level 06 trafodion on')

    ### create and set permission for scratch file dir ###
#    for loc in scratch_locs:
#        # don't set permission for HOME folder
#        if not os.path.exists(loc):
#            run_cmd('mkdir -p %s' % loc)
#        if home_dir not in loc:
#            run_cmd('chmod 777 %s' % loc)

    if dbcfgs['enable_ha'] == 'true':
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

    # set ulimits for trafodion user
    ulimits_file = '/etc/security/limits.d/%s.conf' % traf_user
    ulimits_config = '''
# Trafodion settings
%s   soft   core unlimited
%s   hard   core unlimited
%s   soft   memlock unlimited
%s   hard   memlock unlimited
%s   soft   nofile 32768
%s   hard   nofile 65536
%s   soft   nproc 100000
%s   hard   nproc 100000
''' % ((traf_user,) * 8)

    write_file(ulimits_file, ulimits_config)

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
