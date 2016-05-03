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

/**
 * This class contains defines
 *
 */

public class SecdefsCommon {

     public static final int NONCE_RANDOM = 24;
     public static final int NONCE_SEQNUM = 8;
     public static final int NONCE_SIZE = (NONCE_RANDOM+NONCE_SEQNUM);
     public static final int SESSION_KEYLEN = 32;
     public static final int DIGEST_LENGTH = 32;
     // AES block size used in data encryption
     public static final int AES_BLOCKSIZE = 16;
     public static final int KEY_REFRESH = 30;
     public static final int TIMESTAMP_SIZE = 8;
     public static final int ROLENAME_SIZE  = 128;
     public static final int PROCINFO_SIZE =  8;
     public static final int PWDID_SIZE = 4;
     public static final int EXPDATESIZE = 12;
     public static final int PWDKEY_SIZE_LESS_LOGINDATA = (PWDID_SIZE + ROLENAME_SIZE + DIGEST_LENGTH +  TIMESTAMP_SIZE);
     // For public key encryption, the  number of bytes
     // to be encrypted is 11 bytes less than the public key length
     public static final int UNUSEDBYTES = 11;
     public static final int TOKENSIZE = 68;
     // User tokens begin with byte values 3,4.
     public static final byte USERTOKEN_ID_1 = '\3'; 	// User token identifier, must be a sequence
     public static final byte USERTOKEN_ID_2 = '\4';	// not allowed in password
     public static final int DATA_BLOCK_BIT_SIZE = 128;  // data encryption block size in bits.  Java
                                                         // supports block size of 128 bits for AES
                                                         // algorithm using cryptographic key of 256 bits only.


     // Structure used to describe layout of Encrypted data
     // in login message
     public static class LoginData {
        //000 Session key
        byte[] session_key = new byte[SecdefsCommon.SESSION_KEYLEN];
        //032 Nonce
        byte[] nonce = new byte[SecdefsCommon.NONCE_SIZE];
        Byte password;            // 064 User's password
     }            // 128 for 1024 or 256 for 2048

//  Structure used to describe layout of password key

    public static class PwdKey {
       //000 Key identifier, binary values 1,2,3,4
       //or 1,2,2,4 keys, optional mode only
       byte[] id= new byte[SecdefsCommon.PWDID_SIZE];
       //004 RolenameA
       byte[] rolename = new byte[SecdefsCommon.ROLENAME_SIZE];
       //132 Digest of server id and encrypted data
       byte[] digest = new byte[SecdefsCommon.DIGEST_LENGTH];
       // 164 time stamp
       byte[]  ts = new byte[SecdefsCommon.TIMESTAMP_SIZE];
       LoginData data;             //172 Encrypted data
   }
}
