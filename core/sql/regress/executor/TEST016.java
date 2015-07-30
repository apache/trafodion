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

import java.sql.*;
import java.lang.reflect.*;

class TEST016
{

  public static boolean isWindows()
  {
    String os = System.getProperty("os.name");
    os = os.toUpperCase();
    if (os.startsWith("WINDOWS"))
    {
      return true;
    }
    return false;
  }

  // This method calls into the com.tandem.sqlmx.LmUtility class
  public static void lmGateway(String action, String[] status)
    throws Throwable
  {
    try
    {
      Class[] formalTypes = new Class[2];
      formalTypes[0] = action.getClass();
      formalTypes[1] = status.getClass();

      Object[] args = new Object[2];
      args[0] = action;
      args[1] = status;

      Class lmClass = Class.forName("com.tandem.sqlmx.LmUtility");
      Object o = lmClass.newInstance();
      Method m = lmClass.getMethod("utils", formalTypes);
      Object txName = m.invoke(o, args);
    }
    catch (InvocationTargetException e)
    {
      throw e.getTargetException();
    }
  }

  // SPJ method to return "select * from" a given table
  public static void rs016(String tabname, ResultSet[] rs)
    throws Throwable
  {
    String stmtString = "select * from " + tabname + " order by a";

    if (!isWindows())
    {
      // Execute a Select query
      Connection conn = DriverManager.getConnection("jdbc:default:connection");
      Statement stmt = conn.createStatement();
      rs[0] = stmt.executeQuery(stmtString);
    }
    else
    {
      String[] status = new String[1];
      lmGateway("PutEnv RS_SQL_STMT_1=" + stmtString, status);
    }
  }

}
