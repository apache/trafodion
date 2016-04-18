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
