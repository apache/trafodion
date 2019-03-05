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
from resource_management import *

# config object that holds the configurations declared in the config xml file
config = Script.get_config()

java_home = config['hostLevelParams']['java_home']
java_version = int(config['hostLevelParams']['java_version'])

cluster_name = str(config['clusterName'])

dcs_servers = config['configurations']['dcs-env']['dcs.servers']
dcs_master_port = config['configurations']['dcs-site']['dcs.master.port']
dcs_info_port = config['configurations']['dcs-site']['dcs.master.info.port']
dcs_enable_ha = config['configurations']['dcs-site']['dcs.master.floating.ip']
dcs_floating_ip = config['configurations']['dcs-site']['dcs.master.floating.ip.external.ip.address']
dcs_mast_node_list = default("/clusterHostInfo/traf_dcs_prime_hosts", '')
dcs_back_node_list = default("/clusterHostInfo/traf_dcs_second_hosts", '')
dcs_env_template = config['configurations']['dcs-env']['content']
dcs_log4j_template = config['configurations']['dcs-log4j']['content']

zookeeper_quorum_hosts = ",".join(config['clusterHostInfo']['zookeeper_hosts'])
if 'zoo.cfg' in config['configurations'] and 'clientPort' in config['configurations']['zoo.cfg']:
  zookeeper_clientPort = config['configurations']['zoo.cfg']['clientPort']
else:
  zookeeper_clientPort = '2181'

traf_db_admin = config['configurations']['trafodion-env']['traf.db.admin']

traf_conf_dir = '/etc/trafodion/conf' # path is hard-coded in /etc/trafodion/trafodion_config
traf_env_template = config['configurations']['trafodion-env']['content']
traf_clust_template = config['configurations']['traf-cluster-env']['content']

traf_user = 'trafodion'
traf_group = 'trafodion'
hdfs_user = config['configurations']['hadoop-env']['hdfs_user']
user_group = config['configurations']['cluster-env']['user_group']
hbase_user = config['configurations']['hbase-env']['hbase_user']
hbase_staging = config['configurations']['hbase-site']['hbase.bulkload.staging.dir']

traf_priv_key = config['configurations']['trafodion-env']['traf.sshkey.priv']

traf_node_list = default("/clusterHostInfo/traf_node_hosts", '')

traf_scratch = config['configurations']['trafodion-env']['traf.node.dir']
traf_logdir = config['configurations']['trafodion-env']['traf.log.dir']
traf_vardir = config['configurations']['trafodion-env']['traf.var.dir']

traf_ldap_template = config['configurations']['trafodion-env']['ldap_content']
traf_ldap_enabled = config['configurations']['trafodion-env']['traf.ldap.enabled']
ldap_hosts = ''
for host in config['configurations']['trafodion-env']['traf.ldap.hosts'].split(','):
  ldap_hosts += '  LDAPHostName: %s\n' % host
ldap_port = config['configurations']['trafodion-env']['traf.ldap.port']
ldap_identifiers = ''
for identifier in config['configurations']['trafodion-env']['traf.ldap.identifiers'].split(';'):
  ldap_identifiers += '  UniqueIdentifier: %s\n' % identifier
ldap_user = config['configurations']['trafodion-env']['traf.ldap.user']
ldap_pwd = config['configurations']['trafodion-env']['traf.ldap.pwd']
ldap_encrypt = config['configurations']['trafodion-env']['traf.ldap.encrypt']
ldap_certpath = config['configurations']['trafodion-env']['traf.ldap.certpath']

#HDFS Dir creation
hostname = config["hostname"]
hadoop_conf_dir = "/etc/hadoop/conf"
hdfs_user_keytab = config['configurations']['hadoop-env']['hdfs_user_keytab']
security_enabled = config['configurations']['cluster-env']['security_enabled']
kinit_path_local = functions.get_kinit_path(default('/configurations/kerberos-env/executable_search_paths', None))
hdfs_site = config['configurations']['hdfs-site']
default_fs = config['configurations']['core-site']['fs.defaultFS']
import functools
#create partial functions with common arguments for every HdfsDirectory call
#to create hdfs directory we need to call params.HdfsDirectory in code
HdfsDirectory = functools.partial(
  HdfsResource,
  type="directory",
  hadoop_conf_dir=hadoop_conf_dir,
  user=hdfs_user,
  hdfs_site=hdfs_site,
  default_fs=default_fs,
  security_enabled = security_enabled,
  keytab = hdfs_user_keytab,
  kinit_path_local = kinit_path_local
)

