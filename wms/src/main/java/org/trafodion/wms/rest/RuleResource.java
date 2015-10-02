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
**/

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

import org.trafodion.wms.rest.model.RuleListModel;
import org.trafodion.wms.rest.model.RuleModel;

public class RuleResource extends ResourceBase {
    private static final Log LOG = LogFactory.getLog(RuleResource.class);

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
    public RuleResource() throws IOException {
        super();
    }

    @GET
    @Produces({ MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON })
    public Response get(final @Context UriInfo uriInfo) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("GET " + uriInfo.getAbsolutePath() + " "
                    + uriInfo.getQueryParameters());
        }
        /*
         * RuleListModel ruleListModel = new RuleListModel(); RuleResponse
         * ruleResponse = new RuleResponse();
         * 
         * try { servlet.getAdmin().open(); ruleResponse =
         * servlet.getAdmin().rule(); servlet.getAdmin().close(); } catch
         * (IOException e) { return
         * Response.status(Response.Status.SERVICE_UNAVAILABLE)
         * .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF).build(); }
         * 
         * // To test: // curl -v -X GET -H "Accept: application/json" //
         * http://sqws123.houston.hp.com:50030/rule // // Should see something
         * like this: //
         * {"rules":[{"comment":"Added by Matt","name":"trafodion",
         * "text":"select * from table T1","timestamp":1398215824738}]} // /* if
         * (ruleResponse.getRuleList() != null) { for (Rule aRule :
         * ruleResponse.getRuleList()) { ruleListModel.add(new
         * RuleModel(aRule.getName(), aRule .getValue(), aRule.getComment(),
         * aRule.getTimestamp())); } }
         */
        ResponseBuilder response = null;
        /*
         * if (ruleResponse.getRuleList() == null) { response =
         * Response.ok("[]"); } else { response = Response.ok(ruleListModel); }
         */
        response.cacheControl(cacheControl);
        return response.build();

    }

    @PUT
    @Consumes({ MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON })
    public Response put(final RuleModel model, final @Context UriInfo uriInfo) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("PUT " + uriInfo.getAbsolutePath() + " "
                    + uriInfo.getQueryParameters());
        }

        if (servlet.isReadOnly()) {
            return Response.status(Response.Status.FORBIDDEN)
                    .type(MIMETYPE_TEXT).entity("Forbidden" + CRLF).build();
        }
        // To test:

        // Vertica
        // curl -v -X PUT -H "Accept: application/json" -H
        // "Content-type: application/json" -d '{"name":"vertica","text":"from
        // vertica [ operation == 102 and duration >= 1000 ] insert into action
        // \u0027CANCEL\u0027 as action","comment":"Added by Administrator"}'
        // http://sqws123.houston.hp.com:50030/rule

        // Trafodion
        // curl -v -X PUT -H "Accept: application/json" -H
        // "Content-type: application/json" -d '{"name":"trafodion","text":"from
        // trafodion [ operation == 100 and beginTimestamp >= 123456 ] insert
        // into action \u0027REJECT\u0027 as action","comment":"Added by
        // Administrator"}' http://sqws123.houston.hp.com:50030/rule
        /*
         * Rule rule = new Rule(model.getName(), model.getText(),
         * model.getComment(), System.currentTimeMillis()); try {
         * servlet.getAdmin().open(); servlet.getAdmin().alterRule(rule);
         * servlet.getAdmin().close(); } catch (IOException e) { return
         * Response.status(Response.Status.SERVICE_UNAVAILABLE)
         * .type(MIMETYPE_TEXT) .entity("Unavailable " + e.getMessage() +
         * CRLF).build(); }
         */
        ResponseBuilder response = Response.ok();
        return response.build();
    }

    @POST
    @Consumes({ MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON })
    public Response post(final RuleModel model, final @Context UriInfo uriInfo) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("POST " + uriInfo.getAbsolutePath() + " "
                    + uriInfo.getQueryParameters());
        }

        if (servlet.isReadOnly()) {
            return Response.status(Response.Status.FORBIDDEN)
                    .type(MIMETYPE_TEXT).entity("Forbidden" + CRLF).build();
        }

        // To test:

        // Vertica
        // curl -v -X POST -H "Accept: application/json" -H
        // "Content-type: application/json" -d '{"name":"vertica","text":"from
        // vertica [ operation == 102 and duration >= 1000 ] insert into action
        // \u0027CANCEL\u0027 as action","comment":"Added by Administrator"}'
        // http://sqws123.houston.hp.com:50030/rule

        // Trafodion
        // curl -v -X POST -H "Accept: application/json" -H
        // "Content-type: application/json" -d '{"name":"trafodion","text":"from
        // trafodion [ operation == 100 and beginTimestamp >= 123456 ] insert
        // into action \u0027REJECT\u0027 as action","comment":"Added by
        // Administrator"}' http://sqws123.houston.hp.com:50030/rule
        /*
         * Rule rule = new Rule(model.getName(), model.getText(),
         * model.getComment(), System.currentTimeMillis()); try {
         * servlet.getAdmin().open(); servlet.getAdmin().addRule(rule);
         * servlet.getAdmin().close(); } catch (IOException e) { return
         * Response.status(Response.Status.SERVICE_UNAVAILABLE)
         * .type(MIMETYPE_TEXT) .entity("Unavailable " + e.getMessage() +
         * CRLF).build(); }
         */
        ResponseBuilder response = Response.ok();
        return response.build();
    }

    @DELETE
    public Response delete(final RuleModel model, final @Context UriInfo uriInfo) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("DELETE " + uriInfo.getAbsolutePath() + " "
                    + uriInfo.getQueryParameters());
        }

        if (servlet.isReadOnly()) {
            return Response.status(Response.Status.FORBIDDEN)
                    .type(MIMETYPE_TEXT).entity("Forbidden" + CRLF).build();
        }

        // To test:
        // curl -v -X DELETE -H "Accept: application/json" -H
        // "Content-type: application/json" -d '{"name":"trafodion"}'
        // http://sqws123.houston.hp.com:50030/rule
        /*
         * LOG.debug("Delete rule " + model.toString()); Rule rule = new
         * Rule(model.getName(), model.getText(), model.getComment(),
         * System.currentTimeMillis()); try { servlet.getAdmin().open();
         * servlet.getAdmin().deleteRule(rule); servlet.getAdmin().close(); }
         * catch (IOException e) { return
         * Response.status(Response.Status.SERVICE_UNAVAILABLE)
         * .type(MIMETYPE_TEXT).entity("Unavailable" + CRLF).build(); }
         */
        return Response.ok().build();
    }
}
