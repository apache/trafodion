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
import java.util.Properties;

import org.junit.Test;

public class TestClientInfo {
    @Test
    public void testGetClientInfoProperties() {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            Properties clientInfo = conn.getClientInfo();
            assertTrue("this is not set clientInfo Properties", clientInfo.equals(new Properties()));
            System.out.println(clientInfo);
            conn.close();
        } catch (SQLException e) {
            if (conn != null) {
                try {
                    conn.close();
                } catch (SQLException e1) {
                    e1.printStackTrace();
                }
            }
            e.printStackTrace();
        }
    }

    @Test
    public void testSetAndGetClientInfoProperties() {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            Properties prop = new Properties();
            prop.setProperty("user1", "user1");
            prop.setProperty("user2", "user2");
            conn.setClientInfo(prop);
            Properties prop1 = conn.getClientInfo();
            assertTrue("this is set and get  ClientInfoProperties", prop1.equals(prop));
            System.out.println(prop1.equals(prop));
            conn.close();
        } catch (SQLException e) {
            if (conn != null) {
                try {
                    conn.close();
                } catch (SQLException e1) {
                    e1.printStackTrace();
                }
            }
            e.printStackTrace();
        }
    }
    @Test
    public void testSetAndGetClientInfoByName() {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            conn.setClientInfo("user1", "user1");
            String user1 = conn.getClientInfo("user1");
            System.out.println(user1);
            assertTrue("this is set clientInfo by Name ", user1.equals("user1"));
            conn.close();
        } catch (SQLException e) {
            if (conn != null) {
                try {
                    conn.close();
                } catch (SQLException e1) {
                    e1.printStackTrace();
                }
            }
            e.printStackTrace();
        }
    }
    @Test
    public void testSetAndGetClientInfoByName1() {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            Properties prop = new Properties();
            prop.setProperty("user2", "user2");
            conn.setClientInfo(prop);
            conn.setClientInfo("user1", "user1");
            String user1 = conn.getClientInfo("user1");
            String user2 = conn.getClientInfo("user2");
            String user3 = conn.getClientInfo("user3");
            System.out.println(user1);
            System.out.println(user2);
            System.out.println(user3);
           // assertTrue("this is get clientInfo by Name", user1.equals("user1"));
           // assertTrue("this is get clientInfo by Name", user2.equals("user2"));
            assertTrue("this is get clientInfo by Name", user3==(null));
            conn.close();
        } catch (SQLException e) {
            if (conn != null) {
                try {
                    conn.close();
                } catch (SQLException e1) {
                    e1.printStackTrace();
                }
            }
            e.printStackTrace();
        }
    }
}
