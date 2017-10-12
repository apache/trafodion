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

import static org.junit.Assert.*;

import java.math.BigDecimal;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import org.junit.Test;

public class TestLargeInt {

    @Test
    public void JDBCLargeIntSigned() throws SQLException {
        ResultSet rs = null;
        String sql = "insert into largeint_signed_tbl values (?,?);";
        try (
                Connection conn = Utils.getUserConnection();
                Statement stmt = conn.createStatement();
                PreparedStatement prepStmt = conn.prepareStatement(sql);
                )
        {
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists largeint_signed_tbl (c0 int not null, c1 largeint signed)");
            stmt.executeUpdate("delete from largeint_signed_tbl");

            prepStmt.setInt(1, 1);
            prepStmt.setObject(2, Long.MIN_VALUE);
            prepStmt.addBatch();

            prepStmt.setInt(1, 2);
            prepStmt.setObject(2, Long.MAX_VALUE);
            prepStmt.addBatch();

            prepStmt.executeBatch();

            rs = stmt.executeQuery("select c1 from largeint_signed_tbl;");
            Object[] result = new Object[2];
            int i = 0;
            while (rs.next()) {
                result[i] = rs.getObject(1);
                i++;
            }
            rs.close();
            assertEquals("Rows one returned -9223372036854775808", -9223372036854775808l, result[0]);
            assertEquals("Rows two returned 9223372036854775807", 9223372036854775807l, result[1]);
        } catch (SQLException e) {
            e.printStackTrace();
            fail("exception in test JDBCLargeIntSigned .. " + e.getMessage());
            assertNull(e.getMessage());
        }
    }

    @Test
    public void JDBCLargeIntUnsigned() throws SQLException {
        ResultSet rs = null;
        String sql = "insert into largeint_unsigned_tbl values (?,?);";
        try (
                Connection conn = Utils.getUserConnection();
                Statement stmt = conn.createStatement();
                PreparedStatement prepStmt = conn.prepareStatement(sql);
                )
        {
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists largeint_unsigned_tbl (c0 int not null, c1 largeint unsigned)");
            stmt.executeUpdate("delete from largeint_unsigned_tbl");

            prepStmt.setInt(1, 1);
            prepStmt.setObject(2, 0);
            prepStmt.addBatch();

            prepStmt.setInt(1, 2);
            BigDecimal maxbd = new BigDecimal(Long.MAX_VALUE);
            maxbd = maxbd.add(maxbd).add(BigDecimal.valueOf(1));
            prepStmt.setObject(2, maxbd);
            prepStmt.addBatch();

            prepStmt.executeBatch();

            rs = stmt.executeQuery("select c1 from largeint_unsigned_tbl;");
            Object[] result = new Object[2];
            int i = 0;
            while (rs.next()) {
                result[i] = rs.getObject(1);
                i++;
            }
            rs.close();
            assertEquals("Rows one returned 0", BigDecimal.valueOf(0), result[0]);
            assertEquals("Rows two returned 18446744073709551615", maxbd.toString(), result[1].toString());
        } catch (SQLException e) {
            e.printStackTrace();
            fail("exception in test JDBCLargeIntSigned .. " + e.getMessage());
            assertNull(e.getMessage());
        }
    }
}
