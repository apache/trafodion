// The EMPLOYEEJOB procedure accepts an employee number and returns a job
// code or null value to an output parameter.
//
// See http://trafodion.apache.org/docs/spj_guide/index.html#employeejob-procedure
// for more documentation.
public static void employeeJob( int empNum
			      , java.lang.Integer[] jobCode
			      ) throws SQLException
{
   Connection conn =
      DriverManager.getConnection( "jdbc:default:connection" ) ;

   PreparedStatement getJobcode = 
      conn.prepareStatement( "SELECT jobcode " 
			   + "FROM trafodion.persnl.employee " 
			   + "WHERE empnum = ?"
			   ) ;

   getJobcode.setInt( 1, empNum ) ;
   ResultSet rs = getJobcode.executeQuery() ; 
   rs.next() ;

   int num = rs.getInt(1) ; 
   if ( rs.wasNull() )
      jobCode[0] = null ;
   else
      jobCode[0] = new Integer(num) ; 

   rs.close() ;
   conn.close() ;

} 
