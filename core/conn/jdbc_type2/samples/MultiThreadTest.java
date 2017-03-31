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
import java.io.*;

public class MultiThreadTest
{
    public static void main(String[] args)
    {
        int noOfThreads;
        if (args.length == 1)
            noOfThreads = Integer.parseInt(args[0]);
        else
            noOfThreads = 2;
        try
        {
            Class.forName("org.apache.trafodion.jdbc.t2.T2Driver");
        }
        catch (ClassNotFoundException e1)
        {
            e1.printStackTrace();
        }
        try
        {

            String createEmployee;

            createEmployee = "create table mt_employee " +
            "(Emp_Hire_Date varchar (30), " +
            "Emp_Name varchar(32), " +
            "Emp_ID int not null not droppable, " +
            "Emp_Address varchar(32), " +
            "Emp_City varchar(32), " +
            "Emp_Salary decimal(10,2), " +
            "Dept_ID int, primary key (Emp_ID))";

            Connection con = DriverManager.getConnection("jdbc:t2jdbc:");
            Statement stmt1 = con.createStatement();

            try
            {
                stmt1.executeUpdate("drop table mt_employee");  // Remove table if already present
            }
            catch (SQLException se1) {}                         // Will come here if table not present

            stmt1.executeUpdate(createEmployee);                // Now Create new table

            stmt1.executeUpdate("insert into mt_employee " +    // Add data to table
                                " values ('1/1/1900', " +
                                         "'John Doe', " +
                                         "1, "          +
                                         "'unknown', "  +
                                         " 'unknown', " +
                                         "1111110.09, " +
                                         "1 )");

            stmt1.executeUpdate("insert into mt_employee " +
                                " values ('2/2/1991', " +
                                         "'Jane Doe', " +
                                         "2, "          +
                                         "'unknown', "  +
                                         " 'unknown', " +
                                         "22220.99, "   +
                                         "1 )");

            stmt1.executeUpdate( "insert into mt_employee " +
                                 " values ('3/3/1903', "     +
                                          "'James Smith', "  +
                                          "3, "              +
                                          "'11 Pomoroy', "   +
                                          " 'Santa Clara', " +
                                          "400000.0, "       +
                                          "1 )");

            stmt1.executeUpdate("insert into mt_employee " +
                                " values ('1/1/1998', "   +
                                         "'Tim Thomas', " +
                                         "4, "            +
                                         "'11 mainst', "  +
                                         " 'San Jose', "  +
                                         "50000.0, "      +
                                         "1 )");

            stmt1.executeUpdate("insert into mt_employee " +
                                " values ('2/2/1999', "        +
                                          "'Jane Mars', "      +
                                          "5, "                +
                                          "'11 cala', "        +
                                          " 'San Francisco', " +
                                          "100000.0, "         +
                                          "1 )");

            stmt1.executeUpdate( "insert into mt_employee " +
                                " values ('4/3/1909', "      +
                                          "'Ken Thompson', " +
                                          "6, "              +
                                          "'44 Duke', "      +
                                          " 'Santa Cruz', "  +
                                          "14000.0, "        +
                                          "1 )");


            stmt1.close();
            con.close();
        }
        catch (SQLException e)
        {
            SQLException next;
            next = e;
            do
            {
                System.out.println("MultiThreadTest: Sample Program Failed during table creation.");
                System.out.println("MultiThreadTest: Message : " + e.getMessage());
                System.out.println("MultiThreadTest: Vendor Code : " + e.getErrorCode());
                System.out.println("MultiThreadTest: SQLState : " + e.getSQLState());
            }
            while ((next = next.getNextException()) != null);             // Retrieve next exception if any
        }
        Thread t;
        for (int i = 1 ; i <= noOfThreads ; i ++)
        {
            t = new jdbcThread("Thread " + i);                           // Create new tread object
            t.start();                                                   // Start running the new thread
        }
    }
}

// ***************************************************************************************
// *
// * Description: This is the thread class, and it will run a query on the table. The results
// *              from the query are displayed.
// *
// * CLASS: jdbcThread
// * INPUT: The number of the thread
// *
// ***************************************************************************************

class jdbcThread extends Thread
{
    public jdbcThread(String s)
    {
        super(s);
    }
    public void run()
    {
        boolean   rtn;
        ResultSet rs1;
        String    stmtSource1 = "select * from mt_employee";

        Connection conn1 = null;

        try
        {
            System.out.println("Establishing connection - " + getName());
            conn1 = DriverManager.getConnection("jdbc:t2jdbc:");
            System.out.println("Established connection - " + getName());

            System.out.println("Preparing a query - " + getName());
            PreparedStatement stmt1 = conn1.prepareStatement(stmtSource1);
            System.out.println("Prepared the query - "  + getName());

            System.out.println("Executing Query - "  + getName());
            rs1 = stmt1.executeQuery();
            System.out.println("Query Executed - "  + getName());

            DatabaseMetaData dbmd = conn1.getMetaData();

            System.out.println("Executing getColumns - "  + getName());
            ResultSet rs2 = dbmd.getColumns(null, null, "EMPLOYEE", "%");
            System.out.println("Executed getColumns - "  + getName());
            do                                                                // print the result set
            {
                System.out.println("Fetching a row - "  + getName());
                if ((rtn = rs1.next()) == true)
                {
                    System.out.println("Fetched a row - "  + getName());
                    System.out.println("rs1 C1 " + rs1.getString(1) +" - " + getName());
                    System.out.println("rs1 C2 " + rs1.getString(2) +" - " + getName());
                }
            } while (rtn);
            System.out.println("End of data - "  + getName());
            rs1.close();
            rs2.close();
            conn1.close();
        }
        catch (SQLException e)
        {
            SQLException next;
            next = e;
            do
            {
                System.out.println("Messge : " + e.getMessage());
                System.out.println("Vendor Code : " + e.getErrorCode());
                System.out.println("SQLState : " + e.getSQLState());
            }
            while ((next = next.getNextException()) != null);
            System.out.println("MultiThreadTest: " + getName() + ": Failed.");
        }
    }
}
