import java.sql.* ; 
import java.math.* ;

public class Payroll
{
   // The ADJUSTSALARY procedure accepts an employee number and a percentage
   // value and updates the employee's salary in the database based on that
   // percentage. This method also returns the updated salary to an output  
   // parameter.    
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#adjustsalary-procedure
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

   // The EMPLOYEEJOB procedure accepts an employee number and returns a job
   // code or null value to an output parameter.
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#employeejob-procedure
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

   // The PROJECTTEAM procedure accepts a project code and returns the
   // employee number, first name, last name, and location of the employees
   // assigned to that project.
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#projectteam-procedure
   // for more documentation.
   public static void projectTeam( int projectCode
                                 , ResultSet[] members
                                 ) throws SQLException
   {
      Connection conn =
         DriverManager.getConnection( "jdbc:default:connection" ) ;

      PreparedStatement getMembers = 
         conn.prepareStatement( "SELECT E.empnum, E.first_name, E.last_name, D.location " 
                              + "FROM trafodion.persnl.employee E, trafodion.persnl.dept D, trafodion.persnl.project P "
                              + "WHERE P.projcode = ? " 
                              + "  AND P.empnum = E.empnum " 
                              + "  AND E.deptnum = D.deptnum "
                              ) ; 

       getMembers.setInt( 1, projectCode ) ;
       members[0] = getMembers.executeQuery() ;

   } 

   // The TOPSALESREPS procedure accepts a number representing the fiscal
   // quarter (1, 2, 3, and 4, with each number representing a range of
   // months) and returns the employee number, first name, last name, and sale
   // figures of the top five sales representatives who had the highest sales
   // (unit_price * qty_ordered) that quarter.
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#topsalesreps-procedure
   // for more documentation.
   public static void topSalesReps( int whichQuarter
                                  , ResultSet[] topReps
                                  ) throws SQLException
   {
      if ( whichQuarter < 1 || whichQuarter > 4 )
      {
         throw new SQLException ( "Invalid value for quarter. " 
                                + "Retry the CALL statement " 
                                + "using a number from 1 to 4 " 
                                + "to represent the quarter."
                                , "38001" 
                                ) ;
      }

      Connection conn =
         DriverManager.getConnection( "jdbc:default:connection" ) ;

      PreparedStatement getTopReps = 
        conn.prepareStatement( "SELECT [first 5] e.empnum, e.first_name, " 
                             + "e.last_name, totals.total " 
                             + "FROM trafodion.persnl.employee e, " 
                             + "   ( SELECT o.salesrep, " 
                             + "     SUM( od.unit_price * od.qty_ordered ) as total " 
                             + "     FROM trafodion.sales.orders o, trafodion.sales.odetail od " 
                             + "     WHERE o.ordernum = od.ordernum " 
                             + "       AND QUARTER( o.order_date ) = ? " 
                             + "     GROUP BY o.salesrep " 
                             + "    ) totals " 
                             + "WHERE e.empnum = totals.salesrep " 
                             + "ORDER BY totals.total DESCENDING "
                             ) ;

      getTopReps.setInt( 1, whichQuarter ) ;
      topReps[0] = getTopReps.executeQuery() ;

   } 
}
