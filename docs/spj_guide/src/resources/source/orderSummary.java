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
// See http://trafodion.apache.org/docs/spj_guide/index.html#ordersummary-procedure
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

