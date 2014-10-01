// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

package  org.trafodion.sql.HBaseAccess;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.util.Methods;
import org.apache.hadoop.hbase.util.Pair;
import org.apache.hadoop.security.token.Token;

import java.io.IOException;
import java.util.List;

public class TrafBulkLoadClient {
  private static Class protocolClazz;
  private static Class endpointClazz;
  private Object proxy;
  private HTable table;

  public TrafBulkLoadClient(HTable table) throws IOException {
    this(table, HConstants.EMPTY_START_ROW);
  }

  public TrafBulkLoadClient(HTable table, byte[] startRow) throws IOException {
    try {
      protocolClazz = protocolClazz!=null?protocolClazz:
          Class.forName("org.trafodion.sql.extensions.TrafBulkLoadProtocol");
      endpointClazz = endpointClazz!=null?endpointClazz:
          Class.forName("org.trafodion.sql.extensions.TrafBulkLoadEndpoint");
      //proxy = table.coprocessorProxy(protocolClazz, startRow);
      this.table = table;  
    } catch (ClassNotFoundException e) {
      throw new IOException("Failed to initialize SecureBulkLoad", e);
    }
  }

  public String prepareBulkLoad(byte[] tableName) throws IOException {
    try {
      String bulkToken = (String) Methods.call(protocolClazz, proxy,
          "prepareBulkLoad", new Class[]{byte[].class}, new Object[]{tableName});
      return bulkToken;
    } catch (Exception e) {
      throw new IOException("Failed to prepareBulkLoad", e);
    }
  }

  public void cleanupBulkLoad(String bulkToken) throws IOException {
    try {
      Methods.call(protocolClazz, proxy,
          "cleanupBulkLoad", new Class[]{String.class},new Object[]{bulkToken});
    } catch (Exception e) {
      throw new IOException("Failed to cleanupBulkLoad", e);
    }
  }

  public Path getStagingPath(String bulkToken, byte[] family) throws IOException {
    try {
      return (Path)Methods.call(endpointClazz, null, "getStagingPath",
          new Class[]{Configuration.class, String.class, byte[].class},
          new Object[]{table.getConfiguration(), bulkToken, family});
    } catch (Exception e) {
      throw new IOException("Failed to getStagingPath", e);
    }
  }
}
