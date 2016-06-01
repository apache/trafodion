/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.server;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileNotFoundException;

import java.util.Scanner;
import java.util.Collections;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
import java.util.Date;

import java.text.DateFormat;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.DcsConfiguration;

public class Metrics  {
	private static  final Log LOG = LogFactory.getLog(Metrics.class);
	
	public String getLoad(){               
		int mb = 1024*1024;  
		long total;
		long free;
		long max;
		long used;
    
		Runtime runtime = Runtime.getRuntime();                   
		used = (runtime.totalMemory() - runtime.freeMemory()) / mb;           
		free = runtime.freeMemory() / mb;                   
		total = runtime.totalMemory() / mb;           
		max = runtime.maxMemory() / mb;
		String report = "totalHeap=" + total + ", usedHeap=" + used + ", freeHeap=" + free + ", maxHeap=" + max;
		return report;
	}
	
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append(getLoad());
		return sb.toString();
	}
}

