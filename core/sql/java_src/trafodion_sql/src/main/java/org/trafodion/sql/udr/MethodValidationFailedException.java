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
/* -*-Java-*-
 ******************************************************************************
 *
 * File:         MethodValidationFailedException.java
 * Description:  
 *              
 *
 * Created:      January 2003
 * Language:     Java
 *
 *
 ******************************************************************************
 */

package org.trafodion.sql.udr;

/**
 * MethodValidationFailedException is an extention of Exception class 
 * This stores the method name method signature(in java
 * compressed format) and the classname
 *
 **/
class MethodValidationFailedException extends Exception
{
  //  Used for the Serializable base class
  private static final long serialVersionUID = 1L;

  private String methodName_;
  private String methodSignature_;
  private String className_;

  /**
   * Constructor:
   * @param msg  exception message
   * @param methodName  method name
   * @param methodSignature  method signature
   * @param className  class name
   **/
  public MethodValidationFailedException(String msg,
                         String methodName,
                         String methodSignature,
                         String className)
  { 
    super(msg); 

    methodName_ = methodName;
    methodSignature_ = methodSignature;
    className_ = className;
  }

  /**
   *  Accessor for Method Name
   *  @return method name
   *
   **/
  public String getMethodName()
  {
    return methodName_;
  }

  /**
   *  Accessor for Method Signature
   *  @return method signature
   *
   **/
  public String getMethodSignature()
  {
    return methodSignature_;
  }

  /**
   *  Accessor for Class Name
   *  @return class name
   *
   **/
  public String getClassName()
  {
    return className_;
  }
}
