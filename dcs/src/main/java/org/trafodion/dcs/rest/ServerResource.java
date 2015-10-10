/**********************************************************************
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**********************************************************************/

package org.trafodion.dcs.rest;

import java.io.*;
import java.util.*;

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
import org.trafodion.dcs.rest.RestConstants;
import org.trafodion.dcs.rest.model.ServerModel;

public class ServerResource extends ResourceBase {
	private static final Log LOG =
		LogFactory.getLog(ServerResource.class);

	static CacheControl cacheControl;
	static {
		cacheControl = new CacheControl();
		cacheControl.setNoCache(true);
		cacheControl.setNoTransform(false);
	}

	/**
* @@@ START COPYRIGHT @@@

	Licensed to the Apache Software Foundation (ASF) under one
	or more contributor license agreements.  See the NOTICE file
	distributed with this work for additional information
	regarding copyright ownership.  The ASF licenses this file
	to you under the Apache License, Version 2.0 (the
	"License"); you may not use this file except in compliance
	with the License.  You may obtain a copy of the License at

	  http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing,
	software distributed under the License is distributed on an
	"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
	KIND, either express or implied.  See the License for the
	specific language governing permissions and limitations
	under the License.

* @@@ END COPYRIGHT @@@
	 */
	public ServerResource() throws IOException {
		super();
	}

	@GET
	@Produces({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
	public Response get(final @Context UriInfo uriInfo) {
		if (LOG.isDebugEnabled()) {
			LOG.debug("GET " + uriInfo.getAbsolutePath());
		}

		try {
			List<String> master = servlet.getMaster();
			List<String> running = servlet.getRunning();
			List<String> registered = servlet.getRegistered();
		    
			ZkClient zkc = servlet.getZk();
			Stat stat = null;
			
		    ServerModel model = new ServerModel();
		    
			String data = null;

			if(master != null){
				for(String znode: master) {
					data = null;
					try {
						data = Bytes.toString(zkc.getData(servlet.getParentZnode() + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER + "/" + znode, false, stat));
					} catch (Exception e) {
						LOG.error(e);
					}

					ServerModel.DcsMaster dcsMaster = model.addDcsMaster(znode,data);

					if(running != null){
						Collections.sort(running);
						data = null;
						for(String znodeRun: running) {
							try {
								data = Bytes.toString(zkc.getData(servlet.getParentZnode() + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/" + znodeRun, false, stat));
							} catch (Exception e) {
								LOG.error(e);
							}
							
							ServerModel.DcsServer dcsServer = dcsMaster.addDcsServer(znodeRun,data);

							if(registered != null){
								Collections.sort(registered);
								data = null;
								for(String znodeReg: registered) {
									try {
										data = Bytes.toString(zkc.getData(servlet.getParentZnode() + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED + "/" + znodeReg, false, stat));
									} catch (Exception e) {
										LOG.error(e);
									}
									
									dcsServer.addTrafodionServer(znodeReg,data);
								}
							}
						}
					}
				}
			}
			
			ResponseBuilder response = Response.ok(model);
			response.cacheControl(cacheControl);
			return response.build();
		} catch (IOException e) {
			return Response.status(Response.Status.SERVICE_UNAVAILABLE)
			.type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
			.build();
		}
	}
}
