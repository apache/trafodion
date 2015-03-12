import java.io.IOException;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.TransactionalTable;
import org.apache.hadoop.hbase.util.Bytes;


public class trxTest98 {
  public static void main(String[] args) {
    TransactionalTable ttable  = null;
    TransactionalTable ttable2 = null;
    
    java.util.Scanner reader = new java.util.Scanner(System.in);   
    
    HBaseAdmin hbadmin = null;
    TransactionManager tm = null;
    Configuration conf = HBaseConfiguration.create();
    try {
      
    } catch (Exception e) {
      e.printStackTrace();
    }
    try {
      tm = new TransactionManager(conf);
      hbadmin = new HBaseAdmin(conf);
      if(hbadmin.isTableAvailable("table1")) {
        TableName tablename = TableName.valueOf("table1");
        //HTableDescriptor desc = new HTableDescriptor(tablename);
        hbadmin.disableTable(tablename);
        hbadmin.deleteTable(tablename);
      }
      TableName tablename = TableName.valueOf("table1");

      HTableDescriptor desc = new HTableDescriptor(tablename);
      desc.addFamily(new HColumnDescriptor("cf1"));
      hbadmin.createTable(desc);
      
      if(hbadmin.isTableAvailable("table2")) {
        TableName tablename2 = TableName.valueOf("table2");    
        hbadmin.disableTable(tablename2);
        hbadmin.deleteTable(tablename2);
      }
      TableName tablename2 = TableName.valueOf("table2"); 
      HTableDescriptor desc2 = new HTableDescriptor(tablename2);
      desc2.addFamily(new HColumnDescriptor("cf1"));
      hbadmin.createTable(desc2);
      
      ttable  = new TransactionalTable("table1");
      ttable2 = new TransactionalTable("table2");
      
      TransactionState ts = tm.beginTransaction();
      Put p1 = new Put(Bytes.toBytes("row1"));
      p1.add(Bytes.toBytes("cf1"), Bytes.toBytes("none"), Bytes.toBytes("something"));
      Put p2 = new Put(Bytes.toBytes("row2"));
      p2.add(Bytes.toBytes("cf1"), null, Bytes.toBytes("something2"));
      
      ttable.put(ts, p1);
      ttable.put(ts, p2);
      
      ttable2.put(ts, p1);
      ttable2.put(ts, p2);
      
      System.out.println("Hit Enter to continue, before tryCommit:");
      reader.nextLine();
      //reader.close();
      tm.tryCommit(ts);
      
      // Start of Delete Test
      TransactionState ts2 = tm.beginTransaction();
      Delete d1 = new Delete(Bytes.toBytes("row1"));
      Delete d2 = new Delete (Bytes.toBytes("row2"));
      
      ttable.delete(ts2, d1);
      ttable.delete(ts2, d2);
      
      ttable2.delete(ts2, d1);
      ttable2.delete(ts2, d2);
      
      System.out.println("Hit Enter to continue, before delete of rows:");
      reader.nextLine();
      //reader.close();
      tm.tryCommit(ts2);
      
      Put p3 = new Put(Bytes.toBytes("row3"));
      p3.add(Bytes.toBytes("cf1"), Bytes.toBytes("none"), Bytes.toBytes("something"));
      Put p4 = new Put(Bytes.toBytes("row4"));
      p4.add(Bytes.toBytes("cf1"), null, Bytes.toBytes("something2"));
      
      TransactionState ts3 = tm.beginTransaction();
      
      ttable.put(ts3, p3);
      ttable.put(ts3,p4);
      ttable2.put(ts3, p3);
      ttable2.put(ts3,p4);
      
      System.out.println("Hit Enter to continue, before abort of transaction ts3...");
      reader.nextLine();
      reader.close();
      tm.abort(ts3);
      Thread.sleep(10);
      
      //ttable.close();
      System.out.println("END LINE");                 
    } catch(Exception e) {
      e.printStackTrace();
    } finally {
      try {
        ttable.close();
      } catch (IOException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
      }
    }
    System.out.println("Outside TRY");
    
    //System.exit(0);
  }
}
