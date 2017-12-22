// The SUPPLIERINFO procedure accepts a supplier number and returns the
// supplier's name, street, city, state, and post code to separate output
// parameters.
//
// See http://trafodion.apache.org/docs/spj_guide/index.html#supplierinfo-procedure
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
