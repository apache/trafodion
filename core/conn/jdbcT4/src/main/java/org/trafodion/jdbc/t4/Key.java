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
 * Key.java
 */

package org.trafodion.jdbc.t4;

import java.io.FileInputStream;
import java.io.InputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.cert.X509Certificate;
import java.security.KeyFactory;
import java.security.interfaces.RSAPublicKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.NoSuchAlgorithmException;
import java.security.spec.InvalidKeySpecException;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import javax.crypto.KeyGenerator;


public class Key {
	public Key(){}

	/** Reads the public key from the certificate file and
	 *  stores the key and the length of the public key
	 *  in member data.
	 *  @param X509Certificate - The certificate stored
	 *  the public key
	 */
   public void getPubKeyFromFile(X509Certificate cert)
   {
      m_pubKey = (RSAPublicKey) cert.getPublicKey();
      if (((m_pubKey.getModulus().bitLength()) / 8) > 128)
    	  m_pubKeyLen = 256;
      else
    	  m_pubKeyLen = 128;
   }

   /** Reads the private key from certificate file and
    *  stores the key in the member data.
    *  @param String - The file stored the private key
    *  @throw SecurityException
    */
   public void getPrivKeyFromFile(String inFile) throws SecurityException
   {
      InputStream inStream=null;

      try{
         // Loading private key file
         inStream=new FileInputStream(inFile);
         byte[] keyBytes=new byte[inStream.available()];
         inStream.read(keyBytes);
         inStream.close();

         // Read the private key from file
         PKCS8EncodedKeySpec privKeySpec=new PKCS8EncodedKeySpec(keyBytes);
         KeyFactory keyFactory = KeyFactory.getInstance("RSA");
         m_privKey= (RSAPrivateKey) keyFactory.generatePrivate
                                                       (privKeySpec);

      }catch (FileNotFoundException fnf) {
         throw new SecurityException(fnf, SecClientMsgKeys.FILE_NOTFOUND, new Object[]{inFile});
      }catch (IOException io) {
         throw new SecurityException(io, SecClientMsgKeys.ERR_OPEN_INPUT_FILE, new Object[]{inFile});
      }catch (Exception e) {
         throw new SecurityException(e, SecClientMsgKeys.ERR_RETRIEVE_KEY_FROM_FILE, new Object[]{inFile});
      }finally {
         try {
            if (inStream != null)
               inStream.close();
         }catch (IOException io) {
            // not much we can do at this point
         }
      }
   }

   /**
    * Generates a secret key using AES algorithm and 128 bits key
    * @param sessionKey the session key byte array used for symmetric key
    *                   generation
    * @return the SecretKey
    * @throws SecurityException
    */
   static SecretKey generateSymmetricKey(byte [] sKey) throws SecurityException
   {
	  if (sKey == null)
		  throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL, new Object[]{"sKey"});
	  try {
         // Get the KeyGenerator
         KeyGenerator kgen = KeyGenerator.getInstance("AES");
         synchronized(kgen) {
        	 kgen.init(128);
         }
         // Use the lower 16 bytes of the session key to generate the 128 bits secret
         // key used for data encryption

         SecretKey skey = new SecretKeySpec(sKey, kgen.getAlgorithm());

         return skey;
	  }catch (NoSuchAlgorithmException nae) {
		  throw new SecurityException(nae, SecClientMsgKeys.ERR_CREATE_SYMMETRIC_KEY, null);
	  }
   }

   /**
    *
    * @return the public key
    */
   public RSAPublicKey getPubKey()
   {
      return m_pubKey;
   }

   /**
    *
    * @return the private key
    */
   public RSAPrivateKey getPrivKey()
   {
      return m_privKey;
   }

   /**
    *
    * @return the length of the public key
    */
   public int getPubKeyLen()
   {
      return m_pubKeyLen;
   }

   private RSAPublicKey m_pubKey;
   private RSAPrivateKey m_privKey;
   private int m_pubKeyLen;
}
