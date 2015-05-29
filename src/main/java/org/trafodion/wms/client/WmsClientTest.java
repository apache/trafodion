package org.trafodion.wms.client;

import java.io.*;
import java.net.*;
import java.util.*;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.trafodion.wms.thrift.generated.*;
import org.trafodion.wms.client.WmsClient;

public class WmsClientTest {
    String[] args;
    
    private static void test() {
        try {
            ClientData request = new ClientData();
            ClientData response = new ClientData();
            Map<String,KeyValue> m = new HashMap<String, KeyValue>();
            request.setKeyValues(m);
            request.putKeyValue("operation",Operation.OPERATION_BEGIN);
            request.putKeyValue("state","RUNNING");
            request.putKeyValue("subState","BEGIN");
            request.putKeyValue("type","trafodion");
            request.putKeyValue("queryId","MXID11000001075212235857042874154000000000106U6553500_4_SQL_DATASOURCE_Q8");
            request.putKeyValue("queryText","This is some query text");
            request.putKeyValue("beginTimestamp",System.currentTimeMillis());
            request.putKeyValue("endTimestamp",System.currentTimeMillis());
            
            //Begin a workload
            WmsClient wmsClient = new WmsClient();
            wmsClient.open();
            response = wmsClient.writeread(request);
            
            //Update a workload
            request.putKeyValue("operation",Operation.OPERATION_UPDATE);
            request.putKeyValue("state","RUNNING");
            request.putKeyValue("subState","UPDATE");
            request.putKeyValue("endTimestamp",System.currentTimeMillis());
            request.putKeyValue("workloadId",response.getKeyValueAsString("workloadId"));
            response = wmsClient.writeread(request);
            
            //End a workload
            request.putKeyValue("operation",Operation.OPERATION_UPDATE);
            request.putKeyValue("state","COMPLETED");
            request.putKeyValue("subState","SUCCEEDED");
            request.putKeyValue("endTimestamp",System.currentTimeMillis());
            request.putKeyValue("workloadId",response.getKeyValueAsString("workloadId"));
            response = wmsClient.writeread(request);

            wmsClient.close();

        } catch (Exception e) {
            e.printStackTrace();
            System.exit(-1);
        }
 
/*
        try {
            WmsClient conn = new WmsClient();
            conn.open();
            WmsWorkloadFactory factory = new WmsWorkloadFactory();
            WmsWorkload request = WmsWorkloadFactory.getWorkload("org.trafodion.wms.HadoopWorkload");
            if(request != null) {
                //Lets begin a workload
                request.setOperation(OperationType.BEGIN);
                request.setWorkloadId("");
                request.setParentId("");
                request.setParentKey("");
                request.setJobType(JobType.HADOOP);
                request.setJobInfo("Some job ID");
                request.setJobText("This is some text");
                request.setUserInfo("Administrator");
                request.setJobInfo("some job info");
                request.setJobState("RUNNING");
                request.setJobSubState("BEGIN");
                request.setStartTime(System.currentTimeMillis());
                request.setEndTime(System.currentTimeMillis());
                request.setMapPct(0);
                request.setReducePct(0);
                request.setDuration(request.getEndTime() - request.getStartTime());
                //WorkloadResponse response = new WorkloadResponse();
                WorkloadResponse response = conn.writeread(request); 
                //Lets update the workload
                request.setOperation(OperationType.UPDATE);
                request.setWorkloadId(response.getWorkloadId().toString());
                request.setJobType(JobType.HADOOP);
                request.setJobText("This is my test text");
                request.setJobState("RUNNING");
                request.setJobSubState("UPDATE");
                request.setEndTime(System.currentTimeMillis());
                request.setMapPct(10);
                request.setReducePct(100);
                request.setDuration(request.getEndTime() - request.getStartTime());
                response = conn.writeread(request); 
                //Lets end the workload
                request.setOperation(OperationType.END);
                request.setWorkloadId(response.getWorkloadId().toString());
                request.setJobType(JobType.HADOOP);
                request.setJobText("This is my test text");
                request.setJobState("COMPLETED");
                request.setJobSubState("SUCCESSFUL");
                request.setEndTime(System.currentTimeMillis());
                request.setMapPct(10);
                request.setReducePct(100);
                request.setDuration(request.getEndTime() - request.getStartTime());
                response = conn.writeread(request); 
            }
            conn.close();
            conn = null;
        } catch (Exception e) {
            System.out.print(e);
            e.printStackTrace();
            System.exit(-1);
        }
*/
    }
    
    private static void test2() {
        int TIMEOUT = 30000;
        
        
/*      
        try {
            WmsClient conn = new WmsClient();
            conn.open();
            WmsWorkloadFactory factory = new WmsWorkloadFactory();
            WmsWorkload request = WmsWorkloadFactory.getWorkload("org.trafodion.wms.HadoopWorkload");
            if(request != null) {
                //Lets begin a workload
                request.setOperation(OperationType.BEGIN);
                request.setWorkloadId("");
                request.setParentId("");
                request.setParentKey("");
                request.setJobType(JobType.HADOOP);
                request.setJobInfo("Some job ID");
                request.setJobText("This is some text");
                request.setUserInfo("Administrator");
                request.setJobInfo("some job info");
                request.setJobState("RUNNING");
                request.setJobSubState("BEGIN");
                request.setStartTime(System.currentTimeMillis());
                request.setEndTime(System.currentTimeMillis());
                request.setMapPct(0);
                request.setReducePct(0);
                request.setDuration(request.getEndTime() - request.getStartTime());
                WorkloadResponse response = conn.writeread(request); 
                System.out.println("Response:[" + response.getAction() + "]");
                //Lets update the workload
                request.setOperation(OperationType.UPDATE);
                request.setWorkloadId(response.getWorkloadId().toString());
                request.setJobType(JobType.HADOOP);
                request.setJobText("This is my test text");
                request.setJobState("RUNNING");
                request.setJobSubState("UPDATE");
                request.setEndTime(System.currentTimeMillis());
                request.setMapPct(10);
                request.setReducePct(100);
                request.setDuration(request.getEndTime() - request.getStartTime());
                response = conn.writeread(request); 
                
                System.out.println("Sleeping for " + TIMEOUT/1000 + " seconds");
                try {
                    Thread.sleep(TIMEOUT);
                } catch (InterruptedException e) {  
                }
                
                System.out.println("Awake !");
                conn.close();
                conn = null;
            }
        } catch (Exception e) {
            System.out.print(e);
            System.exit(-1);
        }
*/
    }
    
    private static void test3() {
        int TIMEOUT = 600000;
/*
        try {
            WmsClient conn = new WmsClient();
            conn.open();
            WmsWorkloadFactory factory = new WmsWorkloadFactory();
            WmsWorkload request = WmsWorkloadFactory.getWorkload("org.trafodion.wms.HadoopWorkload");
            if(request != null) {
                //Lets begin a workload
                request.setOperation(OperationType.BEGIN);
                request.setWorkloadId("");
                request.setParentId("");
                request.setParentKey("");
                request.setJobType(JobType.TRAFODION);
                request.setJobInfo("MXID11000001075212235857042874154000000000106U6553500_4_SQL_DATASOURCE_Q8");
                request.setJobText("This is some text");
                request.setUserInfo("Administrator");
                request.setJobState("RUNNING");
                request.setJobSubState("BEGIN");
                request.setStartTime(System.currentTimeMillis());
                request.setEndTime(System.currentTimeMillis());
                request.setMapPct(0);
                request.setReducePct(0);
                request.setDuration(request.getEndTime() - request.getStartTime());
                WorkloadResponse response = conn.writeread(request); 
                System.out.println("Response:[" + response.getAction() + "]");
                System.out.println("Sleeping for " + TIMEOUT/1000 + " seconds");
                try {
                    Thread.sleep(TIMEOUT);
                } catch (InterruptedException e) {  
                }
                
                System.out.println("Awake !");
                conn.close();
                conn = null;
            }
        } catch (Exception e) {
            System.out.print(e);
            System.exit(-1);
        }
*/
    }
    
    public static void main(String [] args) {
        args = args;
        boolean done=false;

        Options opt = new Options();
        CommandLine cmd;
        try {
            cmd = new GnuParser().parse(opt, args);
        } catch (ParseException e) {
            System.out.print("Could not parse: " + e);
            return;
        }
        
        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        while(done==false) {
            System.out.print("\nwms shell>");
            String line = null;

            try {
                line = br.readLine();
                if(line.equalsIgnoreCase("exit") || line.equalsIgnoreCase("quit"))
                    done=true;
                else if(line.equalsIgnoreCase("test"))
                    test();
                else if(line.equalsIgnoreCase("test2"))
                    test2();    
                else if(line.equalsIgnoreCase("test3"))
                    test3();    
                else 
                    System.out.print("Unknown or invalid command\n");
            } catch (IOException e) {
                System.out.print("Error reading your command..." + e);
                done=true;
            } 
        }
        System.exit(0);
    }
}
