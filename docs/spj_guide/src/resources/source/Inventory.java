import java.sql.* ;
import java.math.* ;

public class Inventory
{
   // The SUPPLIERINFO procedure accepts a supplier number and returns the
   // supplier's name, street, city, state, and post code to separate output
   // parameters.
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#supplierinfo-procedure
   // for more documentation.
   public static void supplierInfo( BigDecimal suppNum
                                  , String[] suppName
				  , String[] streetAddr
				  , String[] cityName
				  , String[] stateName
				  , String[] postCode
				  ) throws SQLException
   {
      Connection conn =
         DriverManager.getConnection( "jdbc:default:connection" ) ;

      PreparedStatement getSupplier =
         conn.prepareStatement( "SELECT suppname, street, city, "
	                      + "       state, postcode "
			      + "FROM trafodion.invent.supplier "
			      + "WHERE suppnum = ?"  
			      ) ;

      getSupplier.setBigDecimal( 1, suppNum ) ;
      ResultSet rs = getSupplier.executeQuery() ;
      rs.next() ;

      suppName[0]   = rs.getString( 1 ) ;
      streetAddr[0] = rs.getString( 2 ) ;
      cityName[0]   = rs.getString( 3 ) ;
      stateName[0]  = rs.getString( 4 ) ;
      postCode[0]   = rs.getString( 5 ) ;

      rs.close() ;
      conn.close() ;

   } 

   // The SUPPLYQUANTITIES procedure returns the average, minimum, and maximum
   // quantities of available parts in inventory to separate output
   // parameters.
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#supplyquantities-procedure
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

   // The PARTLOCATIONS procedure accepts a part number and quantity and returns a
   // set of location codes that have the exact quantity and a set of location
   // codes that have more than that quantity.
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#partlocations-procedure
   // for more documentation.
   public static void partLocations( int partNum
                                   , int quantity
				   , ResultSet exactly[]
				   , ResultSet moreThan[]
				   ) throws SQLException

   {
      Connection conn =
         DriverManager.getConnection( "jdbc:default:connection" ) ;

      PreparedStatement getLocationsExact =
         conn.prepareStatement( "SELECT L.loc_code, L.partnum, L.qty_on_hand "
	                      + "FROM trafodion.invent.partloc L "
			      + "WHERE L.partnum = ? "
			      + "  AND L.qty_on_hand = ? "
			      + " ORDER BY L.partnum "
			      ) ;

      getLocationsExact.setInt( 1, partNum ) ;
      getLocationsExact.setInt( 2, quantity) ;

      PreparedStatement getLocationsMoreThan =
         conn.prepareStatement( "SELECT L.loc_code, L.partnum, L.qty_on_hand "
	                      + "FROM trafodion.invent.partloc L "
			      + "WHERE L.partnum = ? "
			      + "  AND L.qty_on_hand > ? "
			      + "ORDER BY L.partnum "
			      ) ;

      getLocationsMoreThan.setInt( 1, partNum ) ;
      getLocationsMoreThan.setInt( 2, quantity) ;

      exactly[0]  = getLocationsExact.executeQuery() ;
      moreThan[0] = getLocationsMoreThan.executeQuery() ;

   } 
}
