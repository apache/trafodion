/**********************************************************************
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
 **********************************************************************/

/**
 * class SecPwd - builds the password key, encrypts password,
 *                creates HMAC message based on password, rolename
 *                process info and time stamp.  It also gets expiration
 *                date of a certificate.
 *
 */

package org.trafodion.jdbc.t4;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.sql.Connection;


public class SecPwd {
	 /**
	  *
      *
      * @return SecPwd
      */
     public static SecPwd getInstance(Connection con, String directory, String fileName,
    		 String serverName, boolean spjMode, byte[] procInfo) throws SecurityException
     {
    	 if (con == null)
    		 throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL, new Object[]{"connection"});
    	 SecPwd secpwd = new SecPwd(directory, fileName, serverName, spjMode, procInfo);

    		 SymCrypto.insert(con, secpwd);


    	 return secpwd;
     }

     public static void removeInstance(Connection con)
     {
    	 SymCrypto.remove(con);
     }

	/**
	 * Ctor for the SecPwd. There are two possible certificates: active
	 * certificate and certificate that is going to be active.
	 *
	 * If autodownload is true, certificate will always come from the
	 * server. In this case, only active certificate is used.
	 *
	 * If autodownload is false, active certificate is used to encrypt the
	 * password. When there is a new certificate, it will be stored in
	 * "certificate". As soon as this new certificate is activated on the
	 * server, the current active certificate will become stale, and the new
	 * certificate will be copied over and becomes the active certificate.
	 *
	 * If spjMode is true, the OS name is NONSTOP_KERNEL and the host name
	 * is the same as the server name then just setSpj mode to true
	 * and does nothing.
	 *
	 * @param directory
	 *            specifies the directory to locate the certificate. The default
	 *            value is %HOME% if set else %HOMEDRIVE%%HOMEPATH%.
	 * @param fileName
	 *            specifies the certificate that is in waiting. The default
	 *            value is the first 5 characters of server name.
	 * @param activeFileName
	 *            specifies the current certificate in use. The default value is
	 *            the first 5 character of server name + Active
	 * @param spjMode
	 *            true - and if os.name == NSK and the host name
	 *            matches the local host - token case.  Certificate is not
	 *            handled in this case.
	 *            false - handles certificate
	 * @param serverName
	 *            server name for this certificate.
	 * @throws SecurityException
	 */
	private SecPwd(String directory, String fileName, 
			String serverName, boolean spjMode, byte[] procInfo) throws SecurityException {
		// check USERID env variable for MXCI testing of SPJs.  If set use normal password
		// encryption
		if ((spjMode == true)  &&
			//	((hostName.substring(0, 5)).compareToIgnoreCase(serverName.substring(0, 5)) == 0) &&
				(System.getenv("USERID") == null))// token
		{
			m_spjMode = spjMode;
		}
		else // password
		{
			if (procInfo == null)
				throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL, new Object[]{"procInfo"});
			// Stores procInfo with the time stamp for data message encryption used
			m_procInfo = new byte [SecdefsCommon.PROCINFO_SIZE + SecdefsCommon.TIMESTAMP_SIZE];
			System.arraycopy(procInfo, 0, m_procInfo, 0, (SecdefsCommon.PROCINFO_SIZE + SecdefsCommon.TIMESTAMP_SIZE));
			directory = (directory != null) ? directory : System.getenv("HOME");
			if (directory == null)
			{
				String hmdrive = System.getenv("HOMEDRIVE");
				String hmpath = System.getenv("HOMEPATH");
				if (hmdrive != null && hmpath != null)
				{
					directory = hmdrive + File.separator + hmpath;
				}
			    else
			    {
			    	directory = System.getProperty("user.home");
			    	if (directory == null)
			    		throw new SecurityException (SecClientMsgKeys.HOME_ENVIRONMENT_VAR_IS_NULL, null);
			    }
            }
			fileName = (fileName != null) ? fileName : serverName + ".cer";

			File dir = new File(directory);
			if (dir.isDirectory() == false)
				throw new SecurityException(SecClientMsgKeys.DIR_NOTFOUND, new Object[]{dir.getPath()});

			certFile = new File(directory, fileName);
		}
	}

	/**
	 * Processes the active certificate when spjMode is false
	 * else does nothing.  The certificate is processed by calling
	 * the Security ctor to creates the password key and initializes it
     * with password id.  Gets public key and the length of the public
     * key from the certificate file.  Generates nonce and session key.
	 * @throws SecurityException
	 */
	public void openCertificate() throws SecurityException {
		if (m_spjMode == false) // do nothing for the token case
			m_sec = new Security(certFile);
	}

	/** This method builds the password key which consists 4 bytes of password id,
	 *  128 bytes of role name which would be 128 spaces when role name is null,
	 *  32 bytes of the digest message calculated using the session key on the data made up of
	 *  the procInfo and the encrypted data and 256 bytes (if the 2048 public key is used) or
	 *  128 bytes (if the1024 public key is used) encrypted data calculated using the public key
	 *  on the plain text made up of the session key, the nonce and the password.
	 *  The password key is generated only when the spjMode is false.  When
	 *  the spjMode is true, 26 bytes of the token is returned instead.
	 * Builds password key
	 * @param pwd
	 * 		 	password to be encrypted
	 * @param rolename
	 * 			role name to build password key
	 * @param procInfo
	 * 			process information (PIN, CPU, segment name and time stamp)
	 * @return pwdkey
	 * 			returns the password key if spjMode is false
	 *          returns the token when spjMode is true
	 * @throws SecurityException
	 */

	public void encryptPwd(byte[] pwd, byte[] rolename, byte[] pwdkey) throws SecurityException {
		// rolename is optional so can be NULL
		if (pwd == null)
			throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL, new Object[]{"password"});
		if (pwdkey == null)
			throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL, new Object[]{"password key"});
		if (m_spjMode == true) // token
		{
			if (pwd.length != SecdefsCommon.TOKENSIZE)
				throw new SecurityException(SecClientMsgKeys.BAD_TOKEN_LEN, null);
			if ((pwd[0] != SecdefsCommon.USERTOKEN_ID_1)
					|| (pwd[1] != SecdefsCommon.USERTOKEN_ID_2))
				throw new SecurityException(
						SecClientMsgKeys.INCORRECT_TOKEN_FORMAT, null);
			ByteBuffer.wrap(pwd).get(pwdkey, 0, SecdefsCommon.TOKENSIZE);
		}
		else
		{
			m_sec.encryptPwd(pwd, rolename, m_procInfo, pwdkey);
		}
	}

	/** Gets length of buffer for password encryption (public)
	 * or the length of the token if it is the SPJ mode
	 * @returns
	 *     If the spjMode is false
	 *        the length of the password key is returnd if success
	 * 	      0 if failed
	 *     If spjMode is true
	 *     	  the length of the token is returned
	 * @throws SecurityException
	 */
	public int getPwdEBufferLen() throws SecurityException {
		if (m_spjMode == true)
			return SecdefsCommon.TOKENSIZE;
		else
			return m_sec.getPwdEBufferLen();
	}

	/** Gets the expiration date of the certificate
	 * @return an array of bytes
	 * 			presents the certificate's
	 * 			expiration day in the format YYMMDDHHMMSS
	 * 			or a zero length byte array if the it is in the SPJ mode
	 */
	public byte[] getCertExpDate() {
		if (m_spjMode == false)
			return m_sec.getCertExpDate();
		else
			return new byte[0];
	}

	/**
	 * When autodownload is on, client will download the certificate from server
	 * when there is no certificate or certificate is stale.
	 *
	 * @param buf
	 *            content of the certificate pushed from server.
	 */
	public void switchCertificate(byte[] buf) throws SecurityException {
		FileChannel outChannel = null;
		try {
			outChannel = new FileOutputStream(certFile).getChannel();
			outChannel.write(ByteBuffer.wrap(buf));
		} catch (Exception e) {
			throw new SecurityException(e, SecClientMsgKeys.ERR_WRITE_CERT_FILE, new Object[]{certFile});
		} finally {
			try {
				if (outChannel != null)
					outChannel.close();
			} catch (Exception e) {
			}
		}
		m_sec = new Security(certFile);
	}

	public byte[] getProcInfo()
	{
		return m_procInfo;
	}

	private Security m_sec;
	private File certFile;
	private boolean m_spjMode;
	private byte[] m_procInfo;   //stores only 4 bytes pid + 4 bytes nid


};
