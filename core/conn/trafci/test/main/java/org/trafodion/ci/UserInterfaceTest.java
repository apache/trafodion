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

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Properties;

import org.junit.Test;

public class UserInterfaceTest {

	@Test
	public void testUserInterfaceMain() throws FileNotFoundException {
		// MySecurityManager secManager = new MySecurityManager();
		/*System.setSecurityManager(secManager);*/
		try {
			FileInputStream fs = new FileInputStream("src/main/resources/trafci.properties");
			Properties prop = new Properties();
			try {
				prop.load(fs);
				String host = prop.getProperty("db.host");
				String user = prop.getProperty("db.user");
				String pwd = prop.getProperty("db.pwd");
				String query = prop.getProperty("db.query");
				assertNotNull(prop);
				assertNotNull(fs);
				System.out.println(host + user + pwd);
				String[] params = (host + user + pwd + query).split("\\s+");
				System.out.println((host + user + pwd + query).split("\\s+"));
				UserInterface.main(params);

			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
		} catch (SecurityException e) {
			assertTrue(true);
		}
	}

	@Test
	public void testWithRuntime() {
		try {
			FileInputStream fs = new FileInputStream("src/main/resources/trafci.properties");
			Properties prop = new Properties();
			prop.load(fs);
			String host = prop.getProperty("db.host");
			String user = prop.getProperty("db.user");
			String pwd = prop.getProperty("db.pwd");
			String query = prop.getProperty("db.query");

			Process p = Runtime.getRuntime().exec("trafci -h " + host + " -u " + user + " -p " + pwd + " -q " + query);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

	@Test
	public void testWithNewProcess() throws FileNotFoundException {
		FileInputStream fs;
		ProcessBuilder pb = null;
		try {
			fs = new FileInputStream("src/main/resources/trafci.properties");

			Properties prop = new Properties();
			prop.load(fs);
			String host = prop.getProperty("db.host");
			String user = prop.getProperty("db.user");
			String pwd = prop.getProperty("db.pwd");
			String query = prop.getProperty("db.query");
			String output=prop.getProperty("output.file.name");

			pb = new ProcessBuilder("trafci", "-h", host, "-u", user, "-p", pwd, "-q", query);
			pb.redirectErrorStream(true);
			File f = new File(output);
			pb.redirectOutput(f);
			Process p = pb.start();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

}
/*
 * class MySecurityManager extends SecurityManager {
 * 
 * @Override public void checkExit(int status) { throw new SecurityException();
 * } }
 */