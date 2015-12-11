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
package org.trafodion.mgmt;

import java.io.File;
import java.io.FileFilter;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class JarFileMgmt {
	private static final Logger LOG = LoggerFactory.getLogger(JarFileMgmt.class);
	private static final String url = "jdbc:default:connection";
	// 100Mb
	private static final long MAX_JAR_FILE_SIZE = 104857600;
	private static final SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
	private static final int MaxDataSize = 102400;
	private static final String CHARTSET = "ISO-8859-1";

	/**
	 * Print help info
	 * 
	 * @param helps:
	 *            INOUT parameter like PUT/LS/...
	 */
	public static void help(String[] helps) {
		String[] help = new String[] {
				"PUT - Upload a JAR. SHOWDDL PROCEDURE DEFAULT_SPJ.PUT for more info.",
				"LS - List JARs. SHOWDDL PROCEDURE DEFAULT_SPJ.LS for more info.",
				"LSALL - List all JARs. SHOWDDL PROCEDURE DEFAULT_SPJ.LSALL for more info.",
				"RM - Remove a JAR. SHOWDDL PROCEDURE DEFAULT_SPJ.RM for more info.",
				"RMREX - Remove JARs by a perticular pattern. SHOWDDL PROCEDURE DEFAULT_SPJ.RMREX for more info.",
				"GETFILE - Download a JAR. SHOWDDL PROCEDURE DEFAULT_SPJ.GETFILE for more info."
		};
		List<String> index = new ArrayList<String>(help.length);
		index.add("PUT");
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

	public static void syncJar(String userPath, String fileName) throws SQLException, IOException {
		checkFileName(fileName);
		LOG.info("syncJars " + fileName);
		String nodes = System.getenv("MY_NODES");
		if (nodes != null && !"".equals(nodes.trim())) {
			execShell("pdcp " + nodes + " " + userPath + fileName.trim() + " " + userPath + " ");
		}
	}

	private static String execShell(String cmd) throws IOException {
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
				fileData[0] = new String(Arrays.copyOf(bArray, bytesRead), CHARTSET);
				fileLength[0] = file.length();
				LOG.info("Download: " + fileName + ", offset:" + offset + ",compressed length:" + fileData[0].length()
						+ ",file length:" + fileLength[0]);
			}
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
	 */
	public static void rm(String fileName) throws SQLException {
		checkFileName(fileName);
		Connection conn = getConn();
		LOG.info("Remove " + fileName);
		String userPath = getCodeFilePath(conn);
		close(conn);
		File file = new File(userPath + fileName);
		if (file.exists()) {
			file.delete();
			LOG.info("Remove " + fileName + " successfully!");
			return;
		} else {
			throw new SQLException("No such file[" + fileName + "]");
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
	 */
	public static void rmRex(String pattern, String[] names) throws SQLException {
		checkFileName(pattern);
		Connection conn = getConn();
		LOG.info("Try to remove files[" + pattern + "]");
		String userPath = getCodeFilePath(conn);
		close(conn);
		File[] files = getFiles(pattern, new File(userPath));
		StringBuilder sb = new StringBuilder();
		sb.append("<rmRex>");
		sb.append(toXML(files, "rmList"));
		sb.append("<message>");
		boolean hasError = false;
		for (File f : files) {
			try {
				f.delete();
			} catch (Exception e) {
				hasError = true;
				LOG.error(e.getMessage(), e);
				sb.append("<error fileName='" + f.getName() + "'>" + e.getMessage() + "</error>");
			}
		}
		if (!hasError) {
			sb.append("Remove the files successfully!");
		}
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
	 * @throws SQLException
	 */
	public static void put(String fileData, String fileName, int appendFlag) throws SQLException {
		checkFileName(fileName);
		try {
			byte[] data = fileData.getBytes(CHARTSET);

			Connection conn = getConn();
			LOG.info("Put " + fileName + ", length: " + data.length + ", file string length:" + fileData.length());
			String userPath = getCodeFilePath(conn);
			close(conn);
			String fname = userPath + fileName;
			checkFile(fname, data.length);
			FileOutputStream fos = null;
			try {
				fos = new FileOutputStream(fname, (appendFlag == 0));
				fos.write(Arrays.copyOf(data, data.length));
				fos.flush();
			} finally {
				if (fos != null)
					fos.close();
			}

			syncJar(userPath, fileName);

		} catch (Throwable t) {
			LOG.error(t.getMessage(), t);
			throw new SQLException(t.getMessage());
		}
	}

	private static void checkFileName(String fileName) throws SQLException {
		if (fileName.contains("/") || fileName.contains("\\"))
			throw new SQLException("Illegal file name: " + fileName
					+ ". File name must not contain \"/\".");
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
		String root = System.getenv("MY_SQROOT");
		if (root == null || "".equals(root.trim())) {
			LOG.error("Cant get your traf installation path!");
			throw new SQLException("Cant get your traf installation path!");
		}
		File file = new File(root + "/udr/lib/" + user);
		if (!file.exists()) {
			file.mkdirs();
		} else if (!file.isDirectory()) {
			throw new SQLException("User Directory is not valide or you dont have permission!");
		}
		LOG.info("SPJ JARs location: " + file.getAbsolutePath());
		return file.getAbsolutePath() + "/";
	}

	private static Connection getConn() throws SQLException {
		Connection conn = null;
		try {
			conn = DriverManager.getConnection(url);
			LOG.info("Create connection successfully.  " + conn);
		} catch (Throwable t) {
			LOG.error("Error encountered while getting connection ", t);
			throw new SQLException(t.getMessage());
		}
		return conn;
	}

	private static String getCurrentUser(Connection conn) throws SQLException {
		Statement st = null;
		ResultSet rs = null;
		String user = null;
		try {
			st = conn.createStatement();
			rs = st.executeQuery("values(session_user)");
			if (rs.next()) {
				user = rs.getString(1);
			}
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
