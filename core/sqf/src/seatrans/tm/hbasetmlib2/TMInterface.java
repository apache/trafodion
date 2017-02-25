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

package org.trafodion.dtm;

import java.util.Collections;
import java.util.Set;
import java.util.HashSet;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;

import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;

public class TM_Interface {

   static final Log LOG = LogFactory.getLog(HBaseTxClient.class);

   static {
       System.loadLibrary("stmlib");
  }

   private native void registerRegion(int port, byte[] hostname, long startcode, byte[] regionInfo);
   private native 
   
   public static void main(String[] args) {
     System.out.println("MAIN ENTRY");      
   }
   
   void setupLog4j() {
       //System.out.println("In setupLog4J");
        String confFile = System.getenv("TRAF_HOME")
            + "/logs/log4j.dtm.config";
        PropertyConfigurator.configure(confFile);