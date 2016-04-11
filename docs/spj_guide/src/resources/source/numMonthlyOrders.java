// The MONTHLYORDERS procedure accepts an integer representing the month
// and returns the number of orders during that month to an output parameter.
// 
// See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#monthlyorders-procedure
// for more documentation.
public static void numMonthlyOrders( int month
				   , int[] numOrders
				   ) throws SQLException

{
   if ( month < 1 || month > 12 )
   {
      throw new SQLException( "Invalid value for month. " 
			    + "Retry the CALL statement " 
			    + "using a number from 1 to 12 " 
			    + "to represent the month."
			    , "38001" 
			    ) ;
   }

   Connection conn =
      DriverManager.getConnection( "jdbc:default:connection" ) ;

   PreparedStatement getNumOrders =
      conn.prepareStatement( "SELECT COUNT( month( order_date ) ) " 
			   + "FROM trafodion.sales.orders " 
			   + "WHERE month( order_date ) = ?"
			   ) ;

   getNumOrders.setInt( 1, month ) ;

   ResultSet rs = getNumOrders.executeQuery() ; 
   rs.next() ;

   numOrders[0] = rs.getInt(1) ; 

   rs.close() ;
   conn.close();

} 

