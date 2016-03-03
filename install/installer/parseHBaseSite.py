#!/usr/bin/python

import os

import xml.etree.ElementTree as ET

hbaseMaster="hbase.master.info.port"
hbaseRegion="hbase.regionserver.info.port"
zooKeeperNodes="hbase.zookeeper.quorum"
pathToHome= os.environ['HOME']

tree = ET.parse( pathToHome + '/hbase-site.xml')

root = tree.getroot()


for x in root.findall('property'):
    name = str(x.find('name').text)
    if name == hbaseMaster:
       value = x.find('value').text
       f = open( '/etc/trafodion/trafodion_config', 'a')
       f.write ( 'export HBASE_MASTER_INFO_PORT="' + value + '"\n' )
       f.close()
    if name == hbaseRegion:
       value = x.find('value').text
       f = open( '/etc/trafodion/trafodion_config', 'a')
       f.write ( 'export REGIONSERVER_INFO_PORT="' + value + '"\n' )
       f.close()
    if name == zooKeeperNodes:
       value = x.find('value').text
       f = open( '/etc/trafodion/trafodion_config', 'a')
       f.write ( 'export ZOOKEEPER_NODES="' + value + '"\n' )
       f.close()

