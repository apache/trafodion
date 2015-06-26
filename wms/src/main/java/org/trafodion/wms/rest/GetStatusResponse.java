package org.trafodion.wms.rest;

import java.io.IOException;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlElementWrapper;
import javax.xml.bind.annotation.XmlElement;

@XmlRootElement
public class GetStatusResponse {
	private String workloadId;
	
	public GetStatusResponse(){
	};
	
	public GetStatusResponse(String value){
		workloadId = value;
	};
	
	@XmlElement
	public String getWorkloadId() {
		return workloadId;
	}
	
	public void setWorkloadId(String value) {
		this.workloadId = value;
	}
}
