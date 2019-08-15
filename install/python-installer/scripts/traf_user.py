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
import socket
from constants import TRAF_CFG_DIR, TRAF_CFG_FILE, TRAF_HSPERFDATA_FILE, SSHKEY_FILE
from common import ParseXML, run_cmd, append_file, mod_file, write_file, \
                   cmd_output, run_cmd_as_user, err, get_default_home

def run():
    """ create trafodion user, bashrc, setup passwordless SSH """
    dbcfgs = json.loads(dbcfgs_json)

    distro = dbcfgs['distro']
    if 'CDH' in distro:
        hadoop_type = 'cloudera'
    elif 'HDP' in distro:
        hadoop_type = 'hortonworks'
    elif 'APACHE' in distro:
        hadoop_type = 'apache'

    home_dir = get_default_home()
    # customize trafodion home dir
    if dbcfgs.has_key('home_dir') and dbcfgs['home_dir']:
        home_dir = dbcfgs['home_dir']

    traf_user = dbcfgs['traf_user']
    traf_user_dir = '%s/%s' % (home_dir, traf_user)
    traf_dirname = dbcfgs['traf_dirname']
    traf_home = '%s/%s' % (traf_user_dir, traf_dirname)
    traf_log = dbcfgs['traf_log']
    traf_var = dbcfgs['traf_var']

    hbase_xml_file = dbcfgs['hbase_xml_file']
    auth_key_file = '%s/.ssh/authorized_keys' % traf_user_dir
    ssh_cfg_file = '%s/.ssh/config' % traf_user_dir
    ulimits_file = '/etc/security/limits.d/%s.conf' % traf_user

    # create trafodion user and group
    if cmd_output('getent passwd %s' % traf_user):
        # trafodion user exists, set actual trafodion group
        traf_group = cmd_output('id -ng %s' % traf_user)
    else:
        # default trafodion group
        traf_group = traf_user
        if not cmd_output('getent group %s' % traf_group):
            run_cmd('groupadd %s > /dev/null 2>&1' % traf_group)
        traf_pwd = dbcfgs['traf_pwd']
        run_cmd('useradd --shell /bin/bash -m %s -g %s --home %s --password "$(openssl passwd %s)"' % (traf_user, traf_group, traf_user_dir, traf_pwd))
    # hbase group is generally either hbase or hadoop, depending on distro
    if cmd_output('getent group hbase'):
        cmd_output('/usr/sbin/usermod -a -G hbase %s' % traf_user)
    if cmd_output('getent group hadoop'):
        cmd_output('/usr/sbin/usermod -a -G hadoop %s' % traf_user)
    if cmd_output('getent group hive'):
        cmd_output('/usr/sbin/usermod -a -G hive %s' % traf_user)

    if not os.path.exists(traf_user_dir):
        run_cmd('mkdir -p %s' % traf_user_dir)
        run_cmd('chmod 700 %s' % traf_user_dir)

    # set ssh key
    run_cmd_as_user(traf_user, 'echo -e "y" | ssh-keygen -t rsa -N "" -f ~/.ssh/id_rsa')
    # the key is generated in copy_file script running on the installer node
    run_cmd('cp %s{,.pub} %s/.ssh/' % (SSHKEY_FILE, traf_user_dir))

    run_cmd_as_user(traf_user, 'cat ~/.ssh/id_rsa.pub > %s' % auth_key_file)
    run_cmd('chmod 644 %s' % auth_key_file)

    ssh_cfg = 'StrictHostKeyChecking=no\nNoHostAuthenticationForLocalhost=yes\n'
    with open(ssh_cfg_file, 'w') as f:
        f.write(ssh_cfg)
    run_cmd('chmod 600 %s' % ssh_cfg_file)

    run_cmd('chown -R %s:%s %s/.ssh/' % (traf_user, traf_group, traf_user_dir))

    hb = ParseXML(hbase_xml_file)
    zk_nodes = hb.get_property('hbase.zookeeper.quorum')
    zk_port = hb.get_property('hbase.zookeeper.property.clientPort')
    # set trafodion_config
    nodes = dbcfgs['node_list'].split(',')
    trafodion_config = """
export TRAF_HOME="%s"
export TRAF_VAR="%s"
export TRAF_CONF="%s"
export TRAF_LOG="%s"
export JAVA_HOME="%s"
export node_count="%s"
export HADOOP_TYPE="%s"
export ENABLE_HA="%s"
export ZOOKEEPER_NODES="%s"
export ZOOKEEPER_PORT="%s"
export SECURE_HADOOP="%s"
export CLUSTERNAME="%s"
""" % (traf_home, traf_var, TRAF_CFG_DIR, traf_log, dbcfgs['java_home'], str(len(nodes)), hadoop_type, dbcfgs['enable_ha'],
       zk_nodes, zk_port, dbcfgs['secure_hadoop'], socket.gethostname())

    # save additonal configs for elastic
    trafodion_config += """
export hbase_xml_file="%s"
export hbase_lib_path="%s"
export traf_user="%s"
export traf_version="%s"
export dcs_cnt_per_node="%s"
""" % (dbcfgs['hbase_xml_file'], dbcfgs['hbase_lib_path'], dbcfgs['traf_user'], dbcfgs['traf_version'], dbcfgs['dcs_cnt_per_node'])

    # save additonal configs for multi instance support
    trafodion_config += """
export TRAF_CLUSTER_NAME="%s"
export TRAF_INSTANCE_NAME="%s"
export TRAF_CLUSTER_ID="%s"
export TRAF_INSTANCE_ID="%s"
export TRAF_ROOT_ZNODE="/%s"
""" % (dbcfgs['cluster_name'], dbcfgs['traf_instance_name'], dbcfgs['traf_cluster_id'], dbcfgs['traf_instance_id'], dbcfgs['traf_user'])

    run_cmd('mkdir -p %s' % TRAF_CFG_DIR)
    write_file(TRAF_CFG_FILE, trafodion_config)

    if 'APACHE' in distro:
        extra_config = """
export HADOOP_PREFIX=%s
export HBASE_HOME=%s
export HIVE_HOME=%s
export PATH=$PATH:$HADOOP_PREFIX/bin:$HADOOP_PREFIX/sbin:$HBASE_HOME/bin
        """ % (dbcfgs['hadoop_home'], dbcfgs['hbase_home'], dbcfgs['hive_home'])
        append_file(TRAFODION_CFG_FILE, extra_config)

    # set permission
    run_cmd('chown -R %s:%s %s*' % (traf_user, traf_group, TRAF_CFG_DIR))

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
''' % ((traf_user,) * 10)

    write_file(ulimits_file, ulimits_config)

    # change permission for hsperfdata
    if os.path.exists(TRAF_HSPERFDATA_FILE):
        run_cmd('chown -R %s:%s %s' % (traf_user, traf_group, TRAF_HSPERFDATA_FILE))

    # clean up unused key file at the last step
    run_cmd('rm -rf %s{,.pub}' % SSHKEY_FILE)

    print 'Setup trafodion user successfully!'

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
