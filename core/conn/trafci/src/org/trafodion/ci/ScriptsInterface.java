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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.sql.SQLException;

public class ScriptsInterface
{
   Session sessObj=null;
   SessionInterface siObj=null;
   ConsoleReader crObj=null;
   ConsoleWriter cwObj=null;


   public ScriptsInterface()
   {

   }

   public void openConnection(String userName,
		      String password,
		      String serverName,
		      String portNumber,
		      String dsnName) throws IOException, SQLException, InstantiationException, IllegalAccessException, ClassNotFoundException
  {
	   String roleName = ""; //primary role
	   openConnection( userName, password, roleName, serverName, portNumber, dsnName);
	   
  }

   public void openConnection(String userName,
      String password,
      String roleName,
      String serverName,
      String portNumber,
      String dsnName) throws IOException, SQLException, InstantiationException, IllegalAccessException, ClassNotFoundException
   {

      //System.setOut(new PrintStream(new BufferedOutputStream(new FileOutputStream("c:\\output.dat"))));
      cwObj=new ConsoleWriter();
      cwObj.setConsoleOut(false);
      cwObj.initialize();
      crObj=new ConsoleReader();
      crObj.initialize();
      siObj=new SessionInterface(crObj,cwObj);
      if (portNumber == null || portNumber.trim().equals(""))
      {
         portNumber=SessionDefaults.portNumber;
      }
      portNumber=":"+portNumber;

	  if (portNumber.endsWith("#"))

	  {

		  portNumber = portNumber.substring(0, portNumber.length() - 1);

	  }


        try {
      sessObj=siObj.createSession(userName,
         roleName,
         password,
         serverName,
         portNumber,
         dsnName,
         0);
  } finally {
		  cwObj.setConsoleOut(true);
	  }

   }

   public String executeQuery(String query) throws IOException
   {
      cwObj.resetStream();
      cwObj.setConsoleOut(false);
      sessObj.setCaller(SessionDefaults.USERI);
      siObj.setScriptFile(null);
      siObj.setLogFile(null,false);
      if (sessObj == null)
      {
         System.out.println("session object is null");

      }

      if (sessObj.getConnObj() == null)
      {
         System.out.println("connection object is null");
      }

      if (sessObj.getConsoleWriter() == null)
      {
         System.out.println("Console writer is null");
      }
      siObj.setConsoleWriter(cwObj);
      siObj.setConsoleReader(crObj);
      siObj.setQueryOptions(true, query);
      String cmdOutput = "";
      
      try {
      siObj.invokeSession(sessObj);
      cmdOutput = cwObj.getResult();
      } finally {
    	  cwObj.setConsoleOut(true);
      }
      
      return cmdOutput;
   }

   public void executeScript(String scriptFile, String logFile) throws IOException
   {
      cwObj.setConsoleOut(true);
      siObj.setScriptFile(scriptFile);
      siObj.setLogFile(logFile,true);
      siObj.setQueryOptions(false,null);
      sessObj.setCaller(SessionDefaults.PRUNI);
      siObj.invokeSession(sessObj);
   }

   public void disconnect() throws SQLException
   {
      this.sessObj.getConnObj().close();
      this.sessObj=null;
      this.siObj=null;
   // This will support multiple connections
   //     System.exit(0); 
   }


}
