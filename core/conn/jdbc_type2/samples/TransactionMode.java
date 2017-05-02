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
//
// The following examples of Transaction mode are demonstrated:
//   - internal
//   - mixed
//   - external
//
import java.sql.*;
import java.util.*;
import javax.naming.*;
import javax.sql.DataSource;
import com.tandem.tmf.*;

public class TransactionMode
{
	static String            PROLOG = "TransactionMode: ";
	static Connection        conn   = null;
	static DataSource        ds     = null;
	static Statement         stmt   = null;
	static PreparedStatement pStmt  = null;
	static ResultSet         rs     = null;
	static int               testType;
	static Current           tx     = null;

	public static void main(String[] args)
	{
		String     tblName = "txnMode";

		if ( args.length != 2 )
		{
			System.out.println(PROLOG + "The dataSources directory path and test type arguments are required.");
			displayUsage();
			System.exit(1);
		}

		try
		{
			System.out.println(PROLOG + "Started");
			initDS(args);

			tx = new Current();
			System.out.println(PROLOG + "Starting an external transaction");
			tx.begin();

			conn = ds.getConnection();
			stmt = conn.createStatement();
			dropTable(tblName);
			createTable(tblName);
			populateTable(tblName);

			System.out.println(PROLOG + "Committing the external transaction");
			tx.commit(true);

			testTransactionMode(conn, testType, tblName);
			dropTable(tblName);
			System.out.println("");
			System.out.println(PROLOG + "Demo Completed");
		}
		catch (SQLException e)
		{
			SQLException nextException;
			nextException = e;
			do
			{
				System.out.println(nextException.getMessage());
				System.out.println(PROLOG + "SQLState   " + nextException.getSQLState());
				System.out.println(PROLOG + "Error Code " + nextException.getErrorCode());

			} while ((nextException = nextException.getNextException()) != null);
			e.printStackTrace();

			try
			{
				tx.rollback();
			}
			catch (com.tandem.util.FSException ex)
			{
				ex.printStackTrace();
			}
		}
		catch (com.tandem.util.FSException ex)
		{
			ex.printStackTrace();
		}
		finally
		{
			try
			{
				if(rs != null) rs.close();
				if(stmt != null) stmt.close();
				if(pStmt != null) pStmt.close();
				if(conn != null) conn.close();
			}
			catch (SQLException ex)
			{
				ex.printStackTrace();
			}
		}
	} // end main

	private static void initDS(String[] args)
	{
		String RootDir = "file://" + args[0] + "/dataSources";
		Hashtable<String,String> env = new Hashtable<String,String>();

		env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.fscontext.RefFSContextFactory");
		env.put(Context.PROVIDER_URL, RootDir);

		try
		{
			Context ctx = new InitialContext(env);
			testType    = Integer.parseInt(args[1]);

			switch (testType)
			{
				case 1:
					System.out.println(PROLOG + "Testing 'internal' transactionMode");
					ds = (DataSource)ctx.lookup("jdbc/TestDataSource");
					((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setTransactionMode("internal");
					break;
				case 2:
					System.out.println(PROLOG + "Testing 'mixed' transactionMode");
					ds = (DataSource)ctx.lookup("jdbc/TestDataSource");
					((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setTransactionMode("mixed");
					break;
				case 3:
					System.out.println(PROLOG + "Testing 'external' transactionMode");
					ds = (DataSource)ctx.lookup("jdbc/TestDataSource");
					((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setTransactionMode("external");
					break;
				default:
					System.out.println(PROLOG + "Invalid test type.");
					displayUsage();
					System.exit(1);
			}
		}
		catch (Exception ex)
		{
			ex.printStackTrace();
			System.exit(1);
		}
	} // end initDS

	private static void createTable(String table) throws SQLException
	{
		String Query = "CREATE TABLE " + table + " (";
		Query += "C1 CHAR(20), ";
		Query += "C2 SMALLINT, ";
		Query += "C3 INTEGER, ";
		Query += "C4 LARGEINT, ";
		Query += "C5 VARCHAR(120), ";
		Query += "C6 DATE, ";
		Query += "C7 TIME, ";
		Query += "C8 TIMESTAMP)";

		System.out.println(PROLOG + "Creating " + table + " table");
		stmt.execute(Query);
		stmt.clearWarnings();
	}

	private static void displayUsage()
	{
		System.out.println(PROLOG + "Usage : java TransactionMode <dataSourceDir> <testType>");
		System.out.println(PROLOG + "Where <dataSource> is the directory that contains the datasource to use and");
		System.out.println(PROLOG + "      <TestType> can be:");
		System.out.println(PROLOG + "      1 : Using DataSource - TransactionMode = 'internal'");
		System.out.println(PROLOG + "      2 : Using DataSource - TransactionMode = 'mixed' - default value");
		System.out.println(PROLOG + "      3 : Using DataSource - TransactionMode = 'external'");
	}

	private static void dropTable(String table)
	{
		try
		{
			System.out.println(PROLOG + "Dropping " + table + " table");
			stmt.execute("drop table " + table);
		}
		catch (SQLException ex3)
		{ } // Ignore exception from drop
	}

	private static void populateTable(String table) throws SQLException
	{
		System.out.println(PROLOG + "Populating " + table + " table");
		stmt.executeUpdate("INSERT INTO " + table +
			" VALUES('Austin', 1, 1412271, 123456789.12, 'Texas', {d '2005-01-01'}, {t '10:11:12'}, {ts '2005-01-01 10:11:12.0'})");
		stmt.executeUpdate("INSERT INTO " + table +
			" VALUES('Pflugerville', 2, 25911, -123456789.12, 'Texas', {d '2004-02-02'}, {t '10:11:12'}, {ts '2004-02-02 10:11:12'})");
		stmt.executeUpdate("INSERT INTO " + table +
			" VALUES('Travis County', 3, 869868, 123456789.12, 'Texas', {d '2004-03-03'}, {t '10:11:12'}, {ts '2004-03-03 10:11:12.0'})");
		stmt.clearWarnings();
	}

	private static void testTransactionMode(Connection con, int testtype, String table)
	{
		ResultSetMetaData rsMD = null;
		String query;
		int rowNo;
		boolean beginTxn = false;

		try
		{
			for (int i = 0; i < 4; i++)
			{
				System.out.println("");
				switch (i)
				{
						// Perform the query within an internally managed jdbcMx transaction
						// or no transaction when in 'external' transactionMode
					case 0:
						query ="select * from " + table + " for browse access";
						System.out.println(PROLOG + "Executing - '" + query + "'");
						rs = stmt.executeQuery(query);
						break;
						// Perform the query using an external transaction
					case 1:
						query = "select c1, c2 from " + table + " where c1 = ?";
						// Note: Performing select queries within an external transaction
						//       in 'internal' transactionMode is not supported.
						//       An "Invalid transaction state" exception will be thrown
						//       upon performing ResultSet operations (i.e. rs.next())
						//       under these conditions.
						if(testtype != 1)
						{
							System.out.println(PROLOG + "Starting an external transaction");
							tx.begin();
							beginTxn = true;
						}
						pStmt = con.prepareStatement(query);
						pStmt.setString(1, "Austin");
						System.out.println(PROLOG + "Executing - '" + query + "'");
						rs = pStmt.executeQuery();
						break;
					case 2:
						System.out.println(PROLOG + "Setting transaction isolation to READ_UNCOMMITTED");
						con.setTransactionIsolation(Connection.TRANSACTION_READ_UNCOMMITTED);
						query = "select c1, c2, c3 from " + table +  " where c2 = ?  or c2 = ? or c5 = ?";
						pStmt = con.prepareStatement(query);
						pStmt.setInt(1, 2);
						pStmt.setInt(2, 3);
						pStmt.setString(3, "Texas");
						System.out.println(PROLOG + "Executing - '" + query + "'");
						rs = pStmt.executeQuery();
						break;
					case 3:
						System.out.println(PROLOG + "Setting transaction isolation to READ_COMMITTED");
						con.setTransactionIsolation(Connection.TRANSACTION_READ_COMMITTED);
						System.out.println(PROLOG + "Starting an external transaction");
						tx.begin();
						beginTxn = true;
						query = "update " + table +  " set c2 = ? where c1 = ?";
						pStmt = con.prepareStatement(query);
						pStmt.setInt(1, 4);
						pStmt.setString(2, "Travis County");
						System.out.println(PROLOG + "Executing - '" + query + "'");
						int rowCount = pStmt.executeUpdate();
						System.out.println(PROLOG + "Number of rows affected = " + rowCount);
						rs = null;
						break;
					default:
						rs = null;
						continue;
				}

				if(rs != null)
				{
					rsMD = rs.getMetaData();
					System.out.println("");
					System.out.println(PROLOG + "Printing ResultSetMetaData ...");
					System.out.println(PROLOG + "No. of Columns " + rsMD.getColumnCount());
					for (int j = 1; j <= rsMD.getColumnCount(); j++)
					{
						System.out.println(PROLOG + "Column " + j + " Data Type: " + rsMD.getColumnTypeName(j) + " Name: " + rsMD.getColumnName(j));
					}
					System.out.println("");
					System.out.println(PROLOG + "Fetching rows...");
					rowNo = 0;
					while (rs.next())
					{
						rowNo++;
						System.out.println("");
						System.out.println(PROLOG + "Printing Row " + rowNo + " using getString(), getObject()");
						for (int j=1; j <= rsMD.getColumnCount(); j++)
						{
							System.out.println(PROLOG + "Column " + j + " - " + rs.getString(j) + "," + rs.getObject(j));
						}
					}
					System.out.println("");
					System.out.println(PROLOG + "End of Data");
					rs.close();
				}
				if(beginTxn)
				{
					System.out.println(PROLOG + "Committing the external transaction");
					tx.commit(true);
					beginTxn = false;
				}
			}
		}
		catch (SQLException e1)
		{
			SQLException nextException;
			nextException = e1;
			do
			{
				System.out.println(nextException.getMessage());
				System.out.println(PROLOG + "SQLState   " + nextException.getSQLState());
				System.out.println(PROLOG + "Error Code " + nextException.getErrorCode());

			} while ((nextException = nextException.getNextException()) != null);
			e1.printStackTrace();
			if(beginTxn)
			{
				try
				{
					System.out.println(PROLOG + "Rolling back the external transaction");
					tx.rollback();
				}
				catch (com.tandem.util.FSException ex)
				{
					ex.printStackTrace();
				}
			}
			System.exit(1);
		}
		catch (com.tandem.util.FSException ex)
		{
			ex.printStackTrace();
		}
	}
}
