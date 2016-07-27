#!/usr/bin/python

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

import xml.etree.ElementTree as ET

hbaseMaster="hbase.master.info.port"
hbaseRegion="hbase.regionserver.info.port"
zooKeeperNodes="hbase.zookeeper.quorum"
zooKeeperClientPort="hbase.zookeeper.property.clientPort"
pathToHome= os.environ['HOME']

hbaseMasterInfoPort="60010"
regionServerInfoPort="60030"
zookeeperNodeNames=""
zookeeperPort="2181"

tree = ET.parse( pathToHome + '/hbase-site.xml')

root = tree.getroot()


for x in root.findall('property'):
    name = str(x.find('name').text)
    if name == hbaseMaster:
       hbaseMasterInfoPort = x.find('value').text
    if name == hbaseRegion:
       regionServerInfoPort = x.find('value').text
    if name == zooKeeperNodes:
       zookeeperNodeNames = x.find('value').text
    if name == zooKeeperClientPort:
       zookeeperPort = x.find('value').text

f = open( '/etc/trafodion/trafodion_config', 'a')
f.write ( 'export HBASE_MASTER_INFO_PORT="' + hbaseMasterInfoPort + '"\n' )
f.write ( 'export REGIONSERVER_INFO_PORT="' + regionServerInfoPort + '"\n' )
f.write ( 'export ZOOKEEPER_NODES="' + zookeeperNodeNames + '"\n' )
f.write ( 'export ZOOKEEPER_PORT="' + zookeeperPort + '"\n' )
f.close()
