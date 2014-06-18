/*

package org.trafodion.wms.rest;

import java.io.IOException;
import java.util.*;
import java.text.DateFormat;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.mortbay.jetty.Handler;
import org.mortbay.jetty.Request;
import org.mortbay.jetty.handler.AbstractHandler;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.ServletException;
import org.trafodion.wms.server.WorkloadsQueue;
import org.trafodion.wms.server.WorkloadItem;
import org.trafodion.wms.thrift.generated.*;

public class RestHandler extends AbstractHandler {
	private static final Log LOG = LogFactory.getLog(RestHandler.class.getName());
	WorkloadsQueue wlq;
	
	public RestHandler(WorkloadsQueue wlq) {
		super();
		this.wlq = wlq;
	}

	public void handle(String target, HttpServletRequest request, HttpServletResponse response, int dispatch)       
	throws IOException, ServletException  {        
		response.setContentType("text/html");        
		response.setStatus(HttpServletResponse.SC_OK); 
		((Request)request).setHandled(true);

		response.getWriter().println("<html>");
		response.getWriter().println("<head>");
		response.getWriter().println("<title>WMS (Hadoop Workload Management Services)</title>");
		response.getWriter().println("<meta http-equiv=\"refresh\" content=\"5\">");
		response.getWriter().println("</head>");
		response.getWriter().println("<body>");
		
		Iterator<WorkloadItem> it = wlq.getWorkloads().iterator();
		if(it.hasNext()) { 
			response.getWriter().println("<table border=\"1\">");
			response.getWriter().println("<tr>");	
			response.getWriter().println("<th>WMS Id</th>");
			response.getWriter().println("<th>Parent WMS Id</th>");
			response.getWriter().println("<th>User</th>");
			response.getWriter().println("<th>Type/Id</th>");
			response.getWriter().println("<th>Text</th>");
			response.getWriter().println("<th>State</th>");
			response.getWriter().println("<th>Sub-State</th>");
			response.getWriter().println("<th>Start Time</th>");
			response.getWriter().println("<th>Stats</th>");
			response.getWriter().println("<th>Last Update</th>");
			response.getWriter().println("<th>Duration</th>");
			response.getWriter().println("</tr>");
			
			while(it.hasNext()) {
				WorkloadItem workload = it.next();
				
				if(workload.getRequest().getJobState().toString().equalsIgnoreCase("Running"))
					response.getWriter().println("<tr bgcolor=\"LightGreen\">");
				else if(workload.getRequest().getJobState().toString().equalsIgnoreCase("Completed"))
					response.getWriter().println("<tr bgcolor=\"LightBlue\">");
				else
					response.getWriter().println("<tr>");
				response.getWriter().println("<td>" + workload.getRequest().getWorkloadId().toString() + "</td>");
				response.getWriter().println("<td>" + workload.getRequest().getParentId().toString() + "</td>");
				response.getWriter().println("<td>" + workload.getRequest().getUserName().toString() + "</td>");
				response.getWriter().println("<td>" + "[" + workload.getRequest().getJobType().toString() + "]" + workload.getRequest().getJobId().toString() + "</td>");
				response.getWriter().println("<td>" + workload.getRequest().getJobText().toString() + "</td>");
				response.getWriter().println("<td>" + workload.getRequest().getJobState().toString() + "</td>");
				response.getWriter().println("<td>" + workload.getRequest().getJobSubState().toString() + "</td>");
				Date date = new Date(workload.getRequest().getStartTimestamp());
				response.getWriter().println("<td>" + DateFormat.getDateTimeInstance().format(date) + "</td>");
				response.getWriter().println("<td>" + "MapRed[" + workload.getRequest().getMapPct().toString() + "][" + workload.getRequest().getReducePct().toString() + "]" + "</td>");
				date = new Date(workload.getRequest().getEndTimestamp());
				response.getWriter().println("<td>" + DateFormat.getDateTimeInstance().format(date) + "</td>");
				String ts = String.format("%1$tH%2$s%1$tM%2$s%1$tS",workload.getRequest().getDuration(),":");
				response.getWriter().println("<td>" + ts + "</td>");
				response.getWriter().println("</tr>");
			}
			response.getWriter().println("</table>");
		} else {
			response.getWriter().println("<h1>No workloads found</h1>");
		}
		response.getWriter().println("</body>");
		response.getWriter().println("</html>");
	}
}
*/