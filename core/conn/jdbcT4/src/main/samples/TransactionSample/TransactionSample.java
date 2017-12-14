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
import common.*;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;

public class TransactionSample {
	public static void main(String args[]) {
		Connection connection = null;
		String table = "TransactionSample";

		try {
			connection = sampleUtils.getPropertiesConnection();

			autoCommit(connection, table);
			rollBack(connection, table);
			sampleUtils.dropTable(connection, table);
		}

		catch (SQLException e) {
			SQLException nextException;
			nextException = e;
			do {
				System.out.println(nextException.getMessage());
				System.out.println("SQLState   " + nextException.getSQLState());
				System.out.println("Error Code " + nextException.getErrorCode());
			} while ((nextException = nextException.getNextException()) != null);
		} finally {
			if (connection != null) {
				try {
					connection.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}
	}

	private static void autoCommit(Connection connection, String table) {
		Statement stmt = null;
		try {
			sampleUtils.dropTable(connection, table);
			sampleUtils.initialTable(connection, table);
			connection.setAutoCommit(false);
			stmt = connection.createStatement();
			stmt.executeUpdate("insert into " + table + " values(1,'row1',11)");
			stmt.executeUpdate("insert into " + table + " values(2,'row2',22)");
			connection.commit();
			// select all information of table after commit
			selectAllfromTable(connection, table);
		} catch (SQLException e) {
			e.printStackTrace();
		} finally {
			try {
				connection.setAutoCommit(true);
			} catch (SQLException e1) {
				e1.printStackTrace();
			}
			if (stmt != null) {
				try {
					stmt.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}

	}

	private static void rollBack(Connection connection, String table) {
		Statement stmt = null;
		try {
			sampleUtils.dropTable(connection, table);
			sampleUtils.initialTable(connection, table);
			connection.setAutoCommit(false);
			stmt = connection.createStatement();
			stmt.executeUpdate("insert into " + table + " values(3,'row1',33)");
			// it will have exception, then will do rollback
			stmt.executeUpdate("insert in " + table + " values(4,'row2',44)");
			connection.commit();
		} catch (SQLException e) {
			if (connection != null) {
				try {
					System.out.println("Rolling back data here....");
					connection.rollback();
				} catch (SQLException e1) {
					e1.printStackTrace();
				}
			}
			SQLException nextException;
			nextException = e;
			do {
				System.out.println(nextException.getMessage());
				System.out.println("SQLState   " + nextException.getSQLState());
				System.out.println("Error Code " + nextException.getErrorCode());
			} while ((nextException = nextException.getNextException()) != null);
		} finally {
			try {
				connection.setAutoCommit(true);
			} catch (SQLException e1) {
				e1.printStackTrace();
			}
			if (stmt != null) {
				try {
					stmt.close();
				} catch (SQLException e) {
				}
			}
		}

	}

	private static void selectAllfromTable(Connection connection, String table) throws SQLException {
		Statement stmt;
		stmt = connection.createStatement();
		ResultSet rs = stmt.executeQuery("select * from " + table);
		int rowNo;
		ResultSetMetaData rsMD = rs.getMetaData();
		System.out.println("");
		System.out.println("Printing ResultSetMetaData ...");
		System.out.println("No. of Columns " + rsMD.getColumnCount());
		for (int j = 1; j <= rsMD.getColumnCount(); j++) {
			System.out.println(
					"Column " + j + " Data Type: " + rsMD.getColumnTypeName(j) + " Name: " + rsMD.getColumnName(j));
		}
		System.out.println("");
		System.out.println("Fetching rows...");
		rowNo = 0;
		while (rs.next()) {
			rowNo++;
			System.out.println("");
			System.out.println("Printing Row " + rowNo + " using getString(), getObject()");
			for (int j = 1; j <= rsMD.getColumnCount(); j++) {
				System.out.println("Column " + j + " - " + rs.getString(j) + "," + rs.getObject(j));
			}
		}
		System.out.println("");
		System.out.println("End of Data");
		rs.close();
		stmt.close();
	}
}
