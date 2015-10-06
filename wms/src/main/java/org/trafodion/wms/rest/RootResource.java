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

package org.trafodion.wms.rest;

import java.io.*;
import java.util.List;
import java.util.Iterator;

import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.QueryParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriInfo;
import javax.ws.rs.core.Response.ResponseBuilder;

import org.trafodion.wms.rest.model.WorkloadModel;
import org.trafodion.wms.rest.model.WorkloadListModel;
import org.trafodion.wms.rest.model.ServerListModel;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

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

    @Path("servers")
    public ServerListResource getServerListResource() throws IOException {
        return new ServerListResource();
    }

    @Path("version")
    public VersionResource getVersionResource() throws IOException {
        return new VersionResource();
    }

    @Path("service")
    public ServiceResource getServiceResource() throws IOException {
        return new ServiceResource();
    }
}
