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

package org.trafodion.ci;


import java.io.UnsupportedEncodingException;



/**
 *  Implementation of a CI or SQL parameter value.
 *
 *
 */
public class ParamStringObject {


   /**
    *  Member variables containing the charset name, is hexadecimal value and
    *  actual parameter value string specified.
    */
   private  String  _charsetName  = null;
   private  String  _hexPrefixStr = null;
   private  String  _inputValue   = null;


   /**
    *  Boolean flag indicating whether or not the input value is in hex.
    */
   private  boolean _isHexInput   = false;



   /**
    *  Default Parameter value constructor.
    *
    *  @param   charsetName  The character set name
    *  @param   hexInput     Prefix if the parameter value is in hex (x'')
    *  @param   theValue     The actual parameter value
    *
    *  @since   R2.4 SP2
    *
    */
   public ParamStringObject(String charsetName, String hexInputPrefix,
                           String theValue) {
      this._charsetName  = charsetName;
      this._hexPrefixStr = hexInputPrefix;
      this._inputValue   = theValue;

      if (null != this._hexPrefixStr) {
         this._hexPrefixStr = this._hexPrefixStr.trim();

         if (this._hexPrefixStr.toLowerCase().equals("x") )
            this._isHexInput = true;

      }


   }  /*  End of  ParamStringValue  Constructor.  */



   /**
    *  Returns the character set name specified for this parameter value.
    *
    *  @return  the SQL character set name
    *
    */
   public  String  getCharacterSetName() {
      return  this._charsetName;

   }  /*  End of  getCharacterSetName  method.  */



   /**
    *  Returns the evaluated parameter value -- any hex input is converted.
    *
    *  @return  the evaluated parameter value.
    *  @since   R2.4 SP2
    *
    */
   public  String  getParameterValue()  throws Exception {
      /**
       *  Trim out the quotes.
       */
      String  theInput = this._inputValue.trim();
      if (theInput.startsWith("'")  &&  theInput.endsWith("'") )
         theInput = theInput.substring(1, theInput.length() - 1);


      /**
       *  If its plain and simple text -- return it as is.
       */
      if (!this._isHexInput)
         return  theInput;


      /**
       *  For hexadecimal values, trim out the spaces inside the quotes.
       */
      theInput = theInput.trim();


      /**
       *  Okay its a hexadecimal stream that we need to convert.
       */
      String[] inputBits = theInput.split(" ");
      String  retValue = ""; 
      int lengthOfInputByte = 0;
      for (int i = 0; i < inputBits.length; i++) {
    	  if (inputBits[i].length() % 2 == 1)
    		  throw new Exception("'" + theInput + "' is not a valid "
						+ "character encoding value. ");
    	  lengthOfInputByte += inputBits[i].length() / 2;
		}
      byte[] inputByte = new byte[lengthOfInputByte];
      int inputPos = 0;

      for (int idx = 0; idx < inputBits.length; idx++) {
    	  String aChunk = inputBits[idx];
    	  int length = aChunk.length() & -4;
		  if (this._charsetName == null
					|| (!this._charsetName.equalsIgnoreCase("_UCS2") && !this._charsetName
							.equalsIgnoreCase("_UTF8"))) {
    		  do {
    	          	 String bitVal = aChunk;
    	          	 if (4 < aChunk.length() )  {
    	          		 bitVal = aChunk.substring(0, 4);
    	          		 aChunk = aChunk.substring(4, aChunk.length() );
    	             } else
    	                aChunk = "";
    	          	 
    	             try {
    	                int val = Integer.parseInt(bitVal, 16);
    	                
    	                byte  b1 = (byte) (val & 0xFF);
    	                byte  b2 = (byte) ((val >> 8) & 0xFF);
    	                byte  b3 = (byte) ((val >> 16) & 0xFF);
    	                byte  b4 = (byte) ((val >> 24) & 0xFF);

    	                if ((0 != b4) ||  (0 != b3) )
    	                   retValue += new String(new byte[] { b4, b3, b2, b1 } );
    	                else if (0 != b2) 
    	                   retValue += new String(new byte[] { b2, b1 } );
    	                else
    	                   retValue += new String(new byte[] { b1 });
    	             } catch(Exception e) {
    	            	 throw new Exception("'" + inputBits[idx]
    	            	         							+ "' is not a valid " + "hexadecimal value. ");
    	             }
    		  } while (0 < aChunk.length() );
    	  }/**
    	    * for UTF-8
		    * modify to fix the defect CRID1331
		    */
			else if (this._charsetName.equalsIgnoreCase("_UTF8")) {
				for (int pos = 0; pos < aChunk.length(); pos += 2) {
					try {
						int val = Integer.parseInt(
								aChunk.substring(pos, pos + 2), 16);
						inputByte[inputPos++] = (byte) (val & 0xFF);
					} catch (NumberFormatException NF_Ex) {
						throw new Exception("'" + aChunk + "' is not a valid "
								+ "hexadecimal value. ");
					}
					//when it comes to the end of the bits, convert it
					if (idx == inputBits.length - 1
							&& pos == aChunk.length() - 2)
						try {
							retValue = new String(inputByte, "UTF8");
						} catch (UnsupportedEncodingException e) {
							throw new Exception(
									"The input hexadecimal is not a valid UTF-8 hexadecimal value. ");
						}
				}
			}else{ // UCS2
    		  /**
        	   * set Param is used to set ucs2 values. CI gives wrong data
        	   * Internal Analysis: by shifting all low bits to right causes UCS2 hex
        	   * to get the first byte chopped off & turn into invalid character
        	   * Fix Description: simply convert hex values to String
        	   */
    		  for ( int pos=0; pos < length; pos += 4) {
        		  int this_char = 0;
        		  try { 
                	   this_char = Integer.parseInt( aChunk.substring( pos,pos+4), 16);
        		  }
        		  catch ( NumberFormatException NF_Ex) { 
        			  throw new Exception("'" 
        			     + aChunk + "' is not a valid " + "hexadecimal value. ");
        		  }
        		  retValue += (char) this_char;
               }
    	  }
      }  /*  End of  FOR  loop  for all the parts in the input.  */
      
      return  retValue;

   }  /*  End of  getParameterValue  method.  */




   /**
    *  Returns the Stringified form of this parameter value.
    *
    *  @return  the stringified form of this parameter value.
    *  @since   R2.4 SP2
    *
    */
   public String toString() {
      String theValue = null;

      if (null != this._charsetName)
         theValue = this._charsetName.toUpperCase();

      if (null == theValue)
         theValue = this._hexPrefixStr;
      else if (null != this._hexPrefixStr)
         theValue += " " + this._hexPrefixStr;


      if (null == theValue)
         theValue = this._inputValue;
      else
         theValue += this._inputValue;

      return  theValue;
 
   }  /*  End of  toString  method.  */



}  /*  End of  class  ParamStringValue.  */


