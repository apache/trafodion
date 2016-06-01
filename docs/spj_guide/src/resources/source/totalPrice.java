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

