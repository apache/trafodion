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
package org.trafodion.wms.script;

import java.io.*; 
import java.util.*; 
import org.apache.log4j.Logger;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ScriptManagerWatcher implements Runnable {
	Thread thrd;
	private static final Log LOG = LogFactory.getLog(ScriptManagerWatcher.class.getName());
	private String dir; 
	
	ScriptManagerWatcher(String name,String dir){
		this.dir = dir;
		thrd = new Thread(this, name);
		thrd.start();
	}
	
	public void run() {
		LOG.info(" Watching directory \"" + dir + "\"");
		File directory = new File(dir);
		long currentLastModified = directory.lastModified();
		while (true) {
			if (directory.lastModified() > currentLastModified) {
				currentLastModified = directory.lastModified();
				File[] files = directory.listFiles();
				for (int i=0; i < files.length; i++) {
					if(files[i].getName().endsWith(".swp"))//Ignore swap files
						continue;
					Long lastModified = files[i].lastModified();
					Date date = new Date(lastModified);
					LOG.info("File \"" + files[i].getName() + "\" last modified " + date);
					ScriptManager.getInstance().removeScript(files[i].getName());
				}
			} else {
				LOG.debug(" Directory \"" + dir + "\" unchanged");
			}
			
			try {
				Thread.sleep(2000);
			} catch (InterruptedException e) {
				LOG.debug("Interrupted!");
			}
		}
	}
}