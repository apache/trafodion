package org.trafodion.wms.rest;

import java.io.IOException;
import java.util.StringTokenizer;
import java.util.List;
import java.util.ArrayList;
import java.util.Collections;

import javax.ws.rs.GET;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.UriInfo;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

//import org.apache.hadoop.hbase.ClusterStatus;
//import org.apache.hadoop.hbase.HServerLoad;
//import org.apache.hadoop.hbase.ServerName;

import org.trafodion.wms.rest.model.ServerListModel;
import org.trafodion.wms.rest.model.ServerModel;

public class ServerListResource extends ResourceBase {
  private static final Log LOG =
    LogFactory.getLog(ServerListResource.class);

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
  public ServerListResource() throws IOException {
    super();
  }
 
  @GET
  @Produces({MIMETYPE_TEXT, MIMETYPE_XML, MIMETYPE_JSON})
  public Response get(final @Context UriInfo uriInfo) {
    if (LOG.isDebugEnabled()) {
      LOG.debug("GET " + uriInfo.getAbsolutePath());
    }
 
    try {
    	ServerListModel serverList = new ServerListModel();
    	serverList.add(new ServerModel("Field1","field2","field3","field4","field5"));

    	ResponseBuilder response = Response.ok(serverList);
    	response.cacheControl(cacheControl);
    	return response.build();
    } catch (IOException e) {
     	return Response.status(Response.Status.SERVICE_UNAVAILABLE)
    	.type(MIMETYPE_TEXT).entity("Unavailable" + CRLF)
    	.build();
    }
  }
}
