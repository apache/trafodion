import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.transactional.RMInterface;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.util.Bytes;


public class trxRMInterfTest {
  public final static String TABLE1 = "tableRMI";
  public static void main(String[] args) {
    HBaseAdmin hbadmin = null;
    RMInterface rm1 = null;
    TransactionState ts = null;
    //RMInterface rm2;
    Configuration conf = HBaseConfiguration.create();
    java.util.Scanner reader = new java.util.Scanner(System.in);   
    
    try {
      
     // rm1 = new RMInterface(conf, "") ;
      
      hbadmin = new HBaseAdmin(conf);
      if(hbadmin.isTableAvailable(TABLE1)) {
        TableName tablename = TableName.valueOf(TABLE1);
        //HTableDescriptor desc = new HTableDescriptor(tablename);
        hbadmin.disableTable(tablename);
        hbadmin.deleteTable(tablename);
      }
      TableName tablename = TableName.valueOf(TABLE1);

      HTableDescriptor desc = new HTableDescriptor(tablename);
      desc.addFamily(new HColumnDescriptor("cf1"));
      hbadmin.createTable(desc);
      rm1 = new RMInterface(conf,TABLE1);
      System.out.println("Enter an int:"); 
      int retInt = reader.nextInt();
      ts = new TransactionState(retInt);
      
      Put p1 = new Put(Bytes.toBytes("row1"));
      p1.add(Bytes.toBytes("cf1"), Bytes.toBytes("none"), Bytes.toBytes("something"));
      
      Put p2 = new Put(Bytes.toBytes("row2"));
      p2.add(Bytes.toBytes("cf1"), null, Bytes.toBytes("something2"));
      
      rm1.put(ts,p1);
      rm1.put(ts,p2);
      reader.close();
      
    } catch(Exception e) {
      e.printStackTrace();
    }   
    reader.close();      
  }
}
