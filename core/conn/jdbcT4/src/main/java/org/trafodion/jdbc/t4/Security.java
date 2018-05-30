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

package org.trafodion.jdbc.t4;

import java.nio.ByteBuffer;
import java.security.SecureRandom;
import java.util.Date;
import java.security.NoSuchAlgorithmException;
import java.io.File;
import javax.crypto.SecretKey;



class Security
{
      public static final Cipher cipher = org.trafodion.jdbc.t4.Cipher.getInstance();
      public static final MessageDigest msgDigest = MessageDigest.getInstance();

      /** Ctor - Creates the password key and initializes it with
       * password id.  Gets public key and the length of the public key
       * from the certificate file.  Generates nonce and session key.
       * @param cert_file - fully qualified name of certificate file
       * @throw SecurityException
       */
      public Security(File certFile) throws SecurityException
      {
         //   m_encrypted = 0;
         m_pwdkey = new SecdefsCommon.PwdKey();
         m_pwdkey.data = new SecdefsCommon.LoginData();
         m_pwdkey.id[0] = '\1';
         m_pwdkey.id[1] = '\2';
         m_pwdkey.id[2] = '\3';
         m_pwdkey.id[3] = '\4';

         try {
            m_keyObj = new Key();
            m_cert = new Certificate(certFile);
            m_keyObj.getPubKeyFromFile(m_cert.getCert());
            generateSessionKey();
         }catch (SecurityException se) {
            throw se;
         }
      }

    /** This method builds the password key which consists 4 bytes of password id,
  	 *  128 bytes of role name which would be 128 spaces when role name is null,
  	 *  32 bytes of the digest message calculated using the session key on the data made up of
  	 *  the procInfo and the encrypted data and 256 bytes (if the 2048 public key is used) or
  	 *  128 bytes (if the1024 public key is used) encrypted data calculated using the public key
  	 *  on the plain text made up of the session key, the nonce and the password.
  	 * Builds password key
  	 * @param pwd
  	 * 		 	password to be encrypted
  	 * @param rolename
  	 * 			role name to build password key
  	 * @param procInfo
  	 * 			process information (PIN, CPU, segment name and time stamp)
  	 * @return pwdkey
  	 * 			returned password key
  	 * @throws SecurityException
  	 */
      public void encryptPwd(byte[] pwd, byte[] rolename, byte[] procInfo,
                            byte[] pwdkey)
                                 throws SecurityException
      {
         // Get public key length
         int pubKeyLen = m_keyObj.getPubKeyLen();
         int maxPlainTextLen = pubKeyLen - SecdefsCommon.UNUSEDBYTES;

         // Password + nonce + session key can't be longer than the public
         // key's length
         if ((SecdefsCommon.NONCE_SIZE + SecdefsCommon.SESSION_KEYLEN
                                       + pwd.length) > maxPlainTextLen)
            throw new SecurityException(SecClientMsgKeys.
                                   PWD_LENGTH_TOO_LONG, null);

         byte[] to_encrypt = new byte[SecdefsCommon.SESSION_KEYLEN +
                                      SecdefsCommon.NONCE_SIZE + pwd.length];
         byte[] cipherText = new byte[pubKeyLen];
         byte[] to_digest = new byte[SecdefsCommon.PROCINFO_SIZE +
                              SecdefsCommon.TIMESTAMP_SIZE + pubKeyLen];
         byte[] digestedMsg = new byte[SecdefsCommon.DIGEST_LENGTH];

         try {
            // Build password key
            // Copy 4 bytes of id
        	 System.arraycopy(m_pwdkey.id, 0, pwdkey, 0, SecdefsCommon.PWDID_SIZE);
            // Copy rolename
            if (rolename != null)
            	System.arraycopy(rolename, 0, pwdkey, SecdefsCommon.PWDID_SIZE,
            			          rolename.length);
            // Copy 12 bytes of procInfo and 8 bytes of timestamp to
            // password key store procInfo in the digest starting from
            // digest[20]
            System.arraycopy(procInfo, 0, pwdkey, (SecdefsCommon.PWDID_SIZE +
            		        SecdefsCommon.ROLENAME_SIZE + SecdefsCommon.DIGEST_LENGTH -
                            SecdefsCommon.PROCINFO_SIZE), (SecdefsCommon.PROCINFO_SIZE +
                    		SecdefsCommon.TIMESTAMP_SIZE));

            // Build plain text to encrypt
            System.arraycopy(m_pwdkey.data.session_key, 0, to_encrypt, 0,
            		        SecdefsCommon.SESSION_KEYLEN);
            System.arraycopy(m_pwdkey.data.nonce, 0, to_encrypt,
            		SecdefsCommon.SESSION_KEYLEN, SecdefsCommon.NONCE_SIZE);
            System.arraycopy(pwd, 0, to_encrypt,
            		(SecdefsCommon.SESSION_KEYLEN + SecdefsCommon.NONCE_SIZE), pwd.length);

            // Encrypt the data
            int cipherTextLen = cipher.encrypt(to_encrypt, cipherText,
                             (java.security.Key)(m_keyObj.getPubKey()));

            if(cipherTextLen != pubKeyLen)
               throw new SecurityException(SecClientMsgKeys.
                                           CIPHER_TEXT_LEN_NOT_EQUAL_KEY_LEN, null);

            // Copy cipherText to pwdkey
            System.arraycopy(cipherText, 0, pwdkey,
            		SecdefsCommon.PWDKEY_SIZE_LESS_LOGINDATA, cipherTextLen);

            // Create digest
            // Get bytes from digest[20] on
            System.arraycopy(pwdkey, (SecdefsCommon.PWDKEY_SIZE_LESS_LOGINDATA -
                    SecdefsCommon.TIMESTAMP_SIZE - SecdefsCommon.PROCINFO_SIZE),
                    to_digest, 0, (SecdefsCommon.PROCINFO_SIZE +
                    		SecdefsCommon.TIMESTAMP_SIZE + cipherTextLen));

            int mdLen = msgDigest.digest(m_pwdkey.data.session_key,
                                 to_digest, digestedMsg);

            if (mdLen != SecdefsCommon.DIGEST_LENGTH)
               throw new SecurityException(SecClientMsgKeys.
                                           BAD_MESSAGE_DIGEST_LEN, null);

            // copy digestedMsg into pwdkey
            System.arraycopy(digestedMsg, 0, pwdkey,
            		(SecdefsCommon.PWDKEY_SIZE_LESS_LOGINDATA
            		- SecdefsCommon.TIMESTAMP_SIZE - SecdefsCommon.DIGEST_LENGTH), mdLen );

         }catch (SecurityException se) {
             throw se;
         }catch (Exception e) {
            throw new SecurityException(e, SecClientMsgKeys.FAILED_BUILDING_PWDKEY, null);
         }finally {
            if (to_digest != null)
               to_digest = null;
            if (digestedMsg != null)
               digestedMsg = null;
            if (to_encrypt != null)
               to_encrypt = null;
         }
      }

      /** Encrypts the data using AES256 algorithm.
	    *
	    * @param  data - data to be encrypted
	    * @return array of bytes of 2 bytes PIN,
	    *         2 bytes of CPU, 8 bytes of seg_name
	    *         and the encrypted data
	    * @throw  SecurityException
	    */

	 public byte[] encryptData(byte[] data) throws SecurityException
	 {
		 //Creates a secret key from the session key
		byte[] skey = new byte[SecdefsCommon.AES_BLOCKSIZE];
		System.arraycopy(m_pwdkey.data.session_key, SecdefsCommon.AES_BLOCKSIZE,
				skey, 0, SecdefsCommon.AES_BLOCKSIZE);
		SecretKey seckey = Key.generateSymmetricKey(skey);
		byte [] iv  = new byte [SecdefsCommon.AES_BLOCKSIZE];
		System.arraycopy(m_pwdkey.data.nonce, SecdefsCommon.AES_BLOCKSIZE,
				iv, 0, SecdefsCommon.AES_BLOCKSIZE);
	    m_cipher = Cipher.getEASInstance("AES/CBC/PKCS5Padding");
	    return Cipher.encryptData(data, seckey, iv, m_cipher);
	 }

      // Currently not implemented
      // Generate message digest
      // str - message to digest
      // hmacMsg - Hashed message in bytes
      // hmacMsgLen - Length of hashed message
      public void HMAC_Message_Generate(byte[] str, byte[] hmacMsg,
                                      int hmacMsgLen) throws SecurityException
      {
         // Not implemented yet
      }

      // Currently not implemented
      // Verify message digest
      // str - message digest
      // length - message digest length
      public boolean HMAC_Message_Verify(byte[] str) throws SecurityException
      {
         // Not implemented yet
         return false;
      }

      /** increment the nonce sequence
       *
       */
      public void incrementNonceSeq ()
      {
         m_nonceSeq++;
      }

      /** Gets length of buffer for password encryption (public)
       * @Return pass word key length if success and 0 if failed
       * @throw SecurityException
       */
      public int getPwdEBufferLen() throws SecurityException
      {
         int pubKLen = m_keyObj.getPubKeyLen();
         if(pubKLen <= 0)
            throw new SecurityException(SecClientMsgKeys.
                                         PUBKEY_LENGTH_IS_ZERO, null);
         else
           return (pubKLen + SecdefsCommon.PWDKEY_SIZE_LESS_LOGINDATA);
      }

      /** Gets certificate's expiration date
       * @Return an array of bytes represents the certificate's
       * expiration day in the string format YYMMDDHHMMSS
       */
      public byte[] getCertExpDate()
      {
         return m_cert.getCertExpDate();
      }

      // Generates session key and nonce
      private void generateSessionKey() throws SecurityException
      {
         //try {
            SecureRandom random = new SecureRandom();
            try {
                random.setSeed(System.currentTimeMillis());

                random.setSeed(Runtime.getRuntime().freeMemory());
                random.setSeed(Runtime.getRuntime().totalMemory());
                random.setSeed(Runtime.getRuntime().maxMemory());

                String p = null;

                p = System.getProperty("java.version", "unknown java version");
                random.setSeed(p.getBytes());
                p = System.getProperty("java.vendor", "unknown vendor");
                random.setSeed(p.getBytes());
                p = System.getProperty("os.name", "unknown os");
                random.setSeed(p.getBytes());
                p = System.getProperty("os.version", "unknown os version");
                random.setSeed(p.getBytes());

                // Add current time again

                random.setSeed(System.currentTimeMillis());
            }
            catch(Exception e ) {
                // Ignore
            }
            byte bytes[] = new byte[SecdefsCommon.SESSION_KEYLEN +
                                    SecdefsCommon.NONCE_SIZE];
            synchronized(random) {
            	random.nextBytes(bytes);
            }

            // Assign bytes to members m_pwdkey.data.session_key
            // and m_pwdkey.data.nonce

            System.arraycopy(bytes, 0, m_pwdkey.data.session_key, 0, SecdefsCommon.SESSION_KEYLEN);
            System.arraycopy(bytes, SecdefsCommon.SESSION_KEYLEN, m_pwdkey.data.nonce, 0, SecdefsCommon.NONCE_SIZE);

            m_nonceSeq = (ByteBuffer.wrap(m_pwdkey.data.nonce)).getLong(
            		                          SecdefsCommon.SESSION_KEYLEN -
                                              SecdefsCommon.NONCE_SEQNUM);

            // Set time when session key is generated
            m_keyTime = (new Date()).getTime();
         /*}catch (NoSuchAlgorithmException nae) {
            throw new SecurityException(SecClientMsgKeys.SESSION_KEY_GENERATION_FAILED, null);
         }*/
      }

      // encryption is required or not for replied message 0-yes, 1-no
      //int m_encrypted;
      // security option - mandatory - 1 or undocumented option - 0
      // Time when session key is generated
      private long m_keyTime;
      // sequence nonce used in nonce increment
      // Need to use 64 bit number type here
      private long m_nonceSeq;
      // certificate
      private Certificate m_cert;
      // key
      private Key m_keyObj;
      // password key
      private SecdefsCommon.PwdKey m_pwdkey;
      private javax.crypto.Cipher m_cipher;

};
