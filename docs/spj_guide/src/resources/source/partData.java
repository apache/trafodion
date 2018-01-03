// The PARTDATA procedure accepts a part number and returns this
// information about the part:
//
// * Part description, price, and quantity available as output parameters.
// * A result set that contains rows from the ORDERS table about when this part was ordered.
// * A result set that contains rows from the PARTLOC table, listing locations that have this 
//   part in stock and the quantity they have on hand.
// * A result set that contains rows from the PARTSUPP table for suppliers who carry this part.
// * A result set that contains rows from the EMPLOYEE table for sales reps who have sold this part.
//
// See http://trafodion.apache.org/docs/spj_guide/index.html#partdata-procedure
// for more documentation.
public static void partData( int partNum
			   , String[] partDescription
			   , BigDecimal[] unitPrice
			   , int[] qtyAvailable
			   , ResultSet[] orders
			   , ResultSet[] locations
			   , ResultSet[] suppliers
			   , ResultSet[] reps
			   ) throws SQLException
{

   Connection conn =
      DriverManager.getConnection( "jdbc:default:connection" ) ;

   // Retrieve detail about this part into the output parameters
   PreparedStatement getPartInfo = 
     conn.prepareStatement( "SELECT P.partdesc, P.price, P.qty_available " 
			  + "FROM trafodion.sales.parts P " 
			  + "WHERE partnum = ? "
			  ) ; 

   getPartInfo.setInt( 1, partNum ) ;

   ResultSet rs = getPartInfo.executeQuery() ; 
   rs.next() ;

   partDescription[0] = rs.getString( 1 ) ; 
   unitPrice[0]       = rs.getBigDecimal( 2 ) ; 
   qtyAvailable[0]    = rs.getInt( 3 ) ;

   rs.close();

   // Return a result set of rows from the ORDERS table listing orders
   // that included this part. Each ORDERS row is augmented with the
   // quantity of this part that was ordered. 
   PreparedStatement getOrders =
      conn.prepareStatement( "SELECT O.*, QTY.QTY_ORDERED " 
			   + "FROM   trafodion.sales.orders O " 
			   + "     , ( select ordernum, sum(qty_ordered) as QTY_ORDERED " 
			   + "         from trafodion.sales.odetail " 
			   + "         where partnum = ? " 
			   + "         group by ordernum ) QTY " 
			   + "WHERE O.ordernum = QTY.ordernum " 
			   + "ORDER BY O.ordernum "
			   ) ;


    getOrders.setInt( 1, partNum ) ; 
    orders[0] = getOrders.executeQuery() ;

    // Return a result set of rows from the PARTLOC table listing
    // locations that have this part in stock and the quantity they
    // have on hand.
    PreparedStatement getLocations = 
       conn.prepareStatement( "SELECT * " 
			    + "FROM trafodion.invent.partloc " 
			    + "WHERE partnum = ? "
			    ) ;

    getLocations.setInt( 1, partNum ) ; 
    locations[0] = getLocations.executeQuery() ;

    // Return a result set of rows from the PARTSUPP table listing
    // suppliers who supply this part.
    PreparedStatement getSuppliers = 
       conn.prepareStatement( "SELECT * " 
			    + "FROM trafodion.invent.partsupp " 
			    + "WHERE partnum = ? "
			    ) ;

    getSuppliers.setInt( 1, partNum ) ; 
    suppliers[0] = getSuppliers.executeQuery() ;

    // Return a result set of rows from the EMPLOYEE table listing
    // sales reps that have sold this part. 
    PreparedStatement getReps =
       conn.prepareStatement( "SELECT * " 
			    + "FROM trafodion.persnl.employee " 
			    + "WHERE empnum in ( SELECT O.salesrep " 
			    + "                  FROM trafodion.sales.orders O, " 
			    + "                       trafodion.sales.odetail D " 
			    + "                  WHERE D.partnum = ? " 
			    + "                    AND O.ordernum = D.ordernum ) " 
			    + "ORDER BY empnum "
			    ) ;

     getReps.setInt( 1, partNum ) ; 
     reps[0] = getReps.executeQuery() ;

} 

