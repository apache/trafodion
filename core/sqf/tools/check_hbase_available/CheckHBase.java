/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;

import org.apache.log4j.PropertyConfigurator;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.*;
import org.apache.hadoop.hbase.client.*;

public class CheckHBase {

    static void setupLog4j() {
       System.out.println("In setupLog4J");
       System.setProperty("hostName", System.getenv("HOSTNAME"));
       String confFile = System.getenv("PWD")
            + "/log4j.util.config";
        PropertyConfigurator.configure(confFile);
    }

    public static void main(String[] args) {
	
	System.out.println("MAIN ENTRY");      
	if (CheckHBase.isHBaseAvailable()) {
	    System.out.println("HBase is available");
	}
	else {
	    System.out.println("HBase is not available");
	}
    }
    
    public CheckHBase () {
	System.out.println("In ctor");
    }

    static public boolean isHBaseAvailable() {

	setupLog4j();

	Configuration lv_config = HBaseConfiguration.create();
	//	lv_config.set("hbase.client.retries.number", "3");
	System.out.println("Checking if HBase is available...");
	try {
	    HBaseAdmin.checkHBaseAvailable(lv_config);
	}
	catch (Exception e) {
	    System.out.println("Caught an exception in HBaseAdmin.checkHBaseAvailable: " + e);
	    return false;
	}
	return true;
    }
    
}
