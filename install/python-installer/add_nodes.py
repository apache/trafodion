#!/usr/bin/env python
# -*- coding: utf8 -*-

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

import os
import re
import socket
import json
import getpass
import time
import sys
reload(sys)
sys.setdefaultencoding("utf-8")
from optparse import OptionParser
from glob import glob
from collections import defaultdict
from threading import Thread
from scripts import wrapper
from db_install import UserInput
from scripts.constants import TRAF_CFG_FILE
from scripts.common import get_sudo_prefix, Remote, run_cmd, run_cmd_as_user, info, ok, \
                           format_output, err_m, expNumRe, cmd_output

TRAF_PKG_FILE = '/tmp/traf_bin.tar.gz'

def get_options():
    usage = 'usage: %prog [options]\n'
    usage += '  Trafodion install main script.'
    parser = OptionParser(usage=usage)
    parser.add_option("-u", "--remote-user", dest="user", metavar="USER",
                      help="Specify ssh login user for remote server, \
                            if not provided, use current login user as default.")
    parser.add_option("-n", "--nodes", dest="nodes", metavar="NODES",
                      help="Specify the node names you want to add, seperated by comma if more than one, \
                            support numeric RE, e.g. node[1-3].com")
    parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False,
                      help="Verbose mode, will print commands.")
    parser.add_option("--enable-pwd", action="store_true", dest="pwd", default=False,
                      help="Prompt SSH login password for remote hosts. \
                            If set, \'sshpass\' tool is required.")
    parser.add_option("--offline", action="store_true", dest="offline", default=False,
                      help="Enable local repository for offline installing Trafodion.")

    (options, args) = parser.parse_args()
    return options

def main():
    """ add_nodes main loop """
    cfgs = defaultdict(str)

    # handle parser option
    options = get_options()
    if not options.nodes:
        err_m('Must specifiy the node names using \'--nodes\' option')

    # get node list from user input
    new_nodes = expNumRe(options.nodes)
    if not new_nodes:
        err_m('Incorrect format')

    if options.pwd:
        pwd = getpass.getpass('Input remote host SSH Password: ')
    else:
        pwd = ''

    u = UserInput(options, pwd)
    g = lambda n: u.get_input(n, cfgs[n], prompt_mode=prompt_mode)

    format_output('Trafodion Elastic Add Nodes Script')

    ### read configs from current trafodion_config and save it to cfgs
    if os.path.exists(TRAF_CFG_FILE):
        with open(TRAF_CFG_FILE, 'r') as f:
            traf_cfgs = f.readlines()
        for traf_cfg in traf_cfgs:
            if not traf_cfg.strip(): continue
            key, value = traf_cfg.replace('export ', '').split('=')
            value = value.replace('"','')
            value = value.replace('\n','')
            cfgs[key.lower()] = value
    else:
        err_m('Cannot find %s, be sure to run this script on one of trafodion nodes' % TRAF_CFG_FILE)

    ### config check
    if not cfgs['hbase_lib_path'] or not cfgs['traf_version']:
        err_m('Missing parameters in Trafodion config file')

    if not cfgs['traf_home'] or not cmd_output('%s ls %s' % (get_sudo_prefix(), cfgs['traf_home'])):
        err_m('Cannot find trafodion binary folder')
    # get trafodion user from traf_home path
    cfgs['traf_user'] = cfgs['traf_home'].split('/')[-2]
    if not cfgs['traf_user']:
        err_m('Cannot detect trafodion user')

    ### parse trafodion user's password
    cfgs['traf_shadow'] = cmd_output("%s grep %s /etc/shadow |awk -F: '{print $2}'" % (get_sudo_prefix(), cfgs['traf_user']))

    def copy_files():
        # package trafodion binary into a tar file
        if not os.path.exists(TRAF_PKG_FILE):
            info('Creating trafodion packages of %s, this will take a while ...' % cfgs['traf_home'])
            run_cmd_as_user(cfgs['traf_user'], 'cd %s; tar czf %s ./* --exclude logs/* --exclude core.* --exclude tmp/*' % (cfgs['traf_home'], TRAF_PKG_FILE))
        else:
            info('Using existing trafodion package %s' % TRAF_PKG_FILE)

        info('Copying trafodion files to new nodes, this will take a while ...')
        run_cmd('%s cp -rf %s/../.ssh /tmp' % (get_sudo_prefix(), cfgs['traf_home']))
        run_cmd('%s chmod -R 755 /tmp/.ssh' % get_sudo_prefix())
        traf_ssh_folder = '/tmp/.ssh'

        hbase_trx_file = cmd_output('ls %s/hbase-trx-*' % cfgs['hbase_lib_path'])
        trafodion_utility_file = cmd_output('ls %s/trafodion-utility-*' % cfgs['hbase_lib_path'])

        files = [TRAF_CFG_FILE, TRAF_PKG_FILE, traf_ssh_folder, hbase_trx_file, trafodion_utility_file]

        remote_insts = [Remote(h, pwd=pwd) for h in new_nodes]
        threads = [Thread(target=r.copy, args=(files, '/tmp')) for r in remote_insts]
        for thread in threads: thread.start()
        for thread in threads: thread.join()

        for r in remote_insts:
            if r.rc != 0: err_m('Failed to copy files to %s' % r.host)

    ### copy trafodion_config/trafodion-package/hbase-trx to the new nodes
    copy_files()

    ### set parameters
    if cfgs['enable_ha'].upper() == 'true':
        g('dcs_backup_nodes')
        cfgs['dcs_ha'] = 'Y'
    else:
        cfgs['dcs_ha'] = 'N'

    if cfgs['trafodion_enable_authentication'] == 'YES':
        cfgs['ldap_security'] = 'Y'
    else:
        cfgs['ldap_security'] = 'N'

    if cfgs['secure_hadoop'].upper() == 'Y':
        g('kdc_server')
        g('admin_principal')
        g('kdcadmin_pwd')

    #TODO: offline support
    cfgs['offline_mode'] = 'N'


    format_output('AddNode sub scripts Start')

    ### run addNode script on new nodes ###
    cfgs['node_list'] = ','.join(new_nodes)
    info('Running add node setup on new node(s) [%s] ...' % cfgs['node_list'])
    wrapper.run(cfgs, options, mode='addnodes_new', pwd=pwd)

    ### run dcs setup script on all nodes ###
    # get current trafodion node list
    current_nodes = cmd_output('%s su - %s -c "trafconf -name 2>/dev/null"' % (get_sudo_prefix(), cfgs['traf_user'])).split()
    all_nodes = list(set(new_nodes + current_nodes))
    cfgs['node_list'] = ','.join(all_nodes)
    info('Running dcs setup on all node(s) [%s] ...' % cfgs['node_list'])
    wrapper.run(cfgs, options, mode='addnodes_all', pwd=pwd)

    ### do sqshell node add/up, sqregen
    # check if trafodion is running
    mon_process = cmd_output('ps -ef|grep -v grep|grep -c "monitor COLD"')
    if int(mon_process) > 0:
        info('Trafodion instance is up, adding node in sqshell ...')

        # cores=0-1;processors=2;roles=connection,aggregation,storage
        sqconfig_ptr = cmd_output('%s su - %s -c "trafconf -node|sed -n 2p|cut -d\\\";\\\" -f3-5"' % (get_sudo_prefix(), cfgs['traf_user']))
        for node in new_nodes:
            info('adding node [%s] in sqshell ...' % node)
            run_cmd_as_user(cfgs['traf_user'], 'echo "node add {node-name %s,%s}" | sqshell -a' % (node, sqconfig_ptr))
            run_cmd_as_user(cfgs['traf_user'], 'echo "node up %s" | sqshell -a' % node)
            ok('Node [%s] added!' % node)

        info('Starting DCS on new nodes ...')
        run_cmd_as_user(cfgs['traf_user'], 'dcsstart')
    else:
        info('Trafodion instance is not up, do sqgen ...')
        run_cmd_as_user(cfgs['traf_user'], 'rm %s/sqconfig.db' % cfgs['traf_var'])
        run_cmd_as_user(cfgs['traf_user'], 'sqgen')
        ok('Setup completed. You need to start trafodion manually')

    ### clean up
    run_cmd('%s rm -rf /tmp/.ssh' % get_sudo_prefix())
    run_cmd('%s rm -rf %s' % (get_sudo_prefix(), TRAF_PKG_FILE))

    format_output('AddNode Complete')
    info('NOTICE: You need to manually restart RegionServer on newly added nodes to take effect')

if __name__ == "__main__":
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        print '\nAborted...'
