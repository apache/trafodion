import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;

import org.apache.log4j.PropertyConfigurator;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.*;
import org.apache.hadoop.hbase.client.*;

public class CheckHBase {

    static void setupLog4j() {
       System.out.println("In setupLog4J");
        String confFile = System.getenv("PWD")
            + "/log4j.util.config";
        PropertyConfigurator.configure(confFile);
    }

    public static void main(String[] args) {
	
	System.out.println("MAIN ENTRY");      
	if (CheckHBase.isHBaseAvailable()) {
	    System.out.println("HBase is available");
	}
	else {
	    System.out.println("HBase is not available");
	}
    }
    
    public CheckHBase () {
	System.out.println("In ctor");
    }

    static public boolean isHBaseAvailable() {

	setupLog4j();

	Configuration lv_config = HBaseConfiguration.create();
	//	lv_config.set("hbase.client.retries.number", "3");
	System.out.println("Checking if HBase is available...");
	try {
	    HBaseAdmin.checkHBaseAvailable(lv_config);
	}
	catch (Exception e) {
	    System.out.println("Caught an exception in HBaseAdmin.checkHBaseAvailable: " + e);
	    return false;
	}
	return true;
    }
    
}
