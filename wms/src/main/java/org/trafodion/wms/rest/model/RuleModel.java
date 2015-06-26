package org.trafodion.wms.rest.model;

import java.io.Serializable;
import java.util.Date;
import java.text.DateFormat;

import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a rule.
 */
@XmlRootElement(name="rule")
public class RuleModel implements Serializable {

	private static final long serialVersionUID = 1L;
	
	private String name;
	private String text;
	private String comment;
	private long timestamp;
	
	/**
	 * Default constructor
	 */
	public RuleModel() {}

	/**
	 * Constructor
	 * @param name
	 */
	public RuleModel(String name,String text,String comment,long timestamp) {
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
		return text;
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
