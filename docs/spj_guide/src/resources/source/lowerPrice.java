// The LOWERPRICE procedure determines which items are selling poorly (that
// is, have less than 50 orders) and lowers the price of these items in the
// database by 10 percent.
//
// See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#lowerprice-procedure
// for more documentation.
public static void lowerPrice() throws SQLException
{
   Connection conn =
      DriverManager.getConnection( "jdbc:default:connection" ) ;

      PreparedStatement getParts = 
         conn.prepareStatement( "SELECT p.partnum, "
                              + "SUM(qty_ordered) AS qtyOrdered " 
                              + "FROM trafodion.sales.parts p " 
                              + "LEFT JOIN trafodion.sales.odetail o " 
                              + "ON p.partnum = o.partnum " 
                              + "GROUP BY p.partnum"
                              ) ;

      PreparedStatement updateParts = 
         conn.prepareStatement( "UPDATE trafodion.sales.parts " 
                              + "SET price = price * 0.9 " 
                              + "WHERE partnum = ?"
                              ) ;

      ResultSet rs = getParts.executeQuery() ; 
      while ( rs.next() )
      {
         BigDecimal qtyOrdered = rs.getBigDecimal( 2 ) ;

         if (( qtyOrdered == null ) || ( qtyOrdered.intValue() < 50 ) )
         {
            BigDecimal partnum = rs.getBigDecimal( 1 ) ; 
            updateParts.setBigDecimal( 1, partnum ) ; 
            updateParts.executeUpdate() ;
         }
      }

      rs.close() ;
      conn.close() ;

} 

