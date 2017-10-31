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
import static org.junit.Assert.assertTrue;

import java.sql.Connection;
import java.sql.SQLException;

import org.junit.Test;

public class TestWrap {

    @Test
    public void testIsWrapFor() {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            boolean result = conn.isWrapperFor(Connection.class);
            assertTrue("It is wrapper for this interface", result);
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    @Test(expected = SQLException.class)
    public void testConnectIsClose() throws SQLException {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            conn.close();
        } catch (SQLException e) {
            e.printStackTrace();
        }
        conn.isWrapperFor(Connection.class) ;
    }

    @Test
    public void testUnwrap() {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            boolean result = conn.unwrap(Connection.class) instanceof Connection;
            assertTrue("It is unwrape for this interface", result);
            result = conn.unwrap(Connection.class) instanceof TestWrap;
            assertTrue("It is unwrape for this interface", !result);
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }
}
