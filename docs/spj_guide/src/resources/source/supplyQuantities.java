// The SUPPLYQUANTITIES procedure returns the average, minimum, and maximum
// quantities of available parts in inventory to separate output
// parameters.
//
// See http://trafodion.apache.org/docs/spj_guide/index.html#supplyquantities-procedure
// for more documentation.
public static void supplyQuantities( int[] avgQty
				   , int[] minQty
				   , int[] maxQty
				   ) throws SQLException
{
   Connection conn =
      DriverManager.getConnection( "jdbc:default:connection" ) ;

   PreparedStatement getQty =
      conn.prepareStatement( "SELECT AVG(qty_on_hand), "
			   + "       MIN(qty_on_hand), "
			   + "       MAX(qty_on_hand) "
			   + "FROM trafodion.invent.partloc"
			   ) ;

   ResultSet rs = getQty.executeQuery() ;
   rs.next() ;

   avgQty[0] = rs.getInt( 1 ) ;
   minQty[0] = rs.getInt( 2 ) ;
   maxQty[0] = rs.getInt( 3 ) ;

   rs.close() ;
   conn.close() ;

} 
