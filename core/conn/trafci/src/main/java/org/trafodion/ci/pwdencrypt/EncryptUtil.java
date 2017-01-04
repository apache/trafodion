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
/**
 *
 */
package org.trafodion.ci.pwdencrypt;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;

public class EncryptUtil {

	/**
	 * Java Cryptor.
	 */
	private static JCE j = new JCE();
	static boolean doTrace = Boolean.getBoolean("trafci.enableTrace");


	/**
	 * Main method.
	 * @param args arguments from command line
	 */
	public static void main(final String[] args) {
		PasswordOpts settings = new PasswordOpts();
		settings.parseCommandLine(args);
		try {
			if (settings.getWktype().equalsIgnoreCase(WorkTypes.ADD)) {
				add(settings.getRef(), settings.getText());
			//	System.out.println("Decrypting the password::");
				//j.initSiteEncryptionService();
				//	read encrypt file
				ArrayList<String> refs = readLines();
				if (!refs.isEmpty()) {
					for (int i=0; i < refs.size(); i++) {
					//	System.out.println("refs["+i+"]" + refs.get(i));
						RefLookup.resolve(refs.get(i));
						/*System.out.println("ref stri = " + ss.substring(3, ss.length()-1));
                        System.out.println("Pw::" + ss);*/
					}
				}
				
				//System.out.println((j.decrypt("{DES}44SnpMxWui2uqvrrVzJwZA==")).toString());
			} else if (settings.getWktype().equalsIgnoreCase(WorkTypes.DEL)) {
				del(settings.getRef());
			} else if (settings.getWktype()
					.equalsIgnoreCase(WorkTypes.INSTALL)) {
				j.setupSiteEncryptionService();
			}
		} catch (Exception e) {
			StringBuffer msg = new StringBuffer();
			msg.append("\n\tError Message: ").append(e.getMessage());
			if (doTrace) { 
				e.printStackTrace();
			}
			System.exit(1);
		}
	}

	/**
	 * Adds a encryption map to the secfile.
	 * @param ref reference test
	 * @param tw text word
	 * @throws DBTransporterException on error
	 */
	private static void add(final String ref, final String tw)
	throws Exception {
		//init encryptor
		j.initSiteEncryptionService();
		final String en =
			j.encrypt(j.sitePair.encryptor, tw);

		//read encrypt file
		ArrayList<String> refs = readLines();

		//remove existing entry
		for (int i=0; i< refs.size(); i++) {
			if (ref != null){
				if (0 == refs.get(i).indexOf(ref + " = {DES}")) {
					refs.remove(refs.get(i));
					break;
					//System.exit(0);
				}
			}
		}
		//add the new entry
		refs.add(ref + " = {DES}" + en);
		writelines(refs);
	}

	/**
	 * Deletes encryption reference.
	 * @param ref reference name
	 */
	private static void del(final String ref) {
		//read in encrypt file
		ArrayList<String> refs = readLines();
		//locate by ref and delete
		//For on-platform, clear the encrypted file when option is del
		for (int i=0; i< refs.size(); i++) {
			if (ref != null){
				if (0 == refs.get(i).indexOf(ref + " = {DES}")) {
					refs.remove(refs.get(i));
					break;
					//System.exit(0);
				}
			}
		}
		if (refs != null){
			//refs.clear();
			writelines(refs);
		}
		//if(doTrace) 
				//System.out.println("Failed in del:: " + ref);

	}
	
	/**
	 * Reads encctlprops file.
	 * @return ArrayList of the reference maps.
	 */
	public static ArrayList<String> readLines() {
		ArrayList<String> refs = new ArrayList<String>();
		
		try {
			//get file in stream






			StringBuilder buff = new StringBuilder();
			StringBuilder ref = new StringBuilder();
			
			File secfile = new File(j.getSecDir() + JCE.encfilename);
			FileInputStream fis = new FileInputStream(secfile);
			InputStreamReader converter = new InputStreamReader(fis);
			BufferedReader in = new BufferedReader(converter);
			String tmpStr="";
			String prevStr=null, nextStr=null;
			
			//This loop will read the complete reference string even if it
			//spans to multiple lines. This way the partial delete or addition 	
			//to the reference is avoided.
			prevStr = in.readLine();
			while(prevStr != null ){
				nextStr = in.readLine();
				
				if (prevStr.contains(" = {DES}")){
					if( nextStr == null || (nextStr!= null && nextStr.contains(" = {DES}"))){
						buff.append(prevStr);
						buff.append('\n');						
					}
					else {
						tmpStr = prevStr;
					}
				}
				else{
					tmpStr = tmpStr + prevStr; 
					if (nextStr == null || nextStr.contains(" = {DES}")){
						buff.append(tmpStr);
						buff.append('\n');						
					}
				}
				prevStr = nextStr;
				
			}
		
			// read and assign to refs list
			char c;
			for(int i =0; i< buff.length(); i++){
				if((c = buff.charAt(i)) != '\n' ){
					ref.append(c);
				}
				else{
					refs.add(ref.toString());
					ref =  new StringBuilder();
				}	
			}
			
			//close stream
			fis.close();
			
		} catch (IOException e) {
			StringBuffer msg = new StringBuffer();
			msg.append("\n\tError Message: ").append(e.getMessage());
			if (doTrace) { // log stack trace
				
				e.printStackTrace();
			}
			
		}
		return refs;
	}
	
	/**
	 * Writes encctlprops using list provided by add or del.
	 * @param refs list provided by add or del
	 */
	private static void writelines(final ArrayList<String> refs) {
		StringBuilder buff = new StringBuilder();
		
		for (String ref : refs) {
			buff.append(ref + '\n');
		}
		
		try {
			String pathName = j.getSecDir() + JCE.encfilename;
			File secfile = new File(pathName);
			FileOutputStream fos = new FileOutputStream(secfile);
			fos.write(buff.toString().getBytes());
			fos.flush();
			fos.close();
			Runtime.getRuntime().exec("chmod 600 " + pathName);
		} catch (IOException e) {
			StringBuffer msg = new StringBuffer();
			msg.append("\n\tError Message: ").append(e.getMessage());
			if (doTrace) { // log stack trace
			e.printStackTrace();
			}
			
		}
	}
}
