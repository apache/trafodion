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
import java.io.FileFilter;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.nio.file.Files;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.permission.FsAction;
import org.apache.hadoop.fs.permission.FsPermission;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class FileMgmt {
	private static final Logger LOG = LoggerFactory.getLogger(FileMgmt.class);
	private static final String url = "jdbc:default:connection";
	// 100Mb
	private static final long MAX_JAR_FILE_SIZE = 104857600;
	private static final SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
	private static final int MaxDataSize = 12800;
	private static final String CHARSET = "ISO-8859-1";

	/**
	 * Print help info
	 * 
	 * @param helps:
	 *            INOUT parameter like PUT/LS/...
	 */
	public static void help(String[] helps) {
		String[] help = new String[] {
				"PUTFILE - Upload a library file. SHOWDDL PROCEDURE [SCHEMA NAME.]PUTFILE for more info.",
				"LS - List library files. SHOWDDL PROCEDURE [SCHEMA NAME.]LS for more info.",
				"LSALL - List all library files. SHOWDDL PROCEDURE [SCHEMA NAME.]LSALL for more info.",
				"RM - Remove a library file. SHOWDDL PROCEDURE [SCHEMA NAME.]RM for more info.",
				"RMREX - Remove library files by a perticular pattern. SHOWDDL PROCEDURE [SCHEMA NAME.]RMREX for more info.",
				"GETFILE - Download a library file. SHOWDDL PROCEDURE [SCHEMA NAME.]GETFILE for more info.",
				"ADDLIB - Create a library. SHOWDDL PROCEDURE [SCHEMA NAME.]ADDLIB for more info.",
				"ALTERLIB - Update a library. SHOWDDL PROCEDURE [SCHEMA NAME.]ALTERLIB for more info.",
				"DROPLIB - Drop a library. SHOWDDL PROCEDURE [SCHEMA NAME.]DROPLIB for more info."
		};
		List<String> index = new ArrayList<String>(help.length);
		index.add("PUTFILE");
		index.add("LS");
		index.add("LSALL");
		index.add("RM");
		index.add("RMREX");
		index.add("GETFILE");
		String tmp = helps[0].trim().toUpperCase();
		helps[0] = "HELP:\r\n";
		switch (index.indexOf(tmp)) {
		case 0:
			helps[0] = help[0];
			break;
		case 1:
			helps[0] = help[1];
			break;
		case 2:
			helps[0] = help[2];
			break;
		case 3:
			helps[0] = help[3];
			break;
		case 4:
			helps[0] = help[4];
			break;
		case 5:
			helps[0] = help[5];
			break;
		default:
			for (String h : help) {
				helps[0] += h + "\r\n";
			}

		}

	}

	/** create a library
	 * @param libName library name
	 * @param fileName related file name
	 * @param hostName host name
	 * @param localFile local file
	 * @throws SQLException
	 */
	public static void addLib(String libName, String fileName, String hostName,
			String localFile) throws SQLException {
		checkFileName(fileName);
		Connection conn = getConn();
		String userPath = getCodeFilePath(conn);
		String sql = "create library " + libName + " file '" + userPath
					+ fileName + "'";

                if (hostName != null && !"".equals(hostName.trim())) {
                        sql += " HOST NAME '" + hostName + "'";
                }
                if (localFile != null && !"".equals(localFile.trim())) {
                        sql += " LOCAL FILE '" + localFile + "'";
                }

                execSQL(sql, conn);
	}

	/**
	 * change the library related attribute
	 *
	 * @param libName
	 *            library name
	 * @param fileName
	 *            uploaded file's name
	 * @param hostName
	 * @param localFile
	 * @throws SQLException
	 */
	public static void alterLib(String libName, String fileName,
			String hostName, String localFile) throws SQLException {
		checkFileName(fileName);
		Connection conn = getConn();
		String userPath = getCodeFilePath(conn);
		String sql = "alter library " + libName + " FILE '" + userPath
				+ fileName + "'";

		if (hostName != null  && !"".equals(hostName.trim())) {
			sql += " HOST NAME '" + hostName + "'";
		}
		if (localFile != null  && !"".equals(localFile.trim())) {
			sql += " LOCAL FILE '" + localFile + "'";
		}

                execSQL(sql, conn);
	}

	/**
	 * drop the library
	 *
	 * @param libName
	 * @param isdefault
	 *            true is RESTRICT false is CASCADE
	 * @throws SQLException
	 */
	public static void dropLib(String libName, String mode) throws SQLException {
		String sql = "drop library " + libName;

                if (mode != null)
                        if (mode.trim().equalsIgnoreCase("RESTRICT"))
                                sql += " RESTRICT";
                        else if (mode.trim().equalsIgnoreCase("CASCADE"))
                                sql += " CASCADE";

                execSQL(sql, getConn());
	}

	public static void syncJar(String userPath, String fileName) throws SQLException, IOException {
                // the local staging directory is the same as the destination directory
                syncJarPdcp(userPath, userPath, fileName);
        }

	public static void syncJarPdcp(String stagingPath,
                                       String dstDir,
                                       String fileName) throws SQLException, IOException {
		checkFileName(fileName);
		String nodes = System.getenv("MY_NODES");
		LOG.info("syncJars " + fileName + ", MY_NODES=" + nodes);
		if (nodes != null && !"".equals(nodes.trim())) {
			String pdcp = System.getenv("SQ_PDCP");
			String pdsh = System.getenv("SQ_PDSH");
			LOG.info("SQ_PDCP=" + pdcp + ", SQ_PDSH=" + pdsh);
			if (pdcp == null) {
				pdcp = "/usr/bin/pdcp";
			}
			if (pdsh == null) {
				pdsh = "/usr/bin/pdsh";
			}
			execShell(pdsh + " " + nodes + " mkdir -p " + dstDir);
			execShell(pdcp + " " + nodes + " " + stagingPath + fileName.trim() + " " + dstDir);
			execShell(pdsh + " " + nodes + " chmod 755 " + dstDir + fileName.trim());
		}
	}
	
	static void syncLibViaHDFS(String fileName, Connection conn) throws SQLException, IOException {
		checkFileName(fileName);

                String stagingDirPath = getLocalStagingDirPath(conn);
                Path srcLocalPath = new Path(stagingDirPath, fileName);
                Path dstHdfsDir   = new Path(getHdfsStagingDirName(getCurrentUser(conn)));
                Path dstHdfsPath  = new Path(dstHdfsDir, fileName);

                try {
                        // using HDFS Java interface
                        Configuration conf = new Configuration(true);
                        FileSystem  fs = FileSystem.get(dstHdfsPath.toUri(),conf);

                        fs.mkdirs(dstHdfsDir, new FsPermission(FsAction.ALL,
                                                               FsAction.NONE,
                                                               FsAction.NONE));
                        fs.copyFromLocalFile(false, // do not delete src
                                             true,  // overwrite tgt
                                             srcLocalPath,
                                             dstHdfsPath);

                        LOG.info("syncJar " + fileName + ", target=" + dstHdfsDir.toString());

                        // now execute a TMUDF that will copy the file across all nodes

                        String sql = "select count(*) from udf(\"_LIBMGR_\".SyncLibUDF('c','"+ fileName + "'))";
                        String result = execSQL(sql, conn, true);

                        fs.delete(dstHdfsPath, false);
                        LOG.info("syncJar " + fileName + ", numSyncedNodes=" + result);
                }
                catch (Exception e1) {
                        try {
                                LOG.warn("syncJar " + fileName + ", sync via HDFS failed, trying pdcp");
                                // try using pdcp instead, this may not
                                // work on all systems
                                syncJarPdcp(stagingDirPath,
                                            getCodeFilePath(conn),
                                            fileName);
                        } catch (Exception e2) {
                                LOG.error(e1.getMessage(), e1);
                                LOG.error(e2.getMessage(), e2);
                                throw new SQLException(e1);
                        }
                }
	}
	
        public static void rmJar(String fileName, Connection conn) throws SQLException, IOException {
		checkFileName(fileName);

                // execute a TMUDF that will delete the file across all nodes

		String sql = "select count(*) from udf(\"_LIBMGR_\".SyncLibUDF('d','"+ fileName + "'))";
                String result = execSQL(sql, conn, true);

                LOG.info("rmJar " + fileName + ", numSyncedNodes=" + result);
	}

	private static String execShell(String cmd) throws IOException {
		LOG.info("Processing command: " + cmd);
		Process p = Runtime.getRuntime().exec(cmd);
		if (p != null) {
			StringBuilder sb = new StringBuilder();
			InputStream in = null;
			try {
				in = p.getInputStream();
				int c = -1;
				while ((c = in.read()) != -1) {
					sb.append((char) c);
				}
			} finally {
				if (in != null)
					in.close();
			}
			try {
				in = p.getErrorStream();
				int c = -1;
				boolean flag = true;
				while ((c = in.read()) != -1) {
					if (flag) {
						sb.append("\r\n");
					} else {
						flag = false;
					}
					sb.append((char) c);
				}
			} finally {
				if (in != null)
					in.close();
			}
			return sb.toString();
		}
		return null;
	}

	/**
	 * Download a JAR file
	 * 
	 * @param fileName
	 * @param offset
	 * @param fileData
	 * @throws SQLException
	 * @throws IOException
	 */
	public static void get(String fileName, int offset, String[] fileData, long[] fileLength)
			throws SQLException, IOException {
		checkFileName(fileName);
		Connection conn = getConn();
		LOG.info("Get " + fileName);
		String userPath = getCodeFilePath(conn);
		close(conn);
		File file = new File(userPath + fileName);
		if (!file.exists()) {
			throw new SQLException("No such file[" + fileName + "]");
		}
		RandomAccessFile rAFile = null;
		try {
			rAFile = new RandomAccessFile(file, "r");
			rAFile.seek(offset);
			byte bArray[] = new byte[MaxDataSize];
			int bytesRead = rAFile.read(bArray, 0, MaxDataSize);
			if (bytesRead != -1) {
				fileData[0] = new String(Arrays.copyOf(bArray, bytesRead), CHARSET);
				fileLength[0] = file.length();
				LOG.info("Download: " + fileName + ", offset:" + offset + ",compressed length:" + fileData[0].length()
						+ ",file length:" + fileLength[0]);
			}
		} catch(IOException e){
			LOG.error(fileName,e);
			throw e;
		} finally {
			if (rAFile != null) {
				try {
					rAFile.close();
				} catch (Exception e) {
					LOG.warn("Something wrong while close file[" + fileName + "] stream: " + e.getMessage());
				}
			}
		}

	}

	/**
	 * Remove exact file
	 * 
	 * @param fileName
	 * @throws SQLException
	 * @throws IOException 
	 */
	public static void rm(String fileName) throws SQLException, IOException {
		checkFileName(fileName);
		Connection conn = getConn();
		LOG.info("Remove " + fileName);
		String userPath = getCodeFilePath(conn);
                try {
                        File file = new File(userPath + fileName);
                        if (file.exists()) {
                            rmJar(fileName, conn);
                            LOG.info("Removed " + fileName + " successfully!");
                            return;
                        } else {
                            LOG.error("No such file[" + fileName + "]");
                            throw new SQLException("No such file[" + fileName + "]");
                        }
                } finally {
                        close(conn);
                }
	}

	/**
	 * Remove files via regular formulation
	 * 
	 * @param pattern:
	 *            to be deleted
	 * @param names
	 *            : file names to be deleted
	 * @throws SQLException
	 * @throws IOException 
	 */
	public static void rmRex(String pattern, String[] names) throws SQLException, IOException {
		checkFileName(pattern);
		Connection conn = getConn();
		LOG.info("Try to remove files[" + pattern + "]");
		String userPath = getCodeFilePath(conn);
		File[] files = getFiles(pattern, new File(userPath));
		StringBuilder sb = new StringBuilder();
		sb.append("<rmRex>");
		sb.append(toXML(files, "rmList"));
		sb.append("<message>");

                try {
                        for (int i = 0; i < files.length; i++) {
                                rmJar(files[i].getName(), conn);
                        }
                } finally {
                        close(conn);
                }

                sb.append("Removed the files successfully!");
		sb.append("</message>");
		sb.append("</rmRex>");
		names[0] = sb.toString();
		LOG.info("Done for removing files[" + pattern + "].");
	}

	public static void lsAll(String[] names) throws SQLException {
		ls("*", names);
	}

	/**
	 * list the Jars matching PATTERN
	 * 
	 * @param pattern:
	 * @param names
	 * @throws SQLException
	 */
	public static void ls(String pattern, String[] names) throws SQLException {
		checkFileName(pattern);
		Connection conn = getConn();
		LOG.info("List files[" + pattern + "]");
		String userPath = getCodeFilePath(conn);
		close(conn);
		File dir = new File(userPath);
		if (!dir.exists() || !dir.isDirectory()) {
			LOG.error("Directory [" + userPath + "] is not found!");
			throw new SQLException("Directory [" + userPath + "] is not found!");
		}
		if (pattern == null) {
			LOG.error("File pattern should not be empty!");
			throw new SQLException("Pattern is empty!");
		}
		File[] files = getFiles(pattern, dir);
		names[0] = toXML(files, "ls");
	}


	/**
	 * upload a JAR file
	 * 
	 * @param fileData
	 * @param fileName
	 * @param appendFlag
	 *            0: append; otherwise overwrite
	 * @param overwriteOnCreate
	 *            when appendFlag is not 0, check if file exists and overwriteOnCreate is not 0,
	 *            throw exception. Otherwise overwrite the file.
	 * @throws SQLException
	 */
	public static void put(String fileData, String fileName, int appendFlag, int overwriteOnCreate) throws SQLException, IOException {
		checkFileName(fileName);
		try {
			byte[] data = fileData.getBytes(CHARSET);

			Connection conn = getConn();
			LOG.info("Put " + fileName + ", length: " + data.length + ", file string length:" + fileData.length());
			String userPath = getCodeFilePath(conn);
			close(conn);
			String fname = userPath + fileName;
			if (overwriteOnCreate != 0 && appendFlag != 0
					&& new File(fname).exists()) {
				throw new IOException("File " + fileName + " already exists!");
			}
			checkFile(fname, data.length);
			FileOutputStream fos = null;
			FileChannel channel = null;
			FileLock lock = null;
			try {
				fos = new FileOutputStream(fname, (appendFlag == 0));
				channel = fos.getChannel();
				lock = channel.tryLock();
				if (lock != null) {
					fos.write(data);
					fos.flush();
				}else{
					throw new SQLException("File "+fileName+" is locked, please try again later.");
				}
			} finally {
				if(lock != null){
					lock.release();
				}
				if(channel !=null){
					channel.close();
				}
				if (fos != null)
					fos.close();
			}

			syncJar(userPath, fileName);
			LOG.info("PUT method out !!! " + fileName);
		} catch (Throwable t) {
			LOG.error(t.getMessage(), t);
			throw new SQLException(t.getMessage());
		}
	}

	/**
	 * upload a library (jar or DLL) in one or more chunks, passed in as strings
	 * 
	 * @param fileData     a string (chunk) of bytes to be added to the file on
         *                     the server side. These bytes are assumed to be in the
         *                     ISO-8859-1 character set, which allows any bit combination,
         *                     so passing arbitrary bit patterns is ok.
	 * @param fileName     unqualified name of the target file (library file)
         * @param isFirstChunk indicates whether this is the first chunk for a file
         * @param isLastChunk  indicates whether this is the last chunk for a file
         * @param overwriteExistingFile (used only for the first chunk) indicates
         *                     whether we should silently overwrite an existing file
         *                     (true) or raise an exception, if file "fileName"
         *                     already exists (false)
	 * @throws SQLException
         * @throws IOException
	 */
        public static void putFile(String fileData,
                                   String fileName,
                                   int isFirstChunk,
                                   int isLastChunk,
                                   int overwriteExistingFile)
            throws SQLException, IOException {
		checkFileName(fileName);
		try {
			byte[] data = fileData.getBytes(CHARSET);

			Connection conn = getConn();
			String stagingPath = getLocalStagingDirPath(conn);
			String fname = stagingPath + fileName;
                        String dstFileName = getCodeFilePath(conn) + fileName;
                        boolean isFirst = (isFirstChunk != 0);
                        boolean isLast = (isLastChunk != 0);

			LOG.info("PutFile " + fileName +
                                 (isFirst && isLast ? "(single chunk)" :
                                  (isFirst ? "(first chunk)" :
                                   (isLast ? "(last chunk)" : "(intermediate chunk)"))) +
                                 ", length: " + data.length + ", file string length:" + fileData.length());

			if (isFirst &&
                            overwriteExistingFile == 0 &&
                            new File(dstFileName).exists()) {
				throw new IOException("File " + fileName + " already exists!");
			}
			checkFile(fname, data.length);
                        File stagingDir = null;
			FileOutputStream fos = null;
			FileChannel channel = null;
			FileLock lock = null;
			try {
                                if (isFirst) {
                                        stagingDir = new File(stagingPath);

                                        if (!stagingDir.exists())
                                            stagingDir.mkdir();
                                }

				fos = new FileOutputStream(fname, !isFirst);
				channel = fos.getChannel();
				lock = channel.tryLock();
				if (lock != null) {
					fos.write(data);
					fos.flush();
				}else{
					throw new SQLException("File "+fileName+" is locked, please try again later.");
				}
                                if (isLast)
                                        syncLibViaHDFS(fileName, conn);
			} finally {
				if(lock != null){
					lock.release();
				}
				if(channel !=null){
					channel.close();
				}
				if (fos != null)
					fos.close();
                                if (isLast) {
                                        // delete the file in the staging area
                                        new File(fname).delete();
                                }
                                close(conn);
			}

			LOG.info("PutFile method out !!! " + fileName);
		} catch (Throwable t) {
			LOG.error(t.getMessage(), t);
			throw new SQLException(t.getMessage());
		}
	}

	static void checkFileName(String fileName) throws SQLException {
		if (fileName.contains("/") || fileName.contains("\\"))
			throw new SQLException("Illegal file name: " + fileName
					+ ". File name must not contain \"/\".");
	}

        static String getHdfsStagingDirName(String userName) {
            return "/user/trafodion/udr/lib/staging/" + userName;
        }

        static String getLocalLibDirName(String userName) throws SQLException {
		String root = System.getenv("TRAF_VAR");
		if (root == null || "".equals(root.trim())) {
			LOG.error("Cant get your traf installation path!");
			throw new SQLException("Cant get your traf installation path!");
		}
		File file = new File(root + "/udr/lib/" + userName);
		if (!file.exists()) {
			file.mkdirs();
		} else if (!file.isDirectory()) {
			throw new SQLException("User Directory is not valid or you dont have permission!");
		}
		return file.getAbsolutePath() + "/";
        }

	private static void checkFile(String fname, int dataSize) throws SQLException {
		File jar = new File(fname);
		if (jar.length() + dataSize > MAX_JAR_FILE_SIZE) {
			LOG.error("Jar file size is over the threshold[100Mb]");
			throw new SQLException("Jar file size is over the threshold[100Mb]");
		}
	}

	private static String getCodeFilePath(Connection conn) throws SQLException {
		String user = getCurrentUser(conn);
                String result = getLocalLibDirName(user);
		LOG.info("SPJ JARs location: " + result);
		return result;
	}

	private static String getLocalStagingDirPath(Connection conn) throws SQLException {
                // this is the local directory where the put() method
                // stores the file as it is being assembled from chunks
                return getCodeFilePath(conn) + "staging/";
	}

	private static Connection getConn() throws SQLException {
		Connection conn = null;
		try {
			conn = DriverManager.getConnection(url);
			LOG.info("Create connection successfully.  " + conn +", autocommit:"+conn.getAutoCommit());
		} catch (Throwable t) {
			LOG.error("Error encountered while getting connection ", t);
			throw new SQLException(t.getMessage());
		}
		return conn;
	}

        private static void execSQL(String sqlString,
                                    Connection conn) throws SQLException {
            execSQL(sqlString, conn, false);
        }

        private static String execSQL(String sqlString,
                                      Connection conn,
                                      boolean executeQuery) throws SQLException {
            Statement st = null;
            ResultSet rs = null;
            String result = null;

            try {
                st = conn.createStatement();

                if (executeQuery) {
                    rs = st.executeQuery(sqlString);
                    if (rs.next()) {
                        result = rs.getString(1);
                    }
                }
                else
                    st.execute(sqlString);
            } catch (Exception e) {
                LOG.error(e.getMessage(), e);
                throw new SQLException(e);
            } finally {
                if (rs != null) {
                    try {
                        rs.close();
                    } catch (Exception e) {
                        LOG.warn(e.getMessage(), e);
                    }
                }
                if (st != null) {
                    try {
                        st.close();
                    } catch (Exception e) {
                        LOG.warn(e.getMessage(), e);
                    }
                }
            }
            return result;
        }

	private static String getCurrentUser(Connection conn) throws SQLException {
            String user = execSQL("values(session_user)", conn, true);
            return user.replaceAll("[\\\\/]", "_");
	}

	private static File[] getFiles(String pattern, File dir) {
		final String p = pattern.replaceAll("\\*", ".*").trim().toUpperCase();
		return dir.listFiles(new FileFilter() {

			@Override
			public boolean accept(File name) {
				if (name == null || !name.isFile()) {
					return false;
				}
				return name.getName().trim().toUpperCase().matches(p);
			}
		});
	}

	private static String toXML(File[] files, String root) {
		StringBuilder sb = new StringBuilder();
		sb.append("<" + root + ">");
		for (File f : files) {
			sb.append("<file name='" + f.getName() + "' lastModifyTime='" + format.format(new Date(f.lastModified()))
					+ "' size='" + f.length() + "'/>");
		}
		sb.append("</" + root + ">");
		return sb.toString();
	}

	private static void close(Connection conn) {
		try {
			conn.close();
			LOG.info("Closed connection");
		} catch (Exception e) {
			LOG.warn(e.getMessage());
		}
	}
}
