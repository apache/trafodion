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

import os
import time
import json
import subprocess
from glob import glob
from threading import Thread
from constants import INSTALLER_LOC, TMP_DIR, SCRCFG_FILE, CONFIG_DIR, SCRIPTS_DIR
from common import err_m, run_cmd, time_elapse, get_logger, get_sudo_prefix, Remote, ParseJson

class RemoteRun(Remote):
    """ run commands or scripts remotely using ssh """

    def __init__(self, host, logger, user='', pwd='', quiet=False):
        self.sudo_prefix = ''
        self.host = host
        self.user = user
        self.pwd = pwd
        self.quiet = quiet # no output
        self.logger = logger

        if not self.user:
            self.sudo_prefix = get_sudo_prefix()
        elif self.user != 'root':
            self.sudo_prefix = 'sudo -n'

    def initialize(self):
        super(RemoteRun, self).__init__(self.host, self.user, self.pwd)
        # create tmp folder
        self.execute('mkdir -p %s' % TMP_DIR)

        # copy all needed files to remote host
        all_files = [CONFIG_DIR, SCRIPTS_DIR]

        self.copy(all_files, remote_folder=TMP_DIR)

        # set permission
        self.execute('chmod a+rx %s/scripts/*.py' % TMP_DIR)

    def __del__(self):
        # clean up
        self.execute('%s rm -rf %s' % (self.sudo_prefix, TMP_DIR), chkerr=False)

    def run_script(self, script, run_user, json_string, verbose=False):
        """ @param run_user: run the script with this user """

        if run_user:
            # format string in order to run with 'su $user -c $cmd'
            json_string = json_string.replace('"', '\\\\\\"').replace(' ', '').replace('{', '\\{').replace('$', '\\\\\\$')
            # this command only works with shell=True
            script_cmd = '"%s su - %s -c \'%s/scripts/%s %s\'"' % (self.sudo_prefix, run_user, TMP_DIR, script, json_string)
            self.execute(script_cmd, verbose=verbose, shell=True, chkerr=False)
        else:
            script_cmd = '%s %s/scripts/%s \'%s\'' % (self.sudo_prefix, TMP_DIR, script, json_string)
            self.execute(script_cmd, verbose=verbose, chkerr=False)

        format1 = 'Host [%s]: Script [%s]: %s' % (self.host, script, self.stdout)
        format2 = 'Host [%s]: Script [%s]' % (self.host, script)

        self.logger.info(format1)

        if self.rc == 0:
            if not self.quiet: state_ok(format2)
            self.logger.info(format2 + ' ran successfully!')
        else:
            if not self.quiet: state_fail(format2)
            msg = 'Host [%s]: Failed to run \'%s\'' % (self.host, script)
            if self.stderr:
                msg += ': ' + self.stderr
                print '\n ' + self.stderr
            self.logger.error(msg)
            exit(1)


def state_ok(msg):
    state(32, ' OK ', msg)

def state_fail(msg):
    state(31, 'FAIL', msg)

def state_skip(msg):
    state(33, 'SKIP', msg)

def state(color, result, msg):
    WIDTH = 80
    print '\n\33[%dm%s %s [ %s ]\33[0m\n' % (color, msg, (WIDTH - len(msg))*'.', result)

class Status(object):
    def __init__(self, stat_file, name):
        self.stat_file = stat_file
        self.name = name

    def get_status(self):
        if not os.path.exists(self.stat_file): os.mknod(self.stat_file)
        with open(self.stat_file, 'r') as f:
            st = f.readlines()
        for s in st:
            try:
                if s.split()[0] == self.name: return True
            except IndexError:
                return False
        return False

    def set_status(self):
        with open(self.stat_file, 'a+') as f:
            f.write('%s OK\n' % self.name)

@time_elapse
def run(dbcfgs, options, mode='install', pwd=''):
    """ main entry
        mode: install/discover
    """
    stat_file = '%s/%s.status' % (INSTALLER_LOC, mode)
    log_file = '%s/logs/%s_%s.log' % (INSTALLER_LOC, mode, time.strftime('%Y%m%d_%H%M'))
    logger = get_logger(log_file)

    verbose = True if hasattr(options, 'verbose') and options.verbose else False
    reinstall = True if hasattr(options, 'reinstall') and options.reinstall else False
    user = options.user if hasattr(options, 'user') and options.user else ''
    threshold = options.fork if hasattr(options, 'fork') and options.fork else 10

    script_output = [] # script output array
    conf = ParseJson(SCRCFG_FILE).load()
    script_cfgs = conf[mode]

    dbcfgs_json = json.dumps(dbcfgs)
    hosts = dbcfgs['node_list'].split(',')

    # handle skipped scripts, skip them if no need to run
    skipped_scripts = []
    if reinstall:
        skipped_scripts += ['hadoop_mods', 'apache_mods', 'apache_restart', 'traf_dep', 'traf_kerberos']

    if dbcfgs['secure_hadoop'] == 'N':
        skipped_scripts += ['traf_kerberos']

    if dbcfgs['traf_start'].upper() == 'N':
        skipped_scripts += ['traf_start']

    if dbcfgs['ldap_security'].upper() == 'N':
        skipped_scripts += ['traf_ldap']

    if 'APACHE' in dbcfgs['distro']:
        skipped_scripts += ['hadoop_mods']
    else:
        skipped_scripts += ['apache_mods', 'apache_restart']

    # set ssh config file to avoid known hosts verify on current installer node
    ssh_cfg_file = os.environ['HOME'] + '/.ssh/config'
    ssh_cfg = 'StrictHostKeyChecking=no\nNoHostAuthenticationForLocalhost=yes\n'
    with open(ssh_cfg_file, 'w') as f:
        f.write(ssh_cfg)
    run_cmd('chmod 600 %s' % ssh_cfg_file)

    def run_local_script(script, json_string, req_pwd):
        cmd = '%s/%s \'%s\'' % (SCRIPTS_DIR, script, json_string)

        # pass the ssh password to sub scripts which need SSH password
        if req_pwd: cmd += ' ' + pwd + ' ' + user

        if verbose: print cmd

        # stdout on screen
        p = subprocess.Popen(cmd, stderr=subprocess.PIPE, shell=True)
        stdout, stderr = p.communicate()

        rc = p.returncode
        if rc != 0:
            msg = 'Failed to run \'%s\'' % script
            if stderr:
                msg += ': ' + stderr
                print stderr
            logger.error(msg)
            state_fail('localhost: Script [%s]' % script)
            exit(rc)
        else:
            state_ok('Script [%s]' % script)
            logger.info('Script [%s] ran successfully!' % script)

        return stdout

    # run sub scripts
    try:
        remote_instances = []
        if mode == 'discover':
            remote_instances = [RemoteRun(host, logger, user=user, pwd=pwd, quiet=True) for host in hosts]
        else:
            remote_instances = [RemoteRun(host, logger, user=user, pwd=pwd) for host in hosts]
        # do init in threads to improve performance
        threads = [Thread(target=r.initialize) for r in remote_instances]
        for t in threads: t.start()
        for t in threads: t.join()

        first_instance = remote_instances[0]
        for instance in remote_instances:
            if instance.host == dbcfgs['first_rsnode']:
                first_rs_instance = instance
                break

        logger.info(' ***** %s Start *****' % mode)
        for cfg in script_cfgs:
            script = cfg['script']
            node = cfg['node']
            desc = cfg['desc']
            run_user = ''
            if not 'run_as_traf' in cfg.keys():
                pass
            elif cfg['run_as_traf'] == 'yes':
                run_user = dbcfgs['traf_user']

            if not 'req_pwd' in cfg.keys():
                req_pwd = False
            elif cfg['req_pwd'] == 'yes':
                req_pwd = True

            status = Status(stat_file, script)
            if status.get_status():
                msg = 'Script [%s] had already been executed' % script
                state_skip(msg)
                logger.info(msg)
                continue

            if script.split('.')[0] in skipped_scripts:
                continue
            else:
                print '\nTASK: %s %s' % (desc, (83 - len(desc))*'*')

            #TODO: timeout exit
            if node == 'local':
                run_local_script(script, dbcfgs_json, req_pwd)
            elif node == 'first':
                first_instance.run_script(script, run_user, dbcfgs_json, verbose=verbose)
            elif node == 'first_rs':
                first_rs_instance.run_script(script, run_user, dbcfgs_json, verbose=verbose)
            elif node == 'all':
                l = len(remote_instances)
                if l > threshold:
                    piece = (l - (l % threshold)) / threshold
                    parted_remote_instances = [remote_instances[threshold*i:threshold*(i+1)] for i in range(piece)]
                    parted_remote_instances.append(remote_instances[threshold*piece:])
                else:
                    parted_remote_instances = [remote_instances]

                for parted_remote_inst in parted_remote_instances:
                    threads = [Thread(target=r.run_script, args=(script, run_user, dbcfgs_json, verbose)) for r in parted_remote_inst]
                    for t in threads: t.start()
                    for t in threads: t.join()

                    if sum([r.rc for r in parted_remote_inst]) != 0:
                        err_m('Script failed to run on one or more nodes, exiting ...\nCheck log file %s for details.' % log_file)

                    script_output += [{r.host:r.stdout.strip()} for r in parted_remote_inst]

            else:
                # should not go to here
                err_m('Invalid configuration for %s' % SCRCFG_FILE)

            status.set_status()
    except KeyboardInterrupt:
        err_m('User quit')

    # remove status file if all scripts run successfully
    os.remove(stat_file)

    # remove ^M dos format in log file
    with open(log_file, 'r') as f:
        lines = f.readlines()
    with open(log_file, 'w') as f:
        for line in lines:
            f.write(line.rstrip('\r\n') + '\n')

    return script_output

if __name__ == '__main__':
    exit(0)
