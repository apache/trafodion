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
package org.trafodion.jdbc.t4

import java.sql._

import org.junit.{Assert, Test}

class T4DatabaseMetaDataTest {
  val url: String = "jdbc:t4jdbc://localhost:23400/:"
  val driverClass: String = "org.trafodion.jdbc.t4.T4Driver"
  val userName: String = "trafodion"
  val pwd: String = "traf123"
  Class.forName(driverClass)

  @Test
  @throws(classOf[SQLException])
  @throws(classOf[ClassNotFoundException])
  def procedure {
    val conn: Connection = DriverManager.getConnection(url, userName, pwd)
    try {
      val md: DatabaseMetaData = conn.getMetaData
      val rs: ResultSet = md.getProcedures("trafodion", "_LIBMGR_", null)
      val rsmd = rs getMetaData
      var flag = false
      while(rs.next()) {
        for(i <- 1 to rsmd.getColumnCount){
          print(rs.getObject(i)+",")
        }
        println()
        flag = true
      }
      Assert.assertTrue(flag)
    } finally {
      if (conn != null) {
        conn.close
      }
    }
  }

  @Test
  @throws(classOf[SQLException])
  @throws(classOf[ClassNotFoundException])
  def procedureColumns {
    val conn: Connection = DriverManager.getConnection(url, userName, pwd)
    try {
      val md: DatabaseMetaData = conn.getMetaData
      val rs: ResultSet = md.getProcedureColumns("trafodion","_LIBMGR_",null,null)
      val rsmd = rs getMetaData
      var flag = false
      while(rs.next()) {
        for(i <- 1 to rsmd.getColumnCount){
          print(rs.getObject(i)+",")
        }
        println()
        flag = true
      }
      Assert.assertTrue(flag)
    } finally {
      if (conn != null) {
        conn.close
      }
    }
  }
}
