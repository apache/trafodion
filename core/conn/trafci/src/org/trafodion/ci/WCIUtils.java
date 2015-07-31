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

import java.io.FileOutputStream;
import java.io.File;
import java.io.InputStream;
import java.io.IOException;

public class WCIUtils
{

   // constants copied from wincon.h

   /**
   *  Characters read by the ReadFile or ReadConsole function
   *  are written to the active screen buffer as they are read.
   *  This mode can be used only if the ENABLE_LINE_INPUT mode
   *  is also enabled.
   */
   private static final int ENABLE_ECHO_INPUT = 4;

   private static boolean loaded = false;

   String tmpDir=System.getProperty("java.io.tmpdir");
   String userName=System.getProperty("user.name");
   String archModel=System.getProperty("sun.arch.data.model");

   public  native void cls();
   private native int getConsoleMode();
   private native void setConsoleMode(final int mode);


   public WCIUtils() throws IOException
   {
      if (!loaded)
      {
         copyDllFromURL();
         if (archModel.equalsIgnoreCase("64"))
            System.load(tmpDir + "WCIUtils64_" + userName + ".dll");
         else
        	System.load(tmpDir + "WCIUtils32_" + userName + ".dll");
         loaded = true;
      }
   }

   public synchronized void enableEcho()
   {
      setConsoleMode( getConsoleMode() | ENABLE_ECHO_INPUT );
   }

   public synchronized void disableEcho()
   {
      setConsoleMode( getConsoleMode() & ~( ENABLE_ECHO_INPUT));
   }


   private synchronized void copyDllFromURL() throws IOException
   {
	  File dest;
      if (archModel.equalsIgnoreCase("64"))
    	  dest = new File( tmpDir, "WCIUtils64_" + userName + ".dll" );
      else
    	  dest = new File( tmpDir, "WCIUtils32_" + userName + ".dll" );


      // if the dll already exists in the local pc
      // recopy the file it is not already loaded
      if (dest.exists())
      {
         try
         {
            if (!dest.delete())
               return;
         }catch (Exception e)
         {
            dest=null;
            return;
         }
      }

      FileOutputStream os = new FileOutputStream( dest );
      InputStream is;
      if (archModel.equalsIgnoreCase("64"))
    	  is=this.getClass().getResourceAsStream("WCIUtils64.dll");
      else
    	  is=this.getClass().getResourceAsStream("WCIUtils32.dll");

      byte data[] = new byte[ 4096 ];
      int ct;

      while ((ct=is.read( data )) >= 0)
         os.write( data, 0, ct );

      is.close();
      os.close();
      os=null;
      is=null;
      dest=null;

   }

}
