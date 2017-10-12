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

import org.junit.Test;
import static org.junit.Assert.*;

import java.sql.*;
import java.io.*;
import java.lang.*;

public class TestMisc 
{
	/* Test using GetMaxRows to change resultset size */
	@Test
	public void JDBCMisc1() throws InterruptedException, SQLException
	{
		//GETMAXROWS
		ResultSet rs = null;
		String sql = null;
		int maxRows = 10, rowSel = 0, i = 0;

		try  (
		        Connection conn = Utils.getUserConnection();
                Statement stmt = conn.createStatement();
        		)
		{
			assertEquals("getMaxRows should return 0", 0, stmt.getMaxRows());
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists maxrowstab (c0 int not null, c1 char(20), c2 smallint, c3 integer, primary key(c0))");
            stmt.executeUpdate("delete from maxrowstab");
            
            while (i < 1000)
            {
            	i++;
            	stmt.executeUpdate("upsert into maxrowstab values(" + i + " , 'Moe', 100, 223)");
            	i++;
            	stmt.executeUpdate("upsert into maxrowstab values(" + i + " , 'Curly', 34, 444)");
            }
            rs = stmt.executeQuery("select count(*) from maxrowstab");
            rs.next();
            rowSel = rs.getInt(1);
            rs.close();
			assertEquals("Rows returned after insert ", 1000, rowSel);
            
            stmt.setMaxRows(20);
            assertEquals("MaxRows", 20, stmt.getMaxRows());
            rs = stmt.executeQuery("select * from maxrowstab");
            rowSel = 0;
            while (rs.next())
            	rowSel++;
            rs.close();
			assertEquals("Rows returned after setMaxRows 20", 20, rowSel);
            
            stmt.setMaxRows(50);            
            assertEquals("MaxRows", 50, stmt.getMaxRows());
            rs = stmt.executeQuery("select * from maxrowstab");
            rowSel = 0;
            while (rs.next())
            	rowSel++;
            rs.close();
			assertEquals("Rows returned after setMaxRows 50", 50, rowSel);
            
            stmt.setMaxRows(0);
            assertEquals("MaxRows", 0, stmt.getMaxRows());
            rs = stmt.executeQuery("select * from maxrowstab");
            rowSel = 0;
            while (rs.next())
            	rowSel++;
            rs.close();
			assertEquals("Rows returned after setMaxRows 1000", 1000, rowSel);
		} 
		catch (SQLException e)
		{
			System.out.println("Exception : " + e.getMessage());
			e.printStackTrace();
			fail("exception in test JDBCMisc1 .." + e.getMessage());
			return;
		}
	}

	/* Fetch explain plan */
	@Test
	public void JDBCMisc2() throws InterruptedException, SQLException
	{
		//EXPLAIN PLAN
		ResultSet rs = null;
		String sql = null;

		try (
		        Connection conn = Utils.getUserConnection();
                Statement stmt = conn.createStatement();
		        )
		{
			sql = "explain options 'f' select count(\"_REPOS_\".metric_query_table.session_id) from "
					+ "\"_REPOS_\".metric_query_table <<+ cardinality 10e5 >>, \"_REPOS_\".metric_query_aggr_table <<+ cardinality 10e4 >> "
			        + "where \"_REPOS_\".metric_query_table.session_id = \"_REPOS_\".metric_query_aggr_table.session_id";
            rs = stmt.executeQuery(sql);
            int rowno = 0;
            while (rs.next())
            {
            	rowno++;
            	//System.out.println(rs.getString(1));
            }
			//System.out.println("Rows returned : " + rs.getRow());
            rs.close();            
            assertNotEquals("Explain rows", 0, rowno);
		} 
		catch (SQLException e)
		{
			System.out.println("Exception : " + e.getMessage());
			e.printStackTrace();
			fail("exception in test JDBCMisc2 .." + e.getMessage());
			return;
		}
	}

	/* Test query that spawns esps */
	@Test
	public void JDBCMisc3() throws InterruptedException, SQLException
	{
		//ESP query
		ResultSet rs = null;
		long rowSel = 0;
		String sql = "select count(\"_REPOS_\".metric_query_table.session_id) from "
				+ "\"_REPOS_\".metric_query_table, \"_REPOS_\".metric_query_aggr_table where "
				+ "\"_REPOS_\".metric_query_table.session_id = \"_REPOS_\".metric_query_aggr_table.session_id";

		try (
        		Connection conn = Utils.getUserConnection();
                Statement stmt = conn.createStatement();
        		)
		{
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            rs = stmt.executeQuery(sql);
            rs.next();
            rowSel = rs.getLong(1);
            rs.close();
		} 
		catch (SQLException e)
		{
			System.out.println("Exception : " + e.getMessage());
			e.printStackTrace();
			fail("exception in test JDBCMisc3 .." + e.getMessage());
			return;
		}
	}
}

