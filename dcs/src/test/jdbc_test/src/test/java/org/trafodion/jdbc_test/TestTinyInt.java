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
import static org.junit.Assert.assertNull;

import java.math.BigDecimal;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import org.junit.Test;

public class TestTinyInt {

    @Test
    public void JDBCTinyIntSigned() throws SQLException {
        Connection conn = null;
        Statement stmt = null;
        PreparedStatement prepStmt = null;
        ResultSet rs = null;
        String sql = "insert into tinyint_signed_tbl values (?,?);";
        try {
            conn = Utils.getUserConnection();
            stmt = conn.createStatement();
            stmt.executeUpdate("cqd traf_tinyint_return_values 'ON'");//
            stmt.executeUpdate("cqd traf_tinyint_input_params 'ON'");//
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists tinyint_signed_tbl (c0 int not null, c1 tinyint signed)");
            stmt.executeUpdate("delete from tinyint_signed_tbl");

            prepStmt = conn.prepareStatement(sql);

            prepStmt.setInt(1, 1);
            prepStmt.setObject(2, -128);
            prepStmt.addBatch();

            prepStmt.setInt(1, 2);
            prepStmt.setObject(2, 127);
            prepStmt.addBatch();

            prepStmt.executeBatch();

            rs = stmt.executeQuery("select c1 from tinyint_signed_tbl;");
            Object[] result = new Object[2];
            int i = 0;
            while (rs.next()) {
                result[i] = Integer.valueOf(rs.getObject(1).toString());
                i++;
            }
            rs.close();
            assertEquals("Rows one returned -128", -128, result[0]);
            assertEquals("Rows two returned 127", 127, result[1]);
        } catch (SQLException e) {
            e.printStackTrace();
            assertNull(e.getMessage());
        } finally {
            if (stmt != null) {
                try {
                    stmt.close();
                }
                catch (SQLException e) {
                }
            }
            if (prepStmt != null) {
                try {
                prepStmt.close();
                }
                catch (SQLException e){
                }
            }

            if (conn != null) {
                conn.close();
            }
        }
    }

    @Test
    public void JDBCTinyIntUnsigned() throws SQLException {
        Connection conn = null;
        Statement stmt = null;
        PreparedStatement prepStmt = null;
        ResultSet rs = null;
        String sql = "insert into tinyint_unsigned_tbl values (?,?);";
        try {
            conn = Utils.getUserConnection();
            stmt = conn.createStatement();
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists tinyint_unsigned_tbl (c0 int not null, c1 tinyint unsigned)");
            stmt.executeUpdate("delete from tinyint_unsigned_tbl");

            prepStmt = conn.prepareStatement(sql);

            prepStmt.setInt(1, 1);
            prepStmt.setObject(2, 0);
            prepStmt.addBatch();

            prepStmt.setInt(1, 2);
            prepStmt.setObject(2, 255);
            prepStmt.addBatch();

            prepStmt.executeBatch();

            rs = stmt.executeQuery("select c1 from tinyint_unsigned_tbl;");
            Object[] result = new Object[2];
            int i = 0;
            while (rs.next()) {
                result[i] = rs.getObject(1);
                i++;
            }
            rs.close();
            assertEquals("Rows one returned 0", 0, result[0]);
            assertEquals("Rows two returned 255", 255, result[1]);
        } catch (SQLException e) {
            e.printStackTrace();
            assertNull(e.getMessage());
        } finally {
            if (stmt != null) {
                try {
                    stmt.close();
                }
                catch (SQLException e){
                }
            }

            if (prepStmt != null) {
                try {
                    prepStmt.close();
                }
                catch (SQLException e) {
                }
            }

            if (conn != null) {
                conn.close();
            }
        }
    }
}
