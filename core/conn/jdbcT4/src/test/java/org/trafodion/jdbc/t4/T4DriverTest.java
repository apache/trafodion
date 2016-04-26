/*
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
*/

package org.trafodion.jdbc.t4;

import java.sql.SQLException;

import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

public class T4DriverTest {
	private static T4Driver driver;

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		driver = (T4Driver) Class.forName("org.trafodion.jdbc.t4.T4Driver").newInstance();
	}


	@Test
	public void acceptsURL() throws SQLException {
		String url = "jdbc:t4jdbc://localhost:23400/:";
		Assert.assertTrue(driver.acceptsURL(url));
		url = "jdbc:abc://localhost:23400/:";
		Assert.assertFalse(driver.acceptsURL(url));
	}

}
