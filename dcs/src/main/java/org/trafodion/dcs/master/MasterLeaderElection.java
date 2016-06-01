/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */
/**
 * Copyright 2007 The Apache Software Foundation
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.trafodion.dcs.master;

import org.apache.zookeeper.*;
import org.apache.zookeeper.data.Stat;

import java.io.IOException;
import java.util.Collections;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;

public class MasterLeaderElection {
    private static final Log LOG = LogFactory
            .getLog(MasterLeaderElection.class);
    private DcsMaster master = null;
    private String myZnode;
    private String parentZnode;
    private String leaderZnode;
    private FloatingIp floatingIp;
    private boolean isLeader = false;
    private boolean isFollower = false;

    public MasterLeaderElection(DcsMaster master) throws IOException,
            InterruptedException, KeeperException {
        this.master = master;
        this.parentZnode = master.getZKParentZnode();

        try {
            this.floatingIp = FloatingIp.getInstance(master);
        } catch (Exception e) {
            if (LOG.isErrorEnabled()) {
                LOG.error("Error creating class FloatingIp [" + e.getMessage() + "]");
            }
            floatingIp = null;
        }

        byte[] data = Bytes.toBytes(master.getServerName() + ":"
                + master.getInstance() + ":");
        setNodePath(master.getZkClient().create(
                parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER_LEADER
                        + "/" + "leader-n_", data, ZooDefs.Ids.OPEN_ACL_UNSAFE,
                CreateMode.EPHEMERAL_SEQUENTIAL));
        elect();
    }

    private void setNodePath(String myZnode) {
        this.myZnode = myZnode;
    }

    private synchronized void elect() throws IOException, InterruptedException,
            KeeperException {

        // If I'm the leader ignore further znode events
        if (isLeader)
            return;

        List<String> znodeList = master.getZkClient().getChildren(
                parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER_LEADER,
                new ElectionNodeWatcher());

        Collections.sort(znodeList);
        leaderZnode = parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER_LEADER + "/"
                + znodeList.get(0);

        if (myZnode.equals(leaderZnode))
            isLeader = true;
        else
            isLeader = false;

        if(LOG.isDebugEnabled())
            LOG.debug("leaderZnode=" + leaderZnode + ", myZnode=" + myZnode + ",isLeader=" + isLeader);

        if (isLeader) {
            LOG.info("I'm the Leader [" + myZnode + "]");
            if (floatingIp != null) {
                try {
                    floatingIp.runScript();
                } catch (Exception e) {
                    if (LOG.isErrorEnabled()) {
                        LOG.error("Error invoking FloatingIp [" + e.getMessage() + "]");
                    }
                }
            }
            master.setIsLeader();
        } else {
            LOG.info("I'm a follower [" + myZnode + "]");
            isFollower = true;// See ServerManager.getZkRunning()
        }
    }

    private class ElectionNodeWatcher implements Watcher {
        // watches /LEADER node's children changes.
        public void process(WatchedEvent event) {
            if (event.getType() == Event.EventType.NodeChildrenChanged) {
                LOG.info("Node changed [" + event.getPath()
                        + "], electing a leader.");
                try {
                    elect();
                } catch (IOException e) {
                    LOG.error(e);
                } catch (InterruptedException e) {
                    LOG.error(e);
                } catch (KeeperException e) {
                    LOG.error(e);
                }
            }
        }
    }

    public boolean isFollower() {
        return isFollower;
    }

}
