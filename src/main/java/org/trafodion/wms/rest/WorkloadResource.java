/*
 * Copyright 2010 The Apache Software Foundation
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.trafodion.wms.rest;

import java.util.*;
import java.io.IOException;

import javax.ws.rs.GET;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.UriInfo;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.joda.time.*;
import org.joda.time.format.*;

import org.trafodion.wms.Constants;
import org.trafodion.wms.thrift.generated.*;
import org.trafodion.wms.rest.model.WorkloadListModel;
import org.trafodion.wms.rest.model.WorkloadModel;
import org.trafodion.wms.thrift.generated.WorkloadResponse;

public class WorkloadResource extends ResourceBase {
  private static final Log LOG =
    LogFactory.getLog(WorkloadResource.class);
  
  static CacheControl cacheControl;
  static {
    cacheControl = new CacheControl();
    cacheControl.setNoCache(true);
    cacheControl.setNoTransform(false);
  }

  /**
   * Constructor
   * @throws IOException
   */
  public WorkloadResource() throws IOException {
    super();

  }
  
  public static String getDateTimeFromJulian(long aJulianTimestamp) {
      DateTimeFormatter fmt = DateTimeFormat.forPattern("yyyy-MM-dd HH:mm:ss");
      long NANO_SECOND_FROM_1970 = 621355968000000000L;
      long millis_till_1970 = 210866760000009L;
      long milliTime = (aJulianTimestamp / 1000) - millis_till_1970;

      long secondsSinceEpoch = milliTime / 1000;
      //DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970);
      DateTime theDateTime = new DateTime(milliTime);
      return theDateTime.toString(fmt);
}

  @GET
  @Produces({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
  public Response get(final @Context UriInfo uriInfo) {
	  if (LOG.isDebugEnabled()) {
		  LOG.debug("GET " + uriInfo.getAbsolutePath());
	  }

	  WorkloadListModel workloadListModel = new WorkloadListModel();
	  WorkloadResponse workloadResponse = new WorkloadResponse();

	  try {
		  servlet.getAdmin().open();
		  workloadResponse = servlet.getAdmin().workload();
		  servlet.getAdmin().close();
	  } catch (IOException e) {
		  return Response.status(Response.Status.SERVICE_UNAVAILABLE)
		  .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
		  .build();
	  }

	  //To test:
	  //curl -v -X GET -H "Accept: application/json" http://sqws123.houston.hp.com:50030/workload
	  //
	  if(workloadResponse.getWorkloadList() != null){
		  for(Request aWorkload: workloadResponse.getWorkloadList()){
			  //Fixed fields
			  String workloadId = aWorkload.getData().getKeyValues().get(Constants.WORKLOAD_ID).getStringValue();
			  String state = aWorkload.getData().getKeyValues().get(Constants.STATE).getStringValue();
			  String subState = aWorkload.getData().getKeyValues().get(Constants.SUBSTATE).getStringValue();
			  String type = aWorkload.getData().getKeyValues().get(Constants.TYPE).getStringValue();
			  String workloadText = null; 
			  if(aWorkload.getData().getKeyValues().get("request") != null)
				  workloadText = aWorkload.getData().getKeyValues().get("request").getStringValue();
			  else if (aWorkload.getData().getKeyValues().get("queryText") != null)
				  workloadText = aWorkload.getData().getKeyValues().get("queryText").getStringValue();  
			  if(workloadText != null)
				  workloadText = workloadText.replaceAll("[\\r\\t]","");
			  //Key value pairs
			  StringBuilder workloadDetails = new StringBuilder();
			  if(aWorkload.getData().getKeyValues() != null){
		          boolean isFirst = true;
				  for (String key: aWorkload.getData().getKeyValues().keySet()) {
					  if(aWorkload.getData().getKeyValues().get(key).isSetByteValue()) {
		                  if(! isFirst) workloadDetails.append(", ");
						  workloadDetails.append(key + "=" + aWorkload.getData().getKeyValues().get(key).getByteValue());
					  } else if(aWorkload.getData().getKeyValues().get(key).isSetShortValue()) {
		                  if(! isFirst) workloadDetails.append(", ");						  
						  workloadDetails.append(key + "=" + aWorkload.getData().getKeyValues().get(key).getShortValue());
					  } else if(aWorkload.getData().getKeyValues().get(key).isSetIntValue()) {
		                  if(! isFirst) workloadDetails.append(", ");						  
						  workloadDetails.append(key + "=" + aWorkload.getData().getKeyValues().get(key).getIntValue());
					  } else if(aWorkload.getData().getKeyValues().get(key).isSetLongValue()) {
						  long time;
		                  if(! isFirst) workloadDetails.append(", ");
						  if(key.equalsIgnoreCase("beginTimestamp"))
							  workloadDetails.append(key + "=" + new Date(aWorkload.getData().getKeyValues().get(Constants.BEGIN_TIMESTAMP).getLongValue()));
						  else if(key.equalsIgnoreCase("endTimestamp"))
							  workloadDetails.append(key + "=" + new Date(aWorkload.getData().getKeyValues().get(Constants.END_TIMESTAMP).getLongValue()));
						  else if(key.equalsIgnoreCase("lastUpdated")) 
							  workloadDetails.append(key + "=" + new Date(aWorkload.getHeader().getServerLastUpdated()));
						  else if(key.equalsIgnoreCase("queryStartTime")) {
							  time = aWorkload.getData().getKeyValues().get("queryStartTime").getLongValue();
							  if(time != 0L)
								  workloadDetails.append(key + "=" + getDateTimeFromJulian(time));
							  else
								  workloadDetails.append(key + "=" + time);
						  } else if(key.equalsIgnoreCase("queryEndTime")) {
							  time = aWorkload.getData().getKeyValues().get("queryEndTime").getLongValue();
							  if(time != 0L)
								  workloadDetails.append(key + "=" + getDateTimeFromJulian(time));
							  else
								  workloadDetails.append(key + "=" + time);
						  } else if(key.equalsIgnoreCase("compileStartTime")) {
							  time = aWorkload.getData().getKeyValues().get("compileStartTime").getLongValue();
							  if(time != 0L)
								  workloadDetails.append(key + "=" + getDateTimeFromJulian(time));
							  else
								  workloadDetails.append(key + "=" + time);
						  } else if(key.equalsIgnoreCase("compileEndTime")) { 
							  time = aWorkload.getData().getKeyValues().get("compileEndTime").getLongValue();
							  if(time != 0L)
								  workloadDetails.append(key + "=" + getDateTimeFromJulian(time));
							  else
								  workloadDetails.append(key + "=" + time);
						  } else 
							  workloadDetails.append(key + "=" + aWorkload.getData().getKeyValues().get(key).getLongValue());
					  } else if(aWorkload.getData().getKeyValues().get(key).isSetFloatValue()) {
		                  if(! isFirst) workloadDetails.append(", ");						  
						  workloadDetails.append(key + "=" + aWorkload.getData().getKeyValues().get(key).getFloatValue());
					  } else if(aWorkload.getData().getKeyValues().get(key).isSetStringValue()) {
						  if(key.equalsIgnoreCase("queryText") || key.equalsIgnoreCase("request")  
							  || key.equalsIgnoreCase("state") || key.equalsIgnoreCase("subState")
							  || key.equalsIgnoreCase("type") || key.equalsIgnoreCase("workloadId")) {
							  continue;
						  } else {
							  String s = aWorkload.getData().getKeyValues().get(key).getStringValue();
							  if(! isFirst) workloadDetails.append(", ");
			                  workloadDetails.append(key + "=" + s);
						  }
					  }
					  
		              isFirst = false;
				  } 
			  }

			  workloadListModel.add(new WorkloadModel(workloadId,state,subState,type,workloadText,workloadDetails.toString()));
		  }
	  }

	  ResponseBuilder response;
	  if(workloadResponse.getWorkloadList() == null){
		  response = Response.ok("[]");
	  } else {
		  response = Response.ok(workloadListModel);
	  }
	  response.cacheControl(cacheControl);
	  return response.build();
  }
}
