package org.trafodion.jdbc.t4;

import java.sql.CallableStatement;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.Statement;


public class SPJTest {

	public static void main(String[] args) throws ClassNotFoundException, SQLException {
		Class.forName("org.trafodion.jdbc.t4.T4Driver");
		String url = "jdbc:t4jdbc://192.168.0.34:23400/:";
		String user = "aaa";
		String password = "aaa";
		Connection conn = DriverManager.getConnection(url, user, password);
		Statement localStatement = conn.createStatement();

		CallableStatement localCallableStatement = conn.prepareCall("call testspj.RS365()");
		localCallableStatement.execute();
		
		conn.close();
	}

}
