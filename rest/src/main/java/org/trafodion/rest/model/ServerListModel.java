/*
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.trafodion.rest.model;

import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.rest.util.RestConfiguration;
import org.trafodion.rest.Constants;

import javax.xml.bind.annotation.XmlElementRef;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a list of servers.
 */
@XmlRootElement(name="ServersList")
public class ServerListModel {
	private static final long serialVersionUID = 1L;
	private List<ServerModel> servers = new ArrayList<ServerModel>();
	private static int refreshSeconds;
	/**
	 * Default constructor
	 */
	public ServerListModel() {
		Configuration conf = RestConfiguration.create();
		refreshSeconds = conf.getInt("trafodion.rest.refresh.seconds",5);
	}

	/**
	 * Add the server to the list
	 * @param server the server model
	 */
	public void add(ServerModel server) {
		servers.add(server);
	}
	
	/**
	 * @param index the index
	 * @return the server model
	 */
	public ServerModel get(int index) {
		return servers.get(index);
	}

	/**
	 * @return the servers
	 */
	public List<ServerModel> getServers() {
		return servers;
	}

	/**
	 * @param servers the list of servers
	 */
	public void setServers(List<ServerModel> servers) {
		this.servers = servers;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
	
		return sb.toString();
	}
}
