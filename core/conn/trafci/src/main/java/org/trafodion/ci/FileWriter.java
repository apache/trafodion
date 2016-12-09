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

import java.io.BufferedWriter;
import java.io.IOException;

public class FileWriter
{
   private BufferedWriter bw=null;
   private boolean append=false;
   private java.io.FileWriter fw=null;
   FileWriter()
   {

   }

   public void setAppend(boolean append)
   {
      this.append=append;
   }

   public void initialize(String fileName) throws IOException
   {
      fw=new java.io.FileWriter(fileName,append);
      bw=new BufferedWriter(fw);
   }

   public void write(String line) throws IOException
   {
      if (line != null)
      {
         if (SessionDefaults.lineSeperator.equals(line))
         {
            writeln();
         }
         else
         {
            bw.write(line);
         }
         bw.flush();
      }
   }

   public void writeln() throws IOException
   {
      bw.newLine();
   }

   public void writeln(String line) throws IOException
   {
      write(line);
      writeln();
   }

   public void close() throws IOException
   {
      bw.close();
      bw=null;
      fw=null;
   }
}
