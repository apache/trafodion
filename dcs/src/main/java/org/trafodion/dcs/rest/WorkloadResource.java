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

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.Bytes;
import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;
import org.trafodion.dcs.rest.model.WorkloadListModel;
import org.trafodion.dcs.rest.model.WorkloadModel;

import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

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
	public WorkloadResource() throws IOException {
		super();

	}

	@GET
	@Produces({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
	public Response get(final @Context UriInfo uriInfo) {
		if (LOG.isDebugEnabled()) {
			LOG.debug("GET " + uriInfo.getAbsolutePath());
		}

		try {
//			ScriptContext scriptContext = new ScriptContext();
//			scriptContext.setScriptName(Constants.JDBCT2UTIL_SCRIPT_NAME);
//			scriptContext.setCommand(Constants.TRAFODION_REPOS_METRIC_SESSION_TABLE);

			try {
//				ScriptManager.getInstance().runScript(scriptContext);//This will block while script is running
			} catch (Exception e) {
				e.printStackTrace();
				throw new IOException(e);
			}

//			StringBuilder sb = new StringBuilder();
//			sb.append("exit code [" + scriptContext.getExitCode() + "]");
//			if(! scriptContext.getStdOut().toString().isEmpty()) 
//				sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
//			if(! scriptContext.getStdErr().toString().isEmpty())
//				sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
//			LOG.info(sb.toString());
			
			JSONArray workloadList = null;

//			try {
//				if(scriptContext.getExitCode() == 0 && (! scriptContext.getStdOut().toString().isEmpty())) {
//					workloadList = new JSONArray(scriptContext.getStdOut().toString());
//				}
//			} catch (Exception e) {
//				e.printStackTrace();
//				LOG.error(e.getMessage());
//				throw new IOException(e);
//			}			

			ResponseBuilder response = Response.ok(workloadList);
			response.cacheControl(cacheControl);
			return response.build();
		} catch (IOException e) {
			return Response.status(Response.Status.SERVICE_UNAVAILABLE)
			.type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
			.build();
		}
	}

}