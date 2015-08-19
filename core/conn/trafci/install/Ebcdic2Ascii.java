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
 
 /*  
 
    This object will only be executed on z/OS systems.
    This object converts all EBCDIC files to Ascii files in trafci/* directory if file.encoding is Cp1047
    
 */
 
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.Reader;
import java.io.Writer;

public class Ebcdic2Ascii {
 
   void listContents(File dir) 
   {
      String [] files = dir.list();
      for (int i = 0; i < files.length; i++) 
      {
         File fl = new File(dir, files[i]);
         if (fl.isDirectory()) 
            listContents(fl);
         else
         {
            if (!files[i].endsWith(".jar")) {
               String nFile = dir.getAbsolutePath()+"/"+files[i];
               writeOutput(readInput(nFile), nFile);
            }
         }
     }
   }
   
   String readInput(String fileName)
   {
      StringBuffer buffer = new StringBuffer();
      try 
      {
         FileInputStream fis = new FileInputStream(fileName);
         InputStreamReader isr = new InputStreamReader(fis, "8859_1") ;
         Reader in = new BufferedReader(isr);
         int ch;
         while ((ch = in.read()) > -1)
         {
            buffer.append((char)ch);
         }
         in.close();
         return buffer.toString();
       } catch (IOException e) {
          e.printStackTrace();
          return null;
       }
   }
   
   void writeOutput(String str, String fileName)
   {
      try 
      {
         FileOutputStream fos = new FileOutputStream(fileName);
         Writer out = new OutputStreamWriter(fos);
         out.write(str);
         out.close();
      } catch (IOException e) {
         e.printStackTrace();
      }
   }
}
