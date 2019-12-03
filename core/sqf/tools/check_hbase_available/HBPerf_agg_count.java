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
import java.util.*;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.*;
import org.apache.hadoop.hbase.client.*;
import org.apache.hadoop.hbase.client.coprocessor.*;
import org.apache.hadoop.hbase.coprocessor.*;
import org.apache.hadoop.hbase.coprocessor.example.*;
import org.apache.hadoop.hbase.ipc.*;
import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos.*;
import org.apache.hadoop.hbase.util.*;

import org.apache.log4j.PropertyConfigurator;

public class HBPerf_agg_count{

    static void setupLog4j() {
        System.out.println("In setupLog4J");
        System.setProperty("hostName", System.getenv("HOSTNAME"));
        String confFile = System.getenv("PWD")
            + "/log4j.util.config";
        PropertyConfigurator.configure(confFile);
    }

    public static long getCount(String pv_table) throws Throwable {

	setupLog4j();

	Configuration config = HBaseConfiguration.create();

	AggregationClient aClient = new AggregationClient(config);

	Scan scan = new Scan();

	final ColumnInterpreter<Long, Long, EmptyMsg, LongMsg, LongMsg> ci =
	    new LongColumnInterpreter();

	long rowCount = aClient.rowCount(org.apache.hadoop.hbase.TableName.valueOf(pv_table),
					 ci,
					 scan);
	return rowCount;
    }

    public static void main(String[] Args) throws Throwable {

      System.out.println("In the Main\n");

      try {
	System.out.println("Row count: " + getCount("table_t1"));
      }
      catch (Exception e) {
	  System.out.println(e);
      }
   }
}
