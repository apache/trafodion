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

package  org.trafodion.sql.extensions;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.classification.InterfaceAudience;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.permission.FsPermission;
import org.apache.hadoop.hbase.CoprocessorEnvironment;
import org.apache.hadoop.hbase.DoNotRetryIOException;
import org.apache.hadoop.hbase.coprocessor.BaseEndpointCoprocessor;
import org.apache.hadoop.hbase.coprocessor.RegionCoprocessorEnvironment;
import org.apache.hadoop.hbase.ipc.RequestContext;
import org.apache.hadoop.hbase.regionserver.HRegion;
import org.apache.hadoop.hbase.security.User;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.Methods;
import org.apache.hadoop.hbase.util.Pair;
import org.apache.hadoop.security.UserGroupInformation;
import org.apache.hadoop.security.token.Token;

import java.io.IOException;
import java.math.BigInteger;
import java.security.PrivilegedAction;
import java.security.SecureRandom;
import java.util.List;



public class TrafBulkLoadEndpoint extends BaseEndpointCoprocessor
    implements TrafBulkLoadProtocol {

  public static final long VERSION = 0L;

  //Random number is 320 bits wide
  private static final int RANDOM_WIDTH = 320;
  //We picked 32 as the radix, so the character set
  //will only contain alpha numeric values
  //320/5 = 64 characters
  private static final int RANDOM_RADIX = 32;

  private static Log LOG = LogFactory.getLog(TrafBulkLoadEndpoint.class);

  private final static FsPermission PERM_ALL_ACCESS = FsPermission.valueOf("-rwxrwxrwx");
  private final static FsPermission PERM_HIDDEN = FsPermission.valueOf("-rwx--x--x");
  private final static String BULKLOAD_STAGING_DIR = "hbase.bulkload.staging.dir";

  private SecureRandom random;
  private FileSystem fs;
  private Configuration conf;

  private Path baseStagingDir;

  private RegionCoprocessorEnvironment env;


  @Override
  public void start(CoprocessorEnvironment env) {
    super.start(env);

    this.env = (RegionCoprocessorEnvironment)env;
    random = new SecureRandom();
    conf = env.getConfiguration();
    baseStagingDir = getBaseStagingDir(conf);

    try {
      fs = FileSystem.get(conf);
      fs.mkdirs(baseStagingDir, PERM_HIDDEN);
      fs.setPermission(baseStagingDir, PERM_HIDDEN);
      //no sticky bit in hadoop-1.0, making directory nonempty so it never gets erased
      fs.mkdirs(new Path(baseStagingDir,"DONOTERASE"), PERM_HIDDEN);
      FileStatus status = fs.getFileStatus(baseStagingDir);
      if(status == null) {
        throw new IllegalStateException("Failed to create staging directory");
      }
      if(!status.getPermission().equals(PERM_HIDDEN)) {
        throw new IllegalStateException("Directory already exists but permissions aren't set to '-rwx--x--x' ");
      }
    } catch (IOException e) {
      throw new IllegalStateException("Failed to get FileSystem instance",e);
    }
  }

  @Override
  public String prepareBulkLoad(byte[] tableName) throws IOException {
    return createStagingDir(baseStagingDir,  tableName).toString();
  }

  @Override
  public void cleanupBulkLoad(String bulkToken) throws IOException {
    fs.delete(createStagingDir(baseStagingDir,
        env.getRegion().getTableDesc().getName(),
        new Path(bulkToken).getName()),
        true);
  }
  
  private Path createStagingDir(Path baseDir,  byte[] tableName) throws IOException {
    String randomDir = Bytes.toString(tableName)+"__"+
        (new BigInteger(RANDOM_WIDTH, random).toString(RANDOM_RADIX));
    return createStagingDir(baseDir, tableName, randomDir);
  }

  private Path createStagingDir(Path baseDir,
                                byte[] tableName,
                                String randomDir) throws IOException {
    Path p = new Path(baseDir, randomDir);
    fs.mkdirs(p, PERM_ALL_ACCESS);
    fs.setPermission(p, PERM_ALL_ACCESS);
    return p;
  }


  public static Path getStagingPath(Configuration conf, String bulkToken, byte[] family) {
    Path stageP = new Path(getBaseStagingDir(conf), bulkToken);
    return new Path(stageP, Bytes.toString(family));
  }

  private static Path getBaseStagingDir(Configuration conf) {
    return new Path(conf.get(BULKLOAD_STAGING_DIR, "/tmp/hbase-staging"));
  }
 
}
