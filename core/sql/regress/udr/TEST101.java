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

class TEST101
{
  public static void putenv(String name, String value)
    throws Throwable
  {
    String[] status = new String[1];
    String cmd = "PutEnv " + name + "=";
    value = value.trim();
    if (value.length() > 0)
      cmd += value;
    lmGateway(cmd, status);
  }

  public static void clearRS() throws Throwable
  {
    if (isWindows())
      for (int i = 1; i <= 255; i++)
        putenv("RS_SQL_STMT_" + i, "");
  }

  public static ResultSet makeRS(String s, int i) throws Throwable
  {
    if (!isWindows())
    {
      Connection conn = DriverManager.getConnection("jdbc:default:connection");
      PreparedStatement ps = conn.prepareStatement(s);
      return ps.executeQuery();
    }
    else
    {
      putenv("RS_SQL_STMT_" + i, s);
      return null;
    }
  }

  public static void rs3a(String s1, String s2, String s3, String[] status)
    throws Throwable
  {
    status[0] = "OK";
  }

  public static void rs3(String s1, String s2, String s3, String[] status,
                         ResultSet[] rs1, ResultSet[] rs2, ResultSet[] rs3)
    throws Throwable
  {
    status[0] = "OK";
    rs1[0] = makeRS(s1, 1);
    rs2[0] = makeRS(s2, 2);
    rs3[0] = makeRS(s3, 3);
  }

  // This method calls into the org.trafodion.sql.udr.LmUtility class
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

      Class lmClass = Class.forName("org.trafodion.sql.udr.LmUtility");
      Object o = lmClass.newInstance();
      Method m = lmClass.getMethod("utils", formalTypes);
      Object txName = m.invoke(o, args);
    }
    catch (InvocationTargetException e)
    {
      throw e.getTargetException();
    }
  }

  // This method calls into the org.trafodion.sql.udr.LmUtility class
  public static void lmRSGateway(String action, String[] status,
                                 java.sql.ResultSet[] rs)
    throws Throwable
  {
    try
    {
      Class[] formalTypes = new Class[3];
      formalTypes[0] = action.getClass();
      formalTypes[1] = status.getClass();
      formalTypes[2] = rs.getClass();

      Object[] args = new Object[3];
      args[0] = action;
      args[1] = status;
      args[2] = rs;

      Class lmClass = Class.forName("org.trafodion.sql.udr.LmUtility");
      Object o = lmClass.newInstance();
      Method m = lmClass.getMethod("rsUtils", formalTypes);
      Object txName = m.invoke(o, args);
    }
    catch (InvocationTargetException e)
    {
      throw e.getTargetException();
    }
  }

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

  static long transIdCount_ = 0;
  public static void Xact(String action, String[] status)
    throws Throwable
  {
    lmGateway(action, status);

    if (action.trim().equalsIgnoreCase("GetTxName"))
    {
      // We do not actually return the TMF trans ID. We increment a
      // counter each time we see a different trans ID and return a
      // string containing value of the counter and the actual trans
      // ID. We store assocations between trans IDs and counter values
      // in system properties.
      String txName = status[0];
      if (txName.equals("null thread"))
      {
        Exception e;
        e = new Exception("No active transaction");
        throw e;
      }
      else
      {
        String prop = System.getProperty(txName);
        if (prop != null)
        {
          status[0] = prop;
        }
        else
        {
          transIdCount_++;
          status[0] = "[UdrTransId " + transIdCount_ + "] " + txName;
          System.setProperty(txName, status[0]);
        }
      }
    }
  }

  public static void orderSummary(String onOrAfterDate,
                                  long[] numOrders,
                                  ResultSet[] rs1,
                                  ResultSet[] rs2)
    throws Throwable
  {
    String[] lmStatus = new String[1];

    String param = "'" + onOrAfterDate + "'";

    Connection conn = null;
    if (!isWindows())
      conn = DriverManager.getConnection("jdbc:default:connection");

    // Get the number of orders on or after this date
    String selectList = "count(ordernum)";
    if (isWindows())
      selectList = "cast(count(ordernum) as char(10))";

    String query1 =
      " SELECT " + selectList + " FROM trafodion.spjrs.orders " +
      " WHERE  order_date >= cast(" + param + " as date) ";


    if (!isWindows())
    {
      Statement stmt = conn.createStatement();
      stmt.execute
          ("control query default robust_query_optimization 'minimum';");
      PreparedStatement s1 = conn.prepareStatement(query1);
      ResultSet rs = s1.executeQuery();
      rs.next();
      numOrders[0] = rs.getLong(1);
      rs.close();
    }
    else
    {
      String stmtTxt =
          "control query default robust_query_optimization 'minimum';";
      lmGateway("ExecSql " + stmtTxt, lmStatus);
      // Call the LmUtility class to retrieve values from SQL/MX via a
      // dynamically prepared SELECT statement. The output columns of
      // this one-row SELECT are returned as one long multi-line
      // string with the delimiter "\n" separating the values.
      lmGateway("FetchSql " + query1, lmStatus);
      numOrders[0] = Long.parseLong(lmStatus[0].trim());
    }

    // Open a result set for <order num, order info> rows
    String query2 =
      " SELECT    AMOUNTS.*, ORDERS.order_date, EMPS.last_name " +
      " FROM      ( select o.ordernum, count(d.partnum) as num_parts, " +
      "               sum(d.unit_price * d.qty_ordered) as amount " +
      "             from   trafodion.spjrs.orders o, trafodion.spjrs.odetail d " +
      "             where  o.ordernum = d.ordernum " +
      "               and  o.order_date >= cast(" + param + " as date) " +
      "             group by o.ordernum ) AMOUNTS, " +
      "           trafodion.spjrs.orders ORDERS, trafodion.spjrs.employee EMPS " +
      " WHERE     AMOUNTS.ordernum = ORDERS.ordernum " +
      "   AND     ORDERS.salesrep = EMPS.empnum " +
      " ORDER BY  ORDERS.ordernum ";
    if (!isWindows())
    {
      PreparedStatement s2 = conn.prepareStatement(query2);
      rs1[0] = s2.executeQuery();
    }
    else
    {
      lmGateway("PutEnv RS_SQL_STMT_1=" + query2, lmStatus);
    }

    // Open a result set for order detail rows
    String query3 =
      " SELECT    D.*, P.partdesc " +
      " FROM      trafodion.spjrs.odetail D, trafodion.spjrs.parts P, " +
      "             trafodion.spjrs.orders O " +
      " WHERE     D.partnum = P.partnum AND D.ordernum = O.ordernum " +
      "   AND     O.order_date >= cast(" + param + " as date) " +
      " ORDER BY  D.ordernum, P.partnum ";
    if (!isWindows())
    {
      PreparedStatement s3 = conn.prepareStatement(query3);
      rs2[0] = s3.executeQuery();
    }
    else
    {
      lmGateway("PutEnv RS_SQL_STMT_2=" + query3, lmStatus);
    }
  }

  public static void partData(int partNum,
                              String[] partDescription,
                              double[] unitPrice,
                              int[] qtyAvailable,
                              ResultSet[] rs1,
                              ResultSet[] rs2,
                              ResultSet[] rs3,
                              ResultSet[] rs4)
    throws Throwable
  {
    String[] lmStatus = new String[1];

    String param = Integer.toString(partNum);

    Connection conn = null;
    if (!isWindows())
      conn = DriverManager.getConnection("jdbc:default:connection");

    // Part description, unit price, and quantity available are
    // returned as output parameters
    String selectList = "partdesc, price, qty_available";
    if (isWindows())
      selectList =
        "partdesc, cast(price as char(20)), " +
        "cast(qty_available as char(20))";

    String query1 =
      " SELECT " + selectList +
      " FROM trafodion.spjrs.parts WHERE partnum = " + param;

    if (!isWindows())
    {
      PreparedStatement s1 = conn.prepareStatement(query1);
      ResultSet rs = s1.executeQuery();
      rs.next();
      partDescription[0] = rs.getString(1);
      unitPrice[0] = rs.getDouble(2);
      qtyAvailable[0] = rs.getInt(3);
      rs.close();
    }
    else
    {
      // Call the LsmUtility class to retrieve values from SQL/MX via a
      // dynamically prepared SELECT statement. The output columns of
      // this one-row SELECT are returned as one long multi-line
      // string with the delimiter "\n" separating the values.
      lmGateway("FetchSql " + query1, lmStatus);
      String[] values = lmStatus[0].split("\n");
      partDescription[0] = values[0].trim();
      unitPrice[0] = Double.parseDouble(values[1].trim());
      qtyAvailable[0] = Integer.parseInt(values[2].trim());
    }

    // Return a result set of rows from the ORDERS table listing orders
    // that included this part. Each ORDERS row is augmented with the
    // quantity of this part that was ordered.
    String query2 =
      " SELECT    O.*, QTY.QTY_ORDERED " +
      " FROM      trafodion.spjrs.orders O, " +
      "           ( select    ordernum, sum(qty_ordered) as QTY_ORDERED " +
      "             from      trafodion.spjrs.odetail " +
      "             where     partnum = " + param + " " +
      "             group by  ordernum) QTY " +
      " WHERE     O.ordernum = QTY.ordernum " +
      " ORDER BY  O.ordernum ";
    if (!isWindows())
    {
      PreparedStatement s2 = conn.prepareStatement(query2);
      rs1[0] = s2.executeQuery();
    }
    else
    {
      lmGateway("PutEnv RS_SQL_STMT_1=" + query2, lmStatus);
    }

    // Return a result set of rows from the PARTLOC table listing
    // locations that have this part in stock and the quantity they
    // have on hand.
    String query3 =
      "select * from trafodion.spjrs.partloc where partnum = " + param +
      " order by LOC_CODE";
    if (!isWindows())
    {
      PreparedStatement s3 = conn.prepareStatement(query3);
      rs2[0] = s3.executeQuery();
    }
    else
    {
      lmGateway("PutEnv RS_SQL_STMT_2=" + query3, lmStatus);
    }

    // Return a result set of rows from the PARTSUPP table listing
    // suppliers who supply this part.
    String query4 =
      "select * from trafodion.spjrs.partsupp where partnum = " + param +
      " order by PARTNUM, SUPPNUM";
    if (!isWindows())
    {
      PreparedStatement s4 = conn.prepareStatement(query4);
      rs3[0] = s4.executeQuery();
    }
    else
    {
      lmGateway("PutEnv RS_SQL_STMT_3=" + query4, lmStatus);
    }

    // Return a result set of rows from the EMPLOYEE table listing
    // sales reps that have sold this part.
    String query5 =
      " SELECT    * " +
      " FROM      trafodion.spjrs.employee " +
      " WHERE     empnum in ( select O.salesrep " +
      "                       from   trafodion.spjrs.orders O, " +
      "                              trafodion.spjrs.odetail D " +
      "                       where  D.partnum = " + param + " " +
      "                         and  O.ordernum = D.ordernum ) " +
      " ORDER BY  empnum ";
    if (!isWindows())
    {
      PreparedStatement s5 = conn.prepareStatement(query5);
      rs4[0] = s5.executeQuery();
    }
    else
    {
      lmGateway("PutEnv RS_SQL_STMT_4=" + query5, lmStatus);
    }
  }

  public static void test101_2rs_errwarn(String cmd1, String cmd2,
                                         ResultSet[] rs1, ResultSet[] rs2)
  throws Throwable
  {
    Connection conn = null;
    if (!isWindows())
      conn = DriverManager.getConnection("jdbc:default:connection");

    String cmd, stmt_text = null;

    for (int i=1; i<=2; i++)
    {
      cmd = (i==1) ? cmd1.trim() : cmd2.trim();

      if (cmd.equalsIgnoreCase("GOOD"))
      {
        stmt_text = "select a, b, c from trafodion.sch.test101_t1";
      }
      else if (cmd.equalsIgnoreCase("ERROR"))
      {
        stmt_text = "select b/a, c from trafodion.sch.test101_t1";
      }
      else if (cmd.equalsIgnoreCase("WARNING"))
      {
        stmt_text = "select a, b, cast (c as char(3)) from trafodion.sch.test101_t1";
      }
      else if (cmd.equalsIgnoreCase("WARNINGERROR"))
      {
        stmt_text = "select b/a, cast (c as char(3)) from trafodion.sch.test101_t1";
      }

      if (i == 1)
        rs1[0] = makeRS(stmt_text, 1);
      else
        rs2[0] = makeRS(stmt_text, 2);
    }
  }

  public static void test101_singleRowFetch(String flag)
  throws java.lang.Throwable
  {
    if (flag.trim().equalsIgnoreCase("TRUE"))
      putenv("UDR_RS_FETCH_SIZE", "1");
    else if (flag.trim().equalsIgnoreCase("FALSE"))
      putenv("UDR_RS_FETCH_SIZE", "");
  }

  // Following methods are used in SPJs executing parallel plans
  public static ResultSet makeRS_p(String s, int i) throws Throwable
  {
    if (!isWindows())
    {
      Connection conn = DriverManager.getConnection("jdbc:default:connection");

      // force parallel plan
      Statement stmtc = conn.createStatement();
      stmtc.execute ("control query shape esp_exchange(cut);");

      PreparedStatement ps = conn.prepareStatement(s);
      return ps.executeQuery();
    }
    else
    {
      putenv("RS_SQL_STMT_" + i, s);
      return null;
    }
  }

  public static void rs1p(String s1, String[] status,
                          ResultSet[] rs1)
    throws Throwable
  {
    status[0] = "OK";
    rs1[0] = makeRS_p(s1, 1);
  }

  public static void rs3p(String s1, String s2, String s3, String[] status,
                          ResultSet[] rs1, ResultSet[] rs2, ResultSet[] rs3)
    throws Throwable
  {
    status[0] = "OK";
    rs1[0] = makeRS_p(s1, 1);
    rs2[0] = makeRS_p(s2, 2);
    rs3[0] = makeRS_p(s3, 3);
  }

  public static void rs0p(String s1, String s2, String s3, String[] status)
    throws Throwable
  {
    status[0] = "OK";
    ResultSet rs1 = makeRS_p(s1, 1);
    ResultSet rs2 = makeRS_p(s2, 2);
    ResultSet rs3 = makeRS_p(s3, 3);
  }

}
