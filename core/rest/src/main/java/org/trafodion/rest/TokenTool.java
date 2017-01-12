package org.trafodion.rest;

import org.trafodion.rest.util.Token;

import java.io.IOException;

import org.codehaus.jettison.json.JSONObject;
import javax.ws.rs.core.Response;

import org.apache.hadoop.conf.Configuration;

public class TokenTool extends ResourceBase {

    public TokenTool() throws IOException {
        super();
    }

    public static Response getResponse(String auth) {
        try{
            Configuration conf=RESTServlet.getInstance().getConfiguration();
            if(!conf.getBoolean("rest.oauth2.enable",false))
                return null;
        }catch(Exception e){
            e.printStackTrace();
        }
        Token tk = new Token();
        String access_token = "";
        String token_type = "";
        String[] to = auth.split(" ");
        if (to.length == 2) {
            access_token = to[1].trim();
            token_type = to[0].trim();
        }

        tk.setToken(access_token);
        if (!token_type.equals("trafodion") || tk.isTimeOut()) {
            JSONObject jsonObject = new JSONObject();
            try {
                jsonObject.put(Token.error, Token.error_invalid_token);
                jsonObject.put(Token.error_description, "The access token expired");

            } catch (Exception e) {
                e.printStackTrace();

            }
            return Response.status(Response.Status.UNAUTHORIZED).type(MIMETYPE_JSON).entity(jsonObject).build();
        }

        return null;

    }

}
