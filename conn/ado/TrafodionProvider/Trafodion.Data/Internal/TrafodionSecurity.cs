/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

using System;
using System.IO;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Text;

// TODO: this whole file was converted from source originally written Java
//          most of this should be reorganized and checked for performance

namespace Trafodion.Data
{
    internal class PwdKey
    {
        public byte[] id = new byte[TrafodionDBSecurity.PWDID_SIZE];
        public byte[] rolename = new byte[TrafodionDBSecurity.ROLENAME_SIZE];
        public byte[] digest = new byte[TrafodionDBSecurity.DIGEST_LENGTH];
        public byte[]  ts = new byte[TrafodionDBSecurity.TIMESTAMP_SIZE];
        public LoginData data;
    }

    internal class LoginData
    {
        public byte[] session_key = new byte[TrafodionDBSecurity.SESSION_KEYLEN];
        public byte[] nonce = new byte[TrafodionDBSecurity.NONCE_SIZE];
    }

    internal class TrafodionDBSecurity
    {
        //TODO: clean these up, rename to C# style const
        internal const int NONCE_RANDOM = 24;
        internal const int NONCE_SEQNUM = 8;
        internal const int NONCE_SIZE = (NONCE_RANDOM+NONCE_SEQNUM);
        internal const int SESSION_KEYLEN = 32;
        internal const int DIGEST_LENGTH = 32;

        internal const int AES_BLOCKSIZE = 16;
        internal const int KEY_REFRESH = 30;
        internal const int TIMESTAMP_SIZE = 8;
        internal const int ROLENAME_SIZE  = 128;
        internal const int PROCINFO_SIZE =  8;
        internal const int PWDID_SIZE = 4;
        internal const int EXPDATESIZE = 12;
        internal const int PWDKEY_SIZE_LESS_LOGINDATA = (PWDID_SIZE + ROLENAME_SIZE + DIGEST_LENGTH +  TIMESTAMP_SIZE);

        internal const int UNUSEDBYTES = 11;
        internal const int TOKENSIZE = 26;
        internal const byte USERTOKEN_ID_1 = 3; //'\3'
        internal const byte USERTOKEN_ID_2 = 4;	//'\4'
        internal const int DATA_BLOCK_BIT_SIZE = 128;

        private X509Certificate2 _cert;
        private PwdKey _pwdKey;

        private string _certFile;
        private byte [] _procInfo;
        private long _nonceSeq;

        public TrafodionDBSecurity(TrafodionDBConnection conn, string directory, string fileName,
                string remoteHost, int remotePin, int remoteCpu, byte [] timestamp)
        {
            DataStream ds;

            //check for null values, assign defaults
            if(directory == null)
            {
                directory = Environment.GetEnvironmentVariable("USERPROFILE");
            }
            if(fileName == null)
            {
                fileName = remoteHost + ".cer";
            }

            //check the directory and setup the final paths
            if (!Directory.Exists(directory))
            {
                throw new Exception("directory does not exist");
            }
            _certFile = Path.Combine(directory, fileName);

            //create the procinfo struct
            _procInfo = new byte[PROCINFO_SIZE + TIMESTAMP_SIZE];

            /*remoteHost = "\\" + remoteHost.ToUpper();
            if(remoteHost.Length > 8)
            {
                remoteHost = remoteHost.Substring(0,8);
            }*/

            ds = new DataStream(_procInfo, conn.ByteOrder);
            ds.WriteInt32(remotePin);
            ds.WriteInt32(remoteCpu);
            //ds.WriteBytes(ASCIIEncoding.ASCII.GetBytes(remoteHost));
            ds.WriteBytes(timestamp);
        }

        public void OpenCertificate()
        {
            //generate secure random bytes
            RandomNumberGenerator rng = new RNGCryptoServiceProvider();
            byte[] buf = new byte[SESSION_KEYLEN + NONCE_SIZE];
            rng.GetBytes(buf);

            //create the pwdKey
            _pwdKey = new PwdKey();
            _pwdKey.data = new LoginData();
            _pwdKey.id[0] = 1;
            _pwdKey.id[1] = 2;
            _pwdKey.id[2] = 3;
            _pwdKey.id[3] = 4;

            Buffer.BlockCopy(buf, 0, _pwdKey.data.session_key, 0, SESSION_KEYLEN);
            Buffer.BlockCopy(buf, SESSION_KEYLEN, _pwdKey.data.nonce, 0, NONCE_SIZE);

            _nonceSeq = BitConverter.ToInt64(_pwdKey.data.nonce, SESSION_KEYLEN - NONCE_SEQNUM);

            //open the certificate
            _cert = new X509Certificate2(_certFile);

            //Check validity of the certificate
            if (_cert.NotAfter <= DateTime.Now)
            {
                throw new ApplicationException("User certificate has expired!");
            }
        }

        public bool SwitchCertificate(byte[] cert)
        {
            try
            {
                FileStream fs = new FileStream(_certFile, FileMode.Create);
                fs.Write(cert, 0, cert.Length);
                fs.Close();
                OpenCertificate();
            }
            catch
            {
                return false;
            }

            return true;
        }

        //TODO: i dont like all the buffer copying -- there has got to be a better way
        public void EncryptPassword(byte[] pwd, byte[] rolename, out byte[] pwdbuf)
        {
            int pubKeyLen = _cert.PublicKey.Key.KeySize/8;
            int maxPlainTextLen = pubKeyLen - UNUSEDBYTES;

            if ((NONCE_SIZE + SESSION_KEYLEN + pwd.Length) > maxPlainTextLen)
                throw new Exception("password is too long");

            byte[] to_encrypt = new byte[SESSION_KEYLEN + NONCE_SIZE + pwd.Length];
            byte[] cipherText = new byte[pubKeyLen];
            byte[] to_digest = new byte[PROCINFO_SIZE + TIMESTAMP_SIZE + pubKeyLen];
            byte[] digestedMsg = new byte[DIGEST_LENGTH];

            //allocate our final buffer
            pwdbuf = new byte[pubKeyLen + PWDKEY_SIZE_LESS_LOGINDATA];

            System.Buffer.BlockCopy(_pwdKey.id, 0, pwdbuf, 0, PWDID_SIZE);
            if (rolename != null)
            {
                Buffer.BlockCopy(rolename, 0, pwdbuf, PWDID_SIZE, rolename.Length);
            }

            Buffer.BlockCopy(_procInfo, 0, pwdbuf,
                (PWDID_SIZE + ROLENAME_SIZE + DIGEST_LENGTH - PROCINFO_SIZE),
                (PROCINFO_SIZE + TIMESTAMP_SIZE));

            // Build plain text to encrypt
            Buffer.BlockCopy(_pwdKey.data.session_key, 0, to_encrypt, 0, SESSION_KEYLEN);
            Buffer.BlockCopy(_pwdKey.data.nonce, 0, to_encrypt, SESSION_KEYLEN, NONCE_SIZE);
            Buffer.BlockCopy(pwd, 0, to_encrypt, (SESSION_KEYLEN + NONCE_SIZE), pwd.Length);

            RSACryptoServiceProvider rsa = new RSACryptoServiceProvider(_cert.PublicKey.Key.KeySize);
            rsa.FromXmlString(_cert.PublicKey.Key.ToXmlString(false));
            cipherText = rsa.Encrypt(to_encrypt, false);

            int cipherTextLen = cipherText.Length;

            if (cipherTextLen != pubKeyLen)
            {
                throw new Exception("cypher length not equal to public key length");
            }

            Buffer.BlockCopy(cipherText, 0, pwdbuf, PWDKEY_SIZE_LESS_LOGINDATA, cipherTextLen);
            Buffer.BlockCopy(pwdbuf, (PWDKEY_SIZE_LESS_LOGINDATA - TIMESTAMP_SIZE - PROCINFO_SIZE),
                        to_digest, 0, (PROCINFO_SIZE + TIMESTAMP_SIZE + cipherTextLen));

            HMACSHA256 hash = new HMACSHA256(_pwdKey.data.session_key);
            digestedMsg = hash.ComputeHash(to_digest);
            if (digestedMsg.Length != DIGEST_LENGTH)
            {
                throw new Exception("bad digest length");
            }

            Buffer.BlockCopy(digestedMsg, 0, pwdbuf, (PWDKEY_SIZE_LESS_LOGINDATA - TIMESTAMP_SIZE - DIGEST_LENGTH), digestedMsg.Length);
        }

        public string EncryptData(byte[] data)
        {
            byte[] temp;

            //setup the basic attributes
            AesCryptoServiceProvider aes = new AesCryptoServiceProvider();
            aes.Mode = CipherMode.CBC;
            aes.Padding = PaddingMode.PKCS7; //this is really just PKCS, 5 vs 7 is implied based on the data
            aes.BlockSize = DATA_BLOCK_BIT_SIZE;

            //use the session_key as the Key
            temp = new byte[AES_BLOCKSIZE];
            Buffer.BlockCopy(this._pwdKey.data.session_key, AES_BLOCKSIZE, temp, 0, AES_BLOCKSIZE);
            aes.Key = temp;

            //use the nonce as the Initial Vector
            temp = new byte[AES_BLOCKSIZE];
            Buffer.BlockCopy(this._pwdKey.data.nonce, AES_BLOCKSIZE, temp, 0, AES_BLOCKSIZE);
            aes.IV = temp;

            ICryptoTransform ict = aes.CreateEncryptor();
            temp = ict.TransformFinalBlock(data, 0, data.Length);

            //prefix with // and procInfo
            byte[] final = new byte[PROCINFO_SIZE + temp.Length];
            Buffer.BlockCopy(this._procInfo, 0, final, 0, PROCINFO_SIZE);
            Buffer.BlockCopy(temp, 0, final, PROCINFO_SIZE, temp.Length);

            return "//" + Convert.ToBase64String(final);
        }

        public string GetCertExpDate()
        {
            DateTime dt = DateTime.Parse(_cert.GetExpirationDateString());
            dt = dt.ToUniversalTime(); //convert to GMT

            return dt.ToString("yyMMddHHmmss");
        }
    }
}
