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

import sys
import os
import re
import getpass
from optparse import OptionParser
from scripts.constants import INSTALLER_LOC, TRAF_CFG_FILE, TRAF_CFG_DIR, TRAF_USER, DBCFG_FILE
from scripts.common import run_cmd, run_cmd_as_user, format_output, err_m, \
                           expNumRe, ParseInI, Remote, info, get_sudo_prefix

def get_options():
    usage = 'usage: %prog [options]\n'
    usage += '  Trafodion uninstall script. It will remove \n\
  trafodion user and home folder.'
    parser = OptionParser(usage=usage)
    parser.add_option("-c", "--config-file", dest="cfgfile", metavar="FILE",
                      help="Json format file. If provided, all install prompts \
                            will be taken from this file and not prompted for.")
    parser.add_option("--enable-pwd", action="store_true", dest="pwd", default=False,
                      help="Prompt SSH login password for remote hosts. \
                            If set, \'sshpass\' tool is required.")
    parser.add_option("--silent", action="store_true", dest="silent", default=False,
                      help="Do not ask user to confirm.")
    parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False,
                      help="Verbose mode, will print commands.")
    (options, args) = parser.parse_args()
    return options

def main():
    """ db_uninstaller main loop """

    # handle parser option
    options = get_options()

    notify = lambda n: raw_input('Uninstall Trafodion on [%s] [N]: ' % n)

    format_output('Trafodion Uninstall Start')

    if options.pwd:
        pwd = getpass.getpass('Input remote host SSH Password: ')
    else:
        pwd = ''

    try:
      traf_var = run_cmd_as_user(TRAF_USER,"echo $TRAF_VAR")
      node_list = run_cmd_as_user(TRAF_USER,"trafconf -name")
    except:
      if options.cfgfile:
        if not os.path.exists(options.cfgfile):
            err_m('Cannot find config file \'%s\'' % options.cfgfile)
        config_file = options.cfgfile
        p = ParseInI(config_file, 'dbconfigs')
        cfgs = p.load()
        traf_var = cfgs['traf_var']
        node_list = cfgs['node_list']
      # user input
      else:
        traf_var = '/var/lib/trafodion'
        node_lists = raw_input('Enter Trafodion node list to uninstall(separated by comma): ')
        if not node_lists: err_m('Empty value')
        node_list = ' '.join(expNumRe(node_lists))

    if not options.silent:
        rc = notify(node_list)
        if rc.lower() != 'y': sys.exit(1)

    nodes = node_list.split()
    first_node = nodes[0]

    remotes = [Remote(node, pwd=pwd) for node in nodes]
    sudo_prefix = get_sudo_prefix()

    # remove trafodion userid and group on all trafodion nodes, together with folders
    for remote in remotes:
        info('Remove Trafodion on node [%s] ...' % remote.host)
        remote.execute('ps -f -u %s|awk \'{print $2}\'|xargs %s kill -9' % (TRAF_USER, sudo_prefix), chkerr=False)
        remote.execute('trafid=`getent passwd %s|awk -F: \'{print $3}\'`; if [[ -n $trafid ]]; then ps -f -u $trafid|awk \'{print $2}\'|xargs %s kill -9; fi' % (TRAF_USER, sudo_prefix), chkerr=False)
        remote.execute('%s /usr/sbin/userdel -rf %s' % (sudo_prefix, TRAF_USER), chkerr=False)
        remote.execute('%s /usr/sbin/groupdel %s' % (sudo_prefix, TRAF_USER), chkerr=False)
        remote.execute('%s rm -rf /etc/security/limits.d/%s.conf %s %s /tmp/hsperfdata_%s 2>/dev/null' % (sudo_prefix, TRAF_USER, TRAF_CFG_DIR, traf_var, TRAF_USER), chkerr=False)

    run_cmd('rm -f %s/*.status %s' % (INSTALLER_LOC, DBCFG_FILE))
    format_output('Trafodion Uninstall Completed')

if __name__ == "__main__":
    main()
