// The ADJUSTSALARY procedure accepts an employee number and a percentage
// value and updates the employee's salary in the database based on that
// percentage. This method also returns the updated salary to an output  
// parameter.    
//
// See http://trafodion.apache.org/docs/spj_guide/index.html#adjustsalary-procedure
// for more documentation.
public static void adjustSalary( BigDecimal empNum
			       , double percent
			       , BigDecimal[] newSalary
			       ) throws SQLException
{
   Connection conn =
      DriverManager.getConnection( "jdbc:default:connection" ) ;

   PreparedStatement setSalary = 
      conn.prepareStatement( "UPDATE trafodion.persnl.employee " 
			   + "SET salary = salary * (1 + (? / 100)) " 
			   + "WHERE empnum = ?"
			   ) ;

   PreparedStatement getSalary = 
      conn.prepareStatement( "SELECT salary " 
			   + "FROM trafodion.persnl.employee " 
			   + "WHERE empnum = ?"
			   ) ;

   setSalary.setDouble( 1, percent ) ; 
   setSalary.setBigDecimal( 2, empNum ) ;
   setSalary.executeUpdate() ;

   getSalary.setBigDecimal( 1, empNum ) ; 
   ResultSet rs = getSalary.executeQuery() ; 
   rs.next() ;

   newSalary[0] = rs.getBigDecimal( 1 ) ; 

   rs.close() ;
   conn.close() ;

} 
