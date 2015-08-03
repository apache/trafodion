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
import java.util.HashMap;
import java.util.List;

public class Reader
{
   private ConsoleReader cReader=null;
   private FileReader sReader=null;
   private FileReader oReader=null;
   private List<FileReader> nestedObeyReaders=null;

//   private boolean obeyFileList = false;
   private int readMode=SessionDefaults.CONSOLE_READ_MODE;
   private int prevReadMode=SessionDefaults.CONSOLE_READ_MODE;
   private String tmpString=null;
   private FileReader tmpReader=null;
   
   private HashMap<String, String> obeyHashMap = null;
   private int MAX_OBEY_DEPTH;
   
   Reader()
   {
       int defaultDepth = 10;
       
       MAX_OBEY_DEPTH = defaultDepth;
       try
       {
           if(System.getProperty("trafci.obeydepth") != null)
               MAX_OBEY_DEPTH = Integer.parseInt(System.getProperty("trafci.obeydepth"));
           
           if(MAX_OBEY_DEPTH < 0)
               MAX_OBEY_DEPTH = defaultDepth;
       }
       catch(Exception ex)
       {
               MAX_OBEY_DEPTH = defaultDepth;
       }
   } 
   
   public String getNonBlankLine() throws IOException, UserInterruption
   {
      
      switch (readMode)
      {
         case SessionDefaults.CONSOLE_READ_MODE:
            return cReader.getLine();
         case SessionDefaults.SCRIPT_READ_MODE:
            return sReader.getNonBlankLine();
         case SessionDefaults.OBEY_READ_MODE:
             
             if(isObeyDepthMax(oReader.getFileName()))
             {
                 obeyHashMap = null;
                 throw new IOException("\n\nERROR: " + SessionError.OBEY_DEPTH_MAX + oReader.getFileName());
             }

            tmpString=oReader.getNonBlankLine();
            if (tmpString == null && ((tmpReader=getLastObeyReader()) != null))
            {
               obeyHashMap.remove(oReader.getFileName());
               decrementHashMap(tmpReader.getFileName());
               oReader.close();
               
               oReader=tmpReader;
               return getNonBlankLine();
            }
            else
                return tmpString;
      }
      return null;
   }

   public String getLine() throws IOException, UserInterruption
   {

      switch (readMode)
      {
         case SessionDefaults.CONSOLE_READ_MODE:
            return cReader.getLine();
         case SessionDefaults.SCRIPT_READ_MODE:
            return sReader.getLine();
         case SessionDefaults.OBEY_READ_MODE:
            tmpString=oReader.getLine();
            if (tmpString == null && ((tmpReader=getLastObeyReader()) != null))
            {
               oReader=tmpReader;
               getLine();
            }
            return tmpString;
      }
      return null;
   }

   public ConsoleReader getConsoleReader()
   {
      return cReader;
   }

   public void setConsoleReader(ConsoleReader reader)
   {
      cReader = reader;
   }

   public FileReader getObeyReader()
   {
      return oReader;
   }
   
   public List<FileReader> getObeyReaderList()
   {
       return this.nestedObeyReaders;
   }

   public void setObeyReader(FileReader reader)
   {
      // if the current mode is already obey read mode..then push the current object to the nested buffer
      // and set the current reader

      if (reader != null && readMode == SessionDefaults.OBEY_READ_MODE)
      {
         this.addObeyReader(oReader);
         //this.obeyFileList = true;
      }
      else
      {
         //this.obeyFileList = false;
         this.nestedObeyReaders = null;
      }
      
      oReader = reader;
   }

   public FileReader getScriptReader()
   {
      return sReader;
   }

   public void setScriptReader(FileReader reader)
   {
      sReader = reader;
   }

   public int getReadMode()
   {
      return readMode;
   }

   public int getPrevReadMode()
   {
      return this.prevReadMode;
   }

   public void setReadMode(int readMode)
   {
      this.prevReadMode=this.readMode;
      this.readMode = readMode;
   }

   private FileReader getLastObeyReader()
   {
      if (this.nestedObeyReaders == null)
      {
         return null;
      }
      FileReader fr=(FileReader)this.nestedObeyReaders.get(this.nestedObeyReaders.size()-1);
      this.nestedObeyReaders.remove(this.nestedObeyReaders.size()-1);
      if (this.nestedObeyReaders.size() == 0)
      {
         this.nestedObeyReaders=null;
      }
      return fr;
   }

   private void addObeyReader(FileReader obeyReader)
   {
      if (this.nestedObeyReaders == null)
      {
         this.nestedObeyReaders = new ArrayList<FileReader>();
      }

      if(this.obeyHashMap == null)
      {
          this.obeyHashMap = new HashMap<String, String>();
      }
      
      incrementHashMap(obeyReader);
   }
   
   public boolean obeyMultipleFiles(){
     /* if (this.nestedObeyReaders == null){
			     return false; 
      } 
     */
	   
      if (oReader.getFileName().equals(""))
	 	   return false;
	  else
		   return true;
   }
   
   private boolean isObeyDepthMax(String path)
   {
       if(obeyHashMap != null)
       {
           String tmpHashObject = obeyHashMap.get(path);
           
           if(tmpHashObject != null)
           {
               int obeyFileCount = Integer.parseInt(tmpHashObject); 
               if(obeyFileCount == MAX_OBEY_DEPTH)
               {
                   return true;
               }
           }
       }
       return false;
   }
   
   private void incrementHashMap(FileReader obeyReader){
       String path = obeyReader.getFileName();
       int obeyFileCount = 1;
       
       if(obeyHashMap.get(path) == null)
       {
           obeyHashMap.put(path, obeyFileCount + "");
       }
       else
       {
           obeyFileCount = Integer.parseInt(obeyHashMap.get(path));
           obeyFileCount++;
           obeyHashMap.put(path, obeyFileCount + "");  
       }
       
       if(obeyFileCount <= MAX_OBEY_DEPTH)
           this.nestedObeyReaders.add(obeyReader);
   }

   private void decrementHashMap(String path){
       if(obeyHashMap.get(path) != null){
           int obeyFileCount = Integer.parseInt(obeyHashMap.get(path));
           obeyFileCount--;
           obeyHashMap.put(path, obeyFileCount + "");   
       }
   }
   
   public boolean isReallyObeyfile()
   {
	   // Try an determine if this is really an obey file or
	   // a stream of multiple commands constructed to run .sec
	   
	   // No obey files are being processed right now
	   if (oReader == null || readMode != SessionDefaults.OBEY_READ_MODE)
		   return false;
	   //Only one obey file see if its for .sec
	   if (this.nestedObeyReaders == null){
		   if ( oReader.getFileName()== "")
			     return false; 
	   }
   	   // One actual obey file or nested obey files
	   return true;
   }

}
