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

package org.trafodion.wms.rest.model;

import java.io.Serializable;
import java.util.Date;
import java.text.DateFormat;

import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a server.
 */
@XmlRootElement(name = "server")
public class ServerModel implements Serializable {

    private static final long serialVersionUID = 1L;

    private String name;
    private String instance;
    private String leader;
    private String timestamp;

    /**
     * Default constructor
     */
    public ServerModel() {
    }

    /**
     * Constructor
     * 
     * @param name
     */
    public ServerModel(String name, String instance, String leader, String ts) {
        super();
        this.name = name;
        this.instance = instance;
        this.leader = leader;
        this.timestamp = ts;
    }

    /**
     * @return the name
     */
    @XmlAttribute
    public String getName() {
        return name;
    }

    /**
     * @param name
     *            the name to set
     */
    public void setName(String value) {
        this.name = value;
    }

    /**
     * @return the instance
     */
    @XmlAttribute
    public String getInstance() {
        return instance;
    }

    /**
     * @param instance
     *            the instance to set
     */
    public void setInstance(String value) {
        this.instance = value;
    }

    /**
     * @return the leader
     */
    @XmlAttribute
    public String getLeader() {
        return leader;
    }

    /**
     * @param leader
     *            the leader to set
     */
    public void setLeader(String value) {
        this.leader = value;
    }

    /**
     * @return the timestamp
     */
    @XmlAttribute
    public String getTimestamp() {
        return timestamp;
    }

    /**
     * @param timestamp
     *            the timestamp
     */
    public void setTimestamp(String value) {
        this.timestamp = value;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("<td>" + name + "</td>\n");
        sb.append("<td>" + instance + "</td>\n");
        sb.append("<td>" + leader + "</td>\n");
        sb.append("<td>" + new Date(timestamp) + "</td>\n");
        sb.append("</tr>\n");
        return sb.toString();
    }
}
