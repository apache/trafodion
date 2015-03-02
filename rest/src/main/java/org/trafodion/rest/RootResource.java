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

package org.trafodion.rest;

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

import org.trafodion.rest.Constants;
import org.trafodion.rest.util.Bytes;
import org.trafodion.rest.RestConstants;

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
	
	@GET
	@Produces({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
	public Response get(final @Context UriInfo uriInfo) {
		if (LOG.isDebugEnabled()) {
			LOG.debug("GET " + uriInfo.getAbsolutePath());
		}
		
		try {
			return new WorkloadResource().get(uriInfo);
		} catch (IOException e) {
			return Response.status(Response.Status.SERVICE_UNAVAILABLE)
			.type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
			.build();
		}

	}

	@Path("/v1/servers")
	public ServerResource getServerResource() throws IOException {
		//To test:
		//curl -v -X GET -H "Accept: application/json" http://<Rest server IP address>:8080/v1/servers
		//
		return new ServerResource();
	}
	
	@Path("/v1/workloads")
	public WorkloadResource getWorkloadResource() throws IOException { 
		//To test:
		//curl -v -X GET -H "Accept: application/json" http://<Rest server IP address>:8080/v1/workloads
		//
		return new WorkloadResource();
	}

	@Path("/v1/version")
	public VersionResource getVersionResource() throws IOException {
		//To test:
		//curl -v -X GET -H "Accept: application/json" http://<Rest server IP address>:8080/v1/version
		//
		return new VersionResource();
	}

}
