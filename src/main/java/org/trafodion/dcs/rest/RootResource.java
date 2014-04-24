/*
 *(C) Copyright 2013 Hewlett-Packard Development Company, L.P.
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
package org.trafodion.dcs.rest;

import java.io.*;
import java.util.List;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;

import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriInfo;
import javax.ws.rs.core.Response.ResponseBuilder;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.Bytes;
import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.rest.model.ServerModel;
import org.trafodion.dcs.rest.model.ServerListModel;

@Path("/")
public class RootResource extends ResourceBase {
	private static final Log LOG = LogFactory.getLog(RootResource.class);

	static CacheControl cacheControl;
	static {
		cacheControl = new CacheControl();
		cacheControl.setNoCache(true);
		cacheControl.setNoTransform(false);
	}

	public RootResource() throws IOException {
		super();
	}
	
	private final ServerListModel getServerList() throws IOException {
		ServerListModel serverList = new ServerListModel();

		List<String> master = servlet.getMaster();
		List<String> running = servlet.getRunning();
		List<String> registered = servlet.getRegistered();

		ZkClient zkc = servlet.getZk();
		Stat stat=null;
		
		if(master != null){
			for(String znode: master) {
				String data=null;
				try {
					data = Bytes.toString(zkc.getData(servlet.getParentZnode() + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER + "/" + znode, false, stat));
				} catch (Exception e) {
					LOG.error(e);
				}
				serverList.add(new ServerModel("MASTER",znode,data));
			}
		}
		if(running != null){
			Collections.sort(running);
			String data=null;
			for(String znode: running) {
				try {
					data = Bytes.toString(zkc.getData(servlet.getParentZnode() + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/" + znode, false, stat));
				} catch (Exception e) {
					LOG.error(e);
				}
				serverList.add(new ServerModel("RUNNING",znode,data));
			}
		}
		if(registered != null){
			Collections.sort(registered);
			String data=null;
			for(String znode: registered) {
				try {
					data = Bytes.toString(zkc.getData(servlet.getParentZnode() + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED + "/" + znode, false, stat));
				} catch (Exception e) {
					LOG.error(e);
				}
				serverList.add(new ServerModel("REGISTERED",znode,data));
			}
		}

		return serverList;
	}

	@GET
	@Produces({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
	public Response get(final @Context UriInfo uriInfo) {
		if (LOG.isDebugEnabled()) {
			LOG.debug("GET " + uriInfo.getAbsolutePath());
		}
		//servlet.getMetrics().incrementRequests(1);
	
		try {
			ResponseBuilder response = Response.ok(getServerList());
			response.cacheControl(cacheControl);
			//servlet.getMetrics().incrementSucessfulGetRequests(1);
			return response.build();
		} catch (IOException e) {
			//servlet.getMetrics().incrementFailedGetRequests(1);
			return Response.status(Response.Status.SERVICE_UNAVAILABLE)
			.type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
			.build();
		}
	}

	@Path("version")
	public VersionResource getVersionResource() throws IOException {
		return new VersionResource();
	}

}
