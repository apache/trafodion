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

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.List;
import java.util.ArrayList;
import java.util.ListIterator;

// Helper class to display help text for dbScripts commands
// This class reads the index file for filename and byte position
// of the given command in the help files and then reads only portion
// of the text from the help file that needs to be displayed to the user

public class Helper {
   final String HELP_FILES_BASE_DIR=SessionDefaults.PKG_NAME + "help/";
   final String HELP_FILE_IDX = "help_file.idx";
   final String NO_HELP_TEXT="Help not available in ";
   final String NO_HELP_FOR_CMD="No help available for this command.";
   final String HELP_KEYWORD_INDICATOR=" @@@@@";
   final String HELP_COMMENT_INDICATOR="#";
   final String FILENAME_SEPERATOR = ",";
   final String COMMON_KEYWORD_INDICATOR="common";
   final Pattern hfiPattern = Pattern.compile("\\s*(\\w*)\\s*:\\s*(\\w*)\\s*=(.*)");

   String helpFileName=null;
   long helpLocInBytes=0;

   List<String> helpFileList = new ArrayList<String>();
   Properties helpIdxProps=null;
   Helper()
   {
   }

   public void printHelpText(Writer writer,String mode,String role, String command,String section) throws IOException
   {
      boolean isSectionRead=false;
      if (section != null)
      {
         isSectionRead=true;
      }

      // Load the help index file as property if the help index property is empty
      //if (helpFileList.size() != 0)
      //{
      try
         {
            loadFileIdx(mode, role);
         }catch (FileNotFoundException fnfe)
         {
            writer.writeln();
            writer.writeln(NO_HELP_TEXT+mode.toUpperCase()+" mode.");
            return;
         }
      //}
      //If no help files associated with this mode/role
      if (helpFileList.size()==0)
      {
         writer.writeln();
         writer.writeln(NO_HELP_TEXT+mode.toUpperCase()+" mode.");
         return;
      }

      // help keys in index file are stored in lower case
      // so convert the command to lower case before checking
      // the property file.
      command=command.toLowerCase().trim();

      if (command.equals(""))
      {
         command="help";
      }

      String helpKeyValue=null;
     
      command=command.replaceAll("\\s+"," ");
      ListIterator<String> listItr = helpFileList.listIterator();
      boolean matchFound = false;
      //Look in each index file for the requested command. First in, first searched.
      while(listItr.hasNext())
      {
         Properties commProp = loadPropertyIdx((String)listItr.next());
         helpKeyValue = (null != commProp) ? commProp.getProperty(command): null;
     
         if (helpKeyValue  != null)    {   // index file is stored in the format of
            // <helpKey>=<filename>:<location>
            matchFound = true;
            helpFileName=helpKeyValue.substring(0,helpKeyValue.indexOf(":"));
            helpLocInBytes=Long.parseLong(helpKeyValue.substring((helpKeyValue.indexOf(":")+1)));
      
            InputStream is=this.getClass().getResourceAsStream(HELP_FILES_BASE_DIR+helpFileName);
            if(null == is)
            {
          	  writer.writeln();
                writer.writeln(NO_HELP_TEXT+mode.toUpperCase()+" mode.");
                return;
            }
      
            InputStreamReader isr=new InputStreamReader(is);
            BufferedReader br= new BufferedReader(isr);
            String currentLine=null;
            br.skip(helpLocInBytes);
      
            // Read all text available till the end of file
            // or till the next command in the help file
      
            while ((currentLine=br.readLine()) != null)
            {
               if (currentLine.startsWith(HELP_KEYWORD_INDICATOR))
               {
                  break;
               }
               if (isSectionRead)
               {
                  if (!currentLine.toUpperCase().startsWith(section.toUpperCase()))
                  {
                     continue;
                  }
                  isSectionRead=false;
               }
               writer.writeln(currentLine.replaceAll("\\s*$",""));
            }
            is=null;
            isr=null;
            br=null;
         }  
      }
      if (helpKeyValue  == null && !matchFound) { 
         //If no entry exists for this command
           writer.writeln();
           writer.writeln(NO_HELP_FOR_CMD);
       }
   }

   private void loadFileIdx(String mode, String role) throws FileNotFoundException,IOException
   {
	   String helpFileIndexContents = null;
	   Matcher mat = null;
	   InputStream hfIS=this.getClass().getResourceAsStream(HELP_FILES_BASE_DIR+HELP_FILE_IDX);
	   //Check for valid stream
	   if(null==hfIS)
		   throw new FileNotFoundException();
	   
	   //Load the help-file index.
	   BufferedReader input = new BufferedReader(new InputStreamReader(hfIS));
	   
	   //For each line in the index file check if the rule matches the current mode and role
	   while ((helpFileIndexContents = input.readLine()) != null) {

		   //Skip index file comments
		   if(helpFileIndexContents.startsWith(HELP_COMMENT_INDICATOR))
		   {continue;}

		   mat = hfiPattern.matcher(helpFileIndexContents);
		   //if the mode and role match the rule
		   if (mat.find() && 
				(mat.group(1).equalsIgnoreCase(mode) ||
						mat.group(1).equalsIgnoreCase(COMMON_KEYWORD_INDICATOR)) &&
				(mat.group(2).equalsIgnoreCase(role) || 
						mat.group(2).equalsIgnoreCase(COMMON_KEYWORD_INDICATOR)))
   	    	{
               //add file-names to the help-file list.
			   String[] fileNames = mat.group(3).split(FILENAME_SEPERATOR);
			   for(int i = 0; i < fileNames.length; i++ ) {
				   this.helpFileList.add(fileNames[i].trim());
			   }              
   	    	}
        }
        input.close();
        hfIS.close();

   }
   
   private Properties loadPropertyIdx(String fileName) throws FileNotFoundException, IOException
   {
	   try{
	 	   InputStream is = null;
	       //Assume the base directory and stream the indicated file
	 	   is = this.getClass().getResourceAsStream(HELP_FILES_BASE_DIR+fileName);
	 	   
	       Properties helpIdxProp = new Properties();

	       //If the file stream succeeded load the properties
	       if (is != null)
	       {
	          helpIdxProp.load(is);
	       } else
	       {
	    	  //otherwise, set properties to null. 
	     	  helpIdxProp = null;
	       }
	       is=null;
	       return helpIdxProp;
	   
	   } catch(Exception e) {
		   return null;
	   }
   }
}
