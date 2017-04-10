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

import java.io.FileInputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;

import java.io.IOException;
import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class RefLookup {
	/** This pattern describes something like 
	 * 'password = {DES}B/+8B2OcDkKWCFaCfwSwgQ==$'.
	 * Or more specifically, the word "password" (case insensitive),
	 * followed by any number of spaces, followed by an equal sign and any
	 * number of spaces.  Then with or without quotes is any word or non-word 
	 * characters .  The reference String is identified as group 1 and encrypted 
	 * strinf * is identified as group 4 in the pattern "refPattern"
	 */
	private static Pattern refPattern = Pattern.compile(
			"([^\\-]([^=](\\w*))*)((\\s*=\\s*\"*)([\\W*\\w*])*(\"*))", 
			Pattern.CASE_INSENSITIVE);

	public static String resolve(final String ref) {
		if (ref == null || ref.length() == 0) {
			return ref;
		}
		try {
			//if (ref.charAt(0) == '$') {
				JCE j = new JCE();

			//	if (ref.charAt(1) == 'E') {
					try{
						if(! new File(j.getSecDir()).exists()){
							System.out.println("File exists !!!");
						}
					//	System.out.println("File being read from ...encprops" + j.getSecDir());
						FileInputStream fis = new FileInputStream(j.getSecDir() + "/encprops.txt");
						j.initSiteEncryptionService();
						String userName = ref.substring(0, ref.indexOf("=")-1);
					//	System.out.println("ref stri = " + userName);
						return new String(j.decrypt(j.sitePair.decryptor, findRef(fis, userName))); //ref.substring(0, ref.length()))));
					}catch (IOException e) {

						e.printStackTrace();
					}

			/*	}else {
					try{
					FileInputStream fis = new FileInputStream(j.getSecDir() + "/ctlprops.txt");
					return findRef(fis, ref.substring(2,ref.length()-1));
			      } catch (IOException e) {
						e.printStackTrace();
					}

				} 
			} */

		} catch (Exception e) {
			StringBuffer msg = new StringBuffer();
			msg.append("\n\tError Message: ").append(e.getMessage());
			e.printStackTrace();


		}
		return ref;
	}

	private static String findRef(FileInputStream fis, String ref) throws IOException{

	  String strLine;
//		String str[] = null;
	    boolean match = false;
	    String enc = null;
	    String refStr=null;
	    //Fixed the password reference matching logic , so now the partial reference inputs
	    //will not be accepted and reports error in that case

	    BufferedReader br = new BufferedReader(new InputStreamReader(fis));
	    
	    while ((strLine = br.readLine()) != null)   {
	    	Matcher matcher = refPattern.matcher(strLine);
	    	if(matcher.find()) {
	    		refStr = matcher.group(1).trim(); //group(1) gives the reference String
		    	//	System.out.println("strLine1=" + refStr);
				if( refStr.equals(ref)){
				       // The password to be encrypted
					enc = matcher.group(4).trim(); //group(4) gives the encrypted value
					match = true;
				//	System.out.println("strLine2=" + enc);
				}
				if(match)
					break;
	    	}
	    }
	    if(!match){
	    	System.out.println("Match failed!!");
	    	System.exit(0);
	    }
	    	
	    //Trims lead " = {DES}" if present. Would be present when Encrypted.
		return enc.replaceFirst("=", "").trim().replace("{DES}", "");
	}
}
