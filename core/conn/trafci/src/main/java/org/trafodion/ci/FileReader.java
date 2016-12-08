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
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


public class FileReader
{
   private BufferedReader br=null;
   private String fileName=null;
   private String line=null;
   private boolean isSectionRead=false;
//   private String sectionName=null;
   String sectionLine=null;
   private boolean isSectionLineRead=false;
   Pattern sectionPat=null;
   Matcher sectionMat=null;

   FileReader()
   {

   }

   public void initialize(String fileName) throws IOException
   {
      this.fileName=fileName;
      java.io.FileReader fr=new java.io.FileReader(this.fileName);
      this.br = new BufferedReader(fr);
   }
   
   public void initializeStream(InputStream theStream, String clzName) throws IOException
   {
      this.fileName=clzName;
      InputStreamReader ir= new InputStreamReader(theStream);
      //dead code
//      if (null == ir)
//    	  throw new IOException("Unable to load stream from class '" + clzName + "' ");
      this.br = new BufferedReader(ir);
   }
   

   public String getLine() throws IOException
   {
      if (next(true))
      {
         return this.line;
      }
      else
      {
         return null;
      }
   }

   public String getNonBlankLine() throws IOException
   {
      if (next(false))
      {
         return this.line;
      }
      else
      {
         return null;
      }
   }

   public boolean next(boolean readBlanks) throws IOException
   {

      while (true)
      {
         // if section option is enabled , return the section line which was read already
         // before reading the remaining lines from the file
         if (this.isSectionRead && !this.isSectionLineRead)
         {
            line=this.sectionLine;
            this.sectionLine=null;
            this.isSectionLineRead=true;
            return true;
         }
         line=this.br.readLine();
         if (line == null)
         {
            return false;
         }
         else if (readBlanks)
         {
            return true;
         }
         else if (!line.trim().equals(""))
         {
            if (this.isSectionRead && line.trim().toUpperCase().startsWith("?SECTION"))
            {
               return false;
            }
            return true;
         }
      }
   }

   public String getFileName()
   {
      return this.fileName;
   }

   public void close() throws IOException
   {
      this.br.close();
      this.br=null;
   
   }

   public boolean isSectionRead()
   {
      return isSectionRead;
   }

   public void setSectionRead(boolean isSectionRead,String sectionName) throws IOException, ScriptSectionNotFound
   {
      if (isSectionRead && sectionName != null)
      {
         sectionPat=Pattern.compile("(?i)^\\s*\\?SECTION\\s+"+sectionName+"\\s*$");
         seekToSection();
//         this.sectionName=sectionName;
         this.isSectionRead = isSectionRead;

      }else
      {
//         this.sectionName=null;
         this.isSectionRead = false;
      }

   }

   private void seekToSection() throws IOException, ScriptSectionNotFound
   {
      String line=null;
      while ((line=this.getNonBlankLine()) != null)
      {
         sectionMat=sectionPat.matcher(line);
         if (sectionMat.find())
         {
            sectionLine=line;
            //section found..
            return;
         }
      }
      throw new ScriptSectionNotFound();
   }



}
