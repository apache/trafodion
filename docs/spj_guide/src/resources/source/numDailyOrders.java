// The DAILYORDERS procedure accepts a date and returns the number of
//orders on that date to an output parameter.
//
// See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#dailyorders-procedure
// for additional documenation.
public static void numDailyOrders( Date date
				 , int[] numOrders 
				 ) throws SQLException
{
   Connection conn =
      DriverManager.getConnection( "jdbc:default:connection" ) ;

   PreparedStatement getNumOrders = 
      conn.prepareStatement( "SELECT COUNT(order_date) " 
			   + "FROM trafodion.sales.orders " 
			   + "WHERE order_date = ?"
			   ) ;

   getNumOrders.setDate( 1, date ) ;

   ResultSet rs = getNumOrders.executeQuery() ; 
   rs.next() ;

   numOrders[0] = rs.getInt( 1 ) ; 

   rs.close() ;
   conn.close() ;

} 
