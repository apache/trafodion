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
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.rest.model;

import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlElementWrapper;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of an DCS instance.
 */
@XmlRootElement(name = "DcsStatus")
public class ServerModel implements Serializable {
    private static final long serialVersionUID = 1L;

    /**
     * Represents a DcsMaster server.
     */
    public static class DcsMaster {
        private String hostName;
        private String listenerPort;
        private String listenerPortRange;
        private String startTimestamp;
        private List<DcsServer> dcsServerList = new ArrayList<DcsServer>();

        /**
         * Default constructor
         */
        public DcsMaster() {
        }

        /**
         * Constructor
         * 
         * @param hostName
         *            the host name
         * @param listenerPort
         *            the port its listening on
         * @param listenerPortRange
         *            the listener port range
         * @param startTimestamp
         *            the start timestamp
         */
        public DcsMaster(String hostName, String listenerPort,
                String listenerPortRange, String startTimestamp) {
            this.hostName = hostName;
            this.listenerPort = listenerPort;
            this.listenerPortRange = listenerPortRange;
            this.startTimestamp = startTimestamp;
        }

        /**
         * @return the host name
         */
        @XmlAttribute
        public String getHostName() {
            return hostName;
        }

        /**
         * @return the listener port number
         */
        @XmlAttribute
        public String getListenerPort() {
            return listenerPort;
        }

        /**
         * @return the listener port range
         */
        @XmlAttribute
        public String getListenerPortRange() {
            return listenerPortRange;
        }

        /**
         * @return the start time
         */
        @XmlAttribute
        public String getStartTimestamp() {
            return startTimestamp;
        }

        /**
         * Add a DcsServer to the list
         * 
         * @param znode
         *            the znode name
         */
        public DcsServer addDcsServer(String znode, String data) {
            Scanner scn = new Scanner(znode);
            scn.useDelimiter(":");
            String hostName = scn.next();// host name
            String instance = scn.next();// instance
            String infoPort = scn.next();// info port
            String startTimestamp = scn.next();
            scn.close();
            ServerModel.DcsServer dcsServer = new ServerModel.DcsServer(
                    hostName, instance, infoPort, startTimestamp);
            dcsServerList.add(dcsServer);
            return dcsServer;
        }

        /**
         * @param index
         *            the index
         * @return the DcsServer name
         */
        public DcsServer getDcsServer(int index) {
            return dcsServerList.get(index);
        }

        /**
         * @return the list of DcsServer
         */
        @XmlElement(name = "DcsServer")
        public List<DcsServer> getDcsServer() {
            return dcsServerList;
        }
    }

    /**
     * Represents a DcsServer server.
     */
    public static class DcsServer {
        // from znode, no data
        private String hostName;
        private String instance;
        private String infoPort;
        private String startTimestamp;
        private List<TrafodionServer> trafServerList = new ArrayList<TrafodionServer>();

        /**
         * Default constructor
         */
        public DcsServer() {
        }

        /**
         * Constructor
         * 
         * @param hostName
         *            the host name
         * @param instance
         *            the instance number
         * @param infoPort
         *            the port
         * @param startTimestamp
         *            the start timestamp
         */
        public DcsServer(String hostName, String instance, String infoPort,
                String startTimestamp) {
            this.hostName = hostName;
            this.instance = instance;
            this.infoPort = infoPort;
            this.startTimestamp = startTimestamp;
        }

        /**
         * @return the host name
         */
        @XmlAttribute
        public String getHostName() {
            return hostName;
        }

        /**
         * @return the instance number
         */
        @XmlAttribute
        public String getInstance() {
            return instance;
        }

        /**
         * @return the listener port range
         */
        @XmlAttribute
        public String getInfoPort() {
            return infoPort;
        }

        /**
         * @return the start timestamp
         */
        @XmlAttribute
        public String getStartTimestamp() {
            return startTimestamp;
        }

        /**
         * Add a TrafodionServer to the list
         * 
         * @param znode
         *            the znode name
         * @param data
         *            the data
         */
        public void addTrafodionServer(String znode, String data) {
            Scanner scn = new Scanner(znode);
            scn.useDelimiter(":");
            String hostName = scn.next();// host name
            String instance = scn.next();// DcsServer's instance ID
            String trafInstance = scn.next();// Traf server's instance ID
            scn.close();
            scn = new Scanner(data);
            scn.useDelimiter(":");
            String state = scn.next();// state
            String timestamp = scn.next();// last updated timestamp
            String dialogueId = scn.next();// dialogue id
            String nid = scn.next();// node id
            String pid = scn.next();// process id
            String processName = scn.next();// process name
            String ipAddress = scn.next();// server ip address
            String port = scn.next();// server port
            String clientHostName = scn.next();// client host name
            String clientIpAddress = scn.next();// client ip address
            String clientPort = scn.next();// client port
            String clientAppl = scn.next();// client application name
            scn.close();

            if (this.hostName.equalsIgnoreCase(hostName)
                    && this.instance.equalsIgnoreCase(instance))
                trafServerList.add(new ServerModel.TrafodionServer(hostName,
                        instance, trafInstance, state, timestamp, dialogueId,
                        nid, pid, processName, ipAddress, port, clientHostName,
                        clientIpAddress, clientPort, clientAppl));
        }

        /**
         * @param index
         *            the index
         * @return the TrafodionServer
         */
        public TrafodionServer getTrafodionServer(int index) {
            return trafServerList.get(index);
        }

        /**
         * @return the list of Trafodion servers
         */
        @XmlElement(name = "TrafodionServer")
        public List<TrafodionServer> getTrafodionServer() {
            return trafServerList;
        }
    }

    /**
     * Represents a TrafodionServer server.
     */
    public static class TrafodionServer {
        // from znode
        private String hostName;
        private String dcsInstance;
        private String instance;
        // from data
        private String state;
        private String timestamp;
        private String dialogueId;
        private String nid;
        private String pid;
        private String processName;
        private String ipAddress;
        private String port;
        private String clientHostName;
        private String clientIpAddress;
        private String clientPort;
        private String clientAppl;

        /**
         * Default constructor
         */
        public TrafodionServer() {
        }

        /**
         * Constructor
         * 
         * @param hostName
         *            the host name
         */
        public TrafodionServer(String hostName, String dcsInstance,
                String instance, String state, String timestamp,
                String dialogueId, String nid, String pid, String processName,
                String ipAddress, String port, String clientHostName,
                String clientIpAddress, String clientPort, String clientAppl) {
            this.hostName = hostName;
            this.dcsInstance = dcsInstance;
            this.instance = instance;
            this.state = state;
            this.timestamp = timestamp;
            this.dialogueId = dialogueId;
            this.nid = nid;
            this.pid = pid;
            this.processName = processName;
            this.ipAddress = ipAddress;
            this.port = port;
            this.clientHostName = clientHostName;
            this.clientIpAddress = clientIpAddress;
            this.clientPort = clientPort;
            this.clientAppl = clientAppl;
        }

        /**
         * @return the server's host name
         */
        @XmlAttribute
        public String getHostName() {
            return hostName;
        }

        /**
         * @return the DCS server's instance ID
         */
        @XmlAttribute
        public String getDcsInstance() {
            return dcsInstance;
        }

        /**
         * @return the Trafodion server's instance number
         */
        @XmlAttribute
        public String getInstance() {
            return instance;
        }

        /**
         * @return the server state
         */
        @XmlAttribute
        public String getState() {
            return state;
        }

        /**
         * @return the server timestamp
         */
        @XmlAttribute
        public String getTimestamp() {
            return timestamp;
        }

        /**
         * @return the dialogueId
         */
        @XmlAttribute
        public String getDialogueId() {
            return dialogueId;
        }

        /**
         * @return the node Id
         */
        @XmlAttribute
        public String getNid() {
            return nid;
        }

        /**
         * @return the process Id
         */
        @XmlAttribute
        public String getPid() {
            return pid;
        }

        /**
         * @return the process name
         */
        @XmlAttribute
        public String getProcessName() {
            return processName;
        }

        /**
         * @return the server's IP address
         */
        @XmlAttribute
        public String getIpAddress() {
            return ipAddress;
        }

        /**
         * @return the server's port number
         */
        @XmlAttribute
        public String getPort() {
            return port;
        }

        /**
         * @return the connected client's host name
         */
        @XmlAttribute
        public String getClientHostName() {
            return clientHostName;
        }

        /**
         * @return the the connected client's IP address
         */
        @XmlAttribute
        public String getClientIpAddress() {
            return clientIpAddress;
        }

        /**
         * @return the connected client's port number
         */
        @XmlAttribute
        public String getClientPort() {
            return clientPort;
        }

        /**
         * @return the connected client's application name
         */
        @XmlAttribute
        public String getClientAppl() {
            return clientAppl;
        }
    }

    /**
     * Default constructor
     */
    public ServerModel() {
    }

    private DcsMaster dcsMaster = null;

    /**
     * Add a DcsMaster
     * 
     * @param znode
     *            the znode
     */
    public DcsMaster addDcsMaster(String znode, String data) {
        Scanner scn = new Scanner(znode);
        scn.useDelimiter(":");
        String hostName = scn.next();// host name
        String listenerPort = scn.next();// listener port
        String listenerPortRange = scn.next();
        String startTimestamp = scn.next();
        scn.close();
        dcsMaster = new ServerModel.DcsMaster(hostName, listenerPort,
                listenerPortRange, startTimestamp);
        return dcsMaster;
    }

    /**
     * @return the DCS Master server
     */
    @XmlElement(name = "DcsMaster")
    public DcsMaster getDcsMaster() {
        return dcsMaster;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        /*
         * if (!dcsMaster.isEmpty()) { //sb.append(String.format(
         * "%d DcsMaster server(s), %d DcsServer server(s)\n\n"
         * ,dcsMaster.size(), dcsMaster.getDcsServer().size());
         * //sb.append(liveNodes.size()); //sb.append(" live servers\n"); for
         * (DcsMaster aServer: dcsMaster) { sb.append("    ");
         * sb.append(node.name); sb.append(' '); sb.append(node.startCode);
         * sb.append("\n        requests="); sb.append(node.requests);
         * sb.append(", regions="); sb.append(node.regions.size());
         * sb.append("\n        heapSizeMB="); sb.append(node.heapSizeMB);
         * sb.append("\n        maxHeapSizeMB="); sb.append(node.maxHeapSizeMB);
         * sb.append("\n\n"); for (Node.Region region: node.regions) {
         * sb.append("        "); sb.append(Bytes.toString(region.name));
         * sb.append("\n            stores="); sb.append(region.stores);
         * sb.append("\n            storefiless=");
         * sb.append(region.storefiles);
         * sb.append("\n            storefileSizeMB=");
         * sb.append(region.storefileSizeMB);
         * sb.append("\n            memstoreSizeMB=");
         * sb.append(region.memstoreSizeMB);
         * sb.append("\n            storefileIndexSizeMB=");
         * sb.append(region.storefileIndexSizeMB);
         * sb.append("\n            readRequestsCount=");
         * sb.append(region.readRequestsCount);
         * sb.append("\n            writeRequestsCount=");
         * sb.append(region.writeRequestsCount);
         * sb.append("\n            rootIndexSizeKB=");
         * sb.append(region.rootIndexSizeKB);
         * sb.append("\n            totalStaticIndexSizeKB=");
         * sb.append(region.totalStaticIndexSizeKB);
         * sb.append("\n            totalStaticBloomSizeKB=");
         * sb.append(region.totalStaticBloomSizeKB);
         * sb.append("\n            totalCompactingKVs=");
         * sb.append(region.totalCompactingKVs);
         * sb.append("\n            currentCompactedKVs=");
         * sb.append(region.currentCompactedKVs); sb.append('\n'); }
         * sb.append('\n'); } }
         */
        return sb.toString();
    }
}
