package org.trafodion.jdbc.t4;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class TimestampTest {

	public static void main(String[] args) throws ClassNotFoundException, SQLException {
		Class.forName("org.trafodion.jdbc.t4.T4Driver");
		String url = "jdbc:t4jdbc://192.168.0.34:23400/:";
		String user = "aaa";
		String password = "aaa";
		Connection conn = DriverManager.getConnection(url, user, password);
		Statement st = conn.createStatement();
		ResultSet rs = st.executeQuery("values(current_timestamp)");
		if (rs.next())
			System.out.println(rs.getObject(1)+", "+rs.getObject(1).getClass());
		rs.close();
		st.close();
		conn.close();
	}

}
