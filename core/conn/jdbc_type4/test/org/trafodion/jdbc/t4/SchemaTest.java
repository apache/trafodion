package org.trafodion.jdbc.t4;

import java.sql.DatabaseMetaData;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;

import org.junit.Assert;
import org.junit.Test;

public class SchemaTest extends BaseTest {

	@Test
	public void getTables() throws SQLException {
		DatabaseMetaData md;
		md = conn.getMetaData();
		ResultSet rs = md.getTableTypes();
		while (rs.next()) {
			System.out.println(rs.getObject(1));
		}
		rs.close();

		ResultSet tables = md.getTables("TRAFODION", "SEABASE", "%", new String[] { "TABLE" });
		while (tables.next()) {
			System.out.println(tables.getObject(1));
		}

		tables.close();
	}

	@Test
	public void getIndexs() throws SQLException {
		DatabaseMetaData md;
		md = conn.getMetaData();
		ResultSet indexes = md.getIndexInfo("TRAFODION", "SEABASE", "B", false, true);
		Object obj = null;
		ResultSetMetaData rsmd = indexes.getMetaData();
		System.out.println("----------"+rsmd.getColumnCount());
		for (int i = 0; i < rsmd.getColumnCount(); i++)
			System.out.print(rsmd.getColumnName(i+1)+", ");
		System.out.println();
		while (indexes.next()) {
			obj = indexes.getObject(1);
			for (int i = 0; i < rsmd.getColumnCount(); i++)
				System.out.print(indexes.getObject(i + 1) + ", ");
			System.out.println();
		}
		Assert.assertNotNull(obj);
		indexes.close();
	}

	@Test
	public void getTypeInfo() {
		try {
			DatabaseMetaData md;
			md = conn.getMetaData();
			ResultSet s = md.getTypeInfo();
			boolean flag = false;
			ResultSetMetaData rsmd = s.getMetaData();
			StringBuilder sb = new StringBuilder();
			while (s.next()) {
				sb.setLength(0);
				for (int i = 1; i <= rsmd.getColumnCount(); i++)
					sb.append(s.getObject(i) + ", ");
				flag = true;
			}
			Assert.assertTrue(flag);

			md = conn.getMetaData();
			s = md.getTypeInfo();
			flag = false;
			rsmd = s.getMetaData();
			while (s.next()) {
				sb.setLength(0);
				for (int i = 1; i <= rsmd.getColumnCount(); i++)
					sb.append(s.getObject(i) + ", ");
				flag = true;
			}
			Assert.assertTrue(flag);
		} catch (SQLException e) {
			e.printStackTrace();
		}
	}

	@Test
	public void showControlAll() throws Exception {
		Statement st = conn.createStatement();
		boolean hasrs = st.execute("showcontrol all");
		Assert.assertTrue(hasrs);
		ResultSet rs = st.getResultSet();
		while (rs.next()) {
		}
		rs.close();
		st.close();

	}

	@Test
	public void getSchemasTest() {
		try {
			DatabaseMetaData md;
			md = conn.getMetaData();
			ResultSet s = md.getSchemas();
			boolean flag = false;
			ResultSetMetaData rsmd = s.getMetaData();
			StringBuilder sb = new StringBuilder();
			while (s.next()) {
				sb.setLength(0);
				for (int i = 1; i <= rsmd.getColumnCount(); i++)
					sb.append(s.getObject(i) + ", ");
				flag = true;
			}
			Assert.assertTrue(flag);
		} catch (SQLException e) {
			e.printStackTrace();
		}
	}

	@Test
	public void getCatalogsTest() {
		try {
			DatabaseMetaData md;
			md = conn.getMetaData();
			ResultSet s = md.getCatalogs();
			boolean flag = false;
			ResultSetMetaData rsmd = s.getMetaData();
			StringBuilder sb = new StringBuilder();
			while (s.next()) {
				sb.setLength(0);
				for (int i = 1; i <= rsmd.getColumnCount(); i++)
					sb.append(s.getObject(i) + ", ");
				flag = true;
			}
			Assert.assertTrue(flag);
		} catch (SQLException e) {
			e.printStackTrace();
		}

	}

}
