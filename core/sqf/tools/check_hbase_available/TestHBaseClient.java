import java.io.IOException;

import org.apache.hadoop.conf.Configuration;
//import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.*;
import org.apache.hadoop.hbase.client.*;
import org.apache.hadoop.hbase.util.Bytes;

public class TestHBaseClient {
    public static void main(String[] args) throws IOException {

	/* The HBaseConfiguration object holds information on where this
       client is going to connect to. It reads configuration 
        file hbase-site.xml. Those files 
	 have to be kept in the classpath (see setenv.bat)
	*/
	
	Configuration config = HBaseConfiguration.create();

	/* The table object leverages the config object (which knows the 
       location of HBase install, etc. and also knows the tables
       that are to be connected to
	*/

        HTableDescriptor desc = new HTableDescriptor("splittab");
        desc.addFamily(new HColumnDescriptor("cf1"));
        HBaseAdmin admin = new HBaseAdmin(config);
	try {
            System.out.println ("Creating the table splittab");
	    admin.createTable(desc);
	}
	catch (TableExistsException e) {
            System.out.println("Table splittab already exists");
        }
	HTable table = new HTable(config, "splittab");

	/* To insert a row into the table, leverage a Put object.
       The put object has to be primed with the rowkey of 
       the object it represents.
	*/
	 
	Put p = new Put(Bytes.toBytes("rowA1"));

	/* On the put object, specify the column family, the column and 
       value to be inserted
	*/

	p.add(Bytes.toBytes("cf1"), Bytes.toBytes("colA1"),
	      Bytes.toBytes("valA1"));

	table.put(p);

	/* Now retrieve the information stored, using a Get 
       object instance
	*/
    
	Get g = new Get(Bytes.toBytes("rowA1"));
	Result r = table.get(g);
	byte [] value = r.getValue(Bytes.toBytes("cf1"),
				   Bytes.toBytes("colA1"));
	String valueStr = Bytes.toString(value);
	System.out.println("GET: " + valueStr);

	/* Leverage the Scan type to scan the table */

	Scan s = new Scan();
	s.addColumn(Bytes.toBytes("cf1"), Bytes.toBytes("colA1"));
	ResultScanner scanner = table.getScanner(s);
	try {
	    for (Result rr : scanner) 
		{
		    System.out.println("Found row: " + rr);
		}

	} finally {
	    scanner.close();
	}
    }
}