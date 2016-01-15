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
import java.net.URL;

import org.apache.commons.configuration.Configuration;
import org.apache.commons.configuration.PropertiesConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.log4j.Logger;

public class CoprocessorUtils {
    private static Logger logger = Logger.getLogger(HBaseClient.class.getName());
    private static String[] coprocessors = null;

    static {
        init();
    }

    private static void init() {
        Configuration config = null;
        try {
            String path = System.getenv("MY_SQROOT") + "/etc/traf_coprocessor.properties";
            config = new PropertiesConfiguration(path);
        } catch (Exception e) {
            logger.error("error when finding trafcoprocess.properties");
            e.printStackTrace();
        }

        if (config != null) {
            coprocessors = config.getStringArray("coprocessors");
        }
    }

    public static void addCoprocessor(String currentAllClassName, HTableDescriptor desc) throws IOException {
        if (coprocessors == null) {
            return;
        }
        for (String coprocess : coprocessors) {
            if (currentAllClassName == null || !currentAllClassName.contains(coprocess)) {
                desc.addCoprocessor(coprocess);
            }
        }
    }

    public static void main(String[] args) {
        System.out.println("================CoprocessorUtils.main======================");
        //init();
        if (coprocessors == null) {
            return;
        }
        for (String coprocess : coprocessors) {
            System.out.println(coprocess);
        }
    }
}
