/**
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
*/

package org.trafodion.rest;

import java.io.IOException;

import javax.servlet.ServletContext;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriInfo;
import javax.ws.rs.core.Response.ResponseBuilder;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.rest.model.VersionModel;

import javax.ws.rs.HeaderParam;
import javax.ws.rs.core.HttpHeaders;

/**
 * Implements REST software version reporting
 * <p>
 * <tt>/version/rest</tt>
 * <p>
 * <tt>/version</tt> (alias for <tt>/version/rest</tt>)
 */
public class VersionResource extends ResourceBase {

    private static final Log LOG = LogFactory.getLog(VersionResource.class);

    static CacheControl cacheControl;
    static {
        cacheControl = new CacheControl();
        cacheControl.setNoCache(true);
        cacheControl.setNoTransform(false);
    }

    /**
     * Constructor
     *
     * @throws IOException
     */
    public VersionResource() throws IOException {
        super();
    }

    /**
     * Build a response for a version request.
     *
     * @param context
     *            servlet context
     * @param uriInfo
     *            (JAX-RS context variable) request URL
     * @return a response for a version request
     */

    @GET
    @Produces({ MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON })
    public Response get(final @Context ServletContext context, final @Context UriInfo uriInfo,
            @HeaderParam(HttpHeaders.AUTHORIZATION) final String auth) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("GET " + uriInfo.getAbsolutePath());
        }

        Response rs = TokenTool.getResponse(auth);
        if (rs != null)
            return rs;

        /*
         * Token tk=new Token(); String access_token=""; String token_type="";
         * String[] to=auth.split(" "); if(to.length==2){
         * access_token=to[1].trim(); token_type=to[0].trim(); }
         *
         * tk.setToken(access_token);
         * if(!token_type.equals("trafodion")||tk.isTimeOut()){ JSONObject
         * jsonObject = new JSONObject(); try { jsonObject.put(Token.error,
         * Token.error_invalid_token); jsonObject.put(Token.error_description,
         * "The access token expired");
         *
         * } catch (Exception e) { e.printStackTrace();
         *
         * } return Response.status(Response.Status.UNAUTHORIZED)
         * .type(MIMETYPE_JSON) .entity(jsonObject) .build(); }
         */

        ResponseBuilder response = Response.ok(new VersionModel(context));
        response.cacheControl(cacheControl);
        return response.build();
    }

    /**
     * Dispatch <tt>/version/rest</tt> to self.
     */
    @Path("rest")
    public VersionResource getVersionResource() {
        return this;
    }
}
