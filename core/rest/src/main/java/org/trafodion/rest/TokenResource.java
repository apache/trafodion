package org.trafodion.rest;

import java.io.IOException;


import javax.ws.rs.POST;
import javax.ws.rs.Produces;
import javax.ws.rs.FormParam;
import javax.ws.rs.core.CacheControl;

import javax.ws.rs.core.Response;

import javax.ws.rs.core.Response.ResponseBuilder;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.codehaus.jettison.json.JSONObject;


import org.trafodion.rest.util.TrafLdap;
import org.trafodion.rest.util.Token;



public class TokenResource extends ResourceBase {
    private static final Log LOG = LogFactory.getLog(TokenResource.class);

      static CacheControl cacheControl;
      static {
        cacheControl = new CacheControl();
        cacheControl.setNoCache(true);
        cacheControl.setNoStore(true);
        cacheControl.setNoTransform(false);
      }

      /**
       * Constructor
       * @throws IOException
       */
      public TokenResource() throws IOException {
        super();
      }

      /**
       * Build a response for a version request.
       * @param context servlet context
       * @param uriInfo (JAX-RS context variable) request URL
       * @return a response for a version request 
       */

      @POST
      @Produces({MIMETYPE_JSON})
      public Response get(final @FormParam("grant_type") String grant_type,
              final @FormParam("username") String username,
              final @FormParam("password") String password
              ) {
            try {
                if (LOG.isDebugEnabled()) {
                    LOG.debug("POST Token: grant_type=" +grant_type+","
                            + "username="+username+",password="+password+"\n");

                }

                if(!grant_type.equals("password")){
                    JSONObject jsonObject = new JSONObject();
                    try {
                        jsonObject.put(Token.error, Token.error_unsupported_grant_type);
                        jsonObject.put(Token.error_description, "only support Resource Owner Password Credentials Grant");

                    } catch (Exception e) {
                        e.printStackTrace();
                        throw new IOException(e);
                    }
                    return Response.status(Response.Status.BAD_REQUEST)
                            .type(MIMETYPE_JSON)
                            .entity(jsonObject)
                            .build();
                }
               TrafLdap tl=new TrafLdap();
               if(tl.validateUser(username, password)){
                   Token tk=new Token();
                   JSONObject jsonObject = new JSONObject();
                   try {
                        jsonObject.put(Token.access_token,tk.getToken());
                        jsonObject.put(Token.token_type,"trafodion");
                        jsonObject.put(Token.expires_in,Long.parseLong(tk.getInterval())/1000);
                    } catch (Exception e) {
                        e.printStackTrace();
                        throw new IOException(e);
                    }
                   ResponseBuilder response = Response.ok(jsonObject);
                    response.cacheControl(cacheControl);
                    return response.build();
               }else{
                   JSONObject jsonObject = new JSONObject();
                    try {
                        jsonObject.put(Token.error, Token.error_invalid_grant);
                        jsonObject.put(Token.error_description, "Ldap validateUser failed");

                    } catch (Exception e) {
                        e.printStackTrace();
                        throw new IOException(e);
                    }
                    return Response.status(Response.Status.BAD_REQUEST)
                            .type(MIMETYPE_JSON)
                            .entity(jsonObject)
                            .build();
               }



            } catch (IOException e) {
                e.printStackTrace();
                JSONObject jsonObject = new JSONObject();
                try{
                    jsonObject.put("info", "Unavailable");
                }catch(Exception ee){

                }
                return Response.status(Response.Status.SERVICE_UNAVAILABLE)
                        .type(MIMETYPE_JSON).entity(jsonObject)
                        .build();
            }

      }
}
