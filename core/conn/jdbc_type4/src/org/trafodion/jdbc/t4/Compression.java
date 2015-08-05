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
package org.trafodion.jdbc.t4;

public class Compression {
	private static int MAXCOUNT = 0xFF;
	// private static int MAXUCOUNT = 0xFFFF;
	private static int EOFCHAR = -1;
	
	
	
	public static int compress(int alg, byte [][] sbuf, int [] sstart, int [] slen, byte[] dbuf, int dstart) {	
		int ret = 0;
		
		switch(alg) {
		case 1:
			ret = compress_whitespace(sbuf, sstart, slen, dbuf, dstart);
			break;
		}
		
		return ret;
	}
	
	public static int uncompress(int alg, byte [] sbuf, int sstart, int slen, byte [] dbuf, int dstart) {
		int ret = 0;
		
		switch(alg) {
		case 1:
			ret = uncompress_whitespace(sbuf, sstart, slen, dbuf, dstart);
			break;
		}
		
		return ret;
	}
	
	// encodes repeated bytes in the byte form <b><b><#>
	// b = byte which is repeated
	// # = number of repetitions (max 255)
	// source buffers, source lengths, destination buffer
	private static int compress_whitespace(byte [][] param_sbuf, int [] param_sstart, int [] param_slen, byte[] dbuf, int dstart) {
		int c = EOFCHAR, p = EOFCHAR; 
		int si = 0, di = dstart; // source, destination indexes
		int cnt = 0; // count of repetition
		
		byte []sbuf; //current source buffer
		int slen; //current source length
		
		for(int i=0;i<param_sbuf.length;i++) {
			sbuf = param_sbuf[i];
			slen = param_slen[i];
			si = param_sstart[i];
			
			while(si < slen) {
				c = sbuf[si++]; // get the next byte from source
				dbuf[di++] = (byte)c; // copy the byte to destination
				cnt = 0; 
				
				if(c == p) { // check repetition
					if(si == slen) {
						c = EOFCHAR;
					}
					while(si < slen) {
		                if ((c = sbuf[si++]) == p) { // found repetition
		                    cnt++;
		                    if (cnt == MAXCOUNT) { // we can only store 255 in
													// a byte
		                        dbuf[di++] = (byte)cnt;
		                        
		                        p = EOFCHAR; // move on
		                        break;
		                    }
		                }
		                else {
		                    dbuf[di++] = (byte)cnt;
		                    dbuf[di++] = (byte) c;
	
		                    p = c;
		                    break;
		                }
		                
		                if (si == slen) {
	    	            	c = EOFCHAR;
	    	            }
		            } 
				}
				else {
					p = c; // set our current as our previous
				}
				
				if (c == EOFCHAR) {
 	            	dbuf[di++] = (byte)cnt;
 	            	/*
 	            	 * ADDED
 	            	 */
 	            	p = EOFCHAR;
 	            }
			}
		}
		
		return di - dstart;
	}
	
	private static int uncompress_whitespace(byte [] sbuf, int sstart, int slen, byte [] dbuf, int dstart) {
		int c = EOFCHAR, p = EOFCHAR;
		int si = sstart, di = dstart, i = 0; // source, dest, and generic indexes
		int cnt = 0;

		while(si < slen ) {
			c = sbuf[si++] & 0xFF;
			
			/*if(di >= dbuf.length) {
				System.out.println("%%%%%failed at " + di);
			}*/
			dbuf[di++] = (byte) c;
			
			if(c == p) {
				cnt = sbuf[si++] & 0xFF;
				
				for(i=0;i<cnt;++i) {
					dbuf[di++] = (byte)c;
				}
				p = EOFCHAR;
			}
			else {
				p = c;
			}
		}
		
		return di;
	}	
}
