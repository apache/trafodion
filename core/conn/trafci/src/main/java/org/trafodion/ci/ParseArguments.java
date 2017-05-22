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

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.net.InetAddress;
import java.net.UnknownHostException;

public class ParseArguments
{

   String[] args=null;
   int noOfArgs=0;
   String userName=null;
   String roleName=null;
   String password=null;
   String serverName=null;
   String portNumber=null;
   String dsnName=null;
   String queryStr=null;
   String fileName=null;
   String defaultDsnName=null;
   String defaultPortNumber=null;
   boolean noConnectOption=false;  // set to true if noconnection option is specified.
   boolean isLaunchConnect=true;
   boolean autoLogin=false;
   boolean dfm=false; 
   /*-------------------------------------------------------------------------------**
   The isLaunchConnect flag is set to true when connection args are validated at
   the application launch time to validate the type of args the can be entered 
   for example -s and -sql can be entered only at launch.
   **-------------------------------------------------------------------------------*/
   ConsoleReader crObj=null;
   ConsoleWriter cwObj=null;
   static int retryCntProp=0;
   int retryCnt=1;
   String osName=null;
   final String  EXIT_PROD_STR = "Exiting " + SessionDefaults.PROD_NAME + " - Please hit <Enter> ... ";
   static
   {
      String loginRetries=null;

      //Prompt for n login retries if property set
      loginRetries=System.getProperty ("trafci.loginRetries");
      if (loginRetries!=null) 
         if ((retryCntProp = Integer.parseInt(loginRetries.trim())) <= 0)
            retryCntProp = 1;
   }

   ParseArguments()
   {

   }

   ParseArguments(ConsoleReader crObj,ConsoleWriter cwObj)
   {
      this.crObj=crObj;
      this.cwObj=cwObj;

      //Prompt for login input retries if in console/interactive mode
      if (crObj.isInteractive()) 
         this.retryCnt=3;

      /*-------------------------------------------------------------------------**
      //Prompt for n login retries if property set
      String loginRetries=System.getProperty ("trafci.loginRetries");
      if (loginRetries!=null) 
      if ((this.retryCnt = Integer.parseInt(loginRetries.trim())) <= 0)
      this.retryCnt = 1;
      **-------------------------------------------------------------------------*/
      if (retryCntProp > 0)
         retryCnt = retryCntProp;

   }

   private void setArgs(String[] args)
   {
      noOfArgs=args.length;
      this.args=args;
   }

   public void setDefaults(String dsnName,String portNumber)
   {
      this.defaultDsnName=dsnName;
      this.defaultPortNumber=portNumber;
   }

   public String[] validateArgs(String[] args,boolean isLaunchConnect) 
      throws InvalidNumberOfArguments, IOException, UserInterruption
   {

      osName = System.getProperty("os.name");
      setArgs(args);
      this.isLaunchConnect=isLaunchConnect;

      List<String> argsList= new ArrayList<String>(Arrays.asList(args));
      
      //Pre-parse args for special cases
      if (argsList.contains("-version"))
      {
         cwObj.print("JDBC Type 4 Driver Build ID : ");
         cwObj.println(JDBCVproc.getVproc());
         cwObj.print("Command Interface Build ID  : ");
         org.trafodion.ci.Vproc.main(args) ;
         cwObj.println("");
         System.exit(0);   //any other args included w/version are ignored
      }

      //Display argument help if -help argument is passed
      if (argsList.contains("-help"))
      {
    	   printUsage();
         System.exit(0);   //any other args included w/help are ignored
      }

      
      // No retries if in query mode
      if (argsList.contains("-q") || argsList.contains("-sql"))
         retryCnt = 1;

      
      if (argsList.remove("-noconnect"))
      {
          noConnectOption=true;
      }

      if (argsList.remove("-dfm"))
      {
          dfm = true;
      }
      
      /* reset args b/c of -noconnect or -dfm, dont need the if but perhaps a small perf gain? */
      if(dfm || noConnectOption)
      {
          args = null;
          args = new String[argsList.size()];
          //args = (String[])
          argsList.toArray(args);
          noOfArgs = args.length;
      }

      // the number of arguments must be an odd number if -noconnect option is specified
      // otherwise it should be an even number

      if (noOfArgs % 2 != 0 || noOfArgs > 14)
      {
         printUsage();
         throw new InvalidNumberOfArguments();
      }
      
      argsList=null;
      
      //Pre-parsing complete
      
      for (int i=0;i < noOfArgs; i++)
      {
         String option=args[i++].trim();
         String value=args[i].trim();
         if (option.equalsIgnoreCase("-u")|| option.equalsIgnoreCase("-user"))
         {
            userName=value;
         }
         else if (option.equalsIgnoreCase("-p")|| option.equalsIgnoreCase("-password"))
         {
            password=value;
         }
         else if (option.equalsIgnoreCase("-r")|| option.equalsIgnoreCase("-role"))
         {
        	 roleName=value;        	 
         }              
         else if (option.equalsIgnoreCase("-h")|| option.equalsIgnoreCase("-host"))
         {
            String[] hostAddr=value.split(":");
            if (hostAddr.length > 0)
               serverName=hostAddr[0];
            else
               serverName="";

            String portValue="";

            for (int j=1; j < hostAddr.length;j++)
            {
               portValue+=":"+hostAddr[j];
            }
            if (portValue.length() > 0)
            {
               portNumber=portValue;
            }

         }
         else if (option.equalsIgnoreCase("-j")|| option.equalsIgnoreCase("-jline")) {
             continue;
         }
         else if (isLaunchConnect && (option.equalsIgnoreCase("-q")|| option.equalsIgnoreCase("-sql")))
         {
            queryStr=value;
            if (fileName != null)
            {
               cwObj.println("-s|script and -q|sql options must not be specified together.");
               this.printUsage();
            }
         }
         else if (isLaunchConnect &&(option.equalsIgnoreCase("-s")|| option.equalsIgnoreCase("-script")))
         {
            fileName=value;
            if (queryStr != null)
            {
               cwObj.println("-s|script and -q|sql options must not be specified together.");
               this.printUsage();
            }
         }
         /* Instead of -cmd we will implement a set cmdecho <on|off> command 
          * and replace -cmd with -DFM which will execute both set lookandfeel mxci and
          * set cmdecho on
          * 
          * else if (isLaunchConnect &&(option.equalsIgnoreCase("-cmd")))
         {
            if (value.equalsIgnoreCase("Y"))
               cmdEcho = true;
         }*/
         else
         {
            cwObj.println("Unknown option " + option + " specified.");
            printUsage();
            throw new UnknownHostException();
         }

      } //end for
      
      if (!noConnectOption)
      {
         do
         {

            if (serverName == null)
            {
               String value= getRequiredArg("Host Name/IP Address: ");
               String[] hostAddr=value.split(":");
               if (hostAddr.length > 0)
                  serverName=hostAddr[0];
               else
                  serverName="";
               String portValue="";
               for (int j=1; j < hostAddr.length;j++)
               {
                  portValue+=":"+hostAddr[j];
               }
               portNumber=null;
               if (portValue.length() > 0)
               {
                  portNumber=portValue;
               }
            }


            try
            {
               if (serverName.trim().equals(""))
                  throw new UnknownHostException("Host Name is empty");
               else
                  InetAddress.getByName(serverName);
            }
            catch (UnknownHostException uh)
            {
               cwObj.println(SessionDefaults.lineSeperator+"Unknown Host: " + serverName + SessionDefaults.lineSeperator);
               serverName=null;

               //Rebuild the argument list to remove the hostname
               args = this.rebuildArgList(args);
               if ((--retryCnt) == 0)
                  throw uh;
            }

         }while (serverName==null && retryCnt > 0);
      }


      if (!noConnectOption && userName == null)
      {
         do
         {
            userName=getRequiredArg("User Name: ");
         }
         while (userName.trim().equals(""));
      }

      if (!noConnectOption && password == null)
      {
         password=getPassword("Password: ");
      }
     
      if (!noConnectOption && roleName == null) 
      {    	 
    		  roleName=getRequiredArg("Role Name [Primary Role]: "); 
    		
    		      	  
      }
      
      if (portNumber == null)
      {
         portNumber=":"+defaultPortNumber;
      }
      
      
      return args;

   }

   public String[] validateArgs(String[] args) throws InvalidNumberOfArguments, IOException, UserInterruption
   {
      return (validateArgs(args,false));
   }

   private String getRequiredArg(String prompt) throws IOException, UserInterruption
   {
      String value=null;
      String breakConnectPrompt = EXIT_PROD_STR;
      if (!this.isLaunchConnect) 
         breakConnectPrompt = "Command Interrupted - Please hit <Enter> ... "+SessionDefaults.lineSeperator;

      if (crObj.isJline()) {
          crObj.setPrompt(prompt,false,false);
      }else {
          crObj.setPrompt(breakConnectPrompt,false,false);
          cwObj.print(prompt);
      }

      //set retry count to 0 if user hits a Ctrl+C while entering login parameters
      try
      {
         value=crObj.getLine();
      } catch (UserInterruption ui)
      {
         retryCnt = 0;
         throw ui;
      }
      return value;
   }

   private void printUsage()
   {
	  String  os_specific_ci_cmd = "trafci";
	  
	  String spacedOutLine = SessionDefaults.lineSeperator;
      for (int idx = 0; idx < os_specific_ci_cmd.length(); idx++)
          spacedOutLine += " ";

      cwObj.println(SessionDefaults.lineSeperator + os_specific_ci_cmd +
              " -h[ost] <databaseserver:port> " +
              spacedOutLine +
              " -u[ser] <username> -p[assword] <password>" +
              spacedOutLine +
              " -r[ole] <rolename>" +
              spacedOutLine +
              " -s[cript] <scriptfilename> -q|-sql <\"querystring\">" +
              spacedOutLine +
              " -noconnect -version -help" );

      cwObj.println(SessionDefaults.lineSeperator+"where:");
      cwObj.println("\t-h[ost]     specifies the database server including the port number.");
      cwObj.println("\t-u[ser]     specifies a user name to log into a database server.");
      cwObj.println("\t-p[assword] specifies the user's password to log into a database server.");
      cwObj.println("\t-r[ole]     specifies the role name associated with the user.");
      cwObj.println("\t-s[cript]   specifies the command file used to customize a session.");
      cwObj.println("\t-q|-sql     specifies the command to be run in non-interactive mode.");
      cwObj.println("\t-noconnect  specifies the session login mode.");
      cwObj.println("\t-version    displays the Trafodion Command Interface and JDBC Type 4 Driver versions, then exits.");
      cwObj.println("\t-help       displays a list of accepted arguments with descriptions, then exits.");     
      cwObj.println("");

   }

   /*------------------------------------------------------------------**
   **    private String getPassword (String prompt) throws IOException { **
   **          MaskingThread et = new MaskingThread(prompt);             **
   **          Thread mask = new Thread(et);                             **
   **          mask.start();                                             **
   **                                                                  **
   **         String password = "";                                      **
   **                                                                  **
   **          password = crObj.getLine();                               **
   **          // stop masking                                           **
   **          et.stopMasking();                                         **
   **          // return the password entered by the user                **
   **          return password;                                          **
   **       }                                                            **
   **------------------------------------------------------------------*/
   
   public String getPassword (String prompt) throws IOException, UserInterruption
   {
      String password = "";
      WCIUtils wcs=null;
      boolean isWindows = System.getProperty("os.name").toUpperCase().startsWith("WINDOW");

      MaskingThread et = new MaskingThread(prompt);
      Thread mask = new Thread(et);
      mask.start();

      // if it is windows envrionment use the dll to disable echo
      if (isWindows)
      {
    	  try { 
	         wcs=new WCIUtils();
	         wcs.disableEcho();
         
    	  } catch(Throwable e) { }
    	  
      }
      crObj.setMaskingThread(et,wcs);

      String breakConnectPrompt = EXIT_PROD_STR;
      if (!this.isLaunchConnect) 
         breakConnectPrompt = "Command Interrupted - Please hit <Enter> ... "+SessionDefaults.lineSeperator;

      if (! crObj.isJline()) {
          crObj.setPrompt(breakConnectPrompt,false,false);
      } else {
          crObj.setPrompt("",false,false);
      }


      try
      {
         password = crObj.getLine();
      }
      catch (Throwable t)
      {
         UserInterruption  uie = null;
         if (t instanceof UserInterruption)
         {
            uie =(UserInterruption) t;
            retryCnt = 0;
         }
         else
            uie = new UserInterruption();


         throw uie;
      }
      finally
      {

         // stop masking
         et.stopMasking();

         if (isWindows)
         {
        	 try {
	            wcs.enableEcho();
	            
        	 } catch(Exception e) { }
        	 
            wcs=null;
         }
         cwObj.print(SessionDefaults.lineSeperator);
      }
      // return the password entered by the user
      return password;
   }

    public String getInputMask (String prompt, boolean isInputMask) throws IOException, UserInterruption
   {
      String password = "";
      WCIUtils wcs=null;
      boolean isWindows = System.getProperty("os.name").toUpperCase().startsWith("WINDOW");

      MaskingThread et = new MaskingThread(prompt);
      Thread mask = new Thread(et);
      mask.start();

      // if it is windows envrionment use the dll to disable echo
      if (isWindows)
      {
    	  try {
	         wcs=new WCIUtils();
	         wcs.disableEcho();
	         
    	  } catch(Throwable e) { }
      }
      crObj.setMaskingThread(et,wcs);

      String breakConnectPrompt = EXIT_PROD_STR;
      if (!this.isLaunchConnect || isInputMask ) 
         breakConnectPrompt = "Command Interrupted - Please hit <Enter> ... "+SessionDefaults.lineSeperator;

      crObj.setPrompt(breakConnectPrompt,false,false);
      
      try
      {
         password = crObj.getLine();
      }
      catch (Throwable t)
      {
         UserInterruption  uie = null;
         if (t instanceof UserInterruption)
         {
            uie =(UserInterruption) t;
            retryCnt = 0;
         }
         else
            uie = new UserInterruption();


         throw uie;
      }
      finally
      {

         // stop masking
         et.stopMasking();

         if (isWindows)
         {
            try {
            	wcs.enableEcho();
            	
            } catch(Exception e) { }
            
            wcs=null;
         }
         cwObj.print(SessionDefaults.lineSeperator);
      }
      // return the password entered by the user
     
      return password;
   }


   public String getInputNomask (String prompt, boolean isNotInputMask) throws IOException, UserInterruption
   {
      String userInput = "";

      String breakConnectPrompt = EXIT_PROD_STR;
      if (isNotInputMask) 
         breakConnectPrompt = "Command Interrupted - Please hit <Enter> ... ";

      crObj.setPrompt(breakConnectPrompt,false,false);
     
      try
      {
         System.out.print( "                                 \r" + prompt  );
         userInput = crObj.getLine();
      }
      catch (Throwable t)
      {
         UserInterruption  uie = null;
         if (t instanceof UserInterruption)
         {
            uie =(UserInterruption) t;
            retryCnt = 0;
         }
         else
            uie = new UserInterruption();


         throw uie;
      }
      finally
      {
         cwObj.print(SessionDefaults.lineSeperator);
      }
      // return characters entered by the user
      return userInput;
   }


   public String[] rebuildArgList(String[] oldArgs)
   {
      int i;
      List<String> argsList= new ArrayList<String> (Arrays.asList (oldArgs));

      if (argsList.isEmpty())
         return oldArgs;

      String argKey1 = null;
      String argKey2 = null;
      String argKey3 = null;
      String argKey4 = null;
      String argKey5 = null;
      String argKey6 = null;

      if (this.userName == null || this.password == null || this.roleName ==null)
      {
         argKey1 = "-u";
         argKey2 = "-p";
         argKey3 = "-user";
         argKey4 = "-password";
         argKey5 = "-r";
         argKey6 = "-role";
      }
      else if (this.dsnName == null)
      {
          argKey1 = "-d";
          argKey3 = "-dsn";
      }
      else if (this.serverName == null)
      {
         argKey1 = "-h";
         argKey3 = "-host";
      }
      else
         return oldArgs;

      do
      {
         if (((i = argsList.indexOf(argKey1)) >= 0) || 
        	 ((i = argsList.indexOf(argKey3)) >= 0) ||
        	 ((i = argsList.indexOf(argKey5)) >= 0) )
         {
            argsList.remove(i);
            argsList.remove(i);
         }
      }
      while (i > -1);

      do
      {
         if ((argKey2 != null) && (((i = argsList.indexOf(argKey2)) >= 0 ) || 
        		                   ((i = argsList.indexOf(argKey4)) >= 0 )) ||
        		                   ((i = argsList.indexOf(argKey6)) >= 0 ))
         {
            argsList.remove(i);
            argsList.remove(i);
         }
      }
      while (i > -1);

      String[] newArgs=new String[argsList.size ()];
      argsList.toArray (newArgs);
      return newArgs;

   }
   
   

   public boolean isAutoLogin()
   {
      return autoLogin;
   }

   public void setAutoLogin(boolean autoLogin)
   {
      this.autoLogin = autoLogin;
   }

   
   
}
