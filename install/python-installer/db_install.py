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
try:
    from prettytable import PrettyTable
except ImportError:
    print 'Python module prettytable is not found. Install python-prettytable first.'
    exit(1)
from scripts import wrapper
from scripts.constants import DEF_PORT_FILE, DBCFG_FILE, USER_PROMPT_FILE, DBCFG_TMP_FILE, INSTALLER_LOC, \
                              DEF_HBASE_XML_FILE, PARCEL_HBASE_LIB, DEF_HBASE_LIB, HDP_HBASE_LIB, TRAF_USER
from scripts.common import Remote, Version, ParseHttp, ParseInI, ParseJson, run_cmd, info, \
                           http_start, http_stop, format_output, err_m, expNumRe

# init global cfgs for user input
cfgs = defaultdict(str)

class HadoopDiscover(object):
    """ discover for hadoop related info """
    def __init__(self, user, pwd, url, cluster_name):
        self.rsnodes = []
        self.users = {}
        self.cluster_name = cluster_name
        self.hg = ParseHttp(user, pwd)
        self.url = url
        self.v1_url = '%s/api/v1/clusters' % self.url
        self.v6_url = '%s/api/v6/clusters' % self.url
        self.cluster_url = '%s/%s' % (self.v1_url, cluster_name.replace(' ', '%20'))
        self._get_distro()
        self._check_version()
        if 'CDH' in self.distro:
            self.cm = self.hg.get('%s/api/v6/cm/deployment' % self.url)

    def _get_distro(self):
        content = self.hg.get(self.v1_url)

        if content['items'][0].has_key('name'):
            # use v6 rest api for CDH to get fullversion
            content = self.hg.get(self.v6_url)

        # loop all managed clusters
        for cluster in content['items']:
            try:
                # HDP
                self.distro = cluster['Clusters']['version']
            except KeyError:
                # CDH
                try:
                    if self.cluster_name == cluster['displayName']:
                        self.distro = 'CDH' + cluster['fullVersion']
                        break
                except KeyError:
                    log_err('Failed to get hadoop distribution info from management url')

    def get_hdfs_srvname(self):
        return self._get_service_name('HDFS')

    def get_hbase_srvname(self):
        return self._get_service_name('HBASE')

    def get_zookeeper_srvname(self):
        return self._get_service_name('ZOOKEEPER')

    def _get_service_name(self, service):
        # CDH uses different service names in multiple clusters
        if 'CDH' in self.distro:
            services_cfgs = self.hg.get(self.cluster_url +'/services')
            for item in services_cfgs['items']:
                if item['type'] == service:
                    return item['name']
        else:
            return service.lower()

    def _check_version(self):
        version = Version()
        if 'CDH' in self.distro: version_list = version.get_version('cdh')
        if 'HDP' in self.distro: version_list = version.get_version('hdp')

        has_version = 0
        for ver in version_list:
            if ver in self.distro: has_version = 1

        if not has_version:
            log_err('Sorry, currently Trafodion doesn\'t support %s version' % self.distro)

    def get_hadoop_users(self):
        if 'CDH' in self.distro:
            self._get_cdh_users()
        elif 'HDP' in self.distro or 'BigInsights' in self.distro:
            self._get_hdp_users()
        return self.users

    def _get_hdp_users(self):
        desired_cfg = self.hg.get('%s/?fields=Clusters/desired_configs' % (self.cluster_url))
        config_type = {'hbase-env':'hbase_user', 'hadoop-env':'hdfs_user'}
        for key, value in config_type.items():
            desired_tag = desired_cfg['Clusters']['desired_configs'][key]['tag']
            current_cfg = self.hg.get('%s/configurations?type=%s&tag=%s' % (self.cluster_url, key, desired_tag))
            self.users[value] = current_cfg['items'][0]['properties'][value]

    def _get_cdh_users(self):
        def _get_username(service_name, hadoop_type):
            cfg = self.hg.get('%s/services/%s/config' % (self.cluster_url, service_name))
            if cfg.has_key('items'):
                for item in cfg['items']:
                    if item['name'] == 'process_username':
                        return item['value']
            return hadoop_type

        hdfs_user = _get_username(self.get_hdfs_srvname(), 'hdfs')
        hbase_user = _get_username(self.get_hbase_srvname(), 'hbase')

        self.users = {'hbase_user':hbase_user, 'hdfs_user':hdfs_user}

    def get_rsnodes(self):
        if 'CDH' in self.distro:
            self._get_rsnodes_cdh()
        elif 'HDP' in self.distro or 'BigInsights' in self.distro:
            self._get_rsnodes_hdp()

        self.rsnodes.sort()
        # use short hostname
        try:
            self.rsnodes = [re.match(r'([\w\-]+).*', node).group(1) for node in self.rsnodes]

        except AttributeError:
            pass
        return self.rsnodes

    def _get_rsnodes_cdh(self):
        """ get list of HBase RegionServer nodes in CDH """
        hostids = []
        for c in self.cm['clusters']:
            if c['displayName'] == self.cluster_name:
                for s in c['services']:
                    if s['type'] == 'HBASE':
                        for r in s['roles']:
                            if r['type'] == 'REGIONSERVER': hostids.append(r['hostRef']['hostId'])
        for i in hostids:
            for h in self.cm['hosts']:
                if i == h['hostId']: self.rsnodes.append(h['hostname'])

    def _get_rsnodes_hdp(self):
        """ get list of HBase RegionServer nodes in HDP """
        hdp = self.hg.get('%s/services/HBASE/components/HBASE_REGIONSERVER' % self.cluster_url)
        self.rsnodes = [c['HostRoles']['host_name'] for c in hdp['host_components']]

    def get_hbase_lib_path(self):
        if 'CDH' in self.distro:
            for c in self.cm['clusters']:
                if c['displayName'] == self.cluster_name:
                    parcels = c['parcels']
            if parcels:
                parcel_config = self.hg.get('%s/api/v6/cm/allHosts/config' % self.url)
                # custom parcel dir exists
                if parcel_config['items'] and parcel_config['items'][0]['name'] == 'parcels_directory':
                    hbase_lib_path = parcel_config['items'][0]['value'] + '/CDH/lib/hbase/lib'
                else:
                    hbase_lib_path = PARCEL_HBASE_LIB
            else:
                hbase_lib_path = DEF_HBASE_LIB
        elif 'HDP' in self.distro:
            hbase_lib_path = HDP_HBASE_LIB

        return hbase_lib_path

class UserInput(object):
    def __init__(self, options, pwd):
        self.in_data = ParseJson(USER_PROMPT_FILE).load()
        self.pwd = pwd

    def _basic_check(self, name, answer):
        isYN = self.in_data[name].has_key('isYN')
        isdigit = self.in_data[name].has_key('isdigit')
        isexist = self.in_data[name].has_key('isexist')
        isfile = self.in_data[name].has_key('isfile')
        isremote_exist = self.in_data[name].has_key('isremote_exist')
        isIP = self.in_data[name].has_key('isIP')
        isuser = self.in_data[name].has_key('isuser')

        # check answer value basicly
        answer = answer.strip()
        if answer:
            if isYN:
                answer = answer.upper()
                if answer != 'Y' and answer != 'N':
                    log_err('Invalid parameter for %s, should be \'Y|y|N|n\'' % name)
            elif isdigit:
                if not answer.isdigit():
                    log_err('Invalid parameter for %s, should be a number' % name)
            elif isexist:
                if not os.path.exists(answer):
                    log_err('%s path \'%s\' doesn\'t exist' % (name, answer))
            elif isfile:
                if not os.path.isfile(answer):
                    log_err('%s file \'%s\' doesn\'t exist' % (name, answer))
            elif isremote_exist:
                hosts = cfgs['node_list'].split(',')
                remotes = [Remote(host, pwd=self.pwd) for host in hosts]

                nodes = ''
                for remote in remotes:
                    # check if directory exists on remote host
                    remote.execute('ls %s 2>&1 >/dev/null' % answer, chkerr=False)
                    if remote.rc != 0:
                        nodes += ' ' + remote.host
                if nodes:
                    log_err('%s path \'%s\' doesn\'t exist on node(s) \'%s\'' % (name, answer, nodes))
            elif isIP:
                try:
                    socket.inet_pton(socket.AF_INET, answer)
                except:
                    log_err('Invalid IP address \'%s\'' % answer)
            elif isuser:
                if re.match(r'\w+', answer).group() != answer:
                    log_err('Invalid user name \'%s\'' % answer)

        else:
            log_err('Empty value for \'%s\'' % name)

    def _handle_prompt(self, name, user_defined):
        prompt = self.in_data[name]['prompt']
        default = user_defined

        if (not default) and self.in_data[name].has_key('default'):
            default = self.in_data[name]['default']

        ispasswd = self.in_data[name].has_key('ispasswd')
        isYN = self.in_data[name].has_key('isYN')

        # no default value for password
        if ispasswd: default = ''

        if isYN:
            prompt = prompt + ' (Y/N) '

        if default:
            prompt = prompt + ' [' + default + ']: '
        else:
            prompt = prompt + ': '

        # no default value for password
        if ispasswd:
            orig = getpass.getpass(prompt)
            confirm = getpass.getpass('Confirm ' + prompt)
            if orig == confirm:
                answer = confirm
            else:
                log_err('Password mismatch')
        else:
            try:
                answer = raw_input(prompt)
            except UnicodeEncodeError:
                log_err('Character Encode error, check user input')
            if not answer and default: answer = default

        return answer

    def get_input(self, name, user_defined='', prompt_mode=True):
        if self.in_data.has_key(name):
            if prompt_mode:
                # save configs to global dict
                cfgs[name] = self._handle_prompt(name, user_defined)

            # check basic values from global configs
            self._basic_check(name, cfgs[name])
        else:
            # should not go to here, just in case
            log_err('Invalid prompt')

    def get_confirm(self):
        answer = raw_input('Confirm result (Y/N) [N]: ')
        if not answer: answer = 'N'

        answer = answer.upper()
        if answer != 'Y' and answer != 'N':
            log_err('Invalid parameter, should be \'Y|y|N|n\'')
        return answer

    def notify_user(self):
        """ show the final configs to user """
        format_output('Final Configs')
        title = ['config type', 'value']
        pt = PrettyTable(title)
        for item in title:
            pt.align[item] = 'l'

        for key, value in sorted(cfgs.items()):
            # only notify user input value
            if self.in_data.has_key(key) and value:
                if self.in_data[key].has_key('ispasswd'): continue
                pt.add_row([key, value])
        print pt
        confirm = self.get_confirm()
        if confirm != 'Y':
            if os.path.exists(DBCFG_FILE): os.remove(DBCFG_FILE)
            run_cmd('rm -rf %s/*.status' % INSTALLER_LOC)
            log_err('User quit')


def log_err(errtext):
    # save tmp config files
    tp = ParseInI(DBCFG_TMP_FILE, 'dbconfigs')
    tp.save(cfgs)
    err_m(errtext)


def user_input(options, prompt_mode=True, pwd=''):
    """ get user's input and check input value """
    global cfgs

    apache = True if hasattr(options, 'apache') and options.apache else False
    offline = True if hasattr(options, 'offline') and options.offline else False
    silent = True if hasattr(options, 'silent') and options.silent else False

    # load from temp config file if in prompt mode
    if os.path.exists(DBCFG_TMP_FILE) and prompt_mode == True:
        tp = ParseInI(DBCFG_TMP_FILE, 'dbconfigs')
        cfgs = tp.load()
        if not cfgs:
            # set cfgs to defaultdict again
            cfgs = defaultdict(str)

    u = UserInput(options, pwd)
    g = lambda n: u.get_input(n, cfgs[n], prompt_mode=prompt_mode)

    ### begin user input ###
    if apache:
        g('node_list')
        node_lists = expNumRe(cfgs['node_list'])

        # check if node list is expanded successfully
        if len([1 for node in node_lists if '[' in node]):
            log_err('Failed to expand node list, please check your input.')
        cfgs['node_list'] = ','.join(node_lists)
        g('hadoop_home')
        g('hbase_home')
        g('hive_home')
        g('hdfs_user')
        g('hbase_user')
        g('first_rsnode')
        cfgs['distro'] = 'APACHE'
    else:
        g('mgr_url')
        if not ('http:' in cfgs['mgr_url'] or 'https:' in cfgs['mgr_url']):
            cfgs['mgr_url'] = 'http://' + cfgs['mgr_url']

        # set cloudera default port 7180 if not provided by user
        if not re.search(r':\d+', cfgs['mgr_url']):
            cfgs['mgr_url'] += ':7180'

        g('mgr_user')
        g('mgr_pwd')

        validate_url_v1 = '%s/api/v1/clusters' % cfgs['mgr_url']
        content = ParseHttp(cfgs['mgr_user'], cfgs['mgr_pwd']).get(validate_url_v1)

        # currently only CDH support multiple clusters
        # so if condition is true, it must be CDH cluster
        if len(content['items']) > 1:
            cluster_names = []
            # loop all managed clusters
            for cluster in content['items']:
                cluster_names.append(cluster['name'])

            for index, name in enumerate(cluster_names):
                print str(index + 1) + '. ' + name
            g('cluster_no')
            c_index = int(cfgs['cluster_no']) - 1
            if c_index < 0 or c_index >= len(cluster_names):
                log_err('Incorrect number')
            cluster_name = cluster_names[int(c_index)]
        else:
            try:
                cluster_name = content['items'][0]['name']
            except (IndexError, KeyError):
                try:
                    cluster_name = content['items'][0]['Clusters']['cluster_name']
                except (IndexError, KeyError):
                    log_err('Failed to get cluster info from management url')


        hadoop_discover = HadoopDiscover(cfgs['mgr_user'], cfgs['mgr_pwd'], cfgs['mgr_url'], cluster_name)
        rsnodes = hadoop_discover.get_rsnodes()
        hadoop_users = hadoop_discover.get_hadoop_users()

        cfgs['distro'] = hadoop_discover.distro
        cfgs['hbase_lib_path'] = hadoop_discover.get_hbase_lib_path()
        cfgs['hbase_service_name'] = hadoop_discover.get_hbase_srvname()
        cfgs['hdfs_service_name'] = hadoop_discover.get_hdfs_srvname()
        cfgs['zookeeper_service_name'] = hadoop_discover.get_zookeeper_srvname()

        cfgs['cluster_name'] = cluster_name.replace(' ', '%20')
        cfgs['hdfs_user'] = hadoop_users['hdfs_user']
        cfgs['hbase_user'] = hadoop_users['hbase_user']
        cfgs['node_list'] = ','.join(rsnodes)
        cfgs['first_rsnode'] = rsnodes[0] # first regionserver node

    # check node connection
    for node in cfgs['node_list'].split(','):
        rc = os.system('ping -c 1 %s >/dev/null 2>&1' % node)
        if rc: log_err('Cannot ping %s, please check network connection and /etc/hosts' % node)

    # set some system default configs
    cfgs['config_created_date'] = time.strftime('%Y/%m/%d %H:%M %Z')
    cfgs['traf_user'] = TRAF_USER
    if apache:
        cfgs['hbase_xml_file'] = cfgs['hbase_home'] + '/conf/hbase-site.xml'
        cfgs['hdfs_xml_file'] = cfgs['hadoop_home'] + '/etc/hadoop/hdfs-site.xml'
    else:
        cfgs['hbase_xml_file'] = DEF_HBASE_XML_FILE

    ### discover system settings, return a dict
    system_discover = wrapper.run(cfgs, options, mode='discover', pwd=pwd)

    # check discover results, return error if fails on any sinlge node
    need_java_home = 0
    has_home_dir = 0
    for result in system_discover:
        host, content = result.items()[0]
        content_dict = json.loads(content)

        java_home = content_dict['default_java']
        if java_home == 'N/A':
            need_java_home += 1
        if content_dict['linux'] == 'N/A':
            log_err('Unsupported Linux version')
        if content_dict['firewall_status'] == 'Running':
            info('Firewall is running, please make sure the ports used by Trafodion are open')
        if content_dict['traf_status'] == 'Running':
            log_err('Trafodion process is found, please stop it first')
        if content_dict['hbase'] == 'N/A':
            log_err('HBase is not found')
        if content_dict['hbase'] == 'N/S':
            log_err('HBase version is not supported')
        else:
            cfgs['hbase_ver'] = content_dict['hbase']
        if content_dict['home_dir']: # trafodion user exists
            has_home_dir += 1
            cfgs['home_dir'] = content_dict['home_dir']
        if content_dict['hadoop_authentication'] == 'kerberos':
            cfgs['secure_hadoop'] = 'Y'
        else:
            cfgs['secure_hadoop'] = 'N'

    if offline:
        g('local_repo_dir')
        if not glob('%s/repodata' % cfgs['local_repo_dir']):
            log_err('repodata directory not found, this is not a valid repository directory')
        cfgs['offline_mode'] = 'Y'
        cfgs['repo_ip'] = socket.gethostbyname(socket.gethostname())
        ports = ParseInI(DEF_PORT_FILE, 'ports').load()
        cfgs['repo_http_port'] = ports['repo_http_port']

    pkg_list = ['apache-trafodion']
    # find tar in installer folder, if more than one found, use the first one
    for pkg in pkg_list:
        tar_loc = glob('%s/*%s*.tar.gz' % (INSTALLER_LOC, pkg))
        if tar_loc:
            cfgs['traf_package'] = tar_loc[0]
            break

    g('traf_package')
    cfgs['req_java8'] = 'N'

    # get basename and version from tar filename
    try:
        pattern = '|'.join(pkg_list)
        cfgs['traf_basename'], cfgs['traf_version'] = re.search(r'.*(%s).*-(\d\.\d\.\d).*' % pattern, cfgs['traf_package']).groups()
    except:
        log_err('Invalid package tar file')

    if not cfgs['traf_dirname']:
        cfgs['traf_dirname'] = '%s-%s' % (cfgs['traf_basename'], cfgs['traf_version'])
    g('traf_dirname')
    if not has_home_dir:
        g('traf_pwd')
    g('dcs_cnt_per_node')
    g('scratch_locs')
    g('traf_start')

    # kerberos
    if cfgs['secure_hadoop'].upper() == 'Y':
        g('kdc_server')
        g('admin_principal')
        g('kdcadmin_pwd')

    # ldap security
    g('ldap_security')
    if cfgs['ldap_security'].upper() == 'Y':
        g('db_root_user')
        g('ldap_hosts')
        g('ldap_port')
        g('ldap_identifiers')
        g('ldap_encrypt')
        if  cfgs['ldap_encrypt'] == '1' or cfgs['ldap_encrypt'] == '2':
            g('ldap_certpath')
        elif cfgs['ldap_encrypt'] == '0':
            cfgs['ldap_certpath'] = ''
        else:
            log_err('Invalid ldap encryption level')

        g('ldap_userinfo')
        if cfgs['ldap_userinfo'] == 'Y':
            g('ldap_user')
            g('ldap_pwd')
        else:
            cfgs['ldap_user'] = ''
            cfgs['ldap_pwd'] = ''

    # DCS HA
    g('dcs_ha')
    cfgs['enable_ha'] = 'false'
    if cfgs['dcs_ha'].upper() == 'Y':
        g('dcs_floating_ip')
        g('dcs_interface')
        g('dcs_backup_nodes')
        # check dcs backup nodes should exist in node list
        if sorted(list(set((cfgs['dcs_backup_nodes'] + ',' + cfgs['node_list']).split(',')))) != sorted(cfgs['node_list'].split(',')):
            log_err('Invalid DCS backup nodes, please pick up from node list')
        cfgs['enable_ha'] = 'true'

    if need_java_home:
        g('java_home')
    else:
        # don't overwrite user input java home
        if not cfgs['java_home']:
            cfgs['java_home'] = java_home


    if not silent:
        u.notify_user()

def get_options():
    usage = 'usage: %prog [options]\n'
    usage += '  Trafodion install main script.'
    parser = OptionParser(usage=usage)
    parser.add_option("-c", "--config-file", dest="cfgfile", metavar="FILE",
                      help="Json format file. If provided, all install prompts \
                            will be taken from this file and not prompted for.")
    parser.add_option("-u", "--remote-user", dest="user", metavar="USER",
                      help="Specify ssh login user for remote server, \
                            if not provided, use current login user as default.")
    parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False,
                      help="Verbose mode, will print commands.")
    parser.add_option("--silent", action="store_true", dest="silent", default=False,
                      help="Do not ask user to confirm configuration result")
    parser.add_option("--enable-pwd", action="store_true", dest="pwd", default=False,
                      help="Prompt SSH login password for remote hosts. \
                            If set, \'sshpass\' tool is required.")
    parser.add_option("--build", action="store_true", dest="build", default=False,
                      help="Build the config file in guided mode only.")
    parser.add_option("--reinstall", action="store_true", dest="reinstall", default=False,
                      help="Reinstall Trafodion without restarting Hadoop.")
    parser.add_option("--apache-hadoop", action="store_true", dest="apache", default=False,
                      help="Install Trafodion on top of Apache Hadoop.")
    parser.add_option("--offline", action="store_true", dest="offline", default=False,
                      help="Enable local repository for offline installing Trafodion.")

    (options, args) = parser.parse_args()
    return options

def main():
    """ db_installer main loop """
    global cfgs
    format_output('Trafodion Installation ToolKit')

    # handle parser option
    options = get_options()

    if options.build and options.cfgfile:
        log_err('Wrong parameter, cannot specify both --build and --config-file')

    if options.build and options.offline:
        log_err('Wrong parameter, cannot specify both --build and --offline')

    if options.cfgfile:
        if not os.path.exists(options.cfgfile):
            log_err('Cannot find config file \'%s\'' % options.cfgfile)
        config_file = options.cfgfile
    else:
        config_file = DBCFG_FILE

    if options.pwd:
        pwd = getpass.getpass('Input remote host SSH Password: ')
    else:
        pwd = ''

    # not specified config file and default config file doesn't exist either
    p = ParseInI(config_file, 'dbconfigs')
    if options.build or (not os.path.exists(config_file)):
        if options.build: format_output('DryRun Start')
        user_input(options, prompt_mode=True, pwd=pwd)

        # save config file as json format
        print '\n** Generating config file to save configs ... \n'
        p.save(cfgs)
    # config file exists
    else:
        print '\n** Loading configs from config file ... \n'
        cfgs = p.load()
        if options.offline and cfgs['offline_mode'] != 'Y':
            log_err('To enable offline mode, must set "offline_mode = Y" in config file')
        user_input(options, prompt_mode=False, pwd=pwd)

    if options.reinstall:
        cfgs['reinstall'] = 'Y'

    if options.offline:
        http_start(cfgs['local_repo_dir'], cfgs['repo_http_port'])
    else:
        cfgs['offline_mode'] = 'N'

    if not options.build:
        format_output('Installation Start')

        ### perform actual installation ###
        wrapper.run(cfgs, options, pwd=pwd)

        format_output('Installation Complete')

        if options.offline: http_stop()

        # rename default config file when successfully installed
        # so next time user can input new variables for a new install
        # or specify the backup config file to install again
        try:
            # only rename default config file
            ts = time.strftime('%y%m%d_%H%M')
            if config_file == DBCFG_FILE and os.path.exists(config_file):
                os.rename(config_file, config_file + '.bak' + ts)
        except OSError:
            log_err('Cannot rename config file')
    else:
        format_output('DryRun Complete')

    # remove temp config file
    if os.path.exists(DBCFG_TMP_FILE): os.remove(DBCFG_TMP_FILE)

if __name__ == "__main__":
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        tp = ParseInI(DBCFG_TMP_FILE, 'dbconfigs')
        tp.save(cfgs)
        http_stop()
        print '\nAborted...'
