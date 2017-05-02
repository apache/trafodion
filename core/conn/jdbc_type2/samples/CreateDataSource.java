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
//
// Filename : CreateDataSource.java
//
// This program creates the SQLMXDataSource object and registers it
// with Java Naming and Directory Interface(JNDI).
//
// Sun's service provider for accessing the file system are used to
// register the SQLMXDataSource object.
// You can download File System Service Provider from the JNDI section of
// java.sun.com. This download include providerutil.jar and fscontext.jar
//
// Surf to http://java.sun.com/products/jndi/downloads/index.html
// for downloading
//
// Create a directory dataSources/jdbc before running this program.
// Make sure providerutil.jar and fscontext.jar are in CLASSPATH
// You can run this program only once successfully.
//
// If you need to run this program again, delete .bindings file from
// dataSources/jdbc.
//
// The program takes the path name of the location of the dataSources
// directory as a command line argument.
//
import java.util.*;
import javax.naming.*;
import org.apache.trafodion.jdbc.t2.SQLMXDataSource;

public class CreateDataSource {

    public static void main(String[] args) {


        if ( args.length == 0 )
        {
            System.out.println("CreateDataSource: The dataSources directory path is required.");
            System.out.println("CreateDataSource: If the dataSource is in current directory use:");
            System.out.println("CreateDataSource:     java CreateDataSource `pwd`");
            return;
        }



        System.out.println("CreateDataSource: Started");
        String   RootDir = "file://" + args[0] + "/dataSources";

        try {
            SQLMXDataSource ds = new SQLMXDataSource();

            ds.setDescription("Test Trafodion Data Source created for Testing data source ");

// As an example, the following lines demonstrate how to alter the default
// location of the Trafodion catalog and schema used for this Data Source.
//            ds.setCatalog("CAT");
//            ds.setSchema("SCH");

            Hashtable<String,String> env = new Hashtable<String,String>();
            env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.fscontext.RefFSContextFactory");
            env.put(Context.PROVIDER_URL, RootDir);
            try {
                Context ctx = new InitialContext(env);
                ctx.rebind("jdbc/TestDataSource", ds);
            }
            catch (Exception e) {
                System.out.println("CreateDataSource: rebind: Exception Occurred");
                System.out.println("CreateDataSource: rebind: Message: " + e.getMessage());
                e.printStackTrace();
                System.out.println("CreateDataSource: Demo Failed");
                return;
            }
        }
        catch (Exception e) {
            System.out.println("CreateDataSource: Exception Occurred");
            System.out.println("CreateDataSource: Message: " + e.getMessage());
            System.out.println("CreateDataSource: Demo Failed");
            return;
        }
        System.out.println("CreateDataSource: Demo Successful");
    }
}

