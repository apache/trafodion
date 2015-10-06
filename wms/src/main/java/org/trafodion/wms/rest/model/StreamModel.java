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
 * Simple representation of a stream.
 */
@XmlRootElement(name="stream")
public class StreamModel implements Serializable {

	private static final long serialVersionUID = 1L;
	
	private String name;
	private String text;
	private String comment;
	private long timestamp;
	
	/**
	 * Default constructor
	 */
	public StreamModel() {}

	/**
	 * Constructor
	 * @param name
	 */
	public StreamModel(String name,String text,String comment,long timestamp) {
		super();
		this.name = name;
		this.text = text;
		this.comment = comment;
		this.timestamp = timestamp;
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
	 * @return the text
	 */
	@XmlAttribute
	public String getText() {
		String s = this.text;
		s = s.replaceAll(",",", ");
		return s;
	}

	/**
	 * @param text the text to set
	 */
	public void setText(String value) {
		this.text = value;
	}
	
	/**
	 * @return the comment
	 */
	@XmlAttribute
	public String getComment() {
		return comment;
	}

	/**
	 * @param comment the comment to set
	 */
	public void setComment(String value) {
		this.comment = value;
	}
	
	/**
	 * @return the timestamp
	 */
	@XmlAttribute
	public String getTimestamp() {
		return new Date(timestamp).toString();
	}

	/**
	 * @param timestamp to set
	 */
	public void setTimestamp(long value) {
		this.timestamp = value;
	}
	
    @Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("<td>" + name + "</td>\n");
		sb.append("<td>" + text + "</td>\n");
		sb.append("<td>" + comment + "</td>\n");
		sb.append("<td>" + new Date(timestamp) + "</td>\n");
		sb.append("</tr>\n");
		return sb.toString();
	}
}
