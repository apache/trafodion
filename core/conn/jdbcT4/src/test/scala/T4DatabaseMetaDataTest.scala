import java.sql._

import org.junit.{Test, Assert}

/**
 * Created by kaihua.xu on 2016/5/26.
 */
class T4DatabaseMetaDataTest {
  val url: String = "jdbc:t4jdbc://192.168.0.34:23400/:"
  val driverClass: String = "org.trafodion.jdbc.t4.T4Driver"
  val userName: String = "trafodion"
  val pwd: String = "traf123"

  @Test
  @throws(classOf[SQLException])
  @throws(classOf[ClassNotFoundException])
  def procedure {
    Class.forName(driverClass)
    val conn: Connection = DriverManager.getConnection(url, userName, pwd)
    try {
      val md: DatabaseMetaData = conn.getMetaData
      val rs: ResultSet = md.getProcedures("trafodion", "_LIBMGR_", null)
      val rsmd = rs getMetaData

      while(rs.next()) {
        for(i <- 1 to rsmd.getColumnCount){
          print(rs.getObject(i)+",")
        }
        println()
      }
      Assert.assertTrue(true)
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
    Class.forName(driverClass)
    val conn: Connection = DriverManager.getConnection(url, userName, pwd)
    try {
      val md: DatabaseMetaData = conn.getMetaData
      val rs: ResultSet = md.getProcedureColumns("trafodion","_LIBMGR_",null,null)
      val rsmd = rs getMetaData

      while(rs.next()) {
        for(i <- 1 to rsmd.getColumnCount){
          print(rs.getObject(i)+",")
        }
        println()
      }
      Assert.assertTrue(true)
    } finally {
      if (conn != null) {
        conn.close
      }
    }
  }
}
