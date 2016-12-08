// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
package org.trafodion.ci;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.sql.SQLException;

public class PythonInterface
{
   ConsoleReader crObj=null;
   ConsoleWriter cwObj=null;
   SessionInterface sessIntObj=null;
   Session sessObj=null;
   boolean doTrace=false;

   public PythonInterface()
   {
      initialize();
      sessIntObj=new SessionInterface(crObj,cwObj);

      String enableTrace=System.getProperty ("trafci.enableTrace");
      if ((enableTrace != null) && enableTrace.trim().equalsIgnoreCase("true"))
         doTrace = true;

   }

   private void initialize()
   {
      cwObj=new ConsoleWriter();

      try
      {
         cwObj.initialize();
      } catch (IOException e1)
      {
         System.out.println("Could not able to initialize the writer"+e1);
         if (doTrace)
            e1.printStackTrace();
         System.exit(1);
      }
      crObj=new ConsoleReader();

      try
      {
         crObj.initialize();
      } catch (IOException e1)
      {
         System.out.println("Could not able to initialize the reader"+e1);
         if (doTrace)
            e1.printStackTrace();
         System.exit(1);
      }
   }

   public void openConnection(String userName, String password, String serverName, String portNumber, String dsnName)
   {
	    String roleName = ""; //primary role
	    openConnection(userName, password, roleName, serverName, portNumber, dsnName );   
   }
   
   public void openConnection(String userName, String password,String roleName, String serverName, String portNumber, String dsnName)
   {
      try
      {
         sessObj=sessIntObj.createSession(userName,
            password,
            roleName,
            serverName,
            portNumber,
            dsnName,
            SessionDefaults.USERI);
      } catch (FileNotFoundException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      } catch (SQLException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      } catch (InstantiationException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      } catch (IllegalAccessException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      } catch (ClassNotFoundException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      } catch (IOException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
   }

   public void executeUI()
   {
      try
      {
         sessIntObj.setQueryOptions(false,null);
         sessIntObj.invokeSession(sessObj);
      } catch (IOException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
   }

   public void executeQuery(String qryString)
   {
      try
      {
         sessIntObj.setQueryOptions(true,qryString);
         sessIntObj.invokeSession(sessObj);
      } catch (IOException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
   }

   public void executeFile(String fileName)
   {
      try
      {
         sessIntObj.setQueryOptions(false,null);
         sessIntObj.setScriptFile(fileName);
         //sessIntObj.setCaller(SessionDefaults.PYTHI);
         sessIntObj.invokeSession(sessObj);

      } catch (IOException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
   }

   public void closeConnection()
   {
      if (this.sessIntObj != null)
      {
         if (sessObj.getConnObj() != null)
         {
            try
            {
               sessObj.getConnObj().close();
            } catch (SQLException e)
            {
               // TODO Auto-generated catch block
               e.printStackTrace();
            }
            sessObj=null;
         }
      }
      sessIntObj=null;
   }
   
}
