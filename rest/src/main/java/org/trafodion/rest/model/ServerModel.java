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
import java.util.Scanner;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of an Trafodion instance.
 */
@XmlRootElement(name="Servers")
public class ServerModel implements Serializable {
	private static final long serialVersionUID = 1L;
	private static final Log LOG = LogFactory.getLog(ServerModel.class);

	private String nidPid;
	private String parent;
	private String priority;
	private String program;
	private String states;
	private String name;
	private String type;

	/**
	 * Default constructor
	 */
	public ServerModel() {}

	/**
	 * Constructor
	 * @param data
	 */
	public ServerModel(String data) {
		super();
		if(LOG.isDebugEnabled());
			LOG.debug("data=" + data);
		Scanner scn = new Scanner(data);
		scn.next(); //skip sqps process name
		this.nidPid = scn.next(); 
		this.priority = scn.next(); 
		this.type = scn.next();
		this.states = scn.next();
		this.name = scn.next();
		this.parent = scn.next();
		this.program = scn.next();
		scn.close();
		if(LOG.isDebugEnabled())
			LOG.debug(toString());
	}
	
	/**
	 * @return the nid,pid
	 */
	@XmlAttribute(name="NIDPID")
	public String getNidPid() {
		return this.nidPid;
	}
	
	/**
	 * @return the priority
	 */
	@XmlAttribute(name="PRIORITY")
	public String getPriority() {
		return this.priority;
	}
	
	/**
	 * @return the type
	 */
	@XmlAttribute(name="TYPE")
	public String getType() {
		return type;
	}
	
	/**
	 * @return the states
	 */
	@XmlAttribute(name="STATES")
	public String getStates() {
		return states;
	}
	
	/**
	 * @return the name
	 */
	@XmlAttribute(name="NAME")
	public String getName() {
		return name;
	}
	
	/**
	 * @return the parent
	 */
	@XmlAttribute(name="PARENT")
	public String getParent() {
		return parent;
	}
	
	/**
	 * @return the program
	 */
	@XmlAttribute(name="PROGRAM")
	public String getProgram() {
		return program;
	}

	/**
	 * @param value the value to set
	 */
	public void setNidPid(String value) {
		this.nidPid = value;
	}

	/**
	 * @param value the value to set
	 */
	public void setPriority(String value) {
		priority = value;
	}
	
	/**
	 * @param value the value to set
	 */
	public void setType(String value) {
		type = value;
	}

	/**
	 * @param value the value to set
	 */
	public void setStates(String value) {
		states = value;
	}
	
	/**
	 * @param value the value to set
	 */
	public void setName(String value) {
		name = value;
	}
	
	/**
	 * @param value the value to set
	 */
	public void setParent(String value) {
		parent = value;
	}
	
	/**
	 * @param value the value to set
	 */
	public void setProgram(String value) {
		program = value;
	}

    @Override
    public String toString() {
    	StringBuilder sb = new StringBuilder();
    	sb.append("nidpid ");
    	sb.append(nidPid);
    	sb.append("] [parent: ");
    	sb.append(parent);
    	sb.append("] [priority: ");
    	sb.append(priority);
    	sb.append("] [program: ");
    	sb.append(program);
    	sb.append("] [states: ");
    	sb.append(states);
    	sb.append("] [name: ");
    	sb.append(name);
    	sb.append("] [type: ");
    	sb.append("type");
  	    sb.append("]\n");
    	return sb.toString();
    }
}
