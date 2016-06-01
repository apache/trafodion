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

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;


// Creates index file for help utility
// help index file will contain the format of
// <helpKeyWord>=<helpFileName>:<byteLocation>
// helpKeyWord - the keyword for example, for obey command the key is obey
// helpFileName - Actual help file name where the help text for the given command is available
// byteLocation - the actual byte location where the help for the given command starts
// this program is called during the build time

public class HelpIndexer
{

   final static String helpIdxKeyIdentifier=" @@@@@";
   final static String HELPER_IDX_FILE="_help.idx";
   
   /*The following modes and roles used tp create the list of 
   /*potential '.help' file names.*/
   final static String[] modes={"SQL","NS","NS-WMS","COMMON"};
   final static String[] roles={"super","services", "mgr", "dba", "user","common"};
   
   HelpIndexer()
   {

   }

   public static void main(String[] args)
   {
      try
      { 
         //for each mode create an index file
         for (int i=0;i<modes.length;i++)
         {
             for (int j=0;j<roles.length;j++)
             {
                 createIndexFile(modes[i],roles[j],args[0]);
             }
         }
      } catch (IOException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
   }

   private static void createIndexFile(String mode,String role,String helpFilesDir) throws IOException
   {
      mode=mode.toLowerCase()+"_";
      role=role.toLowerCase();

      File helpfiles= new File(helpFilesDir);
      String[] hfiles=helpfiles.list();

      RandomAccessFile fr=null;
      for (int i=0;i < hfiles.length ; i++)
      {
         String filename=hfiles[i];

         //process files with the extension .help
         // files with common_ prefix defines the commands which are applicable to all modes
         // so each individual mode help files should have a reference to the mode specified commands and common commands
         if (!filename.endsWith(".help") || !filename.startsWith(mode))
         {
            continue;
         }
         if(!filename.startsWith(role, filename.indexOf("_")+1))
         {
        	 continue;
         }
         //Rename role to match file name. This allows for multiple help files for each mode/role. 
         role=filename.substring(filename.indexOf("_")+1,filename.indexOf("."));
         //Create and initialize Index File
         FileWriter fw=new FileWriter();
         fw.initialize(helpFilesDir+File.separator+mode+role+HELPER_IDX_FILE);
         
         fr=new RandomAccessFile(helpFilesDir+File.separator+filename,"r");
         String currentLine=null;
         while ((currentLine=fr.readLine()) != null)
         {
            if (currentLine.startsWith(helpIdxKeyIdentifier))
            {
               currentLine=currentLine.substring(helpIdxKeyIdentifier.length()).trim();
               //currentLine=currentLine.replaceAll(" ","\\\\ ");
               String[] commandsList=currentLine.split("&");
               for (int cnt=0; cnt < commandsList.length;cnt++)
               {
                  commandsList[cnt]=commandsList[cnt].trim().toLowerCase().replaceAll(" ","\\\\ ");
                  fw.writeln(commandsList[cnt].trim().toLowerCase()+"="+filename+":"+fr.getFilePointer());
               }
            }
            if (currentLine.replaceAll("\\s*$","").length() > 80)
            {
               System.out.println("ERROR: help text line exceeded 80 chars. FileName: "+filename+"\tText line: "+currentLine);
            }
         }

         fr.close();
         fr=null;

         if (fw != null)
         {
            fw.close();
            fw=null;
         }
      }
   }
}

