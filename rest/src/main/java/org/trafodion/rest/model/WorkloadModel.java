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

import java.io.Serializable;
import java.util.Date;
import java.text.DateFormat;

import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a workload.
 */
@XmlRootElement(name="workload")
public class WorkloadModel implements Serializable {
	private static final long serialVersionUID = 1L;
	
	private String type;
	private String znode;
	private String data;

	/**
	 * Default constructor
	 */
	public WorkloadModel() {}

	/**
	 * Constructor
	 * @param type
	 * @param znode
	 * @param data
	 */
	public WorkloadModel(String type,String znode,String data) {
		super();
		this.type = type;
		this.znode = znode;
		this.data = data;
	}
	
	/**
	 * @return the type
	 */
	@XmlAttribute
	public String getType() {
		return type;
	}

	/**
	 * @param value the type to set
	 */
	public void setType(String value) {
		this.type = value;
	}
	
	/**
	 * @return the znode
	 */
	@XmlAttribute
	public String getZnode() {
		return znode;
	}

	/**
	 * @param value the znode to set
	 */
	public void setZnode(String value) {
		this.znode = value;
	}
	
	/**
	 * @return the data
	 */
	@XmlAttribute
	public String getData() {
		return data;
	}

	/**
	 * @param value the value to set
	 */
	public void setData(String value) {
		this.data = value;
	}

    @Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		/*
		sb.append("<td>" + type + "</td>\n");
		sb.append("<td>" + znode + "</td>\n");
		sb.append("<td>" + data + "</td>\n");
		sb.append("</tr>\n");
		*/
		sb.append("<td><div class=\"type\">" + type + "</div></td>\n");  
		sb.append("<td><div class=\"znode\">" + znode + "</div></td>\n");
		sb.append("<td><div class=\"data\">" + data + "</div></td>\n"); 
		return sb.toString();
	}
}
