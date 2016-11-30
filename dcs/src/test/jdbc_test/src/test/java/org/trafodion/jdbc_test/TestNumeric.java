/**
 * @@@ START COPYRIGHT @@@
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * @@@ END COPYRIGHT @@
 */

import static org.junit.Assert.assertEquals;

import java.math.BigDecimal;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import org.junit.Test;

public class TestNumeric {

    @Test
    public void JDBCNumeric() throws SQLException {
        Connection conn = null;
        Statement stmt = null;
        PreparedStatement prepStmt = null;
        ResultSet rs = null;
        String sql = "upsert using load into numeric_tbl values (?,?);";
        try {
            conn = Utils.getUserConnection();
            stmt = conn.createStatement();
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists numeric_tbl (c0 int not null, c1 numeric(20,0))");
            stmt.executeUpdate("delete from numeric_tbl");

            prepStmt = conn.prepareStatement(sql);
            for (int i = 0; i < 1000; i++) {
                prepStmt.setInt(1, i);
                prepStmt.setBigDecimal(2, new BigDecimal(-1));
                prepStmt.addBatch();
            }
            prepStmt.executeBatch();

            rs = stmt.executeQuery("select count(*) from numeric_tbl where c1=-1;");
            int result = 0;
            while (rs.next()) {
                result = rs.getInt(1);
            }
            rs.close();
            assertEquals("Rows returned count should be 1000", 1000, result);
        } catch (SQLException e) {
            e.printStackTrace();
        } finally {
            stmt.close();
            prepStmt.close();
            conn.close();
        }

    }
    @Test
    public void JDBCNumeric2() throws SQLException {
        Connection conn = null;
        Statement stmt = null;
        PreparedStatement prepStmt = null;
        ResultSet rs = null;
        String sql = "upsert using load into numeric_tbl2 values (?,?);";
        try {
            conn = Utils.getUserConnection();
            stmt = conn.createStatement();
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists numeric_tbl2 (c0 int not null, c1 numeric(10,2))");
            stmt.executeUpdate("delete from numeric_tbl2");

            prepStmt = conn.prepareStatement(sql);
            for (int i = 0; i < 1000; i++) {
                prepStmt.setInt(1, i);
                prepStmt.setBigDecimal(2, new BigDecimal(-1));
                prepStmt.addBatch();
            }
            prepStmt.executeBatch();

            rs = stmt.executeQuery("select count(*) from numeric_tbl2 where c1=-1;");
            int result = 0;
            while (rs.next()) {
                result = rs.getInt(1);
            }
            rs.close();
            assertEquals("Rows returned count should be 1000", 1000, result);
        } catch (SQLException e) {
            e.printStackTrace();
        } finally {
            stmt.close();
            prepStmt.close();
            conn.close();
        }

    }
}

