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
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;

import common.sampleUtils;

public class BatchSample {
	public static void main(String[] args) {
		Connection conn;
		Statement stmt;
		PreparedStatement pStmt;
		ResultSet rs;
		String table = "BatchSample";

		try {
			conn = sampleUtils.getPropertiesConnection();
			
			for (int i = 1; i <= 2; i++) {
				switch (i) {
				case 1:
					sampleUtils.initialTable(conn, table);
					stmt = conn.createStatement();
					String sql_1;
					for (int j = 0; j < 10; j++) {
						sql_1 = "insert into " + table + " values(" + j + ",'BatchS1'," + j + ")";
						stmt.addBatch(sql_1);
					}
					stmt.executeBatch();
					sql_1 = "select * from " + table;
					rs = stmt.executeQuery(sql_1);
					showRs(rs);
					rs.close();
					stmt.close();
					sampleUtils.dropTable(conn, table);
					break;
				case 2:
					sampleUtils.initialTable(conn, table);
					String sql_2 = "insert into " + table + " values(?,?,?)";
					pStmt = conn.prepareStatement(sql_2);
					for (int j = 0; j < 10; j++) {
						pStmt.setInt(1, j);
						pStmt.setString(2, "BatchPS" + j);
						pStmt.setInt(3, j * 10 + j);
						pStmt.addBatch();
					}
					pStmt.executeBatch();
					sql_2 = "select * from " + table;
					rs = pStmt.executeQuery(sql_2);
					showRs(rs);
					rs.close();
					pStmt.close();
					sampleUtils.dropTable(conn, table);
					break;
				default:
					break;
				}

			}
			conn.close();
		} catch (

		SQLException e) {
			e.printStackTrace();
		}
	}

	private static void showRs(ResultSet rs) {
		try {
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
		} catch (SQLException e) {
			e.printStackTrace();
		}

	}
}
