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

### The common functions ###

import os
import pty
import sys
reload(sys)
sys.setdefaultencoding("utf-8")
import json
import re
import time
import base64
import subprocess
import logging
try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET
from ConfigParser import ConfigParser
from collections import defaultdict
from constants import VERSION_FILE, PARCEL_HBASE_LIB, DEF_HBASE_LIB, HDP_HBASE_LIB

MARK = '[ERR]'

def ok(msg):
    print '\n\33[32m***[OK]: %s \33[0m' % msg

def info(msg):
    print '\n\33[33m***[INFO]: %s \33[0m' % msg

def err_m(msg):
    """ used by main script """
    sys.stderr.write('\n\33[31m***[ERROR]: %s \33[0m\n' % msg)
    sys.exit(1)

def err(msg):
    """ used by sub script """
    sys.stderr.write(MARK + msg)
    sys.exit(1)

def get_logger(log_file):

    log_dir = os.path.dirname(log_file)
    if not os.path.exists(log_dir): os.mkdir(log_dir)

    logger = logging.getLogger()
    logger.setLevel(logging.INFO)

    formatter = logging.Formatter('[%(asctime)s %(levelname)s]: %(message)s')

    fh = logging.FileHandler(log_file)
    fh.setFormatter(formatter)

    logger.addHandler(fh)

    return logger

def run_cmd(cmd):
    """ check command return value and return stdout """
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
        msg = stderr if stderr else stdout
        err('Failed to run command %s: %s' % (cmd, msg))
    return stdout.strip()

def run_cmd_as_user(user, cmd):
    return run_cmd('%s su - %s -c \'%s\'' % (get_sudo_prefix(), user, cmd))

def cmd_output(cmd):
    """ return command output but not check return value """
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    stdout, stderr = p.communicate()

    return stdout.strip() if stdout else stderr

def get_default_home():
    return cmd_output('%s cat /etc/default/useradd |grep HOME |cut -d "=" -f 2' % get_sudo_prefix())

def get_sudo_prefix():
    """ donnot use sudo prefix if user is root """
    uid = os.getuid()
    if uid == 0:
        return ''
    else:
        return 'sudo -n'

def mod_file(template_file, change_items):
    """
        @params: change_items: a dict includes:
        {regular_expression : replace_string}
    """
    try:
        with open(template_file, 'r') as f:
            lines = f.read()
    except IOError:
        err('Failed to open file %s to modify' % template_file)

    for regexp, replace in change_items.iteritems():
        lines = re.sub(regexp, replace, lines)

    with open(template_file, 'w') as f:
        f.write(lines)

def append_file(template_file, string, position=''):
    try:
        with open(template_file, 'r') as f:
            lines = f.readlines()
        pos = 0
        if position:
            for index, line in enumerate(lines):
                if position in line:
                    pos = index + 1

        if pos == 0: pos = len(lines)
        string_lf = string + '\n'
        newlines = lines[:pos] + [string_lf] + lines[pos:]
        if not string_lf in lines:
            with open(template_file, 'w') as f:
                f.writelines(newlines)
    except IOError:
        err('Failed to open file %s to append' % template_file)


def write_file(template_file, string):
    try:
        with open(template_file, 'w') as f:
            f.write(string)
    except IOError:
        err('Failed to open file %s to write' % template_file)

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
                    err_m('Failed to get hadoop distribution info from management url')

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
            err_m('Sorry, currently Trafodion doesn\'t support %s version' % self.distro)

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
        ## use short hostname
        #try:
        #    self.rsnodes = [re.match(r'([\w\-]+).*', node).group(1) for node in self.rsnodes]
        #except AttributeError:
        #    pass
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

            cdh_parcel_enabled = False
            for parcel in parcels:
                if parcel['product'] == 'CDH':
                    cdh_parcel_enabled = True
                    break

            if cdh_parcel_enabled:
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

class Version(object):
    def __init__(self):
        self.support_ver = ParseJson(VERSION_FILE).load()

    def get_version(self, component):
        if self.support_ver[component] == '':
            err('Failed to get version info for "%s" from config file' % component)

        return self.support_ver[component]

class Remote(object):
    """
        copy files to/fetch files from remote host using ssh
        can also use paramiko, but it's not a build-in module
    """

    def __init__(self, host, user='', pwd=''):
        self.host = host
        self.user = user
        self.rc = 0
        self.pwd = pwd
        self.sshpass = self._sshpass_available()
        self._connection_test()

    @staticmethod
    def _sshpass_available():
        sshpass_available = True
        try:
            p = subprocess.Popen(['sshpass'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            p.communicate()
        except OSError:
            sshpass_available = False

        return sshpass_available

    def _commands(self, method):
        cmd = []
        if self.sshpass and self.pwd: cmd = ['sshpass', '-p', self.pwd]
        cmd += [method]
        if not (self.sshpass and self.pwd): cmd += ['-oPasswordAuthentication=no']
        return cmd

    def _execute(self, cmd, verbose=False, shell=False):
        try:
            if verbose: print 'cmd:', cmd

            master, slave = pty.openpty()
            if shell:
                p = subprocess.Popen(cmd, stdin=slave, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
            else:
                p = subprocess.Popen(cmd, stdin=slave, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            self.stdout, self.stderr = p.communicate()
            if p.returncode:
                self.rc = p.returncode
                # 'ssh -tt' will overwrite stderr, so manually handle it
                if MARK in self.stdout:
                    self.stdout, self.stderr = self.stdout.split(MARK)
                else:
                    self.stderr = self.stdout
        except Exception as e:
            err_m('Failed to run commands on remote host: %s' % e)

    def _connection_test(self):
        self.execute('echo -n', chkerr=False)
        if self.rc != 0:
            msg = 'Host [%s]: Failed to connect using ssh. Be sure:\n' % self.host
            msg += '1. Remote host\'s name and IP is configured correctly in /etc/hosts.\n'
            msg += '2. Remote host\'s sshd service is running.\n'
            msg += '3. Passwordless SSH is set if not using \'enable-pwd\' option.\n'
            msg += '4. \'sshpass\' tool is installed and ssh password is correct if using \'enable-pwd\' option.\n'
            err_m(msg)

    def execute(self, user_cmd, verbose=False, shell=False, chkerr=True):
        """ @params: user_cmd should be a string """
        cmd = self._commands('ssh')
        cmd += ['-tt'] # force tty allocation
        if self.user:
            cmd += ['%s@%s' % (self.user, self.host)]
        else:
            cmd += [self.host]

        # if shell=True, cmd should be a string not list
        if shell:
            cmd = ' '.join(cmd) + ' '
            cmd += user_cmd
        else:
            cmd += user_cmd.split()

        self._execute(cmd, verbose=verbose, shell=shell)

        if chkerr and self.rc != 0:
            err_m('Failed to execute command on remote host [%s]: "%s"' % (self.host, user_cmd))

    def copy(self, files, remote_folder='.'):
        """ copy file to user's home folder """
        for f in files:
            if not os.path.exists(f):
                err_m('Copy file error: %s doesn\'t exist' % f)

        cmd = self._commands('scp')
        cmd += ['-r']
        cmd += files # files should be full path
        if self.user:
            cmd += ['%s@%s:%s/' % (self.user, self.host, remote_folder)]
        else:
            cmd += ['%s:%s/' % (self.host, remote_folder)]

        self._execute(cmd)
        if self.rc != 0: err_m('Failed to copy files to remote nodes')

    def fetch(self, files, local_folder='.'):
        """ fetch file from user's home folder """
        cmd = self._commands('scp')
        cmd += ['-r']
        if self.user:
            cmd += ['%s@%s:~/{%s}' % (self.user, self.host, ','.join(files))]
        else:
            cmd += ['%s:~/{%s}' % (self.host, ','.join(files))]
        cmd += [local_folder]

        self._execute(cmd)
        if self.rc != 0: err('Failed to fetch files from remote nodes')


class ParseHttp(object):
    def __init__(self, user, passwd, json_type=True):
        # httplib2 is not installed by default
        try:
            import httplib2
        except ImportError:
            err_m('Python module httplib2 is not found. Install python-httplib2 first.')

        self.user = user
        self.passwd = passwd
        self.h = httplib2.Http(disable_ssl_certificate_validation=True)
        self.h.add_credentials(self.user, self.passwd)
        self.headers = {}
        self.headers['X-Requested-By'] = 'trafodion'
        if json_type:
            self.headers['Content-Type'] = 'application/json'
        self.headers['Authorization'] = 'Basic %s' % (base64.b64encode('%s:%s' % (self.user, self.passwd)))

    def _request(self, url, method, body=None):
        try:
            resp, content = self.h.request(url, method, headers=self.headers, body=body)
            # return code is not 2xx and 409(for ambari blueprint)
            if not (resp.status == 409 or 200 <= resp.status < 300):
                err_m('Error return code {0} when {1}ting configs: {2}'.format(resp.status, method.lower(), content))
            return content
        except Exception as exc:
            err_m('Error with {0}ting configs using URL {1}. Reason: {2}'.format(method.lower(), url, exc))

    def get(self, url):
        try:
            return defaultdict(str, json.loads(self._request(url, 'GET')))
        except ValueError:
            err_m('Failed to get data from URL, check password if URL requires authentication')

    def put(self, url, config):
        if not isinstance(config, dict): err_m('Wrong HTTP PUT parameter, should be a dict')
        result = self._request(url, 'PUT', body=json.dumps(config))
        if result: return defaultdict(str, json.loads(result))

    def post(self, url, config=None):
        try:
            if config:
                if not isinstance(config, dict): err_m('Wrong HTTP POST parameter, should be a dict')
                body = json.dumps(config)
            else:
                body = None

            result = self._request(url, 'POST', body=body)
            if result: return defaultdict(str, json.loads(result))

        except ValueError as ve:
            err_m('Failed to send command to URL: %s' % ve)


class ParseXML(object):
    """ handle *-site.xml with format
        <property><name></name><value></value></proerty>
    """
    def __init__(self, xml_file):
        self.__xml_file = xml_file
        if not os.path.exists(self.__xml_file): err_m('Cannot find xml file %s' % self.__xml_file)
        try:
            self._tree = ET.parse(self.__xml_file)
        except Exception as e:
            err_m('failed to parsing xml: %s' % e)

        self._root = self._tree.getroot()
        self._properties = self._root.findall('property')
        # name, value list
        self._nvlist = [[elem.text for elem in p] for p in self._properties]

    def __indent(self, elem):
        """Return a pretty-printed XML string for the Element."""
        if len(elem):
            if not elem.text: elem.text = '\n' + '  '
            if not elem.tail: elem.tail = '\n'
            for subelem in elem:
                self.__indent(subelem)
        else:
            if not elem.tail: elem.tail = '\n' + '  '

    def get_property(self, name):
        try:
            return [x[1] for x in self._nvlist if x[0] == name][0]
        except:
            return ''

    def rm_property(self, name):
        for p in self._properties:
            if p[0].text == name:
                self._root.remove(p)

    def add_property(self, name, value):
        # don't add property if already exists
        if self.get_property(name): return

        elem_p = ET.Element('property')
        elem_name = ET.Element('name')
        elem_value = ET.Element('value')

        elem_name.text = name
        elem_value.text = value
        elem_p.append(elem_name)
        elem_p.append(elem_value)

        self._nvlist.append([name, value])
        self._root.append(elem_p)

    def write_xml(self):
        self.__indent(self._root)
        self._tree.write(self.__xml_file)

    def print_xml(self):
        for name, value in self._nvlist:
            print name, value

class ParseJson(object):
    def __init__(self, js_file):
        self.__js_file = js_file

    def load(self):
        """ load json file to a dict """
        if not os.path.exists(self.__js_file): err_m('Cannot find json file %s' % self.__js_file)
        with open(self.__js_file, 'r') as f:
            tmparray = f.readlines()
        content = ''
        for t in tmparray:
            content += t

        try:
            return defaultdict(str, json.loads(content))
        except ValueError:
            err_m('No json format found in config file %s' % self.__js_file)

    def save(self, dic):
        """ save dict to json file with pretty format """
        with open(self.__js_file, 'w') as f:
            f.write(json.dumps(dic, indent=4))
        return 0


class ParseInI(object):
    def __init__(self, ini_file, section):
        self.__ini_file = ini_file
        self.section = section

    def load(self):
        """ load content from ini file and return a dict """
        if not os.path.exists(self.__ini_file):
            err_m('Cannot find ini file %s' % self.__ini_file)

        cfgs = {}
        cf = ConfigParser()
        cf.read(self.__ini_file)

        if not cf.has_section(self.section):
            return {}

        for cfg in cf.items(self.section):
            cfgs[cfg[0]] = cfg[1]

        return defaultdict(str, cfgs)

    def save(self, dic):
        """ save a dict as an ini file """
        cf = ConfigParser()
        cf.add_section(self.section)
        for key, value in dic.iteritems():
            cf.set(self.section, key, value)

        with open(self.__ini_file, 'w') as f:
            cf.write(f)

def http_start(repo_dir, repo_port):
    info('Starting temporary python http server')
    os.system("cd %s; python -m SimpleHTTPServer %s > /dev/null 2>&1 &" % (repo_dir, repo_port))

def http_stop():
    #info('Stopping temporary python http server')
    os.system("ps -ef|grep SimpleHTTPServer |grep -v grep | awk '{print $2}' |xargs kill -9 >/dev/null 2>&1")

def format_output(text):
    num = len(text) + 4
    print '*' * num
    print '  ' + text
    print '*' * num

def expNumRe(text):
    """
    expand numeric regular expression to list
    e.g. 'n[01-03],n1[0-1]': ['n01','n02','n03','n10','n11']
    e.g. 'n[09-11].com': ['n09.com','n10.com','n11.com']
    """
    explist = []
    for regex in text.split(','):
        regex = regex.strip()
        r = re.match(r'(.*)\[(\d+)-(\d+)\](.*)', regex)
        if r:
            h = r.group(1)
            d1 = r.group(2)
            d2 = r.group(3)
            t = r.group(4)

            convert = lambda d: str(('%0' + str(min(len(d1), len(d2))) + 'd') % d)
            if d1 > d2: d1, d2 = d2, d1
            explist.extend([h + convert(c) + t for c in range(int(d1), int(d2)+1)])

        else:
            # keep original value if not matched
            explist.append(regex)

    return explist

def time_elapse(func):
    """ time elapse decorator """
    def wrapper(*args, **kwargs):
        start_time = time.time()
        output = func(*args, **kwargs)
        end_time = time.time()
        seconds = end_time - start_time
        hours = seconds / 3600
        seconds = seconds % 3600
        minutes = seconds / 60
        seconds = seconds % 60
        print '\nTime Cost: %d hour(s) %d minute(s) %d second(s)' % (hours, minutes, seconds)
        return output
    return wrapper

def retry(func, maxcnt, interval, msg):
    """ retry timeout function """
    retry_cnt = 0
    rc = False
    try:
        while not rc:
            retry_cnt += 1
            rc = func()
            flush_str = '.' * retry_cnt
            print '\rCheck %s status (timeout: %d secs) %s' % (msg, maxcnt * interval, flush_str),
            sys.stdout.flush()
            time.sleep(interval)
            if retry_cnt == maxcnt:
                err_m('Timeout exit')
    except KeyboardInterrupt:
        err_m('user quit')
    ok('%s successfully!' % msg)

if __name__ == '__main__':
    exit(0)
