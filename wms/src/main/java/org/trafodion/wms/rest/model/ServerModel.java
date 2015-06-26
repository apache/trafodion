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
