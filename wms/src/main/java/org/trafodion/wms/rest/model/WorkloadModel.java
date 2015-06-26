package org.trafodion.wms.rest.model;

import java.io.Serializable;
import java.util.Date;
import java.text.DateFormat;

import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a workload.
 */
@XmlRootElement(name = "workload")
public class WorkloadModel implements Serializable {
    private static final long serialVersionUID = 1L;
    private String workloadId;
    private String state;
    private String subState;
    private String type;
    private String workloadText;
    private String workloadDetails;

    /**
     * Default constructor
     */
    public WorkloadModel() {
    }

    /**
     * Constructor
     * 
     * @param name
     */
    public WorkloadModel(String workloadId, String state, String subState,
            String type, String workloadText, String workloadDetails) {
        super();

        this.workloadId = workloadId;
        this.state = state;
        this.subState = subState;
        this.type = type;
        this.workloadText = workloadText;
        this.workloadDetails = workloadDetails;
    }

    /**
     * @return the workoad Id
     */
    @XmlAttribute
    public String getWorkloadId() {
        return workloadId;
    }

    /**
     * @param value
     *            the workload Id to set
     */
    public void setWorkloadId(String value) {
        this.workloadId = value;
    }

    /**
     * @return the state
     */
    @XmlAttribute
    public String getState() {
        return state;
    }

    /**
     * @param value
     *            the state to set
     */
    public void setState(String value) {
        this.state = value;
    }

    /**
     * @return the subState
     */
    @XmlAttribute
    public String getSubState() {
        return subState;
    }

    /**
     * @param name
     *            the subState to set
     */
    public void setSubState(String value) {
        this.subState = value;
    }

    /**
     * @return the type
     */
    @XmlAttribute
    public String getType() {
        return type;
    }

    /**
     * @param value
     *            the type to set
     */
    public void setType(String value) {
        this.type = value;
    }

    /**
     * @return the workloadText
     */
    @XmlAttribute
    public String getWorkloadText() {
        return workloadText;
    }

    /**
     * @param value
     *            the workloadText to set
     */
    public void setWorkloadText(String value) {
        this.workloadText = value;
    }

    /**
     * @return the workload details
     */
    @XmlAttribute
    public String getWorkloadDetails() {
        return workloadDetails;
    }

    /**
     * @param value
     *            the workload details to set
     */
    public void setWorkloadDetails(String value) {
        this.workloadDetails = value;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("<td>" + workloadId + "</td>\n");
        sb.append("<td>" + state + "</td>\n");
        sb.append("<td>" + subState + "</td>\n");
        sb.append("<td>" + workloadDetails + "</td>\n");
        sb.append("</tr>\n");
        return sb.toString();
    }
}
