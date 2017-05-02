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

import java.sql.*;
import java.math.BigDecimal;
import org.apache.trafodion.jdbc.t2.*;

public class JdbcRowSetSample
{
	public static void main(String args[])
	{

		Connection          connection = null;
		SQLMXJdbcRowSet	    jrs	       = null;
		Statement           stmt       = null;
		ResultSet           rs	       = null;
		DatabaseMetaData    dbMeta     = null;
		ResultSetMetaData   rsMD       = null;
		int                 rowNo;

		try
		{
			Class.forName("org.apache.trafodion.jdbc.t2.T2Driver");
		}
		catch (Exception e)
		{
			e.printStackTrace();
			System.out.println(e.getMessage());
			return;
		}
		try
		{
			String url = "jdbc:t2jdbc:";
			connection = DriverManager.getConnection(url);
			stmt = connection.createStatement();
			try
			{
				stmt.executeUpdate("drop table jdbcrsdemo");
			}
			catch (SQLException e)
			{
			}
			stmt.executeUpdate("create table jdbcrsdemo (c1 char(20), c2 smallint, c3 integer, c4 largeint, c5 varchar(120), c6 numeric(10,2), c7 decimal(10,2),c8 date, c9 time, c10 timestamp, c11 float, c12 double precision)");
			stmt.executeUpdate("insert into jdbcrsdemo values('Doberman Pinscher', 100, 12345678, 123456789012, 'Canine', 100.12, 100.12, {d '2005-12-23'}, {t '10:11:12'}, {ts '2000-12-23 10:11:12'}, 100.12, 100.12)");
			stmt.executeUpdate("insert into jdbcrsdemo values('Rottweiler', 150, -12345678, -123456789012, 'Canine', -100.12, -100.12, {d '2005-12-24'}, {t '10:11:12'}, {ts '2005-12-24 10:11:12'}, -100.12, -100.12)");
			stmt.executeUpdate("insert into jdbcrsdemo values('Tiger', 101, -12345678, 123456789012, 'Feline', -100.12, 100.12, {d '2005-12-25'}, {t '10:11:12'}, {ts '2005-12-25 10:11:12.0'}, -100.12, 100.12)");

			System.out.println("Creating JdbcRowSet");
			jrs = new SQLMXJdbcRowSet();
			jrs.setUrl(url);

			for (int i = 0; i < 5 ; i++)
			{
				switch (i)
				{
					case 0:
						System.out.println("");
						System.out.println("Simple Select ");
						jrs.setCommand("select * from jdbcrsdemo");
						jrs.execute();
						break;
					case 1:
						System.out.println("");
						System.out.println("Parameterized Select - CHAR");
						jrs.setCommand("select c1, c2 from jdbcrsdemo where c1 = ?");
						jrs.setString(1, "Rottweiler");
						jrs.execute();
						break;
					case 2:
						System.out.println("");
						System.out.println("Parameterized Select - INT");
						jrs.setCommand("select c1, c2, c3 from jdbcrsdemo where c2 = ?  or c2 = ?");
						jrs.setInt(1, 100);
						jrs.setInt(2, 150);
						jrs.execute();
						break;
					case 3:
						System.out.println("");
						System.out.println("Parameterized Select - TIMESTAMP");
						jrs.setCommand("select c1, c2, c3, c10 from jdbcrsdemo where c10 = ?");
						jrs.setTimestamp(1, Timestamp.valueOf("2005-12-25 10:11:12.0"));
						jrs.execute();
						break;
					case 4:
						System.out.println("");
						System.out.println("Parameterized Select - DECIMAL");
						jrs.setCommand("select c1, c2, c3, c7 from jdbcrsdemo where c7 = ? or c7 = ?");
						jrs.setBigDecimal(1, new BigDecimal("100.12"));
						jrs.setBigDecimal(2, new BigDecimal("-100.12"));
						jrs.execute();
						break;
				}
				rsMD = jrs.getMetaData();
				System.out.println("");
				System.out.println("Printing JdbcRowSet ResultSetMetaData ...");
				System.out.println("No. of Columns " + rsMD.getColumnCount());
				for (int j = 1; j <= rsMD.getColumnCount(); j++)
				{
					System.out.println("Column " + j + " Data Type: " + rsMD.getColumnTypeName(j) + " Name: " + rsMD.getColumnName(j));
				}
				System.out.println("");
				System.out.println("Fetching JdbcRowSet rows...");
				rowNo = 0;
				while (jrs.next())
				{
					rowNo++;
					System.out.println("");
					System.out.println("Printing Row " + rowNo + " using getString(), getObject()");
					for (int j=1; j <= rsMD.getColumnCount(); j++)
					{
						System.out.println("Column " + j + " - " + jrs.getString(j) + "," + jrs.getObject(j));
					}
				}
				System.out.println("");
			}

			System.out.println("Calling JdbcRowSet.getDatabaseMetaData()");
			dbMeta = jrs.getDatabaseMetaData();

			for (int k = 0; k < 4 ; k++)
			{
				switch (k)
				{
					case 0:
						System.out.println("");
						System.out.println("getTypeInfo() ");
						rs = dbMeta.getTypeInfo();
						break;
					case 1:
						System.out.println("");
						System.out.println("getCatalogs()");
						rs = dbMeta.getCatalogs();
						break;
					case 2:
						System.out.println("");
						System.out.println("getTables() ");
						rs = dbMeta.getTables(null, null, "JDB%", null);
						break;
					case 3:
						System.out.println("");
						System.out.println("getColumns()");
						rs = dbMeta.getColumns(null, null, "JDBCRSDEMO", "%");
						break;
					default:
						rs = null;
						continue;
				}

				if (rs != null)
				{
					rsMD = rs.getMetaData();
					System.out.println("");

					System.out.println("Printing ResultSetMetaData ...");
					System.out.println("No. of Columns " + rsMD.getColumnCount());
					for (int j = 1; j <= rsMD.getColumnCount(); j++)
					{
						System.out.println("Column " + j + " Data Type: " + rsMD.getColumnTypeName(j) + " Name: " + rsMD.getColumnName(j));
					}
					System.out.println("");
					System.out.println("Fetching resultSet rows...");
					rowNo = 0;
					while (rs.next())
					{
						rowNo++;
						System.out.println("");
						System.out.println("Printing Row " + rowNo + " using getString(), getObject()");
						for (int j=1; j <= rsMD.getColumnCount(); j++)
						{
							System.out.println("Column " + j + " - " + rs.getString(j) + "," + rs.getObject(j));
						}
					}
				}
				System.out.println("");
				System.out.println("End of Data");
				if (rs != null) rs.close();
			}
			System.out.println("Demo Completed");
		}
		catch (SQLException e)
		{
			SQLException nextException;

			nextException = e;
			do
			{
				System.out.println(nextException.getMessage());
				System.out.println("SQLState   " + nextException.getSQLState());
				System.out.println("Error Code " + nextException.getErrorCode());
			} while ((nextException = nextException.getNextException()) != null);
		}
		finally
		{
			try
			{
				if(connection!=null) connection.close();
				if(jrs!=null) jrs.close();
			}
			catch(SQLException sqle)
			{
				sqle.printStackTrace();
				System.out.println(sqle.getMessage());
			}
		}
	}
}
