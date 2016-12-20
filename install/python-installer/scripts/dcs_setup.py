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

### this script should be run on all nodes with trafodion user ###

import os
import sys
import json
from common import append_file, write_file, mod_file, cmd_output, run_cmd, \
                   ParseInI, ParseXML, DEF_PORT_FILE, err

def run():
    dbcfgs = json.loads(dbcfgs_json)

    TRAF_HOME = os.environ['TRAF_HOME']
    TRAF_VER = dbcfgs['traf_version']
    HBASE_XML_FILE = dbcfgs['hbase_xml_file']

    DCS_INSTALL_ENV = 'export DCS_INSTALL_DIR=%s/dcs-%s' % (TRAF_HOME, TRAF_VER)
    REST_INSTALL_ENV = 'export REST_INSTALL_DIR=%s/rest-%s' % (TRAF_HOME, TRAF_VER)

    DCS_CONF_DIR = '%s/dcs-%s/conf' % (TRAF_HOME, TRAF_VER)
    DCS_SRV_FILE = DCS_CONF_DIR + '/servers'
    DCS_MASTER_FILE = DCS_CONF_DIR + '/master'
    DCS_BKMASTER_FILE = DCS_CONF_DIR + '/backup-masters'
    DCS_ENV_FILE = DCS_CONF_DIR + '/dcs-env.sh'
    DCS_SITE_FILE = DCS_CONF_DIR + '/dcs-site.xml'
    REST_SITE_FILE = '%s/rest-%s/conf/rest-site.xml' % (TRAF_HOME, TRAF_VER)
    TRAFCI_FILE = TRAF_HOME + '/trafci/bin/trafci'
    SQENV_FILE = TRAF_HOME + '/sqenvcom.sh'

    ### dcs setting ###
    # servers
    nodes = dbcfgs['node_list'].split(',')
    dcs_cnt = dbcfgs['dcs_cnt_per_node']
    dcs_servers = ''
    for node in nodes:
        dcs_servers += '%s %s\n' % (node, dcs_cnt)

    write_file(DCS_SRV_FILE, dcs_servers)

    ### modify dcs config files ###
    # modify master
    dcs_master = nodes[0]
    append_file(DCS_MASTER_FILE, dcs_master)

    # modify sqenvcom.sh
    append_file(SQENV_FILE, DCS_INSTALL_ENV)
    append_file(SQENV_FILE, REST_INSTALL_ENV)

    # modify dcs-env.sh
    mod_file(DCS_ENV_FILE, {'.*DCS_MANAGES_ZK=.*':'export DCS_MANAGES_ZK=false'})

    ports = ParseInI(DEF_PORT_FILE, 'ports').load()
    dcs_master_port = ports['dcs_master_port']
    # modify trafci
    mod_file(TRAFCI_FILE, {'HNAME=.*':'HNAME=%s:%s' % (dcs_master, dcs_master_port)})

    # modify dcs-site.xml
    net_interface = run_cmd('ip route |grep default|awk \'{print $5}\'')
    hb = ParseXML(HBASE_XML_FILE)
    zk_hosts = hb.get_property('hbase.zookeeper.quorum')
    zk_port = hb.get_property('hbase.zookeeper.property.clientPort')

    p = ParseXML(DCS_SITE_FILE)
    p.add_property('dcs.zookeeper.property.clientPort', zk_port)
    p.add_property('dcs.zookeeper.quorum', zk_hosts)
    p.add_property('dcs.dns.interface', net_interface)

    if dbcfgs['dcs_ha'] == 'Y':
        dcs_floating_ip = dbcfgs['dcs_floating_ip']
        dcs_backup_nodes = dbcfgs['dcs_backup_nodes']
        p.add_property('dcs.master.floating.ip', 'true')
        p.add_property('dcs.master.floating.ip.external.interface', net_interface)
        p.add_property('dcs.master.floating.ip.external.ip.address', dcs_floating_ip)
        p.rm_property('dcs.dns.interface')

        # modify backup_master
        write_file(DCS_BKMASTER_FILE, dcs_backup_nodes)

    p.write_xml()

    ### rest setting ###
    p = ParseXML(REST_SITE_FILE)
    p.add_property('rest.zookeeper.property.clientPort', zk_port)
    p.add_property('rest.zookeeper.quorum', zk_hosts)
    p.write_xml()

    ### run sqcertgen ###
    run_cmd('sqcertgen')

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
run()
