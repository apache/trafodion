package org.trafodion.wms.rest.model;

import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.Constants;

import javax.xml.bind.annotation.XmlElementRef;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a list of servers.
 */
@XmlRootElement(name="ServerList")
public class ServerListModel {
	private static final long serialVersionUID = 1L;
	private List<ServerModel> servers = new ArrayList<ServerModel>();
	/**
	 * Default constructor
	 */
	public ServerListModel() throws IOException {
		Configuration conf = WmsConfiguration.create();
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
	 * @return the workloads
	 */
	public List<ServerModel> getServers() {
		return servers;
	}

	/**
	 * @param workloads the workloads to set
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
		sb.append("<html>\n");
		sb.append("<head>\n");
		sb.append("<title>WMS (Workload Management Services)</title>\n");
		sb.append("</head>\n");
		sb.append("<body>\n");
		//
		sb.append("<table border=\"1\">\n");
		sb.append("<tr>\n");	
		sb.append("<th>Server</th>\n");
		sb.append("<th>Instance</th>\n");		
		sb.append("<th>Leader</th>\n");
		sb.append("<th>Thrift Port</th>\n");
		sb.append("<th>Started</th>\n");
		sb.append("</tr>\n");

		for(ServerModel aServer : servers) {
			sb.append(aServer.toString());
			sb.append('\n');
		}

		sb.append("</table>\n");
		sb.append("</body>\n");
		sb.append("</html>\n");
		return sb.toString();
	}
}
