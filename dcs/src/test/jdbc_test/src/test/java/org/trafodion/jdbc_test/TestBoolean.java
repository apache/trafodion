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

public class TestBoolean {

    @Test
    public void JDBCBoolean() throws SQLException {
        Connection conn = null;
        Statement stmt = null;
        PreparedStatement prepStmt = null;
        ResultSet rs = null;
        String sql = "insert into boolean_tbl values (?,?);";
        try {
            conn = Utils.getUserConnection();
            stmt = conn.createStatement();
            stmt.executeUpdate("cqd traf_boolean_io 'ON'");
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists boolean_tbl (c0 int not null, c1 boolean)");
            stmt.executeUpdate("delete from boolean_tbl");

            prepStmt = conn.prepareStatement(sql);
            prepStmt.setInt(1, 1);
            prepStmt.setBoolean(2, true);
            prepStmt.addBatch();

            prepStmt.setInt(1, 2);
            prepStmt.setBoolean(2, false);
            prepStmt.addBatch();

            prepStmt.executeBatch();

            rs = stmt.executeQuery("select c1 from boolean_tbl;");
            boolean[] result = new boolean[2];
            int i = 0;
            while (rs.next()) {
                result[i] = Boolean.valueOf(rs.getObject(1).toString());
                i++;
            }
            rs.close();
            assertEquals("Rows one returned true", true, result[0]);
            assertEquals("Rows two returned false", false, result[1]);
        } catch (SQLException e) {
            e.printStackTrace();
            assertNull(e.getMessage());
        } finally {
            if (stmt != null) {
                stmt.close();
            }

            if (prepStmt != null) {
                prepStmt.close();
            }

            if (conn != null) {
                conn.close();
            }
        }

    }

}
