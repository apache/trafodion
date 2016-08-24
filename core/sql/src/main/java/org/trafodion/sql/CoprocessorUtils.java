// @@@ START COPYRIGHT @@@
// //
// // Licensed to the Apache Software Foundation (ASF) under one
// // or more contributor license agreements.  See the NOTICE file
// // distributed with this work for additional information
// // regarding copyright ownership.  The ASF licenses this file
// // to you under the Apache License, Version 2.0 (the
// // "License"); you may not use this file except in compliance
// // with the License.  You may obtain a copy of the License at
// //
// //   http://www.apache.org/licenses/LICENSE-2.0
// //
// // Unless required by applicable law or agreed to in writing,
// // software distributed under the License is distributed on an
// // "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// // KIND, either express or implied.  See the License for the
// // specific language governing permissions and limitations
// // under the License.
// //
// // @@@ END COPYRIGHT @@@

package org.trafodion.sql;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.commons.configuration.Configuration;
import org.apache.commons.configuration.ConfigurationException;
import org.apache.commons.configuration.PropertiesConfiguration;
import org.apache.commons.lang.StringUtils;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.Coprocessor;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.log4j.Logger;

public class CoprocessorUtils {
    private static Logger logger = Logger.getLogger(CoprocessorUtils.class.getName());
    private static List<String> coprocessors = new ArrayList<String>();
    private static List<String> buildInCoprocessors = new ArrayList<String>();
    private static String MVCC = null;
    private static String SSCC = null;
    private static Path hdfsPath = null;
    private static String HBASE_TRX_JAR_NAME = null;
    static {
        init();
    }

    private static void init() {
        Configuration config = null;
        try {
            String path = System.getenv("MY_SQROOT") + "/etc/traf_coprocessor.properties";
            config = new PropertiesConfiguration(path);
        } catch (ConfigurationException e) {
            logger.error("error when finding trafcoprocess.properties");
            e.printStackTrace();
        }

        if (config != null) {
            for (String coprocessor : config.getStringArray("coprocessors")) {
                coprocessors.add(coprocessor);
            }
            for (String coprocessor : config.getStringArray("build_in_coprocessors")) {
            	buildInCoprocessors.add(coprocessor);
            }
            MVCC = config.getString("MVCC");
            SSCC = config.getString("SSCC");
            HBASE_TRX_JAR_NAME= config.getString("HBASE_TRX_JAR_NAME");
        }
        org.apache.hadoop.conf.Configuration conf = HBaseConfiguration.create();
        String dynamicDir = conf.get("hbase.dynamic.jars.dir");
        Path hPath = new Path(dynamicDir);

        try {
            FileSystem fs = FileSystem.get(conf);
            FileStatus[] status = fs.listStatus(hPath);
            Path[] listedPaths = FileUtil.stat2Paths(status);

            for (Path p : listedPaths) {
                if (p.getName().contains(HBASE_TRX_JAR_NAME)) {
                    hdfsPath = p;
                    break;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        if (hdfsPath == null) {
            throw new RuntimeException("can't find hbase-trx*.jar in HDFS, pls make sure trafodion installation is correct!");
        }

    }

    //boolean as return ,to make sure whether changes take place in HTableDescriptor
    public static boolean addCoprocessor(String currentAllClassName, HTableDescriptor desc, boolean isMVCC) throws IOException {
        boolean retVal = false;
        if (coprocessors.size()==0 && buildInCoprocessors.size()==0 && StringUtils.isBlank(MVCC) && StringUtils.isBlank(SSCC)) {
            return retVal;
        }

        for (String coprocess : buildInCoprocessors) {
            if ((currentAllClassName == null || !currentAllClassName.contains(coprocess)) && !desc.hasCoprocessor(coprocess)) {
                desc.addCoprocessor(coprocess);
                retVal = true;
            }
        }

        for (String coprocess : coprocessors) {
            if ((currentAllClassName == null || !currentAllClassName.contains(coprocess)) && !desc.hasCoprocessor(coprocess)) {
                desc.addCoprocessor(coprocess, hdfsPath, Coprocessor.PRIORITY_USER, null);
                retVal = true;
            }
        }
        
        if (isMVCC && (currentAllClassName == null || !currentAllClassName.contains(MVCC)) && !desc.hasCoprocessor(MVCC)) {
            desc.addCoprocessor(MVCC, hdfsPath, Coprocessor.PRIORITY_USER, null);
            retVal = true;
        } else if (!isMVCC && (currentAllClassName == null || !currentAllClassName.contains(SSCC)) && !desc.hasCoprocessor(SSCC)) {
            desc.addCoprocessor(SSCC, hdfsPath, Coprocessor.PRIORITY_USER, null);
            retVal = true;
        }

        return retVal;
    }

    public static boolean addCoprocessor(String currentAllClassName, HTableDescriptor desc) throws IOException {
        return addCoprocessor(currentAllClassName, desc, true);
    }
    public static void main(String[] args) throws IOException {
        System.out.println("================CoprocessorUtils.main======================");
        String currentAllClassName = "";
        HTableDescriptor desc = new HTableDescriptor();
        boolean isMVCC = true;
        addCoprocessor(currentAllClassName, desc, isMVCC);

        List<String> list = desc.getCoprocessors();

        for (String string : list) {
            System.out.println(string);
        }
        System.out.println("================CoprocessorUtils.main======================");
    }
}

