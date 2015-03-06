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

import org.apache.hadoop.conf.Configuration;
import org.trafodion.rest.util.RestConfiguration;
import org.trafodion.rest.Constants;

import javax.xml.bind.annotation.XmlElementRef;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Simple representation of a list of workloads.
 */
@XmlRootElement(name="Workloads")
public class WorkloadListModel {
	private static final long serialVersionUID = 1L;
	private List<WorkloadModel> workloads = new ArrayList<WorkloadModel>();
	private static int refreshSeconds;
	/**
	 * Default constructor
	 */
	public WorkloadListModel() {
		Configuration conf = RestConfiguration.create();
		refreshSeconds = conf.getInt("trafodion.rest.refresh.seconds",5);
	}

	/**
	 * Add the workload to the list
	 * @param workload the workload model
	 */
	public void add(WorkloadModel workload) {
		workloads.add(workload);
	}
	
	/**
	 * @param index the index
	 * @return the workload model
	 */
	public WorkloadModel get(int index) {
		return workloads.get(index);
	}

	/**
	 * @return the workloads
	 */
	public List<WorkloadModel> getWorkloads() {
		return workloads;
	}

	/**
	 * @param workloads the list of workloads
	 */
	public void setWorkloads(List<WorkloadModel> workloads) {
		this.workloads = workloads;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		/*
		sb.append("<html>\n");
		sb.append("<head>\n");
		sb.append("<title>DCS (Data Connectivity Services)</title>\n");
		if(refreshSeconds > 0)
			sb.append("<meta http-equiv=\"refresh\" content=\"5\">\n");
		sb.append("</head>\n");
		sb.append("<body>\n");
		//
		sb.append("<table border=\"1\">\n");
		sb.append("<tr>\n");	
		sb.append("<th>Type</th>\n");
		sb.append("<th>Znode</th>\n");
		sb.append("<th>Data</th>\n");		
		sb.append("</tr>\n");

		for(WorkloadModel aWorkload : workloads) {
			sb.append(aWorkload.toString());
			sb.append('\n');
		}

		sb.append("</table>\n");
		sb.append("</body>\n");
		sb.append("</html>\n");
		*/
		
		
		sb.append("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
		sb.append("<html>\n");
		sb.append("<head>\n");
		sb.append("<meta http-equiv=\"Content-Type\" content=\"text/html\"; charset=ISO-8859-1\">\n");
		sb.append("<title>(DCS) Data Connectivity Services</title>\n");
		sb.append("<style type=\"text/css\">\n"); 
		sb.append("  * { padding: 0; margin: 0; }\n");
		sb.append("  table.dcs {\n"); 
		sb.append("    font-family: verdana, arial, helvetica, sans-serif;\n");
		sb.append("    font-size: 11px;\n");
		sb.append("    cellspacing: 0;\n");
		sb.append("    border-collapse: collapse;\n"); 
		sb.append("    width: 535px;\n");    
		sb.append("    }\n");
		sb.append("  table.dcs td {\n"); 
		sb.append("    border-left: 1px solid #999;\n"); 
		sb.append("    border-top: 1px solid #999;\n"); 
		sb.append("    padding: 2px 4px;\n");
		sb.append("    }\n");
		sb.append("  table.dcs tr:first-child td {\n");
		sb.append("    border-top: none;\n");
		sb.append("  }\n");
		sb.append("  table.dcs th { \n");
		sb.append("    border-left: 1px solid #999;\n");
		sb.append("    padding: 2px 4px;\n");
		sb.append("    background: #6b6164;\n");
		sb.append("    color: white;\n");
		sb.append("    font-variant: small-caps;\n");
	    sb.append("    }\n");
		sb.append("  table.dcs td { background: #eee; overflow: hidden; }\n");
		  
		sb.append("  div.scrollableContainer {\n"); 
		sb.append("    position: relative;\n"); 
		sb.append("    width: 750px;\n"); 
		sb.append("    padding-top: 2em;\n"); 
		sb.append("    margin: 40px;\n");    
		sb.append("    border: 1px solid #999;\n");
		sb.append("    background: #6b6164;\n");
		sb.append("    }\n");
		sb.append("  div.scrollingArea {\n"); 
		sb.append("    height: 240px;\n"); 
		sb.append("    overflow: auto;\n"); 
		sb.append("    }\n");
		 
		sb.append("  table.scrollable thead tr {\n");
		sb.append("    left: -1px; top: 0;\n");
		sb.append("    position: absolute;\n");
		sb.append("    }\n");
		 
		sb.append("  table.dcs .type  div { width: 100px; }\n");
		sb.append("  table.dcs .znode div { width: 100px; }\n");
		sb.append("  table.dcs .data  div { width: 100px; }\n");
		 
		sb.append("</style>\n");
		sb.append("</head>\n");
		sb.append("<body>\n");
		 
		sb.append("<div class=\"scrollableContainer\">\n");
		  sb.append("<div class=\"scrollingArea\">\n");
		  	sb.append("<table class=\"dcs scrollable\">\n");
		  	  sb.append("<thead>\n");
		  			sb.append("<tr>\n");
		  	      sb.append("<th><div class=\"type\">Type</div></th>\n");
		  	      sb.append("<th><div class=\"znode\">Znode</div></th>\n");
		  	      sb.append("<th><div class=\"data\">Data</div></th>\n");
		  			        sb.append("</tr>\n");
		  		    sb.append("</thead>\n");
		  		    sb.append("<tbody>\n");
		  		  
		  		for(WorkloadModel aWorkload : workloads) {
		  			sb.append("<tr>\n");
		  			sb.append(aWorkload.toString());
		  			sb.append("</tr>\n");
		  		  }

		            sb.append("</tbody>\n");
		  	sb.append("</table>\n");
			sb.append("</div>\n");
		sb.append("</div>\n");
		sb.append("<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.4.4/jquery.min.js\"></script>\n");
		sb.append("<script type=\"text/javascript\">\n"); 
		  sb.append("$(document).ready(function() {\n");
		    sb.append("$(\".scrollingArea\").height( $(window).height()-100 );\n");
		    sb.append("$(window).resize(function() { $(\".scrollingArea\").height( $(window).height()-100 ); } );\n");
		  sb.append("});\n");
		sb.append("</script>\n");
		sb.append("</body>\n");
		sb.append("</html>\n");
		return sb.toString();
	}
}
