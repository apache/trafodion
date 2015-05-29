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

import java.io.IOException;

import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.PUT;
import javax.ws.rs.Produces;
import javax.ws.rs.Consumes;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.HttpHeaders;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.UriInfo;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.wms.rest.model.StreamListModel;
import org.trafodion.wms.rest.model.StreamModel;
import org.trafodion.wms.thrift.generated.Stream;
import org.trafodion.wms.thrift.generated.StreamResponse;

public class StreamResource extends ResourceBase {
  private static final Log LOG = LogFactory.getLog(StreamResource.class);
  
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
  public StreamResource() throws IOException {
    super();
  }

  @GET
  @Produces({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
  public Response get(final @Context UriInfo uriInfo) {
	  if (LOG.isDebugEnabled()) {
		  LOG.debug("GET " + uriInfo.getAbsolutePath()
				  + " " + uriInfo.getQueryParameters());
	  }

	  StreamListModel streamListModel = new StreamListModel();
	  StreamResponse streamResponse = new StreamResponse();
	  
	  try {
		  servlet.getAdmin().open();
		  streamResponse = servlet.getAdmin().stream();
		  servlet.getAdmin().close();
	  } catch (IOException e) {
		  return Response.status(Response.Status.SERVICE_UNAVAILABLE)
		  .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
		  .build();
	  }
	  
	  //To test:
	  //curl -v -X GET -H "Accept: application/json" http://sqws123.houston.hp.com:50030/stream
	  //
	  //Should see something like this:
	  //{"streams":[{"comment":"Added by Administrator","name":"vertica","text":"user,string,application,string,transactionId,string,poolName,string,requestId,string,memoryAllocated,string,rowsSent,string,requestType,string,type,string,duration,string,sessionId,string,request,string,userName,string,statementId,string","timestamp":1398215824738}]}
	  //
	  if(streamResponse.getStreamList() != null){
		  for(Stream aStream: streamResponse.getStreamList()){
			  streamListModel.add(new StreamModel(aStream.getName(),aStream.getValue(),aStream.getComment(),aStream.getTimestamp()));
		  }
	  }
	  
	  ResponseBuilder response;
	  if(streamResponse.getStreamList() == null){
		  response = Response.ok("[]");
	  } else {
		  response = Response.ok(streamListModel);
	  }
	  
	  response.cacheControl(cacheControl);
	  return response.build();

  }
  
  @PUT
  @Consumes({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
  public Response put(final StreamModel model,
		  final @Context UriInfo uriInfo) {
	  if (LOG.isDebugEnabled()) {
		  LOG.debug("PUT " + uriInfo.getAbsolutePath()
				  + " " + uriInfo.getQueryParameters());
	  }
	  
	  if (servlet.isReadOnly()) {
		  return Response.status(Response.Status.FORBIDDEN)
		  .type(MIMETYPE_TEXT).entity("Forbidden" + CRLF)
		  .build();
	  }
	  //To test:
	  //Vertica
	  //curl -v -X PUT -H "Accept: application/json" -H "Content-type: application/json" -d '{"name":"vertica","text":"user,string,application,string,transactionId,string,poolName,string,requestId,string,memoryAllocated,string,rowsSent,string,requestType,string,type,string,duration,string,sessionId,string,request,string,userName,string,statementId,string"}' http://sqws123.houston.hp.com:50030/stream
	  
	  //Trafodion
	  //curl -v -X PUT -H "Accept: application/json" -H "Content-type: application/json" -d '{"name":"trafodion","text":"type,string,userName,string,sessionId,string,aggrEstimatedRowsUsed,float,deltaEstimatedRowsUsed,float,deltaEstimatedRowsAccessed,float,aggrEstimatedRowsAccessed,float,deltaNumRows,long,deltaRowsRetrieved,long,deltaRowsAccessed,long,aggrRowsRetrieved,long,aggrRowsAccessed,long,aggrNumRowsIUD,long"}' http://sqws123.houston.hp.com:50030/stream
	  LOG.debug("Alter stream " + model.toString());
	  Stream stream = new Stream(model.getName(),model.getText(),model.getComment(),System.currentTimeMillis());
	  try {
		  servlet.getAdmin().open();
		  servlet.getAdmin().alterStream(stream);
		  servlet.getAdmin().close();
	  } catch (IOException e) {
		  return Response.status(Response.Status.SERVICE_UNAVAILABLE)
		  .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
		  .build();
	  }
	  ResponseBuilder response = Response.ok();
	  return response.build();
  }
  
  @POST
  @Consumes({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
  public Response post(final StreamModel model,
		  final @Context UriInfo uriInfo) {
	  if (LOG.isDebugEnabled()) {
		  LOG.debug("POST " + uriInfo.getAbsolutePath()
				  + " " + uriInfo.getQueryParameters());
	  }
	  
	  if (servlet.isReadOnly()) {
		  return Response.status(Response.Status.FORBIDDEN)
		  .type(MIMETYPE_TEXT).entity("Forbidden" + CRLF)
		  .build();
	  }
	  
	  //To test:
	  
	  //Vertica
	  //curl -v -X POST -H "Accept: application/json" -H "Content-type: application/json" -d '{"name":"vertica","text":"user,string,application,string,transactionId,string,poolName,string,requestId,string,memoryAllocated,string,rowsSent,string,requestType,string,type,string,duration,string,sessionId,string,request,string,userName,string,statementId,string"}' http://sqws123.houston.hp.com:50030/stream
	  
	  //Trafodion
	  //curl -v -X POST -H "Accept: application/json" -H "Content-type: application/json" -d '{"name":"trafodion","text":"type,string,userName,string,sessionId,string,aggrEstimatedRowsUsed,float,deltaEstimatedRowsUsed,float,deltaEstimatedRowsAccessed,float,aggrEstimatedRowsAccessed,float,deltaNumRows,long,deltaRowsRetrieved,long,deltaRowsAccessed,long,aggrRowsRetrieved,long,aggrRowsAccessed,long,aggrNumRowsIUD,long"}' http://sqws123.houston.hp.com:50030/stream
	  LOG.debug("Add stream " + model.toString());
	  Stream stream = new Stream(model.getName(),model.getText(),model.getComment(),System.currentTimeMillis());
	  try {
		  servlet.getAdmin().open();
		  servlet.getAdmin().addStream(stream);
		  servlet.getAdmin().close();
	  } catch (IOException e) {
		  return Response.status(Response.Status.SERVICE_UNAVAILABLE)
		  .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
		  .build();
	  }
	  ResponseBuilder response = Response.ok();
	  return response.build();
  }
  
  @DELETE
  public Response delete(final StreamModel model,
		  final @Context UriInfo uriInfo) {
	  if (LOG.isDebugEnabled()) {
		  LOG.debug("DELETE " + uriInfo.getAbsolutePath()
				  + " " + uriInfo.getQueryParameters());
	  }

	  if (servlet.isReadOnly()) {
		  return Response.status(Response.Status.FORBIDDEN)
		  .type(MIMETYPE_TEXT).entity("Forbidden" + CRLF)
		  .build();
	  }

	  //To test:
	  
	  //Vertica
	  //curl -v -X DELETE -H "Accept: application/json" -H "Content-type: application/json" -d '{"name":"vertica"}' http://sqws123.houston.hp.com:50030/stream
	  
	  //Trafodion
	  //curl -v -X DELETE -H "Accept: application/json" -H "Content-type: application/json" -d '{"name":"trafodion"}' http://sqws123.houston.hp.com:50030/stream
	  LOG.debug("Delete stream " + model.toString());
	  Stream stream = new Stream(model.getName(),model.getText(),model.getComment(),System.currentTimeMillis());
	  try {
		  servlet.getAdmin().open();
		  servlet.getAdmin().deleteStream(stream);
		  servlet.getAdmin().close();
	  } catch (IOException e) {
		  return Response.status(Response.Status.SERVICE_UNAVAILABLE)
		  .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
		  .build();
	  }
	  return Response.ok().build();
  }
}
