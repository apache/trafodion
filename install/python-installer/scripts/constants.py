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

### The common constants ###

import os
import re

INSTALLER_LOC = re.search('(.*)/\w+',os.path.dirname(os.path.abspath(__file__))).groups()[0]

CONFIG_DIR = INSTALLER_LOC + '/configs'
SCRIPTS_DIR = INSTALLER_LOC + '/scripts'
TEMPLATES_DIR = INSTALLER_LOC + '/templates'

USER_PROMPT_FILE = CONFIG_DIR + '/prompt.json'
SCRCFG_FILE = CONFIG_DIR + '/script.json'
VERSION_FILE = CONFIG_DIR + '/version.json'
MODCFG_FILE = CONFIG_DIR + '/mod_cfgs.json'
DEF_PORT_FILE = CONFIG_DIR + '/default_ports.ini'

DBCFG_FILE = INSTALLER_LOC + '/db_config'
DBCFG_TMP_FILE = INSTALLER_LOC + '/.db_config_temp'

SSH_CONFIG_FILE = '/etc/ssh/sshd_config'

SSHKEY_FILE = '/tmp/id_rsa'
TMP_DIR = '/tmp/.trafodion_install_temp'

DEF_HBASE_HOME = '/usr'
DEF_HBASE_XML_FILE = '/etc/hbase/conf/hbase-site.xml'
DEF_CORE_SITE_XML = '/etc/hadoop/conf/core-site.xml'
DEF_HDFS_BIN = '/usr/bin/hdfs'
DEF_HBASE_LIB = '/usr/lib/hbase/lib'
HDP_HBASE_LIB = '/usr/hdp/current/hbase-regionserver/lib'
PARCEL_DIR = '/opt/cloudera/parcels'
PARCEL_HBASE_LIB = PARCEL_DIR + '/CDH/lib/hbase/lib'
PARCEL_HDFS_BIN = PARCEL_DIR + '/CDH/bin/hdfs'

TRAF_HSPERFDATA_FILE = '/tmp/hsperfdata_trafodion'
TRAF_SUDOER_FILE = '/etc/sudoers.d/trafodion'
TRAF_CFG_DIR = '/etc/trafodion/conf'
TRAF_CFG_FILE = '/etc/trafodion/trafodion_config'
TRAF_USER = 'trafodion'
