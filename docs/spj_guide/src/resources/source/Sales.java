import java.sql.* ;
import java.math.* ;

public class Sales
{
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

   // The TOTALPRICE procedure accepts the quantity, shipping speed, and price
   // of an item, calculates the total price, including tax and shipping
   // charges, and returns the total price to an input/output parameter.
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#totalprice-procedure
   // for more documentation.
   public static void totalPrice( BigDecimal qtyOrdered
                                , String shippingSpeed
                                , BigDecimal[] price
                                ) throws SQLException
   {
      BigDecimal shipcharge = new BigDecimal( 0 ) ;

      if ( shippingSpeed.equals( "economy" ) )
      {
         shipcharge = new BigDecimal( 1.95 ) ;
      }
      else if ( shippingSpeed.equals( "standard" ) )
      {
         shipcharge = new BigDecimal( 4.99 ) ;
      }
      else if ( shippingSpeed.equals( "nextday" ) )
      {
         shipcharge = new BigDecimal( 14.99 ) ;
      }
      else
      {
         throw new SQLException( "Invalid value for shipping speed. " 
                               + "Retry the CALL statement using " 
                               + "'economy' for 7 to 9 days, " 
                               + "'standard' for 3 to 5 days, or " 
                               + "'nextday' for one day."
                               , "38002" 
                               ) ;
      }
 
      BigDecimal subtotal   = price[0].multiply( qtyOrdered ) ; 
      BigDecimal tax        = new BigDecimal( 0.0825 ) ;
      BigDecimal taxcharge  = subtotal.multiply( tax ) ;
      BigDecimal charges    = taxcharge.add( shipcharge ) ; 
      BigDecimal totalprice = subtotal.add( charges ) ;

      totalprice = totalprice.setScale( 2, BigDecimal.ROUND_HALF_EVEN ) ;
      price[0] = totalprice ;

   } 

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
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#partdata-procedure
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
                               + " WHERE partnum = ? "
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

   // The ORDERSUMMARY procedure accepts a date, which is formatted as a
   // string, and returns this information about the orders on or after that
   // date:
   //
   // * The number of orders as an output parameter
   // * A result set that contains one row for each order. Each row contains
   //   fields for the order number, the number of parts ordered, total dollar
   //   amount, order date, and the name of the sales representative.
   // * A result set that contains details about each order. Each order has
   //   one or more rows that provide details about the ordered parts. Each row
   //   contains fields for the order number, part number, unit price, quantity
   //   ordered, and part description.
   //
   // See http://trafodion.incubator.apache.org/docs/spj_guide/index.html#ordersummary-procedure
   // for more documentation.
   public static void orderSummary( java.lang.String onOrAfter
                                  , long[] numOrders
                                  , java.sql.ResultSet[] orders
                                  , java.sql.ResultSet[] detail
                                  ) throws SQLException
   {
      java.lang.String s ; 

      java.sql.Connection conn =
         DriverManager.getConnection( "jdbc:default:connection" ) ;

      // Get the number of orders on or after this date
      s =   "SELECT COUNT(ordernum) FROM trafodion.sales.orders " 
          + "WHERE order_date >= CAST(? AS DATE) "
          ;

      java.sql.PreparedStatement ps1 = conn.prepareStatement( s ) ;
      ps1.setString( 1, onOrAfter ) ;

      java.sql.ResultSet rs = ps1.executeQuery() ; 
      rs.next() ;

      numOrders[0] = rs.getLong( 1 ) ; 
      rs.close() ;

      // Open a result set for order num, order info rows
      s =   "SELECT amounts.*, orders.order_date, emps.last_name " 
          + "FROM ( SELECT " 
          + "         o.ordernum "
          + "       , COUNT(d.partnum) AS num_parts " 
          + "       , SUM(d.unit_price * d.qty_ordered) AS amount " 
          + "       FROM trafodion.sales.orders o, trafodion.sales.odetail d " 
          + "       WHERE o.ordernum = d.ordernum " 
          + "          AND o.order_date >= CAST(? AS DATE) " 
          + "       GROUP BY o.ordernum "
          + "      ) amounts " 
          + "      , trafodion.sales.orders orders "
          + "      , trafodion.persnl.employee emps " 
          + "WHERE amounts.ordernum = orders.ordernum " 
          + "   AND orders.salesrep = emps.empnum " 
          + "ORDER BY orders.ordernum "
          ;

       java.sql.PreparedStatement ps2 = conn.prepareStatement( s ) ;
       ps2.setString( 1, onOrAfter ) ;
       orders[0] = ps2.executeQuery() ;

       // Open a result set for order detail rows
       s =   "SELECT d.*, p.partdesc " 
           + "FROM trafodion.sales.odetail d, trafodion.sales.parts p, trafodion.sales.orders O " 
           + "WHERE d.partnum = p.partnum AND d.ordernum = o.ordernum " 
           + "   AND o.order_date >= CAST(? AS DATE) " 
           + "ORDER BY d.ordernum "
           ;

       java.sql.PreparedStatement ps3 = conn.prepareStatement( s ) ;
       ps3.setString( 1, onOrAfter ) ;
       detail[0] = ps3.executeQuery() ;

   } 
}
