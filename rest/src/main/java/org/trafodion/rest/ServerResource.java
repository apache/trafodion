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
import javax.ws.rs.core.Request;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriInfo;
import javax.ws.rs.core.MultivaluedMap;
import javax.ws.rs.core.Response.ResponseBuilder;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;

import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

import org.trafodion.rest.script.ScriptManager;
import org.trafodion.rest.script.ScriptContext;
import org.trafodion.rest.Constants;
import org.trafodion.rest.util.Bytes;
import org.trafodion.rest.util.RestConfiguration;
import org.trafodion.rest.zookeeper.ZkClient;
import org.trafodion.rest.RestConstants;

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
	 * Constructor
	 * @throws IOException
	 */
	public ServerResource() throws IOException {
		super();
	}
	
	private String sqcheck(String operation) throws IOException {
	    ScriptContext scriptContext = new ScriptContext();
	    scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
	    scriptContext.setCommand("sqcheck -j -c " + operation);

	    try {
	        ScriptManager.getInstance().runScript(scriptContext);//This will block while script is running
	    } catch (Exception e) {
	        e.printStackTrace();
	        throw new IOException(e);
	    }

        if(LOG.isDebugEnabled()) {
            StringBuilder sb = new StringBuilder();
            sb.append("exit code [" + scriptContext.getExitCode() + "]");
            if(! scriptContext.getStdOut().toString().isEmpty()) 
                sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
            if(! scriptContext.getStdErr().toString().isEmpty())
                sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
            LOG.debug(sb.toString());
        }

	    // sqcheck will return:
	    //-1 - Not up ($?=255)
	    // 0 - Fully up and operational
	    // 1 - Partially up and operational
	    // 2 - Partially up and NOT operational
	    String state = null;
	    String subState = null;
	    int exitCode = scriptContext.getExitCode();
	    switch(exitCode) {
	    case 0:  
	        state = "UP";
	        subState = "OPERATIONAL";
	        break;
	    case 1:   
	        state = "PARTIALLY UP";
	        subState = "OPERATIONAL";
	        break;
	    case 2:   
	        state = "PARTIALLY UP";
	        subState = "NOT OPERATIONAL";
	        break;
	    case 255:  
	        state = "DOWN";
	        subState = "NOT OPERATIONAL";
	        break;              
	    default:
	        state = "UNKNOWN";
	        subState = "UNKNOWN";
	    }

	    JSONObject jsonObject = new JSONObject();
	    try {
	        jsonObject.put("STATE", state);
	        jsonObject.put("SUBSTATE", subState);
	        JSONArray jsonArray = new JSONArray(scriptContext.getStdOut().toString());
	        jsonObject.put("PROCESSES",jsonArray);
	    } catch (Exception e) {
	        e.printStackTrace();
	        throw new IOException(e);
	    }

	    return jsonObject.toString();
	}

	private String pstack(String program) throws IOException {
	    ScriptContext scriptContext = new ScriptContext();
	    scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
	    scriptContext.setCommand("sqpstack " + program);
	    scriptContext.setStripStdOut(false);

	    try {
	        ScriptManager.getInstance().runScript(scriptContext);//This will block while script is running
	    } catch (Exception e) {
	        e.printStackTrace();
	        throw new IOException(e);
	    }

	    if(LOG.isDebugEnabled()) {
	        StringBuilder sb = new StringBuilder();
	        sb.append("exit code [" + scriptContext.getExitCode() + "]");
	        if(! scriptContext.getStdOut().toString().isEmpty()) 
	            sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
	        if(! scriptContext.getStdErr().toString().isEmpty())
	            sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
	        LOG.debug(sb.toString());
	    }

	    JSONArray json = null;
	    try {
	        json = new JSONArray();
	        StringBuilder sb = new StringBuilder();
	        boolean pstack = false;
	        Scanner scanner = new Scanner(scriptContext.getStdOut().toString()); 
	        while(scanner.hasNextLine()) {
	            String line = scanner.nextLine();
	            if(line.contains("pstack-ing monitor")) {
	                continue;
	            } else if (line.contains("pstack")) {
	                if(pstack == true) {
	                    json.put(new JSONObject().put("PROGRAM", sb.toString()));
	                    sb.setLength(0);
	                    sb.append(line + "\n");
	                } else {
	                    pstack = true;
	                    sb.append(line + "\n");
	                }
	            } else {
	                sb.append(line + "\n");
	            }
	        }
            scanner.close();
	    } catch (Exception e) {
	        e.printStackTrace();
	        throw new IOException(e);
	    }
	    
	    return json.toString();
	}
	
    private String dcs() throws IOException {

        JSONArray json = new JSONArray();
        try {
            List<RunningServer> servers = servlet.getDcsServersList();
            if(LOG.isDebugEnabled())
                LOG.debug("servers=" + servers);

            for (RunningServer aRunningServer: servers) {
                for (RegisteredServer aRegisteredServer: aRunningServer.getRegistered()) {
                    JSONObject obj = new JSONObject();
                    obj.put("HOSTNAME",aRunningServer.getHostname());
                    obj.put("INSTANCE",aRunningServer.getInstance());
                    obj.put("START_TIME",aRunningServer.getStartTimeAsDate());
                    obj.put("REGISTERED", aRegisteredServer.getIsRegistered());
                    obj.put("STATE",aRegisteredServer.getState());
                    obj.put("NID",aRegisteredServer.getNid());
                    obj.put("PID",aRegisteredServer.getPid());
                    obj.put("PROCESS_NAME",aRegisteredServer.getProcessName());
                    obj.put("IP_ADDRESS",aRegisteredServer.getIpAddress());
                    obj.put("PORT",aRegisteredServer.getPort());
                    obj.put("LAST_UPDATED",aRegisteredServer.getTimestampAsDate());
                    obj.put("CLIENT_NAME",aRegisteredServer.getClientName());
                    obj.put("CLIENT_APPL",aRegisteredServer.getClientAppl());
                    obj.put("CLIENT_IP_ADDRESS",aRegisteredServer.getClientIpAddress());
                    obj.put("CLIENT_PORT",aRegisteredServer.getClientPort());
                    json.put(obj);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            throw new IOException(e);
        }

        return json.toString();
    }
    
    private String nodes() throws IOException {
        ScriptContext scriptContext = new ScriptContext();
        scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
        scriptContext.setCommand("sqnodestatus json");

        try {
            ScriptManager.getInstance().runScript(scriptContext);//This will block while script is running
        } catch (Exception e) {
            e.printStackTrace();
            throw new IOException(e);
        }

        if(LOG.isDebugEnabled()) {
            StringBuilder sb = new StringBuilder();
            sb.append("exit code [" + scriptContext.getExitCode() + "]");
            if(! scriptContext.getStdOut().toString().isEmpty()) 
                sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
            if(! scriptContext.getStdErr().toString().isEmpty())
                sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
            LOG.debug(sb.toString());
        }

        return scriptContext.getStdOut().toString();
    }
    
    @GET
    @Produces({MIMETYPE_JSON})
    public Response getAll(
            final @Context UriInfo uriInfo,
            final @Context Request request) {
        try {
            if (LOG.isDebugEnabled()) {
                LOG.debug("GET " + uriInfo.getAbsolutePath());

                MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
                String output = " Query Parameters :\n";
                for (String key : queryParams.keySet()) {
                    output += key + " : " + queryParams.getFirst(key) +"\n";
                }
                LOG.debug(output);

                MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
                output = " Path Parameters :\n";
                for (String key : pathParams.keySet()) {
                    output += key + " : " + pathParams.getFirst(key) +"\n";
                }
                LOG.debug(output);
            }

            String result = sqcheck("all");
 
            ResponseBuilder response = Response.ok(result);
            response.cacheControl(cacheControl);
            return response.build();
        } catch (IOException e) {
            e.printStackTrace();
            return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                    .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
                    .build();
        }
    }    
    
    @GET
    @Path("/dtm")
    @Produces({MIMETYPE_JSON})
    public Response getDtm(
            final @Context UriInfo uriInfo,
            final @Context Request request) {
        try {
            if (LOG.isDebugEnabled()) {
                LOG.debug("GET " + uriInfo.getAbsolutePath());

                MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
                String output = " Query Parameters :\n";
                for (String key : queryParams.keySet()) {
                    output += key + " : " + queryParams.getFirst(key) +"\n";
                }
                LOG.debug(output);

                MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
                output = " Path Parameters :\n";
                for (String key : pathParams.keySet()) {
                    output += key + " : " + pathParams.getFirst(key) +"\n";
                }
                LOG.debug(output);
            }

            String result = sqcheck("dtm");
 
            ResponseBuilder response = Response.ok(result);
            response.cacheControl(cacheControl);
            return response.build();
        } catch (IOException e) {
            e.printStackTrace();
            return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                    .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
                    .build();
        }
    }

    @GET
    @Path("/rms")
    @Produces({MIMETYPE_JSON})
    public Response getRms(
            final @Context UriInfo uriInfo,
            final @Context Request request) {
        try {
            if (LOG.isDebugEnabled()) {
                LOG.debug("GET " + uriInfo.getAbsolutePath());

                MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
                String output = " Query Parameters :\n";
                for (String key : queryParams.keySet()) {
                    output += key + " : " + queryParams.getFirst(key) +"\n";
                }
                LOG.debug(output);

                MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
                output = " Path Parameters :\n";
                for (String key : pathParams.keySet()) {
                    output += key + " : " + pathParams.getFirst(key) +"\n";
                }
                LOG.debug(output);
            }

            String result = sqcheck("rms");
 
            ResponseBuilder response = Response.ok(result);
            response.cacheControl(cacheControl);
            return response.build();
        } catch (IOException e) {
            e.printStackTrace();
            return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                    .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
                    .build();
        }
    }
    
    @GET
    @Path("/dcs")
    @Produces({MIMETYPE_JSON})
    public Response getDcs(
            final @Context UriInfo uriInfo,
            final @Context Request request) {
        try {
            if (LOG.isDebugEnabled()) {
                LOG.debug("GET " + uriInfo.getAbsolutePath());

                MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
                String output = " Query Parameters :\n";
                for (String key : queryParams.keySet()) {
                    output += key + " : " + queryParams.getFirst(key) +"\n";
                }
                LOG.debug(output);

                MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
                output = " Path Parameters :\n";
                for (String key : pathParams.keySet()) {
                    output += key + " : " + pathParams.getFirst(key) +"\n";
                }
                LOG.debug(output);
            }

            String result = sqcheck("dcs");
 
            ResponseBuilder response = Response.ok(result);
            response.cacheControl(cacheControl);
            return response.build();
        } catch (IOException e) {
            e.printStackTrace();
            return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                    .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
                    .build();
        }
    }   
    
    @GET
    @Path("/dcs/connections")
    @Produces({MIMETYPE_JSON})
    public Response getConnections(
            final @Context UriInfo uriInfo,
            final @Context Request request) {
        try {
            if (LOG.isDebugEnabled()) {
                LOG.debug("GET " + uriInfo.getAbsolutePath());

                MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
                String output = " Query Parameters :\n";
                for (String key : queryParams.keySet()) {
                    output += key + " : " + queryParams.getFirst(key) +"\n";
                }
                LOG.debug(output);

                MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
                output = " Path Parameters :\n";
                for (String key : pathParams.keySet()) {
                    output += key + " : " + pathParams.getFirst(key) +"\n";
                }
                LOG.debug(output);
            }

            String result = dcs();
 
            ResponseBuilder response = Response.ok(result);
            response.cacheControl(cacheControl);
            return response.build();
        } catch (IOException e) {
            e.printStackTrace();
            return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                    .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
                    .build();
        }
    }
    
    @GET
    @Path("/nodes")
    @Produces({MIMETYPE_JSON})
    public Response getNodes(
            final @Context UriInfo uriInfo,
            final @Context Request request) {
        try {
            if (LOG.isDebugEnabled()) {
                LOG.debug("GET " + uriInfo.getAbsolutePath());

                MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
                String output = " Query Parameters :\n";
                for (String key : queryParams.keySet()) {
                    output += key + " : " + queryParams.getFirst(key) +"\n";
                }
                LOG.debug(output);

                MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
                output = " Path Parameters :\n";
                for (String key : pathParams.keySet()) {
                    output += key + " : " + pathParams.getFirst(key) +"\n";
                }
                LOG.debug(output);
            }

            String result = nodes();
 
            ResponseBuilder response = Response.ok(result);
            response.cacheControl(cacheControl);
            return response.build();
        } catch (IOException e) {
            e.printStackTrace();
            return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                    .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
                    .build();
        }
    }
    
    @GET
    @Path("/pstack")
    @Produces({MIMETYPE_JSON})
    public Response getPstack(
            final @Context UriInfo uriInfo,
            final @Context Request request) {
        try {
            if (LOG.isDebugEnabled()) {
                LOG.debug("GET " + uriInfo.getAbsolutePath());

                MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
                String output = " Query Parameters :\n";
                for (String key : queryParams.keySet()) {
                    output += key + " : " + queryParams.getFirst(key) +"\n";
                }
                LOG.debug(output);

                MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
                output = " Path Parameters :\n";
                for (String key : pathParams.keySet()) {
                    output += key + " : " + pathParams.getFirst(key) +"\n";
                }
                LOG.debug(output);
            }

            String result = pstack("");

            ResponseBuilder response = Response.ok(result);
            response.cacheControl(cacheControl);
            return response.build();
        } catch (IOException e) {
            e.printStackTrace();
            return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                    .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
                    .build();
        }
    }
    
    @GET
    @Path("/pstack/program/{program}")
    @Produces({MIMETYPE_JSON})
    public Response getPstackProgram(
            final @Context UriInfo uriInfo,
            final @Context Request request,
            @PathParam("program") String program) {
        try {
            if (LOG.isDebugEnabled()) {
                LOG.debug("GET " + uriInfo.getAbsolutePath());

                MultivaluedMap<String, String> queryParams = uriInfo.getQueryParameters();
                String output = " Query Parameters :\n";
                for (String key : queryParams.keySet()) {
                    output += key + " : " + queryParams.getFirst(key) +"\n";
                }
                LOG.debug(output);

                MultivaluedMap<String, String> pathParams = uriInfo.getPathParameters();
                output = " Path Parameters :\n";
                for (String key : pathParams.keySet()) {
                    output += key + " : " + pathParams.getFirst(key) +"\n";
                }
                LOG.debug(output);
            }
            String result = pstack(program);
 
            ResponseBuilder response = Response.ok(result);
            response.cacheControl(cacheControl);
            return response.build();
        } catch (IOException e) {
            e.printStackTrace();
            return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                    .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
                    .build();
        }
    }    
}
