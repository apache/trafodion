/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
*/
package org.trafodion.dcs.zookeeper;

import java.io.*;
import java.util.concurrent.CountDownLatch;
import java.util.ArrayList;
import java.util.List;
import java.util.LinkedList;
import java.util.Iterator;
import java.nio.charset.Charset;

import org.apache.hadoop.conf.Configuration;
import org.apache.zookeeper.AsyncCallback;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.Op;
import org.apache.zookeeper.OpResult;
import org.apache.zookeeper.ZooKeeper.States;
import org.apache.zookeeper.data.ACL;
import org.apache.zookeeper.proto.CreateRequest;
import org.apache.zookeeper.proto.SetDataRequest;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.util.RetryCounter;
import org.trafodion.dcs.util.RetryCounterFactory;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.Bytes;

public class ZkClient implements Watcher {
	private static final Log LOG = LogFactory.getLog(ZkClient.class.getName());
	private String path;
	private List<String> children;
	private static final Charset CHARSET = Charset.forName("UTF-8");
	private Configuration conf;
	private CountDownLatch connectedSignal = new CountDownLatch(1);
	private String zkServers;
	private int port;
	private int sessionTimeout = 0;
	private ZooKeeper zk = null;
	private RetryCounterFactory retryCounterFactory;
	private int maxRetries = 0;
	private int retryIntervalMillis = 0;
	private final String identifier = null;
	private final byte[] id = null;
	private String parentZnode;
    private String checkPath;
    private boolean sessionRecoverSuccessful = true;

	// The metadata attached to each piece of data has the
	// format:
	//   <magic> 1-byte constant
	//   <id length> 4-byte big-endian integer (length of next field)
	//   <id> identifier corresponding uniquely to this process
	// It is prepended to the data supplied by the user.

	// the magic number is to be backward compatible
	private static final byte MAGIC =(byte) 0XFF;
	private static final int MAGIC_SIZE = Bytes.SIZEOF_BYTE;
	private static final int ID_LENGTH_OFFSET = MAGIC_SIZE;
	private static final int ID_LENGTH_SIZE =  Bytes.SIZEOF_INT;
	
	private void init() {
		this.parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);	   	
		this.path = this.parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT;
	    this.port = conf.getInt(Constants.ZOOKEEPER_CLIENT_PORT, Constants.DEFAULT_ZOOKEEPER_CLIENT_PORT);
	    String[] servers = conf.getStrings(Constants.ZOOKEEPER_QUORUM,Constants.LOCALHOST);
	    StringBuffer hosts = new StringBuffer();
	    for (int i = 0; i < servers.length; ++i) {
	    	if(i != 0 )
	    		hosts.append(",");
	    	hosts.append(servers[i]);
	    	hosts.append(":");
	    	hosts.append(port);
	    }
	    this.zkServers = hosts.toString();
		this.sessionTimeout = conf.getInt(Constants.ZK_SESSION_TIMEOUT,Constants.DEFAULT_ZK_SESSION_TIMEOUT);
		this.maxRetries = conf.getInt(Constants.ZK_RECOVERY_RETRY,Constants.DEFAULT_ZK_RECOVERY_RETRY);
		this.retryIntervalMillis = conf.getInt(Constants.ZK_RECOVERY_RETRY_INTERVAL_MILLIS,Constants.DEFAULT_ZK_RECOVERY_RETRY_INTERVAL_MILLIS);
		retryCounterFactory = new RetryCounterFactory(maxRetries, retryIntervalMillis);
		LOG.debug("ZooKeeper Servers:" + zkServers + ",SessionTimeout:" + this.sessionTimeout + ",MaxRetries:" + maxRetries + ",RetryIntervalMillis:" + retryIntervalMillis);
	}
	
	public String getZkQuorum(){
		return this.zkServers;
	}
	
	public ZkClient() {
		this.conf = DcsConfiguration.create();
    	init();
	}

	public ZkClient(int sessionTimeout,int maxRetries,int retryIntervalMillis) {
		this.conf = DcsConfiguration.create();
		this.conf.setInt(Constants.ZK_SESSION_TIMEOUT,sessionTimeout);
		this.conf.setInt(Constants.ZK_RECOVERY_RETRY,maxRetries);
		this.conf.setInt(Constants.ZK_RECOVERY_RETRY_INTERVAL_MILLIS,retryIntervalMillis);
		init();
	}
	
	public ZkClient(String zkhost, int zkport) {
		this.conf = DcsConfiguration.create();
		this.conf.setStrings(Constants.ZOOKEEPER_QUORUM,zkhost);
		this.conf.setInt(Constants.ZOOKEEPER_CLIENT_PORT, zkport);
		init();
	}

    public void connect() throws IOException, InterruptedException {
        connect(false);
    }

	public void connect(boolean force) throws IOException, InterruptedException {
	    if (force) {
            LOG.debug("Force reconnect to Zookeeper.");
            connectedSignal = new CountDownLatch(1);
        }

		if(zk==null || force) {
			this.zk = new ZooKeeper(zkServers, sessionTimeout, this);
			
			//wait 3 seconds to connect
			int retries=0; 
			while(this.zk.getState() != ZooKeeper.States.CONNECTED) {
				LOG.debug("Zookeeper.State=" + this.zk.getState());
				try {
					Thread.sleep(1000L);//1 second
					retries++;
				} catch (InterruptedException ie) {}
				
				if(retries > 3)
					break;
			}
			
			if(this.zk.getState() != ZooKeeper.States.CONNECTED) {
				this.zk.close();
				LOG.error("Zookeeper.State [" + this.zk.getState() + "]");
				this.zk=null;
				throw new IOException("Cannot connect to Zookeeper");
			}

            // Solve the forcible reconnection
            // When zk reconn, the backup-master may take over the master,
            // so current master should restart, and queues in /dcs/master/leader
            if (LOG.isDebugEnabled()) {
                LOG.debug("force = [" + force + "]. checkPath = [" + checkPath + "]");
            }
            if (force && checkPath != null) {
                try {
                    Stat stat = zk.exists(checkPath, false);
                    if (LOG.isDebugEnabled()) {
                        LOG.debug("stat = [" + stat + "].");
                    }
                    if (stat == null) {
                        // this means master has change.
                        setSessionRecoverSuccessful(false);
                    }
                } catch (KeeperException e) {
                    throw new IOException(e);
                }
            }

			connectedSignal.await();
		}
	}
	
	public void resetZk() throws IOException, InterruptedException {
		zk = null;
	}
	
	public void close() throws InterruptedException {
		if(zk != null) 
			zk.close();
		this.sessionTimeout = 0;
		zk = null;
	}
	
	public ZooKeeper getZk() {
		return zk;
	}
	
	@Override
	public void process(WatchedEvent event) {
		if(event.getState() == Watcher.Event.KeeperState.SyncConnected) {
			connectedSignal.countDown();
		} else if (event.getState() == Watcher.Event.KeeperState.Expired) {
            LOG.info("session expired. now rebuilding");
            // session expired, may be never happending. but if it happen there
            // need to close old client and rebuild new client
            try {
                connect(true);
            } catch (IOException e) {
                setSessionRecoverSuccessful(false);
                LOG.error("session expired and throw IOException while do reconnect: " + e.getMessage(), e);
            } catch (InterruptedException e) {
                LOG.error("session expired and throw InterruptedException while do reconnect: " + e.getMessage(), e);
            }
        }
	}

	public void create(String path, String value, boolean ephemeral) 
	throws KeeperException, InterruptedException {
		Stat stat = zk.exists(path,false);
		if(stat==null) {
			if(ephemeral==true)
				zk.create(path,value.getBytes(CHARSET),ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.EPHEMERAL);
			else
				zk.create(path,value.getBytes(CHARSET),ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.PERSISTENT);
		} else {
			zk.setData(path,value.getBytes(CHARSET), -1);
		}
	}
	
	public String get(String path, Watcher watcher) 
	throws KeeperException, InterruptedException {
		byte[] data = zk.getData(path, watcher, null/*stat*/);
		return new String(data, CHARSET);
	}
	
	/**
	 * delete is an idempotent operation. Retry before throwing exception.
	 * This function will not throw NoNodeException if the path does not
	 * exist.
	 */
	public void delete(String path, int version)
	throws InterruptedException, KeeperException {
		RetryCounter retryCounter = retryCounterFactory.create();
		boolean isRetry = false; // False for first attempt, true for all retries.
		while (true) {
			try {
				zk.delete(path, version);
				return;
			} catch (KeeperException e) {
				switch (e.code()) {
				case NONODE:
					if (isRetry) {
						LOG.info("Node " + path + " already deleted. Assuming that a " +
						"previous attempt succeeded.");
						return;
					}
					LOG.warn("Node " + path + " already deleted, and this is not a " +
					"retry");
					throw e;

				case CONNECTIONLOSS:
				case SESSIONEXPIRED:
				case OPERATIONTIMEOUT:
					retryOrThrow(retryCounter, e, "delete");
					break;

				default:
					throw e;
				}
			}
			retryCounter.sleepUntilNextRetry();
			retryCounter.useRetry();
			isRetry = true;
		}
	}

	/**
	 * exists is an idempotent operation. Retry before throwing exception
	 * @return A Stat instance
	 */
	public Stat exists(String path, Watcher watcher)
	throws KeeperException, InterruptedException {
		RetryCounter retryCounter = retryCounterFactory.create();
		while (true) {
			try {
				return zk.exists(path, watcher);
			} catch (KeeperException e) {
				switch (e.code()) {
				case CONNECTIONLOSS:
				case SESSIONEXPIRED:
				case OPERATIONTIMEOUT:
					retryOrThrow(retryCounter, e, "exists");
					break;

				default:
					throw e;
				}
			}
			retryCounter.sleepUntilNextRetry();
			retryCounter.useRetry();
		}
	}

	/**
	 * exists is an idempotent operation. Retry before throwing exception
	 * @return A Stat instance
	 */
	public Stat exists(String path, boolean watch)
	throws KeeperException, InterruptedException {
		RetryCounter retryCounter = retryCounterFactory.create();
		while (true) {
			try {
				return zk.exists(path, watch);
			} catch (KeeperException e) {
				switch (e.code()) {
				case CONNECTIONLOSS:
				case SESSIONEXPIRED:
				case OPERATIONTIMEOUT:
					retryOrThrow(retryCounter, e, "exists");
					break;

				default:
					throw e;
				}
			}
			retryCounter.sleepUntilNextRetry();
			retryCounter.useRetry();
		}
	}
	
	private void retryOrThrow(RetryCounter retryCounter, KeeperException e,
			String opName) throws KeeperException {
		LOG.warn("Possibly transient ZooKeeper exception: " + e);
		if (!retryCounter.shouldRetry()) {
			LOG.error("ZooKeeper " + opName + " failed after "
					+ retryCounter.getMaxRetries() + " retries");
			throw e;
		}
	}

	/**
	 * getChildren is an idempotent operation. Retry before throwing exception
	 * @return List of children znodes
	 */
	public List<String> getChildren(String path, Watcher watcher)
	throws KeeperException, InterruptedException {
		RetryCounter retryCounter = retryCounterFactory.create();
		while (true) {
			try {
				return zk.getChildren(path, watcher);
			} catch (KeeperException e) {
				switch (e.code()) {
				case CONNECTIONLOSS:
				case SESSIONEXPIRED:
				case OPERATIONTIMEOUT:
					retryOrThrow(retryCounter, e, "getChildren");
					break;

				default:
					throw e;
				}
			}
			retryCounter.sleepUntilNextRetry();
			retryCounter.useRetry();
		}
	}
	
	  /**
	   * getData is an idempotent operation. Retry before throwing exception
	   * @return Data
	   */
	  public byte[] getData(String path, Watcher watcher, Stat stat)
	  throws KeeperException, InterruptedException {
	    RetryCounter retryCounter = retryCounterFactory.create();
	    while (true) {
	      try {
	        byte[] revData = zk.getData(path, watcher, stat);       
	        return this.removeMetaData(revData);
	      } catch (KeeperException e) {
	        switch (e.code()) {
	          case CONNECTIONLOSS:
	          case SESSIONEXPIRED:
	          case OPERATIONTIMEOUT:
	            retryOrThrow(retryCounter, e, "getData");
	            break;

	          default:
	            throw e;
	        }
	      }
	      retryCounter.sleepUntilNextRetry();
	      retryCounter.useRetry();
	    }
	  }

	  /**
	   * getData is an idemnpotent operation. Retry before throwing exception
	   * @return Data
	   */
	  public byte[] getData(String path, boolean watch, Stat stat)
	  throws KeeperException, InterruptedException {
	    RetryCounter retryCounter = retryCounterFactory.create();
	    while (true) {
	      try {
	        byte[] revData = zk.getData(path, watch, stat);
	        return this.removeMetaData(revData);
	      } catch (KeeperException e) {
	        switch (e.code()) {
	          case CONNECTIONLOSS:
	          case SESSIONEXPIRED:
	          case OPERATIONTIMEOUT:
	            retryOrThrow(retryCounter, e, "getData");
	            break;

	          default:
	            throw e;
	        }
	      }
	      retryCounter.sleepUntilNextRetry();
	      retryCounter.useRetry();
	    }
	  }

	  /**
	   * setData is NOT an idempotent operation. Retry may cause BadVersion Exception
	   * Adding an identifier field into the data to check whether 
	   * badversion is caused by the result of previous correctly setData
	   * @return Stat instance
	   */
	  public Stat setData(String path, byte[] data, int version)
	  throws KeeperException, InterruptedException {
	    RetryCounter retryCounter = retryCounterFactory.create();
//	    byte[] newData = appendMetaData(data);
	    byte[] newData = data;
	    while (true) {
	      try {
	        return zk.setData(path, newData, version);
	      } catch (KeeperException e) {
	        switch (e.code()) {
	          case CONNECTIONLOSS:
	          case SESSIONEXPIRED:
	          case OPERATIONTIMEOUT:
	            retryOrThrow(retryCounter, e, "setData");
	            break;
	          case BADVERSION:
	            // try to verify whether the previous setData success or not
	            try{
	              Stat stat = new Stat();
	              byte[] revData = zk.getData(path, false, stat);
	              if (Bytes.equals(revData, newData)) {
	                // the bad version is caused by previous successful setData
	                return stat;
	              }
	            } catch(KeeperException keeperException){
	              // the ZK is not reliable at this moment. just throwing exception
	              throw keeperException;
	            }
	            break;
	          
	          // throw other exceptions and verified bad version exceptions
	          default:
	            throw e;
	        }
	      }
	      retryCounter.sleepUntilNextRetry();
	      retryCounter.useRetry();
	    }
	  }

	  /**
	   * <p>
	   * NONSEQUENTIAL create is idempotent operation. 
	   * Retry before throwing exceptions.
	   * But this function will not throw the NodeExist exception back to the
	   * application.
	   * </p>
	   * <p>
	   * But SEQUENTIAL is NOT idempotent operation. It is necessary to add 
	   * identifier to the path to verify, whether the previous one is successful 
	   * or not.
	   * </p>
	   * 
	   * @return Path
	   */
	  public String create(String path, byte[] data, List<ACL> acl,
	      CreateMode createMode)
	  throws KeeperException, InterruptedException {
//	    byte[] newData = appendMetaData(data);
	    byte[] newData = data;		  
	    switch (createMode) {
	      case EPHEMERAL:
	      case PERSISTENT:
	        return createNonSequential(path, newData, acl, createMode);

	      case EPHEMERAL_SEQUENTIAL:
	      case PERSISTENT_SEQUENTIAL:
	        return createSequential(path, newData, acl, createMode);

	      default:
	        throw new IllegalArgumentException("Unrecognized CreateMode: " + 
	            createMode);
	    }
	  }

	  private String createNonSequential(String path, byte[] data, List<ACL> acl, 
	      CreateMode createMode) throws KeeperException, InterruptedException {
	    RetryCounter retryCounter = retryCounterFactory.create();
	    boolean isRetry = false; // False for first attempt, true for all retries.
	    while (true) {
	      try {
	        return zk.create(path, data, acl, createMode);
	      } catch (KeeperException e) {
	        switch (e.code()) {
	          case NODEEXISTS:
	            if (isRetry) {
	              // If the connection was lost, there is still a possibility that
	              // we have successfully created the node at our previous attempt,
	              // so we read the node and compare. 
	              byte[] currentData = zk.getData(path, false, null);
	              if (currentData != null &&
	                  Bytes.compareTo(currentData, data) == 0) { 
	                // We successfully created a non-sequential node
	                return path;
	              }
	              LOG.error("Node " + path + " already exists with " + 
	                  Bytes.toStringBinary(currentData) + ", could not write " +
	                  Bytes.toStringBinary(data));
	              throw e;
	            }
	            LOG.info("Node " + path + " already exists and this is not a " +
	                "retry");
	            throw e;

	          case CONNECTIONLOSS:
	          case SESSIONEXPIRED:
	          case OPERATIONTIMEOUT:
	            retryOrThrow(retryCounter, e, "create");
	            break;

	          default:
	            throw e;
	        }
	      }
	      retryCounter.sleepUntilNextRetry();
	      retryCounter.useRetry();
	      isRetry = true;
	    }
	  }
	  
	  private String createSequential(String path, byte[] data, 
	      List<ACL> acl, CreateMode createMode)
	  throws KeeperException, InterruptedException {
	    RetryCounter retryCounter = retryCounterFactory.create();
	    boolean first = true;
	    //String newPath = path+this.identifier;
	    String newPath = path;
	    while (true) {
	      try {
	        if (!first) {
	          // Check if we succeeded on a previous attempt
	          String previousResult = findPreviousSequentialNode(newPath);
	          if (previousResult != null) {
	            return previousResult;
	          }
	        }
	        first = false;
	        return zk.create(newPath, data, acl, createMode);
	      } catch (KeeperException e) {
	        switch (e.code()) {
	          case CONNECTIONLOSS:
	          case SESSIONEXPIRED:
	          case OPERATIONTIMEOUT:
	            retryOrThrow(retryCounter, e, "create");
	            break;

	          default:
	            throw e;
	        }
	      }
	      retryCounter.sleepUntilNextRetry();
	      retryCounter.useRetry();
	    }
	  }

	  /**
	   * Convert Iterable of {@link ZKOp} we got into the ZooKeeper.Op
	   * instances to actually pass to multi (need to do this in order to appendMetaData).
	   */
	  private Iterable<Op> prepareZKMulti(Iterable<Op> ops)
	  throws UnsupportedOperationException {
	    if(ops == null) return null;

	    List<Op> preparedOps = new LinkedList<Op>();
	    for (Op op : ops) {
	      if (op.getType() == ZooDefs.OpCode.create) {
	        CreateRequest create = (CreateRequest)op.toRequestRecord();
	        preparedOps.add(Op.create(create.getPath(), appendMetaData(create.getData()),
	          create.getAcl(), create.getFlags()));
	      } else if (op.getType() == ZooDefs.OpCode.delete) {
	        // no need to appendMetaData for delete
	        preparedOps.add(op);
	      } else if (op.getType() == ZooDefs.OpCode.setData) {
	        SetDataRequest setData = (SetDataRequest)op.toRequestRecord();
	        preparedOps.add(Op.setData(setData.getPath(), appendMetaData(setData.getData()),
	          setData.getVersion()));
	      } else {
	        throw new UnsupportedOperationException("Unexpected ZKOp type: " + op.getClass().getName());
	      }
	    }
	    return preparedOps;
	  }

	  /**
	   * Run multiple operations in a transactional manner. Retry before throwing exception
	   */
	  public List<OpResult> multi(Iterable<Op> ops)
	  throws KeeperException, InterruptedException {
	    RetryCounter retryCounter = retryCounterFactory.create();
	    Iterable<Op> multiOps = prepareZKMulti(ops);
	    while (true) {
	      try {
	        return zk.multi(multiOps);
	      } catch (KeeperException e) {
	        switch (e.code()) {
	          case CONNECTIONLOSS:
	          case SESSIONEXPIRED:
	          case OPERATIONTIMEOUT:
	            retryOrThrow(retryCounter, e, "multi");
	            break;

	          default:
	            throw e;
	        }
	      }
	      retryCounter.sleepUntilNextRetry();
	      retryCounter.useRetry();
	    }
	  }

	  private String findPreviousSequentialNode(String path)
	    throws KeeperException, InterruptedException {
	    int lastSlashIdx = path.lastIndexOf('/');
	    assert(lastSlashIdx != -1);
	    String parent = path.substring(0, lastSlashIdx);
	    String nodePrefix = path.substring(lastSlashIdx+1);

	    List<String> nodes = zk.getChildren(parent, false);
	    List<String> matching = filterByPrefix(nodes, nodePrefix);
	    for (String node : matching) {
	      String nodePath = parent + "/" + node;
	      Stat stat = zk.exists(nodePath, false);
	      if (stat != null) {
	        return nodePath;
	      }
	    }
	    return null;
	  }
	  
	  public byte[] removeMetaData(byte[] data) {
	    if(data == null || data.length == 0) {
	      return data;
	    }
	    // check the magic data; to be backward compatible
	    byte magic = data[0];
	    if(magic != MAGIC) {
	      return data;
	    }
	    
	    int idLength = Bytes.toInt(data, ID_LENGTH_OFFSET);
	    int dataLength = data.length-MAGIC_SIZE-ID_LENGTH_SIZE-idLength;
	    int dataOffset = MAGIC_SIZE+ID_LENGTH_SIZE+idLength;

	    byte[] newData = new byte[dataLength];
	    System.arraycopy(data, dataOffset, newData, 0, dataLength);
	    
	    return newData;
	    
	  }
	  
	  private byte[] appendMetaData(byte[] data) {
	    if(data == null || data.length == 0){
	      return data;
	    }
	    
	    byte[] newData = new byte[MAGIC_SIZE+ID_LENGTH_SIZE+id.length+data.length];
	    int pos = 0;
	    pos = Bytes.putByte(newData, pos, MAGIC);
	    pos = Bytes.putInt(newData, pos, id.length);
	    pos = Bytes.putBytes(newData, pos, id, 0, id.length);
	    pos = Bytes.putBytes(newData, pos, data, 0, data.length);

	    return newData;
	  }

	  public long getSessionId() {
	    return zk.getSessionId();
	  }

	  public States getState() {
	    return zk.getState();
	  }

	  public ZooKeeper getZooKeeper() {
	    return zk;
	  }

	  public byte[] getSessionPasswd() {
	    return zk.getSessionPasswd();
	  }

	  public void sync(String path, AsyncCallback.VoidCallback cb, Object ctx) {
	    this.zk.sync(path, null, null);
	  }
	  /**
	   * Filters the given node list by the given prefixes.
	   * This method is all-inclusive--if any element in the node list starts
	   * with any of the given prefixes, then it is included in the result.
	   *
	   * @param nodes the nodes to filter
	   * @param prefixes the prefixes to include in the result
	   * @return list of every element that starts with one of the prefixes
	   */
	  private static List<String> filterByPrefix(List<String> nodes, 
	      String... prefixes) {
	    List<String> lockChildren = new ArrayList<String>();
	    for (String child : nodes){
	      for (String prefix : prefixes){
	        if (child.startsWith(prefix)){
	          lockChildren.add(child);
	          break;
	        }
	      }
	    }
	    return lockChildren;
	  }	  

    public String getCheckPath() {
        return checkPath;
    }

    public void setCheckPath(String checkPath) {
        this.checkPath = checkPath;
    }

    public boolean isSessionRecoverSuccessful() {
        return sessionRecoverSuccessful;
    }

    public void setSessionRecoverSuccessful(boolean sessionRecoverSuccessful) {
        this.sessionRecoverSuccessful = sessionRecoverSuccessful;
    }
	public static void main(String [] args) throws Exception {
		ZkClient zkc = new ZkClient();
	}
}
