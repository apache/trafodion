// The TOPSALESREPS procedure accepts a number representing the fiscal
// quarter (1, 2, 3, and 4, with each number representing a range of
// months) and returns the employee number, first name, last name, and sale
// figures of the top five sales representatives who had the highest sales
// (unit_price * qty_ordered) that quarter.
//
// See http://trafodion.apache.org/docs/spj_guide/index.html#topsalesreps-procedure
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
