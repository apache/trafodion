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
import json
from common import ParseXML, run_cmd, append_file, mod_file, write_file, \
                   cmd_output, run_cmd_as_user, err, TMP_DIR

def run():
    """ create trafodion user, bashrc, setup passwordless SSH """
    dbcfgs = json.loads(dbcfgs_json)

    DISTRO = dbcfgs['distro']
    if 'CDH' in DISTRO:
        hadoop_type = 'cloudera'
    elif 'HDP' in DISTRO:
        hadoop_type = 'hortonworks'
    elif 'APACHE' in DISTRO:
        hadoop_type = 'apache'

    TRAF_USER = dbcfgs['traf_user']
    TRAF_PWD = dbcfgs['traf_pwd']
    TRAF_GROUP = TRAF_USER
    HOME_DIR = cmd_output('cat /etc/default/useradd |grep HOME |cut -d "=" -f 2').strip()
    # customize trafodion home dir
    if dbcfgs.has_key('home_dir') and dbcfgs['home_dir']:
        HOME_DIR = dbcfgs['home_dir']

    TRAF_USER_DIR = '%s/%s' % (HOME_DIR, TRAF_USER)
    TRAF_DIRNAME = dbcfgs['traf_dirname']
    TRAF_HOME = '%s/%s' % (TRAF_USER_DIR, TRAF_DIRNAME)

    TRAFODION_CFG_DIR = '/etc/trafodion/'
    TRAFODION_CFG_FILE = '/etc/trafodion/trafodion_config'
    HBASE_XML_FILE = dbcfgs['hbase_xml_file']
    KEY_FILE = '/tmp/id_rsa'
    AUTH_KEY_FILE = '%s/.ssh/authorized_keys' % TRAF_USER_DIR
    SSH_CFG_FILE = '%s/.ssh/config' % TRAF_USER_DIR
    BASHRC_TEMPLATE = '%s/sqf/sysinstall/home/trafodion/.bashrc' % TRAF_HOME
    BASHRC_FILE = '%s/.bashrc' % TRAF_USER_DIR
    ULIMITS_FILE = '/etc/security/limits.d/%s.conf' % TRAF_USER
    HSPERFDATA_FILE = '/tmp/hsperfdata_trafodion'

    # create trafodion user and group
    if not cmd_output('getent group %s' % TRAF_GROUP):
        run_cmd('groupadd %s > /dev/null 2>&1' % TRAF_GROUP)

    if not cmd_output('getent passwd %s' % TRAF_USER):
        run_cmd('useradd --shell /bin/bash -m %s -g %s --home %s --password "$(openssl passwd %s)"' % (TRAF_USER, TRAF_GROUP, TRAF_USER_DIR, TRAF_PWD))
        # copy bashrc to trafodion's home only if user doesn't exist
        run_cmd('cp %s %s' % (BASHRC_TEMPLATE, BASHRC_FILE))
        run_cmd('chown -R %s:%s %s*' % (TRAF_USER, TRAF_GROUP, BASHRC_FILE))
    elif not os.path.exists(TRAF_USER_DIR):
        run_cmd('mkdir -p %s' % TRAF_USER_DIR)
        run_cmd('chmod 700 %s' % TRAF_USER_DIR)

    # set ssh key
    run_cmd_as_user(TRAF_USER, 'echo -e "y" | ssh-keygen -t rsa -N "" -f ~/.ssh/id_rsa')
    # the key is generated in copy_file script running on the installer node
    run_cmd('cp %s{,.pub} %s/.ssh/' % (KEY_FILE, TRAF_USER_DIR))

    run_cmd_as_user(TRAF_USER, 'cat ~/.ssh/id_rsa.pub > %s' % AUTH_KEY_FILE)
    run_cmd('chmod 644 %s' % AUTH_KEY_FILE)

    ssh_cfg = 'StrictHostKeyChecking=no\nNoHostAuthenticationForLocalhost=yes\n'
    with open(SSH_CFG_FILE, 'w') as f:
        f.write(ssh_cfg)
    run_cmd('chmod 600 %s' % SSH_CFG_FILE)

    run_cmd('chown -R %s:%s %s/.ssh/' % (TRAF_USER, TRAF_GROUP, TRAF_USER_DIR))

    hb = ParseXML(HBASE_XML_FILE)
    zk_nodes = hb.get_property('hbase.zookeeper.quorum')
    zk_port = hb.get_property('hbase.zookeeper.property.clientPort')
    # set trafodion_config
    nodes = dbcfgs['node_list'].split(',')
    trafodion_config = """
export TRAF_HOME="%s"
export MY_SQROOT=$TRAF_HOME # for compatibility
export JAVA_HOME="%s"
export NODE_LIST="%s"
export MY_NODES="%s"
export node_count="%s"
export HADOOP_TYPE="%s"
export ENABLE_HA="%s"
export ZOOKEEPER_NODES="%s"
export ZOOKEEPER_PORT="%s"
""" % (TRAF_HOME, dbcfgs['java_home'], ' '.join(nodes), ' -w ' + ' -w '.join(nodes),
       str(len(nodes)), hadoop_type, dbcfgs['enable_ha'], zk_nodes, zk_port)

    run_cmd('mkdir -p %s' % TRAFODION_CFG_DIR)
    write_file(TRAFODION_CFG_FILE, trafodion_config)

    if 'APACHE' in DISTRO:
        extra_config = """
export HADOOP_PREFIX=%s
export HBASE_HOME=%s
export PATH=$PATH:$HADOOP_PREFIX/bin:$HADOOP_PREFIX/sbin:$HBASE_HOME/bin
        """ % (dbcfgs['hadoop_home'], dbcfgs['hbase_home'])
        append_file(TRAFODION_CFG_FILE, extra_config)

    # set permission
    run_cmd('chown -R %s:%s %s*' % (TRAF_USER, TRAF_GROUP, TRAFODION_CFG_DIR))


    # set ulimits for trafodion user
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
%s   soft nofile 8192
%s   hard nofile 65535
''' % ((TRAF_USER,) * 10)

    write_file(ULIMITS_FILE, ulimits_config)

    # change permission for hsperfdata
    if os.path.exists(HSPERFDATA_FILE):
        run_cmd('chown -R %s:%s %s' % (TRAF_USER, TRAF_GROUP, HSPERFDATA_FILE))

    # clean up unused key file at the last step
    run_cmd('rm -rf %s{,.pub}' % KEY_FILE)

    print 'Setup trafodion user successfully!'

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
