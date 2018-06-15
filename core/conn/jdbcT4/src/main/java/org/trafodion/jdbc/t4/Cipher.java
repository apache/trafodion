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
  * class Cipher - Encrypts and decrypts data and password using
  *                symmetric key and key pair
  *
  */

package org.trafodion.jdbc.t4;

import javax.crypto.spec.IvParameterSpec;
import javax.crypto.SecretKey;



public class Cipher
{
   private static class Holder
   {
      private static Cipher instance = new Cipher();
   }

   /**
     *
     * @return Cipher
     */
   public static Cipher getInstance()
   {
      return Holder.instance;
   }

   /** Encrypts plain text and stores the
    * cipher text in cipherText using public key for password encryption.
    * @param plainText - plain text to be encrypted
    * @param cipherText - encrypted plain text is returned
    * @param key - password encryption: public key
    * @Return the length of the cipher text or -1 in case of error
    * @throws SecurityException
    */
   public int encrypt(byte[] plainText, byte[] cipherText,
               java.security.Key key) throws SecurityException
   {
      int len = 0;

      if (plainText == null)
         throw new SecurityException(SecClientMsgKeys.
                                         INPUT_PARAMETER_IS_NULL, new Object[]{"plainText"});
      if (cipherText == null)
    	  throw new SecurityException(SecClientMsgKeys.
                   						 INPUT_PARAMETER_IS_NULL, new Object[]{"cipherText"});
      if (key == null)
    	  throw new SecurityException(SecClientMsgKeys.
                  						 INPUT_PARAMETER_IS_NULL, new Object[]{"key"});

      try {
         // Obtain a RSA Cipher Object
         // RSA algorithm, ECB:Electronic Codebook Mode mode,
         // PKCS1Padding: PKCS1 padding
         javax.crypto.Cipher cipher = javax.crypto.Cipher.
                                           getInstance("RSA/ECB/PKCS1Padding");
         byte[] tmpCipherText;
         synchronized(cipher) {
        	 cipher.init(javax.crypto.Cipher.ENCRYPT_MODE, key);

        	 tmpCipherText = cipher.doFinal(plainText);
         }

         System.arraycopy(tmpCipherText, 0, cipherText, 0, tmpCipherText.length);

         len = cipherText.length;
      }catch (Exception ex) {
         throw new SecurityException(ex, SecClientMsgKeys.ENCRYPTION_FAILED, null);
      }
      return len;
   }

   public static javax.crypto.Cipher getEASInstance(String algorithm) throws SecurityException
   {
	   if (algorithm == null)
		   throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL, new Object[]{"algorithm"});
	   try {
		   // Obtain a AES Cipher Object
	       // AES algorithm using a cryptographic key of 128 bits
		   // to encrypt data in blocks of 128 bits,
		   // CBC cipher mode, PKCS5Padding padding

	       return javax.crypto.Cipher.getInstance("AES/CBC/PKCS5Padding");
	   } catch (Exception ex) {
		   throw new SecurityException(ex, SecClientMsgKeys.ENCRYPTION_FAILED, null);
	   }
   }

   /** Encrypts the plain text data message using "EAS256/CBC/PKCS7"
    * @param plainText - plain text to be encrypted
    * @param key - session key is used as secret key
    * @param iv - 16 lower bytes of the nonce is used as the initial vector.
    *             Can't use the whole 32 bytes nonce because the IV size has
    *             to be equal to the block size which is a Java requirement.
    * @Return the cipher text in byte
    * @throws SecurityException
    */
   public static byte[] encryptData(byte[] plainText, SecretKey key, byte[] iv,
		   javax.crypto.Cipher cipher)
                                              throws SecurityException
   {
	   if (plainText == null)
	      throw new SecurityException(SecClientMsgKeys.
	                                         INPUT_PARAMETER_IS_NULL, new Object[]{"plainText"});
	   if (key == null)
		   throw new SecurityException(SecClientMsgKeys.
                   							 INPUT_PARAMETER_IS_NULL, new Object[]{"key"});
	   if (iv == null)
		   throw new SecurityException(SecClientMsgKeys.
                                             INPUT_PARAMETER_IS_NULL, new Object[]{"iv"});

	   try {
	  	   IvParameterSpec initialVector = new IvParameterSpec(iv);

	  	   synchronized (cipher) {
	  		   cipher.init(javax.crypto.Cipher.ENCRYPT_MODE, key, initialVector);

	  		   return cipher.doFinal(plainText);
	  	   }

	   }catch (Exception ex) {
	       throw new SecurityException(ex, SecClientMsgKeys.DATA_ENCRYPTION_FAILED, null);
	   }
   }

   /** Decrypts cipherText and stores the
    * plain text in plainText using private key for password
    * decryption or symmetric key for data decryption
    *
    * @param cipherText cipher text to be decrypted
    * @param plainText decrypted cipher text is returned
    * @param key password decryption: private key
    *        message encryption: session key
    * @param iv 8 sequence nonce is used as the initial vector for symmetric
    *      key encryption.  Null if is private key encryption
    * @return the length of the plain text or -1 in case of error
    * @throws SecurityException
    */
   public int decrypt(byte[] cipherText, byte[] plainText,
               java.security.Key key, byte[] iv) throws SecurityException
   {
      int len = 0;
      if (iv == null) //Private key decyption
      {
         if (plainText == null)
            throw new SecurityException(SecClientMsgKeys.
                                         INPUT_PARAMETER_IS_NULL, new Object[]{"plainText"});
         if (cipherText == null)
        	 throw new SecurityException(SecClientMsgKeys.
                     					 INPUT_PARAMETER_IS_NULL, new Object[]{"cipherText"});
         if (key == null)
        	 throw new SecurityException(SecClientMsgKeys.
                     					 INPUT_PARAMETER_IS_NULL, new Object[]{"key"});
         try {
            // Obtain a RSA Cipher Object
            // RSA algorithm, ECB:Electronic Code book Mode mode,
            // PKCS1Padding: PKCS1 padding
            javax.crypto.Cipher cipher = javax.crypto.Cipher.
                                           getInstance("RSA/ECB/PKCS1Padding");
            byte[] tmpPlainText ;
            synchronized (cipher) {
            	cipher.init(javax.crypto.Cipher.DECRYPT_MODE, key);
            	tmpPlainText = cipher.doFinal(cipherText);
            }
            System.arraycopy(tmpPlainText, 0, plainText, 0, tmpPlainText.length);

            len = plainText.length;
         }catch (Exception ex) {
            throw new SecurityException(ex, SecClientMsgKeys.DECRYPTION_FAILED, null);
         }
      }

      return len;
   }
}
