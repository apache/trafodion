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
