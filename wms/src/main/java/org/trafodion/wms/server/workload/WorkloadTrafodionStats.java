package org.trafodion.wms.server.store;

import java.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public final class WorkloadTrafodionStats extends WorkloadStats {
	private static  final Log LOG = LogFactory.getLog(WorkloadTrafodionStats.class);
//	private int maxCpuBusy=100;
//	private int maxMemUsage=50;
//	private int maxExecMinutes=5;
	
	public WorkloadTrafodionStats() {
		super();
	}
/*	
	public void setMaxCpuBusy(int value){
		maxCpuBusy = value;
	}
	public int getMaxCpuBusy(){
		return maxCpuBusy;
	}
	public void setMaxMemUsage(int value){
		this.maxMemUsage = value;
	}
	public int getMaxMemUsage(){
		return maxMemUsage;
	}
	public void setMaxExecMinutes(int value){
		this.maxExecMinutes = value;
	}
	public int getMaxExecMinutes(){
		return maxExecMinutes;
	}
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("MaxCpuBusy[" + getMaxCpuBusy() + "],");
		sb.append("MaxMemUsage[" + getMaxMemUsage() + "],");
		sb.append("MaxExecMinutes[" + getMaxExecMinutes() + "]");
		return sb.toString();
	}
*/
}