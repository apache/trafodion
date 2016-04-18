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

package org.trafodion.jdbc.t4;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Properties;

import javax.naming.Context;
import javax.naming.Name;
import javax.naming.RefAddr;
import javax.naming.Reference;

public class HPT4ConnectionPoolDataSourceFactory implements javax.naming.spi.ObjectFactory {
	public HPT4ConnectionPoolDataSourceFactory() {
	}

	public Object getObjectInstance(Object refobj, Name name, Context nameCtx, Hashtable env) throws Exception {
		Reference ref = (Reference) refobj;
		HPT4ConnectionPoolDataSource ds;
		RefAddr refAddr;
		String tmp;

		if (ref.getClassName().equals("org.trafodion.jdbc.t4.HPT4ConnectionPoolDataSource")) {
			Properties props = new Properties();
			for (Enumeration enum2 = ref.getAll(); enum2.hasMoreElements();) {
				RefAddr tRefAddr = (RefAddr) enum2.nextElement();
				String type = tRefAddr.getType();
				String content = (String) tRefAddr.getContent();
				props.setProperty(type, content);
			}
			ds = new HPT4ConnectionPoolDataSource(props);
			/*
			 * tmp = props.getProperty("initialPoolSize"); if (tmp != null) {
			 * try { ds.setInitialPoolSize(Integer.parseInt(tmp)); } catch
			 * (NumberFormatException e1) { } } tmp =
			 * props.getProperty("maxIdleTime"); if (tmp != null) { try {
			 * ds.setMaxIdleTime(Integer.parseInt(tmp)); } catch
			 * (NumberFormatException e4) { } } tmp =
			 * props.getProperty("propertyCycle"); if (tmp != null) { try {
			 * ds.setPropertyCycle(Integer.parseInt(tmp)); } catch
			 * (NumberFormatException e5) { } }
			 */
			return ds;
		} else {
			return null;
		}
	}
}
