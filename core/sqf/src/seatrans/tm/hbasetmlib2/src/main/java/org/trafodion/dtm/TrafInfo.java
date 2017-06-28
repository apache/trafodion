// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.dtm;

import java.io.IOException;
import java.util.Collection;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.ConnectionFactory;
//H98import org.apache.hadoop.hbase.ipc.HMasterInterface;
//H98import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
//H98import org.apache.hadoop.hbase.ipc.HRegionInterface;

import org.trafodion.sql.TrafConfiguration;

public class TrafInfo {

    private Connection connection;
    Configuration     config;
    //    HMasterInterface  hmaster;

    public TrafInfo() throws IOException {
        init();
    }

    public void init() throws IOException {
        this.config = TrafConfiguration.create(TrafConfiguration.HBASE_CONF);
        this.connection = ConnectionFactory.createConnection(config);
    }

    public static void printHelp() {
        System.out.println("Run: $JAVA_HOME/bin/java org.trafodion.dtm.TrafInfo <command>");
        System.out.println("Commands to gather Transactional Region information:");
        System.out.println("    active      ::  active transactions per region");
        System.out.println("    committed   ::  committed transactions per region by sequence number");
        System.out.println("    indoubt     ::  in-doubt transactions per region");
        System.out.println("    <command> -v::  shows metadata tables");
    }

    public void getActivePendingTrans(String transType, String showmd){
        System.out.println("\n====================================================================");
        System.out.println("\t\tActive Pending Transactions");
        getTransactions(transType, showmd);
    }

    public void getCommittedTransactions(String transType, String showmd){
        System.out.println("\n====================================================================");
        System.out.println("\t\tCommitted Transactions by Sequence Number");
        getTransactions(transType, showmd);
    }

    public void getInDoubtTransactions(String transType, String showmd){
        System.out.println("\n====================================================================");
        System.out.println("\t\tIn-Doubt Transactions");
        getTransactions(transType, showmd);
    }

    public void getTransactions(String transType, String showmd){
        String regionName, tableName;
        int idx;

	/* H98
        Collection<ServerName> sn = hmaster.getClusterStatus().getServers();
        for(ServerName sname : sn) {
            System.out.println("===================================================================="
                             + "\nServer Name: " + sname.toString() + "\n"
                             + "\nTransId    RegionId             TableName");

            try {

                HRegionInterface regionServer = connection.getHRegionConnection(sname.getHostname(), sname.getPort());
                List<HRegionInfo> regions = regionServer.getOnlineRegions();
                connection.close();

                TransactionalRegionInterface transactionalRegionServer = (TransactionalRegionInterface)this.connection
                                                            .getHRegionConnection(sname.getHostname(), sname.getPort());

                for (HRegionInfo rinfo: regions) {

                    regionName = rinfo.getRegionNameAsString();
                    idx = regionName.indexOf(',');
                    tableName = regionName.substring(0, idx);

                    if(!showmd.contains("-v")){
                        if((tableName.contains("TRAFODION._MD_.")) || (tableName.contains("-ROOT-")) ||
                           (tableName.equals(".META."))){
                            continue;
                        }
                    }
                    if(tableName.contains("TRAFODION._DTM_.")){
                        continue;
                    }

                    System.out.println("--------------------------------------------------------------------"
                                     + "\n\t   " + rinfo.getRegionId()
                                     + "\t"      + tableName);

                    if(transType.equals("active")){
                        List<Long> result = transactionalRegionServer.getPendingTrans(rinfo.getRegionName());
                        for(Long res : result)
                            System.out.println(res);
                    }
                    else if(transType.equals("committed")){
                        List<Long> result = transactionalRegionServer.getCommittedTrans(rinfo.getRegionName());
                        for(Long res : result)
                            System.out.println(res);
                    }
                    else if(transType.equals("indoubt")){
                        List<Long> result = transactionalRegionServer.getInDoubtTrans(rinfo.getRegionName());
                            for(Long res : result)
                            System.out.println(res);
                    }
                }

            } catch(IOException e) {
                System.out.println("ERROR: Unable to get region info, Exiting");
                e.printStackTrace();
                System.exit(1);
            }
        }
	*/
    }

    public static void main(String[] args) throws IOException {

        if(args.length == 0) {
            TrafInfo.printHelp();
            System.exit(0);
        }

        TrafInfo ti = new TrafInfo();

        if(args.length == 1){
            if(args[0].equals("help"))
                TrafInfo.printHelp();
            else if(args[0].equals("active"))
                ti.getActivePendingTrans(args[0], "");
            else if(args[0].equals("committed"))
                ti.getCommittedTransactions(args[0], "");
            else if(args[0].equals("indoubt"))
                ti.getInDoubtTransactions(args[0], "");
            else {
                TrafInfo.printHelp();
                System.exit(0);
            }
        }
        // Verbose shows Metadata tables
        else if(args.length == 2){
            if(args[0].equals("active") && args[1].equals("-v"))
                ti.getActivePendingTrans(args[0], args[1]);
            else if(args[0].equals("committed") && args[1].equals("-v"))
                ti.getCommittedTransactions(args[0], args[1]);
            else if(args[0].equals("indoubt") && args[1].equals("-v"))
                ti.getInDoubtTransactions(args[0], args[1]);
            else {
                TrafInfo.printHelp();
                System.exit(0);
            }
        }
        else {
                TrafInfo.printHelp();
                System.exit(0);
        }
    }
}
