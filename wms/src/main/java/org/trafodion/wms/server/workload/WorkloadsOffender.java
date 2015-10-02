/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

package org.trafodion.wms.server.store;

import java.util.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.KeeperException;

import org.trafodion.wms.Constants;
import org.trafodion.wms.zookeeper.ZkClient;

public final class WorkloadsOffender implements Runnable {
    private static final Log LOG = LogFactory.getLog(WorkloadsOffender.class);
    private Thread thrd;
    private static ZkClient zkc = null;
    private static String parentZnode = null;

    public WorkloadsOffender(ZkClient zkc, String parentZnode) {
        this.zkc = zkc;
        this.parentZnode = parentZnode;
        thrd = new Thread(this);
        thrd.setDaemon(true);
        thrd.start();
    }

    public void run() {
        while (true) {
            LOG.debug("Checking for offending workloads");
            publishOffenders();

            try {
                Thread.sleep(30000);
            } catch (InterruptedException e) {
            }
        }
    }

    public synchronized void publishOffenders() {
        int published = 0;

        try {
            /*
             * workloads = target.getWorkloads(); Date now = new Date(); for
             * (int i=0; i < workloads.size(); i++) { WorkloadItem workload =
             * workloads.get(i);
             * 
             * if(workload.getRequest().getJobType() == JobType.TRAFODION) {
             * if(now.getTime() - workload.getRequest().getStartTimestamp() >=
             * Constants.THIRTY_SECONDS ){ Stat stat = zkc.exists(parentZnode +
             * Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION_RMS + "/" +
             * workload.getRequest().getJobId().toString(),false); if(stat ==
             * null) { zkc.create(parentZnode +
             * Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION_RMS + "/" +
             * workload.getRequest().getJobId().toString(),new
             * byte[0],ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.EPHEMERAL);
             * LOG.debug("Published offending workload [" + parentZnode +
             * Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION_RMS + "/" +
             * workload.getRequest().getJobId().toString() + "] to " +
             * parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION_RMS);
             * published++; } } }
             * 
             * } //LOG.debug("[" + published +
             * "] offending workload(s) published to " + parentZnode +
             * Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION_RMS);
             */
        } catch (Exception e) {
            zkc = null;
            LOG.error(e);
        }
    }
}
