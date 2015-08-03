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

package org.trafodion.sql.udr;

/**
   *  This is the exception to throw when an error occurs in a UDR.
   * <p>
   *  The SQLState value must be a value between 38000 and 38999,
   *  since the SQL standard reserves SQLState class 38 for user-written
   *  code. SQLState values 38950 to 38999 are reserved for use by
   *  Trafodion code. Trafodion will produce SQL error code -11252 when
   *  this exception is thrown.
   */
public class UDRException extends Exception
  {

/**
 * Serial id for "Serializable" base class
 */
	private static final long serialVersionUID = 7406408752369853225L;

/**
 *  Constructor with an integer value for SQLSTATE
 *  @param sqlState ISO/ANSI SQLSTATE value to produce for this error.
 *                  According to the standard, this must be a value in
 *                  the range of 38000 - 38999 (note that since we use
 *                  an integer, non-numeric SQLSTATE values cannot be
 *                  generated.
 *  @param printf_format a format string like it is used in printf,
 *                  with a variable list of arguments to be substituted.
 *                  Example:
 *                  new UDRException(38001, "num %d, string %s", 1, "a");
 */
      public UDRException(int sqlState, String printf_format, Object ... args) 
      {
          super(String.format(printf_format, args));
          String val = Integer.toString(sqlState);
          if (val.length() > 5)
              sqlState_ = val.substring(0,5);
          else
              sqlState_ = val;
      }

/**
 *  Constructor with a string value for SQLSTATE
 *  @param sqlState ISO/ANSI SQLSTATE value to produce for this error.
 *                  According to the standard, this must be a value of
 *                  the form 38xxx, with the xxx being digits or upper
 *                  case letters.
 *  @param printf_format a format string like it is used in printf,
 *                  with a variable list of arguments to be substituted.
 */
      public UDRException(String sqlState, String printf_format, Object ... args)
      {
          super(String.format(printf_format, args));
          if (sqlState.length() > 5)
              sqlState_ = sqlState.substring(0,5);
          else
              sqlState_ = sqlState;
      }

/**
 *  Get the SQLSTATE value for this exception
 *  @return A string, representing the SQLSTATE. 
 */
      public String getSQLState() {
          return sqlState_ ;
      }
/**
 *  Get the error message associated with this exception
 *
 *  @return A string, representing the error message, including
 *          any substituted text with the additional arguments
 *          in the constructor.
 *
 *  @deprecated Use getMessage(), provided by the base class, instead.
 */
      @Deprecated
      public String getText() {
          return getMessage();
      }

    private String sqlState_;
  }
