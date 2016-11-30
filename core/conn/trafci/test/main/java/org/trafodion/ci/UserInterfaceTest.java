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

import static org.junit.Assert.*;

import org.junit.Test;

public class UserInterfaceTest {

	@Test
	public void test() {
		MySecurityManager secManager = new MySecurityManager();
		System.setSecurityManager(secManager);
		try {
			String[] params = "-h 10.10.12.99:23400 -u trafodion -p traf123 -q values(1);".split("\\s+");
			UserInterface.main(params);
		} catch (SecurityException e) {
			assertTrue(true);
		}
	}

}

class MySecurityManager extends SecurityManager {
	@Override
	public void checkExit(int status) {
		throw new SecurityException();
	}
}
