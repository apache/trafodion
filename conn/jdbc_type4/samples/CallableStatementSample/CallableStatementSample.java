// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
import common.*;

import java.sql.*;
public class CallableStatementSample
{
    public static void main(String args[]) throws Exception
    {
        try
        {
            Connection conn = sampleUtils.getPropertiesConnection();
            Statement stmt = conn.createStatement();

            try
            {
              String st = "drop procedure CallableStatementSample";
              try {
              stmt.executeUpdate(st);
              } catch (Exception e) {}

      	      st = "drop library qaspj";
	      try {
	          stmt.executeUpdate(st);
	      } catch (Exception e) {}


	      String path = System.getProperty("serverJarPath"); 
	     
	      st = "create library qaspj file '" + path + "/qaspj.jar'";
              stmt.executeUpdate(st);

              st = "create procedure CallableStatementSample(out OUT_PARAM INTEGER) EXTERNAL NAME 'IntegerSPJ.Integer_Proc(int[])' LANGUAGE JAVA PARAMETER STYLE JAVA NO ISOLATE LIBRARY QASPJ";
              stmt.executeUpdate(st);
              stmt.close();
            }
            catch (SQLException e)
            {
              e.printStackTrace();
              System.exit(0);
            }


            // get the CallableStatement object
            CallableStatement cstmt = conn.prepareCall("{call CallableStatementSample(?)}");
            //System.out.println("The Callable Statement " + cstmt);

            //register the output parameters
            cstmt.registerOutParameter(1, java.sql.Types.VARCHAR);

            //execute the procedure
            cstmt.execute();

            //invoke getInt method
            int nRetVal = cstmt.getInt(1);

            System.out.println("Out parameter is " + cstmt.getInt(1));
        }
        catch(Exception ex)
        {
            System.err.println("Unexpected Exception" + ex.getMessage());
            throw new Exception("Call to getWarnings is Failed!");
        }
    }
}
