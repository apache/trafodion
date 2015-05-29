package org.trafodion.wms.rest.model;

import java.io.Serializable;
import java.util.Date;
import java.text.DateFormat;

import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a server.
 */
@XmlRootElement(name="server")
public class ServerModel implements Serializable {

	private static final long serialVersionUID = 1L;
	
	private String name;
	private String instance;
	private String leader;
 	private String thriftPort;
	private String timestamp;
	
	/**
	 * Default constructor
	 */
	public ServerModel() {}

	/**
	 * Constructor
	 * @param name
	 */
	public ServerModel(String name,String instance,String leader,String thriftPort,String ts) {
		super();
		this.name = name;
		this.instance = instance;
		this.leader = leader;
		this.thriftPort = thriftPort;
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
	 * @param name the name to set
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
	 * @param instance the instance to set
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
	 * @param leader the leader to set
	 */
	public void setLeader(String value) {
		this.leader = value;
	}
	
	/**
	 * @return the thrift port  
	 */
	@XmlAttribute
	public String getthriftPort() {
		return thriftPort;
	}

	/**
	 * @param thriftPort  
	 */
	public void setthriftPort(String value) {
		this.thriftPort = value;
	}
	
	/**
	 * @return the timestamp
	 */
	@XmlAttribute
	public String getTimestamp() {
		return timestamp;
	}

	/**
	 * @param timestamp the timestamp
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
		sb.append("<td>" + thriftPort + "</td>\n");
		sb.append("<td>" + new Date(timestamp) + "</td>\n");
		sb.append("</tr>\n");
		return sb.toString();
	}
}
