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

import java.net.*;
import java.sql.*;

import org.junit.Test;
import static org.junit.Assert.*;


/*  The test case is added for bug #1451707
 *  Call ResultSet.next() function failed when expect to select 200k utf8 column size from a table
 *
 */
public class TestBigColumnSize {
    @Test
    public void test$200KColSizeWithUTF8() {
        int iRet = 0;
        String sql = "";
        ResultSet rs = null;
        ResultSetMetaData rsMD = null;
        String type = "";

        try (
                Connection conn =  Utils.getUserConnection();
                Statement stmt = conn.createStatement();
                )
        {
            assertNotNull(conn);
            assertNotNull(stmt);
            
            iRet = stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            assertEquals(0, iRet);
            sql = "drop table if exists tblcolumnsize200kWithUTF8";
            iRet = stmt.executeUpdate(sql);
            if (iRet != 0) { iRet = stmt.executeUpdate("cleanup table tblcolumnsize200kWithUTF8"); }
            assertEquals(0, iRet);
            sql = "create table tblcolumnsize200kWithUTF8(c1 varchar(50000) character set UTF8 collate default null, c2 varchar(50000) character set UTF8 collate default null)";
            iRet = stmt.executeUpdate(sql);
            assertEquals(0, iRet);

            String a = "";
            String b = "";

            for (int i = 1; i < 50000 - 1; i++) {
                a += "a";
                b += "b";
            }
            a += "EE";
            b += "EE";

            a = new String(a.getBytes(), "UTF-8");
            b = new String(b.getBytes(), "UTF-8");

            sql = "insert into tblcolumnsize200kWithUTF8(c1, c2) values('" + a + "', '" + b +"')";
            iRet = stmt.executeUpdate(sql);
            assertEquals(1, iRet);

            sql = "select left(rtrim(t.c1), 50000) as o1 from tblcolumnsize200kWithUTF8 as t";

            rs = stmt.executeQuery(sql);

            rsMD = rs.getMetaData();
            assertEquals(1, rsMD.getColumnCount());
            type = rsMD.getColumnTypeName(1);
            assertTrue(type.equals("VARCHAR"));

            rs.next();
            assertEquals(50000, (URLDecoder.decode(rs.getObject(1).toString(), "UTF-8")).length());

            assertEquals(rs.getString(1), a);

            sql = "select left(rtrim(t.c2), 50000) as o1 from tblcolumnsize200kWithUTF8 as t";
            rs = stmt.executeQuery(sql);
            rsMD = rs.getMetaData();
            assertEquals(1, rsMD.getColumnCount());
            type = rsMD.getColumnTypeName(1);
            assertTrue(type.equals("VARCHAR"));
            rs.next();
            assertEquals(50000, rs.getObject(1).toString().length());
            assertEquals(rs.getString(1), b);

            rs.close();

            System.out.println("200KColSizeWithUTF8 : Pass");
        } catch (Exception e) {
            System.out.println(e.toString());
            System.out.println(e.getMessage());
            e.printStackTrace();
            fail("exception in test test$200KColSizeWithUTF8 in TestBigCollumnSize .." + e.getMessage());
        }
    }

    @Test
    public void test$32KColSizeWithUTF8() {
        int iRet = 0;
        String sql = "";
        ResultSet rs = null;
        ResultSetMetaData rsMD = null;
        String type = "";

        try (
                Connection conn =  Utils.getUserConnection();
                Statement stmt = conn.createStatement();
                )
        {
            System.out.println("---");
            assertNotNull(conn);
            assertNotNull(stmt);
            
            iRet = stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            assertEquals(0, iRet);
            sql = "drop table if exists tblcolumnsize32kWithUTF8";
            iRet = stmt.executeUpdate(sql);
            if (iRet != 0) { iRet = stmt.executeUpdate("cleanup table tblcolumnsize32kWithUTF8"); }
            assertEquals(0, iRet);
            sql = "create table tblcolumnsize32kWithUTF8(c1 varchar(8000) character set UTF8 collate default null, c2 varchar(8000) character set UTF8 collate default null)";
            iRet = stmt.executeUpdate(sql);
            assertEquals(0, iRet);

            String a = "";
            String b = "";

            for (int i = 0; i < 8000 - 2; i++) {
                a += "a";
                b += "b";
            }

            a += "EE";
            b += "EE";

            a = new String(a.getBytes(), "UTF-8");
            b = new String(b.getBytes(), "UTF-8");

            sql = "insert into tblcolumnsize32kWithUTF8(c1, c2) values('" + a + "', '" + b +"')";
            iRet = stmt.executeUpdate(sql);
            assertEquals(1, iRet);

            sql = "select left(rtrim(t.c1), 8000) as o1 from tblcolumnsize32kWithUTF8 as t";

            rs = stmt.executeQuery(sql);

            rsMD = rs.getMetaData();
            assertEquals(1, rsMD.getColumnCount());
            type = rsMD.getColumnTypeName(1);
            assertTrue(type.equals("VARCHAR"));

            rs.next();
            assertEquals(8000, (URLDecoder.decode(rs.getObject(1).toString(), "UTF-8")).length());

            assertEquals(rs.getString(1), a);

            sql = "select left(rtrim(t.c2), 8000) as o1 from tblcolumnsize32kWithUTF8 as t";
            rs = stmt.executeQuery(sql);
            rsMD = rs.getMetaData();
            assertEquals(1, rsMD.getColumnCount());
            type = rsMD.getColumnTypeName(1);
            assertTrue(type.equals("VARCHAR"));
            rs.next();
            assertEquals(8000, rs.getObject(1).toString().length());
            assertEquals(rs.getString(1), b);

            rs.close();

            System.out.println("32KColSizeWithUTF8 : Pass");
        } catch (Exception e) {
            System.out.println(e.toString());
            System.out.println(e.getMessage());
            e.printStackTrace();
            fail("exception in test test$32KColSizeWithUTF8 in TestBigCollumnSize .." + e.getMessage());
        }
    }

}
