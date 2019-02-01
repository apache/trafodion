/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
 */
package org.trafodion.libmgmt;

import java.io.File;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;

import org.trafodion.sql.udr.UDR;
import org.trafodion.sql.udr.UDRInvocationInfo;
import org.trafodion.sql.udr.UDRPlanInfo;
import org.trafodion.sql.udr.UDRException;

public class SyncLibUDF extends UDR {

    // default constructor
    public SyncLibUDF()
    {}

    @Override
    public void describeParamsAndColumns(UDRInvocationInfo info)
        throws UDRException
    {
        // This TMUDF takes no table-valued inputs, two string
        // parameters, and generates a table with a single integer
        // column (one row per parallel instance). It assumes that it
        // was created with the following DDL, without specifying
        // parameters or return values:
        //
        //  create table_mapping function SyncLibUDF()
        //  external name 'org.trafodion.libmgmt.SyncLibUDF'
        //  library ...;
        //
        // The call is like this: select ... from udf(synclibudf(<op>,<file>))
        //
        // <op> is 'c' to create/copy a file from the HDFS staging area for
        //             the current user into a local file on every node, or
        //         'd' to delete/drop a local file on every node
        // <file> is an unqualified file name (no '/' in the name)
        //
        // Example:
        //
        // select * from udf("_LIBMGR_".synclibudf('c', 'mylib.jar'));
        //
        // INSTANCE_NUM
        // ------------
        //
        //            0
        //            1
        //
        // --- 2 row(s) selected.
        // >>


        if (info.par().getNumColumns() != 2)
            throw new UDRException(38970, "This UDF needs to be called with two input parameters");
        if (info.getNumTableInputs() > 0)
            throw new UDRException(38971, "This UDF needs to be called without table-valued inputs");

        for (int i=0; i<2; i++)
            info.addFormalParameter(info.par().getColumn(i));
        info.out().addIntColumn("INSTANCE_NUM", false);
    }

    @Override
    public void describeDesiredDegreeOfParallelism(UDRInvocationInfo info,
                                                   UDRPlanInfo plan)
        throws UDRException
    {
        boolean usesVirtualNodes = false;

        // check for configurations with virtual nodes. Run the UDF serially
        // in those cases, since all the virtual nodes share the same node.
        String cmd[] = new String[] { "grep",
                                      "^[ \t]*_virtualnodes ",
                                      System.getenv("TRAF_CONF") +
                                      "/sqconfig" };

        try {
            Process p = Runtime.getRuntime().exec(cmd);
            int exitValue = p.waitFor();

            usesVirtualNodes = (exitValue == 0);
        } catch (Exception e) {
            // if this fails, we assume that we are using no virtual nodes
        }

        if (!usesVirtualNodes)
            // this TMUDF needs to run once on each node, since every
            // parallel instance will be writing to a local file on
            // that node
            plan.setDesiredDegreeOfParallelism(
                UDRPlanInfo.SpecialDegreeOfParallelism.
                ONE_INSTANCE_PER_NODE.getSpecialDegreeOfParallelism());
        else
            plan.setDesiredDegreeOfParallelism(1);
    }

    @Override
    public void processData(UDRInvocationInfo info,
                            UDRPlanInfo plan)
        throws UDRException
    {
        String operation  = info.par().getString(0);
        String fileName   = info.par().getString(1);
        Path srcHdfsPath  = null;
        Path dstLocalPath = null;

        try {
            FileMgmt.checkFileName(fileName);

            String user        = info.getSessionUser();
            File dstLocalDir   = new File(FileMgmt.getLocalLibDirName(user));
            File dstLocalFile  = new File(dstLocalDir, fileName);

            srcHdfsPath   = new Path(FileMgmt.getHdfsStagingDirName(user), fileName);
            dstLocalPath  = new Path(dstLocalDir.getPath(), fileName);

            // create the local directory or remove an existing file,
            // this is common for all operations
            if (!dstLocalDir.isDirectory())
                dstLocalDir.mkdirs();
            if (dstLocalFile.exists())
                dstLocalFile.delete();

            if (operation.compareTo("c") == 0) {
                // Using the HDFS Java interface, copy the file to a local file.
                // Note: The Hadoop configuration file (core-site.xml) needs
                // to be in the classpath
                Configuration conf = new Configuration(true);
                FileSystem  fs = FileSystem.get(srcHdfsPath.toUri(),conf);

                fs.copyToLocalFile(false, // do not delete src
                                   srcHdfsPath,
                                   dstLocalPath);
            }
        }
        catch (Exception e) {
            throw new UDRException(38973, "Error copying HDFS file %s to local file %s, reason: %s",
                                   (srcHdfsPath != null) ? srcHdfsPath.toString() : "null",
                                   (dstLocalPath != null) ? dstLocalPath.toString() : "null",
                                   e.getMessage());
        }

        // return my own instance number as the result
        info.out().setInt(0, info.getMyInstanceNum());
        emitRow(info);
    }
}
