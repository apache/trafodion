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
**********************************************************************/
package org.trafodion.ci.pwdencrypt;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.spec.AlgorithmParameterSpec;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.KeySpec;
import java.util.Arrays;
import java.util.Properties;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.KeyGenerator;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESKeySpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.PBEParameterSpec;


public class JCE {
	public static final String SECRETKEY_ALGORITHM = "PBEWithMD5AndDES";
	public static final String SYMMETRICKEY_ALGORITHM = "DES";
	public static final String DIGEST_ALGORITHM = "MD5";

    private static final byte[] salt = { (byte)0xFF, (byte)0x20, (byte)0xDF,
    	(byte)0xA6, (byte)0x5D, (byte)0x0D, (byte)0x34, (byte)0xD2 };

    private static final char[] pass = { '\u00FF','\u0010','\u00EF', '\u00F6',
    	 '\u004B', '\u0004', '\u0056', '\u0088' };

    protected static final String encfilename = "/encprops.txt";
	private static final int count = 20;

	private String ciHome;

	private String secDir;

	private String secFile;

	private static final String securityDir = ".ciconf";

	private static final String securityFile = "security.props";

	CipherPair basePair;

	CipherPair sitePair;

	MessageDigest md5;


	/**
	 *
	 */
	public JCE() {
		this.ciHome = System.getenv("USERHOME"); //System.getProperty("user.dir");
		if (ciHome ==null){
			System.out.println("USERHOME set failed, Please check trafci/bin/ciencr.sh");
		}
		this.secDir = this.ciHome + '/' + securityDir; // "/sql/scripts/" + securityDir;
		this.secFile = this.secDir + '/' + securityFile;
		
		try {
			md5 = initDigest();
			basePair = initCiphers(pass, salt, count);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}


	public void setupSiteEncryptionService() throws Exception {
		try {
			makeSecurityDir();

			KeyGenerator keyGen = KeyGenerator.getInstance("DES");

			SecretKey sKey = keyGen.generateKey();
			byte[] encKey = sKey.getEncoded();
			// We write two files, a digest of the key and the
			// encrypted key itself
			md5.reset();
			byte[] digest = md5.digest(encKey);
			sun.misc.BASE64Encoder ben = new sun.misc.BASE64Encoder();
			String d = ben.encode(digest);
			// Now encrypt the generated key with DBT cipher
			byte[] enc = basePair.getEncryptor().doFinal(encKey);
			String k = ben.encode(enc);
			Properties props = new Properties();
			props.put("d", d);
			props.put("k", k);
			File theSecFile = new File(secFile);
			FileOutputStream fos = new FileOutputStream(theSecFile);
			props.store(fos, "TRAFCI Security Initialization File");
			fos.flush();
			fos.close();
			Runtime.getRuntime().exec("chmod 600 " + secFile);
		} catch (IllegalBlockSizeException ibse) {
			throw (Exception) new Exception("Invalid cipher block size")
					.initCause(ibse);
		} catch (BadPaddingException bpe) {
			throw (Exception) new Exception("Incorrect cipher padding")
					.initCause(bpe);
		} catch (FileNotFoundException fnfe) {
			throw (Exception) new Exception("Encryption utility not installed under current NVTHOME")
					.initCause(fnfe);

		} catch (NoSuchAlgorithmException nsae) {
			throw (Exception) new Exception("Invalid cipher algorithm")
					.initCause(nsae);
		} catch (IOException ioe) {
			throw (Exception) new Exception("IO Exception")
					.initCause(ioe);
		}
	}

	public void initSiteEncryptionService() throws Exception {
		try {
			FileInputStream fis = new FileInputStream(secFile);
			Properties props = new Properties();
			props.load(fis);
			String k = props.getProperty("k");
			String d = props.getProperty("d");
			sun.misc.BASE64Decoder bde = new sun.misc.BASE64Decoder();
			// Decrypt the key
			byte[] key = decrypt(basePair.getDecryptor(), k);
			byte[] digest = bde.decodeBuffer(d);
			md5.reset();
			byte[] d2 = md5.digest(key);
			if (!Arrays.equals(digest, d2)) {
				throw (Exception) new Exception("FATAL -- Tampered key");
			}
			sitePair = initCiphers(key);

			// Get rid of the base pair

			basePair.setEncryptor(null);
			basePair.setDecryptor(null);
			basePair = null;
		} catch (FileNotFoundException fnfe) {
			throw (Exception) new Exception("Encryption utility not installed under current TRAFCIHOME")
					.initCause(fnfe);

		} catch (IOException ioe) {
			throw (Exception) new Exception("IO Exception")
					.initCause(ioe);
		}
	}

	private void makeSecurityDir() throws Exception {
		final File dir = new File(secDir);

		if (!dir.exists()) {
			dir.mkdirs();
		}
		final File encfile = new File(
				secDir + '/' + "encprops.txt");
		
		try{
			if (!encfile.exists()) {
				encfile.createNewFile();
				Runtime.getRuntime().exec("chmod 600 " + secDir + '/' + "encprops.txt");
			}
			
		} catch (IOException e) {
			throw (Exception) new Exception("IO Exception")
			.initCause(e);
		}
			
		
		

	}

	public MessageDigest initDigest() throws Exception {
		try {
			MessageDigest md5 = MessageDigest.getInstance(DIGEST_ALGORITHM);
			return md5;
		} catch (NoSuchAlgorithmException nsae) {
			throw (Exception) new Exception("Unknown digest")
					.initCause(nsae);
		}
	}

	public CipherPair initCiphers(byte[] keyData) throws Exception {
		try {

			DESKeySpec dspec = new DESKeySpec(keyData);

			SecretKeyFactory skf = SecretKeyFactory.getInstance(SYMMETRICKEY_ALGORITHM);
			SecretKey sKey = skf.generateSecret(dspec);

			Cipher encryptor = Cipher.getInstance(SYMMETRICKEY_ALGORITHM);
			Cipher decryptor = Cipher.getInstance(SYMMETRICKEY_ALGORITHM);
			encryptor.init(Cipher.ENCRYPT_MODE, sKey);
			decryptor.init(Cipher.DECRYPT_MODE, sKey);
			return new CipherPair(encryptor, decryptor);

		} catch (InvalidKeySpecException ikse) {
			throw (Exception) new Exception("Invalid Key specification")
					.initCause(ikse);
		} catch (InvalidKeyException ikse) {
			throw (Exception) new Exception("Invalid Key")
					.initCause(ikse);
		} catch (NoSuchAlgorithmException nsae) {
			throw (Exception) new Exception("No such algorithm")
					.initCause(nsae);
		} catch (NoSuchPaddingException nsae) {
			throw (Exception) new Exception("No such padding")
					.initCause(nsae);
		}
	}

	public CipherPair initCiphers(SecretKey key, byte[] salt, int count)
			throws Exception {
		try {
			Cipher encryptor = Cipher.getInstance(SECRETKEY_ALGORITHM);

			Cipher decryptor = Cipher.getInstance(SECRETKEY_ALGORITHM);

			AlgorithmParameterSpec paramSpec = new PBEParameterSpec(salt, count);

			encryptor.init(Cipher.ENCRYPT_MODE, key, paramSpec);

			decryptor.init(Cipher.DECRYPT_MODE, key, paramSpec);

			return new CipherPair(encryptor, decryptor);
		} catch (NoSuchAlgorithmException nsae) {
			throw (Exception) new Exception("Invalid cipher algorithm")
					.initCause(nsae);
		} catch (InvalidAlgorithmParameterException iape) {
			throw (Exception) new Exception("Invalid algorithm parameter")
					.initCause(iape);
		} catch (InvalidKeyException ike) {
			throw (Exception) new Exception("Invalid Key").initCause(ike);
		} catch (NoSuchPaddingException nspe) {
			throw (Exception) new Exception("Unknown padding")
					.initCause(nspe);
		}

	}


	private char[] getPass(char ePass[]) {
		int len = ePass.length;
		byte p[] = new byte[len];
		char ret[] = new char[len];

		Arrays.fill(p, (byte)0);
		for (int row = 0; row < len; ++row) {
			for (int col = 0; col < len; ++col) {
				int bit = (int)((ePass[row] >>> ( len - 1 - col)) & 0x1);
				p[col] |= (int)(bit << (len - 1 - row));
			}
		}

		for (int i = 0; i <  len; ++i) {
			ret[i] = (char) (~p[i] & 0xFF);
		}
		return ret;
	}

	public CipherPair initCiphers(char[] pass, byte[] salt, int count)
			throws Exception {
		try {

			KeySpec keySpec = new PBEKeySpec(getPass(pass), salt, count);

			SecretKey key = SecretKeyFactory.getInstance(SECRETKEY_ALGORITHM)
					.generateSecret(keySpec);

			return initCiphers(key, salt, count);

		} catch (NoSuchAlgorithmException nsae) {
			throw (Exception) new Exception("Invalid cipher algorithm")
					.initCause(nsae);
		} catch (InvalidKeySpecException ike) {
			throw (Exception) new Exception("Invalid Key specification")
					.initCause(ike);
		}
	}

	public String encrypt(Cipher encryptor, String msg) throws Exception {

		byte[] bytes;

		bytes = msg.getBytes();
		byte[] enc = null;
		try {
			enc = encryptor.doFinal(bytes);
		} catch (IllegalBlockSizeException ibse) {
			throw (Exception) new Exception("Invalid cipher block size")
					.initCause(ibse);
		} catch (BadPaddingException bpe) {
			throw (Exception) new Exception("Incorrect cipher padding")
					.initCause(bpe);
		}
		String encrypted = new sun.misc.BASE64Encoder().encode(enc);
		
		if (encrypted.contains("\n")){
			encrypted.replaceAll("\r\n", "");
		}
		return  encrypted;
	}

	/**
	 * This calls the other version of encrypt which requires access to this
	 * class' package protected sitePair.  Allows callers outside of this
	 * package to encrypt strings.
	 * @param msg the string to be encrypted
	 * @return the encrypted string
	 * @throws DBTransporterException when encryption fails
	 */
	public String encrypt(String msg) throws Exception {
		return encrypt(sitePair.encryptor, msg);
	}

	public byte[] decrypt(Cipher decryptor, String enc) throws Exception {
		byte[] bytes = null;
		try {
			byte[] dec = new sun.misc.BASE64Decoder().decodeBuffer(enc);
			bytes = decryptor.doFinal(dec);
		} catch (java.io.IOException ioe) {
			throw (Exception) new Exception("Decoding exception")
					.initCause(ioe);
		} catch (IllegalBlockSizeException ibse) {
			throw (Exception) new Exception("Invalid cipher block size")
					.initCause(ibse);
		} catch (BadPaddingException bpe) {
			throw (Exception) new Exception("Incorrect cipher padding")
					.initCause(bpe);
		}
		return bytes;
	}

	/**
	 * This calls the other version of decrypt which requires access to this
	 * class' package protected sitePair.  Allows callers outside of this
	 * package to decrypt strings.
	 * @param msg the string to be decrypted
	 * @return the decrypted string
	 * @throws DBTransporterException when decryption fails
	 */
	public byte[] decrypt(String msg) throws Exception {
		return decrypt(sitePair.decryptor, msg);
	}

	public class CipherPair {
		Cipher encryptor;

		Cipher decryptor;

		private CipherPair() {
		}

		private CipherPair(Cipher encryptor, Cipher decryptor) {
			this.encryptor = encryptor;
			this.decryptor = decryptor;
		}

		private Cipher getDecryptor() {
			return decryptor;
		}

		private void setDecryptor(Cipher decryptor) {
			this.decryptor = decryptor;
		}

		private Cipher getEncryptor() {
			return encryptor;
		}

		private void setEncryptor(Cipher encryptor) {
			this.encryptor = encryptor;
		}
	}

	/**
	 * @return the secDir
	 */
	public String getSecDir() {
		return secDir;
	}
}
