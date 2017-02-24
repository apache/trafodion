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

public class InterfaceSyntaxError
{


   private StringBuffer errMesg=null;
   private String errPrefix = SessionError.SYNTAX_ERROR_PREFIX; //"ERROR: A syntax error occurred at or before:";

   InterfaceSyntaxError()
   {

   }

   public void setSyntaxError(String qryString ,String remainderStr)
   {
      if (errMesg == null)
      {
         errMesg=new StringBuffer();
      }
      this.errMesg.delete(0,errMesg.length());
      //this.errMesg.append(SessionDefaults.lineSeperator);
      this.errMesg.append(errPrefix);
      this.errMesg.append(SessionDefaults.lineSeperator);
      this.errMesg.append(qryString);
      this.errMesg.append(SessionDefaults.lineSeperator);

      this.errMesg.append(qryString.replaceAll("\\S"," "));

      /*for(int i=0; i < (qryString.length()-remainderStr.length());i++)
      this.errMesg.append(" ");*/
      for (int i=0; i < remainderStr.length();i++)
         this.errMesg.delete(errMesg.length()-1,errMesg.length());

      this.errMesg.append("^");

      this.errMesg.replace(0,this.errMesg.length(),this.errMesg.toString().replaceAll("(?m)^\\s+$",""));
      //this.errMesg.append("^");
   }

   public ErrorObject getSyntaxError(String qryString,String remainderStr )
   {

      if (qryString != null)
      {
         qryString=qryString.replaceAll("\\s+$","");
      }

      if (remainderStr == null)
      {
         remainderStr="";
      }
      else
      {
         remainderStr=remainderStr.trim();
      }

      setSyntaxError(qryString,remainderStr);
      return new ErrorObject(SessionError.GENERIC_SYNTAX_ERROR_CODE, errMesg.toString());
   }


}
