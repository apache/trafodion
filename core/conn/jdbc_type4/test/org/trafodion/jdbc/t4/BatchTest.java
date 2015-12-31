package org.trafodion.jdbc.t4;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Statement;
import java.util.Locale;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class BatchTest {

	private static Connection conn;

	public static void main(String[] args) throws Exception {
		setUpBeforeClass();
		result(null);
		setUpAfterClass();
	}

	@Test
	public static void result(String[] args) throws SQLException {
		String sql = "insert into mylocaltest.t1 values(?,?,?)";
		PreparedStatement ps = conn.prepareStatement(sql);
		for (int i = 0; i < 10; i++) {
			if (i == 7) {
				ps.setLong(1, Long.MAX_VALUE);
			} else
				ps.setInt(1, i);
			if (i == 5) {
				ps.setString(2,
						"ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss");
			} else
				ps.setString(2, "sss");
			ps.setFloat(3, 3.3F);
			ps.addBatch();
		}
		int[] rs = ps.executeBatch();
		for (int r : rs) {
			System.out.println("" + r);
		}
		SQLWarning warn = ps.getWarnings();
		while(warn!=null){
			System.out.println(warn.getMessage());
			warn=warn.getNextWarning();
		}
	}

	private static void prepareData() throws Exception {
		Statement st = conn.createStatement();
		st.execute("create schema if not exists mylocaltest");
		st.execute("create table if not exists mylocaltest.t1 (id int, name varchar(20),weight numeric(18,2))");
		st.close();
	}

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		System.out.println(Locale.CHINA.toLanguageTag());
		System.setProperty("t4jdbc.language", Locale.CANADA.toLanguageTag());
		System.out.println();
		Class.forName("org.trafodion.jdbc.t4.T4Driver");
		String url = "jdbc:t4jdbc://192.168.0.34:23400/:";
		String user = "aaa";
		String password = "aaa";
		conn = DriverManager.getConnection(url, user, password);
		prepareData();
	}

	@AfterClass
	public static void setUpAfterClass() throws Exception {
		if (conn != null) {
			conn.close();
		}
	}

}
