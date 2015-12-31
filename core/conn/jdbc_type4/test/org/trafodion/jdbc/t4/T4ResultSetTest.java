package org.trafodion.jdbc.t4;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.Locale;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class T4ResultSetTest extends BaseTest{
	
	private static Connection conn;

	
	
	@Test
	public void cursorMsgTest() throws SQLException{
		Statement s = conn.createStatement();
		s.execute("CQD parallel_num_esps '24'");
		s.close();
		
		Statement st = conn.createStatement();
		ResultSet rs = st.executeQuery("get schemas");
		try {
			rs.getObject(1);
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		System.out.println("-----------");
		while(rs.next()){
			System.out.println(rs.getObject(1));
		}
		System.out.println("-----------");
		
		try {
			rs.getObject(1);
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		rs.close();
		st.close();
		System.out.println("success");
	}
	

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		System.out.println(Locale.CHINA.toLanguageTag());
		System.setProperty("t4jdbc.language", Locale.CANADA.toLanguageTag());
		System.out.println();
		Class.forName("org.trafodion.jdbc.t4.T4Driver");
		String url= "jdbc:t4jdbc://192.168.0.34:23400/:";
		String user="aaa";
		String password="aaa";
		conn = DriverManager.getConnection(url, user, password);
	}
	
	
	@AfterClass
	public static void setUpAfterClass() throws Exception {
		if(conn !=null){
			conn.close();
		}
	}

	
}
