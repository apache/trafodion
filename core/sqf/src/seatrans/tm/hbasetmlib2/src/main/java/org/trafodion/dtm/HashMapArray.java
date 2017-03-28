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

import java.util.*;
import java.util.Map;
import java.util.HashMap;
import java.util.ArrayList;


public class HashMapArray {

    private static String regionKey = "RegionInfo";
    private static String delimiter = "|";
    private HashMap<String, String> inMap;
    private ArrayList<HashMap<String, String>> outList;
    private HashMap<Long, String> regionInfoMap;

    public HashMapArray(){
       inMap = new HashMap<String, String>();
       outList = new ArrayList<HashMap<String, String>>();
       regionInfoMap = new HashMap<Long, String>();
    }

    //-------------------------------------------------------
    // addElement
    // Purpose: Adds element to the HashMap
    //-------------------------------------------------------
    public void addElement(int tnum, String key, String value) {
       if(!outList.isEmpty()){
          try{
             inMap = outList.get(tnum);
             if(inMap.containsKey(key)){
                return;
             }
             else{
                inMap.put(key, value);
                return;
             }
          }catch ( IndexOutOfBoundsException e){
             inMap = new HashMap<String, String>();
             inMap.put(key, value);
             outList.add(inMap);
             return;
          }
       }
       // List is empty or map for transid does not exist.
       inMap = new HashMap<String, String>();
       inMap.put(key, value);
       outList.add(inMap);
   }

   public void appendRegionInfo(long tnum, String value) {
       //if(outList.size() <= tnum) {
       if(regionInfoMap.containsKey(tnum)) {
          String regionInfo = regionInfoMap.get(tnum);
          regionInfo = regionInfo + HashMapArray.delimiter + value;
          regionInfoMap.put(tnum, regionInfo);
       }
       else {
          regionInfoMap.put(tnum, value);
       }
   }

   //---------------------------------------------------------
   // getElement
   // Purpose: gets element from the HashMap
   //---------------------------------------------------------
   public String getElement(int index){
       inMap = outList.get(index);
        
       if (inMap==null) {
          System.out.println("HashMapArray::getElement:: inMap NULL");
          return null;
       }
       for(Map.Entry<String, String> entry : inMap.entrySet()){
          String key = entry.getKey();
          String value = entry.getValue();
       }
      return inMap.get("Transid").toString();
    }
    //
    //-------------------------------------------------------
    // getRegInfo
    // Purpose: gets tableName from the HashMap
    //-------------------------------------------------------
    public String getRegionInfo(long tnum){

       if(!regionInfoMap.containsKey(tnum))
          return new String("");
       return regionInfoMap.get(tnum);
    }

    //-------------------------------------------------------
    // getTableName
    // Purpose: gets tableName from the HashMap
    //-------------------------------------------------------
    public String getTableName(int index){

       inMap = outList.get(index);
       if (inMap==null) {
          System.out.println("HashMapArray::getTableName:: inMap NULL");
          return null;
       }
       return inMap.get("TableName").toString(); 
    }

    //-------------------------------------------------------
    // getEncodedRegionName
    // Purpose: gets EncodedRegionName from the HashMap
    //-------------------------------------------------------
    public String getEncodedRegionName(int index){

       inMap = outList.get(index);
       if (inMap==null) {
          System.out.println("HashMapArray::getEncodedRegionName:: inMap NULL");
          return null;
       }
       return inMap.get("EncodedRegionName").toString();
    }

    //-------------------------------------------------------
    // getRegionName
    // Purpose: gets regionName from the HashMap
    //-------------------------------------------------------
    public String getRegionName(int index){

       inMap = outList.get(index);
       if (inMap==null) {
          System.out.println("HashMapArray::getRegionName:: inMap NULL");
          return null;
       }
       return inMap.get("RegionName").toString();
    }

    //-------------------------------------------------------
    // getRegionOfflineStatus
    // Purpose: gets RegionOfflineStatus from the HashMap
    //-------------------------------------------------------
    public String getRegionOfflineStatus(int index){

       inMap = outList.get(index);
       if (inMap==null) {
          System.out.println("HashMapArray::getRegionOfflineStatus:: inMap NULL");
          return null;
       }
       return inMap.get("RegionOffline").toString();
    }

    //-------------------------------------------------------
    // getRegionId
    // Purpose: gets RegionId from the HashMap
    //-------------------------------------------------------
    public String getRegionId(int index){

       inMap = outList.get(index);
       if (inMap==null) {
          System.out.println("HashMapArray::getRegionId:: inMap NULL");
          return null;
       }
       return inMap.get("RegionID").toString();
    }

    //-------------------------------------------------------
    // getHostName
    // Purpose: gets HostName from the HashMap
    //-------------------------------------------------------
    public String getHostName(int index){

       inMap = outList.get(index);
       if (inMap==null) {
          System.out.println("HashMapArray::getHostName:: inMap NULL");
          return null;
       }
       return inMap.get("Hostname").toString();
    }

    //-------------------------------------------------------
    // getPort
    // Purpose: gets Port from the HashMap
    //-------------------------------------------------------
    public String getPort(int index){

       inMap = outList.get(index);
       if (inMap==null) {
          System.out.println("HashMapArray::getPort:: inMap NULL");
          return null;
       }
       return inMap.get("Port").toString();
    }

    //-------------------------------------------------------
    // getSize
    // Purpose: gets size of the list
    //-------------------------------------------------------
    public int getSize() {
       int listSize = outList.size();
       return listSize;
    }

    public HashMap<String, String> getInMap(int index){
       inMap = outList.get(index);
       return inMap;
    } 

 
    public static void main(String[] Args) {
       HashMap<String, String> map; 
      
       HashMapArray hm = new HashMapArray();

       hm.addElement(0, "Niece", "Caro");
       hm.addElement(0, "Mom", "Doris");
       hm.addElement(1, "Pet", "Nenna");
       hm.addElement(1, "Dad", "Andres");
       hm.addElement(1, "TableName", "SEABASE.T1");

       map = hm.getInMap(0);

       System.out.println();
       

       for(int i=0; i<hm.getSize(); i++){
          map = hm.getInMap(i);
          for(Map.Entry<String, String> entry : map.entrySet()){
             String key = entry.getKey();
             String value = entry.getValue();

             System.out.println("index: " + i + "  " + key + " = " + value);
          }
       }
    }
}



