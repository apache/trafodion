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
 * File:         test.java
 * Description:  Container for validateMethod, the internal SPJ used in 
 *               CREATE PROCEDURE 
 *
 * Created:      October 2002
 * Language:     Java
 * @deprecated   As of SPJ Result Sets feature implementation, moved to 
 *               org.trafodion.sql.udr.LmUtility
 *
 *
 ******************************************************************************
 */

package org.trafodion.sql.udr;

class test
{
  public static void validateMethod(String className,
                                    String methodName,
                                    String externalPath,
                                    String signature,
                                    int numParam,
                                    String optionalSig)
  throws Exception
  {
    LmUtility lmUtil;
    LmClassLoader lmcl;
    Class targetClass;

    //
    // Load the class first using LmClassLoader
    //
    try
    {  
      lmUtil = new LmUtility();

      lmcl = lmUtil.createClassLoader(externalPath, 0);

      lmcl.removeCpURLs();
      targetClass = lmcl.findClass(className);
      lmcl.addCpURLs();
    }
    catch (ClassNotFoundException cnfe)
    {
      throw new MethodValidationFailedException("Class not found: " +
                                                cnfe.getMessage(),
                                                methodName,
                                                signature,
                                                className);

    }
    catch (NoClassDefFoundError cdnfe)
    {
      throw new MethodValidationFailedException("No class definition found: " +
                                                cdnfe.getMessage(),
                                                methodName,
                                                signature,
                                                className);
    }
    catch (Exception e)
    {
      throw new MethodValidationFailedException(e.toString(),
                                                methodName,
                                                signature,
                                                className);
    }

    //
    // Check if the method exists
    //
    lmUtil.verifyMethodSignature(targetClass, methodName, signature, numParam);
  }
}

