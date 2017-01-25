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

### this script should be run on local node ###

import time
import sys
import json
from common import ParseHttp, ParseJson, MODCFG_FILE, err, info, retry

modcfgs = ParseJson(MODCFG_FILE).load()

MOD_CFGS = modcfgs['MOD_CFGS']
HBASE_MASTER_CONFIG = modcfgs['HBASE_MASTER_CONFIG']
HBASE_RS_CONFIG = modcfgs['HBASE_RS_CONFIG']
HDFS_CONFIG = modcfgs['HDFS_CONFIG']
ZK_CONFIG = modcfgs['ZK_CONFIG']

CLUSTER_URL_PTR = '%s/api/v1/clusters/%s'
RESTART_URL_PTR = CLUSTER_URL_PTR + '/commands/restart'
RESTART_SRV_URL_PTR = CLUSTER_URL_PTR + '/services/%s/commands/restart'
SRVCFG_URL_PTR = CLUSTER_URL_PTR + '/services/%s/config'
RSGRP_BASEURL_PTR = '%s/api/v6/clusters/%s/services/%s/roleConfigGroups'
DEPLOY_CFG_URL_PTR = '%s/api/v6/clusters/%s/commands/deployClientConfig'
CMD_STAT_URL_PTR = '%s/api/v1/commands/%s'

class CDHMod(object):
    """ Modify CDH configs for trafodion and restart CDH services """
    def __init__(self, user, passwd, url, cluster_name):
        self.url = url
        self.cluster_name = cluster_name
        self.p = ParseHttp(user, passwd)

    def mod(self):
        hdfs_service = dbcfgs['hdfs_service_name']
        hbase_service = dbcfgs['hbase_service_name']
        zk_service = dbcfgs['zookeeper_service_name']
        services = {hdfs_service:HDFS_CONFIG, hbase_service:HBASE_MASTER_CONFIG, zk_service:ZK_CONFIG}

        for srv, cfg in services.iteritems():
            srvcfg_url = SRVCFG_URL_PTR % (self.url, self.cluster_name, srv)
            self.p.put(srvcfg_url, cfg)

        # set configs in each regionserver group
        rsgrp_baseurl = RSGRP_BASEURL_PTR % (self.url, self.cluster_name, hbase_service)
        rscfg = self.p.get(rsgrp_baseurl)
        rsgrp_urls = ['%s/%s/config' % (rsgrp_baseurl, r['name']) for r in rscfg['items'] if r['roleType'] == 'REGIONSERVER']

        for rsgrp_url in rsgrp_urls:
            self.p.put(rsgrp_url, HBASE_RS_CONFIG)

    def restart(self):
        restart_url = RESTART_URL_PTR % (self.url, self.cluster_name)
        deploy_cfg_url = DEPLOY_CFG_URL_PTR % (self.url, self.cluster_name)

        def __retry(url, maxcnt, interval, msg):
            rc = self.p.post(url)
            stat_url = CMD_STAT_URL_PTR % (self.url, rc['id'])
            get_stat = lambda: self.p.get(stat_url)['success'] is True and self.p.get(stat_url)['active'] is False
            retry(get_stat, maxcnt, interval, msg)

        info('Restarting CDH services ...')
        __retry(restart_url, 40, 15, 'CDH services restart')

        info('Deploying CDH client configs ...')
        __retry(deploy_cfg_url, 30, 10, 'CDH services deploy')


class HDPMod(object):
    """ Modify HDP configs for trafodion and restart HDP services """
    def __init__(self, user, passwd, url, cluster_name):
        self.url = url
        self.cluster_name = cluster_name
        self.p = ParseHttp(user, passwd, json_type=False)

    def mod(self):
        cluster_url = CLUSTER_URL_PTR % (self.url, self.cluster_name)
        desired_cfg_url = cluster_url + '?fields=Clusters/desired_configs'
        cfg_url = cluster_url + '/configurations?type={0}&tag={1}'
        desired_cfg = self.p.get(desired_cfg_url)

        hdp = self.p.get('%s/services/HBASE/components/HBASE_REGIONSERVER' % cluster_url)
        rsnodes = [c['HostRoles']['host_name'] for c in hdp['host_components']]

        hbase_hregion_property = 'hbase.hregion.impl'
        hbase_config_group = {
            'ConfigGroup': {
                'cluster_name': self.cluster_name,
                'group_name': 'hbase-regionserver',
                'tag': 'HBASE',
                'description': 'HBase Regionserver configs for Trafodion',
                'hosts': [{'host_name': host} for host in rsnodes],
                'desired_configs': [
                    {
                        'type': 'hbase-site',
                        'tag': 'traf_cg',
                        'properties': {hbase_hregion_property: MOD_CFGS['hbase-site'].pop(hbase_hregion_property)}
                    }
                ]
            }
        }
        self.p.post('%s/config_groups' % cluster_url, hbase_config_group)

        if dbcfgs['secure_hadoop'].upper() == 'Y':
            secure_cfgs = ',org.apache.hadoop.hbase.security.access.AccessController,org.apache.hadoop.hbase.security.token.TokenProvider'
            MOD_CFGS['hbase-site']['hbase.coprocessor.region.classes'] += secure_cfgs

        for config_type in MOD_CFGS.keys():
            desired_tag = desired_cfg['Clusters']['desired_configs'][config_type]['tag']
            current_cfg = self.p.get(cfg_url.format(config_type, desired_tag))
            tag = 'version' + str(int(time.time() * 1000000))
            new_properties = current_cfg['items'][0]['properties']
            new_properties.update(MOD_CFGS[config_type])
            config = {
                'Clusters': {
                    'desired_config': {
                        'type': config_type,
                        'tag': tag,
                        'properties': new_properties
                    }
                }
            }
            self.p.put(cluster_url, config)

    def restart(self):
        info('Restarting HDP services ...')
        def startstop(srvs, action, count, interval):
            srv_baseurl = CLUSTER_URL_PTR % (self.url, self.cluster_name) + '/services/'
            state = 'INSTALLED' if action == 'stop' else 'STARTED'
            for srv in srvs:
                srv_url = srv_baseurl + srv
                config = {'RequestInfo': {'context' :'%s %s services' % (action, srv)}, 'ServiceInfo': {'state' : state}}
                rc = self.p.put(srv_url, config)

                # check startstop status
                if rc:
                    get_stat = lambda: self.p.get(srv_url)['ServiceInfo']['state'] == state
                    retry(get_stat, count, interval, 'HDP service %s %s' % (srv, action))
                else:
                    if action == 'stop': action += 'p'
                    info('HDP service %s had already been %sed' % (srv, action))

        srvs = ['HBASE', 'HDFS', 'ZOOKEEPER']
        startstop(srvs, 'stop', 30, 10)
        time.sleep(10)
        srvs.reverse()
        startstop(srvs, 'start', 60, 10)

def run():
    if 'CDH' in dbcfgs['distro']:
        hadoop_mod = CDHMod(dbcfgs['mgr_user'], dbcfgs['mgr_pwd'], dbcfgs['mgr_url'], dbcfgs['cluster_name'])
    elif 'HDP' in dbcfgs['distro']:
        hadoop_mod = HDPMod(dbcfgs['mgr_user'], dbcfgs['mgr_pwd'], dbcfgs['mgr_url'], dbcfgs['cluster_name'])

    hadoop_mod.mod()
    hadoop_mod.restart()

# main
try:
    dbcfgs_json = sys.argv[1]
except IndexError:
    err('No db config found')
dbcfgs = json.loads(dbcfgs_json)
run()
