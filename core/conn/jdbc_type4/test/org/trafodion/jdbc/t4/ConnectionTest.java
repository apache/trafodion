package org.trafodion.jdbc.t4;

import java.sql.Connection;
import java.sql.DriverManager;

import org.junit.Test;

public class ConnectionTest {
	@Test
	public void validate() throws Exception {
		Class.forName("org.trafodion.jdbc.t4.T4Driver");
		String url = "jdbc:t4jdbc://10.10.10.136:23400/:";
		String user = "aaa";
		String password = "aaa";
		Connection conn = null;
		try {
			conn = DriverManager.getConnection(url, user, password);
			 System.out.println(conn.isValid(10000));
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			conn.close();

		}

		System.out.println("success");
	}
}
