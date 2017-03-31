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

/* -*-java-*-
 * Filename	: SQLMXConnectionPoolDataSourceFactory.java
 * Description :
 */

package org.apache.trafodion.jdbc.t2;

import java.util.Enumeration;
import java.util.Hashtable;
import javax.naming.Context;
import javax.naming.Name;
import java.util.Properties;
import javax.naming.Reference;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import javax.naming.RefAddr;

public class SQLMXConnectionPoolDataSourceFactory implements
		javax.naming.spi.ObjectFactory {
	public SQLMXConnectionPoolDataSourceFactory() {
		if (JdbcDebugCfg.entryActive) {
			debug[methodId_SQLMXConnectionPoolDataSourceFactory]
					.methodEntry(JdbcDebug.debugLevelPooling);
			debug[methodId_SQLMXConnectionPoolDataSourceFactory].methodExit();
		}
	}

	public Object getObjectInstance(Object refobj, Name name, Context nameCtx,
			Hashtable env) throws Exception {
		if (JdbcDebugCfg.entryActive)
			debug[methodId_getObjectInstance]
					.methodEntry(JdbcDebug.debugLevelPooling);
		if (JdbcDebugCfg.traceActive)
			debug[methodId_getObjectInstance].methodParameters("refobj="
					+ JdbcDebug.debugObjectStr(refobj) + ", name="
					+ JdbcDebug.debugObjectStr(name) + ", nameCtx="
					+ JdbcDebug.debugObjectStr(nameCtx) + ", env="
					+ JdbcDebug.debugObjectStr(env));
		try {
			Reference ref = (Reference) refobj;
			SQLMXConnectionPoolDataSource ds = null;
			RefAddr refAddr;
			String tmp;

			if (ref.getClassName().equals(
					"org.apache.trafodion.jdbc.t2.SQLMXConnectionPoolDataSource")) {
				Properties props = new Properties();
				for (Enumeration enum2 = ref.getAll(); enum2.hasMoreElements();) {
					RefAddr tRefAddr = (RefAddr) enum2.nextElement();
					String type = tRefAddr.getType();
					String content = (String) tRefAddr.getContent();
					if(content !=null){
						props.setProperty(type, content);
					}
				}
				ds = new SQLMXConnectionPoolDataSource(props);

			}
			if (JdbcDebugCfg.traceActive) {
				if (ds == null)
					debug[methodId_getObjectInstance].methodReturn("ds=null");
				else
					debug[methodId_getObjectInstance].methodReturn("ds="
							+ JdbcDebug.debugObjectStr(ds));
			}
			return ds;
		} finally {
			if (JdbcDebugCfg.entryActive)
				debug[methodId_getObjectInstance].methodExit();
		}
	}

	private static int methodId_getObjectInstance = 0;
	private static int methodId_SQLMXConnectionPoolDataSourceFactory = 1;
	private static int totalMethodIds = 2;
	private static JdbcDebug[] debug;

	static {
		String className = "SQLMXConnectionPoolDataSourceFactory";
		if (JdbcDebugCfg.entryActive) {
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getObjectInstance] = new JdbcDebug(className,
					"getObjectInstance");
			debug[methodId_SQLMXConnectionPoolDataSourceFactory] = new JdbcDebug(
					className, "SQLMXConnectionPoolDataSourceFactory");
		}
	}
}
