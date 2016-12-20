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

import re
import json
import sys
import platform
from glob import glob
from common import cmd_output, err, Version, ParseXML

PREFIX = 'get_'
NA = 'N/A' # not available
NS = 'N/S' # not supported
OK = 'OK'

def deco(func):
    def wrapper(self):
        if PREFIX in func.__name__:
            name = func.__name__.replace(PREFIX, '')
            return name, func(self)
        else:
            return
    return wrapper


class Discover(object):
    """ discover functions, to add a new discover function,
        simply add a new def with name get_xx and decorated
        by 'deco', then return result in string format:

        @deco
        def get_xx(self):
            # do something
            return result
    """

    def __init__(self, dbcfgs):
        self.CPUINFO = cmd_output('cat /proc/cpuinfo')
        self.MEMINFO = cmd_output('cat /proc/meminfo')
        self.SYSCTLINFO = cmd_output('sysctl -a')
        self.version = Version()
        self.dbcfgs = dbcfgs

    def _parse_string(self, info, string):
        try:
            info = info.split('\n')
            string_line = [line for line in info if string in line][0]
        except IndexError:
            err('Cannot get %s info' % string)

        return string_line

    def _get_cpu_info(self, string):
        return self._parse_string(self.CPUINFO, string).split(':')[1].strip()

    def _get_mem_info(self, string):
        return self._parse_string(self.MEMINFO, string).split(':')[1].split()[0]

    def _get_sysctl_info(self, string):
        return self._parse_string(self.SYSCTLINFO, string).split('=')[1].strip()

    @deco
    def get_linux(self):
        """ get linux version """
        os_dist, os_ver = platform.dist()[:2]
        if os_dist not in self.version.get_version('linux'):
            return NA
        else:
            if not os_ver.split('.')[0] in self.version.get_version(os_dist):
                return NA
        return '%s-%s' % (os_dist, os_ver)

    @deco
    def get_firewall_status(self):
        """ get firewall running status """
        iptables_stat = cmd_output('iptables -nL|grep -vE "(Chain|target)"').strip()
        if iptables_stat:
            return 'Running'
        else:
            return 'Stopped'

    @deco
    def get_pidmax(self):
        """ get kernel pid max setting """
        return self._get_sysctl_info('kernel.pid_max')

    @deco
    def get_default_java(self):
        """ get default java version """
        jdk_path = glob('/usr/java/*') + \
                   glob('/usr/jdk64/*') + \
                   glob('/usr/lib/jvm/java-*-openjdk.x86_64')

        jdk_list = {} # {jdk_version: jdk_path}
        for path in jdk_path:
            jdk_ver = cmd_output('%s/bin/javac -version' % path)

            try:
                main_ver, sub_ver = re.search(r'(\d\.\d\.\d)_(\d+)', jdk_ver).groups()
                # don't support JDK version less than 1.7.0_65
                if main_ver == '1.7.0' and int(sub_ver) < 65:
                    continue
                jdk_list[main_ver] = path
            except AttributeError:
                continue

        if not jdk_list:
            return NA
        else:
            # use JDK1.8 first
            if jdk_list.has_key('1.8.0'):
                return jdk_list['1.8.0']
            elif jdk_list.has_key('1.7.0'):
                return jdk_list['1.7.0']

    @deco
    def get_hive(self):
        """ get Hive status """
        hive_stat = cmd_output('which hive')
        if 'no hive' in hive_stat:
            return NA
        else:
            return OK

    def _get_core_site_xml(self):
        if self.dbcfgs.has_key('hadoop_home'): # apache distro
            CORE_SITE_XML = '%s/etc/hadoop/core-site.xml' % self.dbcfgs['hadoop_home']
        else:
            CORE_SITE_XML = '/etc/hadoop/conf/core-site.xml'
        p = ParseXML(CORE_SITE_XML)
        return p

    @deco
    def get_hadoop_authentication(self):
        p = self._get_core_site_xml()
        return p.get_property('hadoop.security.authentication')

    @deco
    def get_hadoop_authorization(self):
        p = self._get_core_site_xml()
        return p.get_property('hadoop.security.authorization')

    @deco
    def get_hbase(self):
        """ get HBase version """
        if self.dbcfgs.has_key('hbase_home'): # apache distro
            hbase_home = self.dbcfgs['hbase_home']
            hbase_ver = cmd_output('%s/bin/hbase version | head -n1' % hbase_home)
        else:
            hbase_ver = cmd_output('hbase version | head -n1')

        support_hbase_ver = self.version.get_version('hbase')
        try:
            hbase_ver = re.search(r'HBase (\d\.\d)', hbase_ver).groups()[0]
        except AttributeError:
            return NA
        if hbase_ver not in support_hbase_ver:
            return NS
        return hbase_ver

    @deco
    def get_cpu_model(self):
        """ get CPU model """
        return self._get_cpu_info('model name')

    @deco
    def get_cpu_cores(self):
        """ get CPU cores """
        return self.CPUINFO.count('processor')

    @deco
    def get_arch(self):
        """ get CPU architecture """
        arch = platform.processor()
        if not arch:
            arch = 'Unknown'
        return arch

    @deco
    def get_mem_total(self):
        """ get total memory size """
        mem = self._get_mem_info('MemTotal')
        memsize = mem.split()[0]

        return "%0.1f GB" % round(float(memsize) / (1024 * 1024), 2)

    @deco
    def get_mem_free(self):
        """ get current free memory size """
        free = self._get_mem_info('MemFree')
        buffers = self._get_mem_info('Buffers')
        cached = self._get_mem_info('Cached')
        memfree = float(free) + float(buffers) + float(cached)

        return "%0.1f GB" % round(memfree / (1024 * 1024), 2)

    @deco
    def get_ext_interface(self):
        """ get external network interface """
        return cmd_output('ip route |grep default|awk \'{print $5}\'')

    @deco
    def get_rootdisk_free(self):
        """ get root disk space left """
        space = cmd_output('df -h|grep "\/$" | awk \'{print $4}\'')
        return space.strip()

    @deco
    def get_python_ver(self):
        """ get python version """
        return platform.python_version()

    @deco
    def get_home_dir(self):
        if self.dbcfgs.has_key('traf_user'): # apache distro
            traf_user = self.dbcfgs['traf_user']
            return cmd_output("getent passwd %s | awk -F: '{print $6}' | sed 's/\/%s//g'" % (traf_user, traf_user))
        else:
            return ''

    @deco
    def get_traf_status(self):
        """ get trafodion running status """
        mon_process = cmd_output('ps -ef|grep -v grep|grep -c "monitor COLD"')
        if int(mon_process) > 0:
            return 'Running'
        else:
            return 'Stopped'

def run():
    try:
        dbcfgs_json = sys.argv[1]
    except IndexError:
        err('No db config found')
    dbcfgs = json.loads(dbcfgs_json)
    discover = Discover(dbcfgs)
    methods = [m for m in dir(discover) if m.startswith(PREFIX)]
    result = {}
    for method in methods:
        key, value = getattr(discover, method)() # call method
        result[key] = value

    print json.dumps(result)


# main
if __name__ == '__main__':
    run()
