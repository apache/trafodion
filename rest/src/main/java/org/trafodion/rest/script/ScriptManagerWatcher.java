/**
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.trafodion.rest.script;

import java.io.*; 
import java.util.*; 
import org.apache.log4j.Logger;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.commons.io.monitor.FileAlterationListener;
import org.apache.commons.io.monitor.FileAlterationListenerAdaptor;
import org.apache.commons.io.monitor.FileAlterationMonitor;
import org.apache.commons.io.monitor.FileAlterationObserver;

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
		final long pollingInterval = 5 * 1000;// 5 seconds
		File folder = new File(dir);

		if (!folder.exists()) {
			throw new RuntimeException("Directory not found: " + dir);
		}

		try {
			FileAlterationObserver observer = new FileAlterationObserver(folder);
			FileAlterationMonitor monitor =
				new FileAlterationMonitor(pollingInterval);
			FileAlterationListener listener = new FileAlterationListenerAdaptor() {
				// Is triggered when a file is changed in the monitored folder
				@Override
				public void onFileChange(File file) {
					try {
						LOG.info("File changed: " + file.getCanonicalPath());
						ScriptManager.getInstance().removeScript(file.getName());
					} catch (IOException e) {
						e.printStackTrace(System.err);
					}
				}
			};

			observer.addListener(listener);
			monitor.addObserver(observer);
			monitor.start();
		} catch (Exception e) {
			e.printStackTrace();
			LOG.error(e.getMessage());
		}
	}
}