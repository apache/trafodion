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

import java.sql.*;
import java.util.*;
import java.io.*;
import org.junit.Test;
import static org.junit.Assert.*;

import static org.junit.Assert.fail;

/*  The test case is added for bug #1452993;
 *  T2 don't read the property file from System Properties but T4 do it.
 *
 *  The test need run with a property file, like t2prop.
 *  java -Dprop=t2prop PropTes
 */
public class PropTest
{
    @Test
    public void  testDefaultPropertiesConnection() throws SQLException {
        Connection conn = null;
        try {
            conn = Utils.getUserConnection();
        }
        catch (Exception e) {
            fail("failed to create connection" + e.getMessage());
        }
        try {
            // The option -Dproperties=propFile can be used to instead of System.setProperty()
            System.setProperty("properties", System.getProperty("trafjdbc.properties"));
            System.out.println("Catalog : " + conn.getCatalog());
            assertEquals("Catalog should be the same as the properties file defined",Utils.catalog, conn.getCatalog());
            System.out.println("testDefaultPropertiesConnection : PASS");
            conn.close();
        }
        catch (Exception e) {
            if (conn != null) {
                try {
                    conn.close();
                } catch (SQLException e1) {
                    e1.printStackTrace();
                }
            }
        }
    }
}
