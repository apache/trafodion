/*
/* @@@ START COPYRIGHT @@@
/*
/*
Licensed to the Apache Software Foundation (ASF) under one
/*
or more contributor license agreements.  See the NOTICE file
/*
distributed with this work for additional information
/*
regarding copyright ownership.  The ASF licenses this file
/*
to you under the Apache License, Version 2.0 (the
/*
"License"); you may not use this file except in compliance
/*
with the License.  You may obtain a copy of the License at
/*
/*
  http://www.apache.org/licenses/LICENSE-2.0
/*
/*
Unless required by applicable law or agreed to in writing,
/*
software distributed under the License is distributed on an
/*
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
/*
KIND, either express or implied.  See the License for the
/*
specific language governing permissions and limitations
/*
under the License.
/*
/* @@@ END COPYRIGHT @@@
/*/
import static org.junit.Assert.assertTrue;

import java.sql.Connection;
import java.sql.SQLException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.junit.Test;

public class TestNetworkTimeout {
	@Test
	public void testSetAndGetNetworkTimeout() {
		Connection conn = null;
		try {
			System.out.println("Connecting to database...");
			conn = Utils.getUserConnection();
			ExecutorService es = Executors.newSingleThreadExecutor();
			conn.setNetworkTimeout(es, 100);
			es.shutdown();
			int result = conn.getNetworkTimeout();
			assertTrue("this is networkTimeout", result == 100);
		} catch (SQLException e) {
			e.printStackTrace();
		}
	}
	@Test
	public void testGetNetworkTimeout() {
		Connection conn = null;
		try {
			System.out.println("Connecting to database...");
			conn = Utils.getUserConnection();
			int result = conn.getNetworkTimeout();
			assertTrue("this is networkTimeout", result == 0);
		} catch (SQLException e) {
			e.printStackTrace();
		}
	}
}
