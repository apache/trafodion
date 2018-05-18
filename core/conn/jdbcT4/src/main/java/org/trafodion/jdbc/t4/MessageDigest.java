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
  * class MessageDigest - Computes the message authentication code using
  *                      the SHA256 hash function
  */

package org.trafodion.jdbc.t4;

public class MessageDigest
{
   private static class Holder
   {
      private static MessageDigest instance = new MessageDigest();
   }

   /**
     *
     * @return MessageDigest
     */
   public static MessageDigest getInstance()
   {
      return Holder.instance;
   }

   /**Digests message using HmacSHA256 algorithm.
    * @param key - session key to use for create secret key used in HMAC digest
    * @param data - The data used to create the digest
    * @param md - returns the digested message
    * @return the digested message's length or -1 in case of failure
    * @throw SecurityException
    */
   public int digest(byte[] key, byte[] data, byte[] md) throws SecurityException
   {
      if (key == null)
         throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL,
        		 new Object[]{"key"});
      if (data == null)
          throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL,
         		 new Object[]{"data"});
      if (md == null)
          throw new SecurityException(SecClientMsgKeys.INPUT_PARAMETER_IS_NULL,
         		 new Object[]{"md"});
      try {
    	  javax.crypto.spec.SecretKeySpec keySpec =
    		new javax.crypto.spec.SecretKeySpec(key, "HmacSHA256");
    	  javax.crypto.Mac mac = javax.crypto.Mac.getInstance("HmacSHA256");
    	  byte[] tmpMd;
    	  synchronized (mac) {
    		  mac.init(keySpec);

    		  tmpMd = mac.doFinal(data);
    	  }
    	  System.arraycopy(tmpMd, 0, md, 0, tmpMd.length);

         return tmpMd.length;
      }catch (Exception ex) {
          throw new SecurityException(ex, SecClientMsgKeys.HMAC_FAILED, null);
      }

   }
}
