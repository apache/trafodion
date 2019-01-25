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
 * File:         LmUtility.java
 * Description:  Language Manager's Java Utility Class
 *
 * Created:      08/24/2001
 * Language:     Java
 *
 *
 ******************************************************************************
 */
package org.trafodion.sql.udr;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.io.*;
import java.net.URL;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.sql.ResultSet;
import java.lang.reflect.Field;
import java.lang.Class;
@SuppressWarnings("rawtypes")

/**
 * Utility class of Language Manager for Java.
 *
 **/
public class LmUtility {

    //
    // We allow some LM aspects of LM behavior to be controlled via
    // Java system properties:
    //
    // - How much of the Java heap to use for loaded classes before
    //   attempting to unload some classes that are no longer
    //   referenced. 
    //
    // - Whether or not to return errors when the class cache limit is
    // - reached.
    //
    static int classCacheSizeKB_ = 32 * 1024; // 32 megabytes
    static int classCacheEnforceLimit_ = 0;   // 0 means NO, 1 means YES

    static final int MULTIPLE_COMPATIBLE_METHODS  = 11230;
    static final int METHOD_NOT_PUBLIC	          = 11231;
    static final int METHOD_NOT_STATIC	          = 11232;
    static final int METHOD_NOT_VOID	          = 11233;
    static final int METHOD_NOT_FOUND	          = 11234;
    static final int CLASS_NOT_FOUND              = 11205;
    static final int CLASS_DEF_NOT_FOUND          = 11206;
    static final int CLASS_LOAD_ERROR             = 11201;
    static final int RS_INFO_ERROR                = 11235;
    static final int NO_COMPATIBLE_METHODS        = 11239;

    static final String SQLMXRS_CLASS = "org.apache.trafodion.jdbc.t2.SQLMXResultSet";
    static final String RSINFO_CLASS = "org.apache.trafodion.jdbc.t2.ResultSetInfo";
    static final String GETRSINFO_METHOD = "getResultSetInfo";
    static final String GETVERSION_METHOD = "getVersion";

    static Class rsT2Cls_ = null;
    static Class rsInfoCls_ = null;
    static Method rsInfoMID_ = null;
    static Field[] rsInfoFlds_ = null;
    static Method getVersionMID_ = null;

    // static variables related to result set and statement interfaces
    static Class rsCls_ = null;             // java.sql.ResultSet
    static Method getTypeMID_ = null;       // ResultSet.getType()

    // static variables related to T4 driver
    static Class rsT4Cls_ = null;            // org.trafodion.jdbc.t4.TrafT4ResultSet
    static Method getConnectionMID_ = null;  // TrafT4ResultSet.getConnection()
    static Method getProxySyntaxMID_ = null; // TrafT4ResultSet.getProxySyntax()
    static Method getSequenceNumberMID_ = null;
                                       // TrafT4ResultSet.getSequenceNumber()
    static Method isClosedMID_ = null; // TrafT4ResultSet.isClosed()
    static Method hasLOBColumnsMID_ = null; // TrafT4ResultSet.hasLOBColumns()

    static final int JDBC_UNKNOWN_CONNECTION = -1;
    static final int JDBC_TYPE4_CONNECTION = 1;
    static final int JDBC_TYPE2_CONNECTION = 2;

    public static void init()
        throws Exception
    {
        //
        // All we are currently doing here is looking at some Java
        // system properties, then using their values to configure the
        // Java environment and also to initialize some static
        // variables in this class. Currently none of this behavior is
        // externalized so we are not worrying if our error reporting
        // is not as complete as it could be.
        //
        String msg;

        // Container manager class cache size
        String s = System.getProperty("sqlmx.udr.class.cache.kbytes");
        if (s != null && s.length() > 0)
        {
          try
          {
              classCacheSizeKB_ = Integer.valueOf(s).intValue();
          }
          catch (Throwable t)
          {
              msg = "An error occurred trying to set the "
                  + "Language Manager class cache size.";
              throw new Exception(msg, t);
          }
        }
        
        // Container manager flag to enforce cache limits
        s = System.getProperty("sqlmx.udr.class.cache.enforce");
        if (s != null && s.length() > 0)
        {
            try
            {
                classCacheEnforceLimit_ = Integer.valueOf(s).intValue();
            }
            catch (Throwable t)
            {
                msg = "An error occurred trying to set the "
                    + "Language Manager class cache \"enforceLimits\" flag.";
                throw new Exception(msg, t);
            }
        }
        
        // Standard input stream
        s = System.getProperty("sqlmx.udr.infile");
        if (s != null && s.length() > 0)
        {
            try
            {
                System.setIn(new FileInputStream(s));
            }
            catch (Throwable t)
            {
                msg = "An error occurred trying to set the "
                    + "standard input stream.";
                throw new Exception(msg, t);
            }
        }
        
        // Standard output stream
        s = System.getProperty("sqlmx.udr.outfile");
        if (s != null && s.length() > 0)
        {
            try
            {
                System.setOut(new PrintStream(new FileOutputStream(s), true));
            }
            catch (Throwable t)
            {
                msg = "An error occurred trying to set the "
                    + "standard output stream.";
                throw new Exception(msg, t);
            }
        }
        
        // Standard error stream
        s = System.getProperty("sqlmx.udr.errfile");
        if (s != null && s.length() > 0)
        {
            try
            {
                System.setErr(new PrintStream(new FileOutputStream(s), true));
            }
            catch (Throwable t)
            {
                msg = "An error occurred trying to set the "
                    + "standard error stream.";
                throw new Exception(msg, t);
            }
        }

        // Nonblocking JDBC. We set the nonblocking property to OFF if
        // the user did not already specify either sqlmx_nowait or
        // jdbcmx.sqlmx_nowait.
        //
        // Notes
        // - As of release 2 it is required to set this property
        //   BEFORE the JDBC driver is loaded.
        // - We do this because MXUDR is currently single-threaded. We
        //   probably want to remove this code if and when MXUDR becomes
        //   multi-threaded.
        s = System.getProperty("sqlmx_nowait");
        if (s == null || s.length() == 0)
        {
            s = System.getProperty("jdbcmx.sqlmx_nowait");
            if (s == null || s.length() == 0)
            {
                try
                {
                    System.setProperty("sqlmx_nowait", "OFF");
                    System.setProperty("jdbcmx.sqlmx_nowait", "OFF");
                }
                catch (Throwable t)
                {
                    msg = "An error occurred trying to set the "
                        + "sqlmx_nowait system property.";
                    throw new Exception(msg, t);
                }
            }
        }
    }
    
    public static boolean isWindows()
    {
      String os = System.getProperty("os.name");
      os = os.toUpperCase();
      if (os.startsWith("WINDOWS"))
      {
        return true;
      }
      return false;
    }


    /**
     * Builds an array of Class type for the given signature.
     * This method parses the signature and returns the Class object
     * array. If we hit '[' then set outParam and revisit the same parameter.
     * @param   signature           java signature
     * @param   numParam            number of parameters in the signature
     * @param   returnAllParams     flag to specify return of all params in signature
     * @return  Array of Class objects represting the parameter types 
     *           in the signature
     * @throws  ClassNotFoundException
     *
     **/
    private static Class[] getParamArray(String signature, 
                                         int numParam, 
                                         boolean returnAllParams)
    throws ClassNotFoundException
    {
        ArrayList<Class> param = new ArrayList<Class>();
        boolean outMode = false;

        for(int index=0; index < signature.length(); index++)
        {
            switch (signature.charAt(index))
            {
                case ')' : 
                break;            
                
                case '[' :
                outMode = true;  // This param has OUT/INOUT mode
                break;

                case 'V' :
                break;

                case 'S' :
                if (outMode == true)
                    param.add((new short[1]).getClass());
                else
                    param.add(Short.TYPE);
                outMode = false;
                break;

                case 'I' :
                if (outMode == true)
                    param.add((new int[1]).getClass());
                else
                    param.add(Integer.TYPE);
                outMode = false;
                break;

                case 'J' :
                if (outMode == true)
                    param.add((new long[1]).getClass());
                else
                    param.add(Long.TYPE);
                outMode = false;
                break;

                case 'F' :
                if (outMode == true)
                    param.add((new float[1]).getClass());
                else
                    param.add(Float.TYPE);
                outMode = false;
                break;

                case 'D' :
                if (outMode == true)
                    param.add((new double[1]).getClass());
                else
                    param.add(Double.TYPE);
                outMode = false;
                break;

                case 'L' :
                String substr = signature.substring(index, signature.indexOf(";", index));
                if (substr.equals("Ljava/lang/String"))
                {
                    if (outMode == true)
                        param.add((new java.lang.String[1]).getClass());
                    else
                        param.add(Class.forName("java.lang.String"));
                    index += 17;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/math/BigDecimal"))
                {
                    if (outMode == true)
                        param.add((new java.math.BigDecimal[1]).getClass());
                    else
                        param.add(Class.forName("java.math.BigDecimal"));
                    index += 21;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/sql/Date"))
                {
                    if (outMode == true)
                        param.add((new java.sql.Date[1]).getClass());
                    else
                        param.add(Class.forName("java.sql.Date"));
                    index += 14;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/sql/Timestamp"))
                {
                    if (outMode == true)
                        param.add((new java.sql.Timestamp[1]).getClass());
                    else
                        param.add(Class.forName("java.sql.Timestamp"));
                    index += 19;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/sql/Time"))
                {
                    if (outMode == true)
                        param.add((new java.sql.Time[1]).getClass());
                    else
                        param.add(Class.forName("java.sql.Time"));
                    index += 14;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/sql/ResultSet"))
                {
                    if (outMode == true)
                        param.add((new java.sql.ResultSet[1]).getClass());
                    else
                        param.add(Class.forName("java.sql.ResultSet"));
                    index += 19;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/lang/Integer"))
                {
                    if (outMode == true)
                        param.add((new java.lang.Integer[1]).getClass());
                    else
                        param.add(Class.forName("java.lang.Integer"));
                    index += 18;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/lang/Long"))
                {
                    if (outMode == true)
                        param.add((new java.lang.Long[1]).getClass());
                    else
                        param.add(Class.forName("java.lang.Long"));
                    index += 15;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/lang/Float"))
                {
                    if (outMode == true)
                        param.add((new java.lang.Float[1]).getClass());
                    else
                        param.add(Class.forName("java.lang.Float"));
                    index += 16;
                    outMode = false;
                    break;
                }

                if (substr.equals("Ljava/lang/Double"))
                {
                    if (outMode == true)
                        param.add((new java.lang.Double[1]).getClass());
                    else
                        param.add(Class.forName("java.lang.Double"));
                    index += 17;
                    outMode = false;
                    break;
                }
                outMode = false;
                break;
            }  // switch ends
        }  // for loop ends

        Class[] retParam;
        int paramCount = param.size();

        if(!returnAllParams)
            paramCount = numParam;
 
        retParam = new Class[paramCount];
        for(int i=0; i < paramCount; i++)
            retParam[i] = param.get(i);

        return retParam;

    }   // method getParamArray() ends


    /**
     * Check for a given method existence in the given class.
     * Calls reflection method getMethod() on the class object and
     * checks for return type and the required modifiers.
     * @param  targetClass  class
     * @param  methodName   method name to check for
     * @param  signature    signature of the above method
     * @param  numParam     number of parameters in the signature
     * @throws NoSuchMethodException
     *
     **/
    public static void verifyMethodSignature(Class targetClass,
                                             String methodName,
                                             String signature,
                                             int numParam)
     throws Exception

    {

      try
         {
            Class [] paramArray;
            boolean optSig = false;
            paramArray = getParamArray(signature, numParam, optSig);
            @SuppressWarnings("unchecked")
			Method m = targetClass.getMethod(methodName, paramArray);

            // Check if return type is void. getMethod() doesn't consider
            // the return type. Currently only return type 'void' is supported.
            Class retType = m.getReturnType();
            if (retType.getName().equals("void") == false)
               throw new MethodValidationFailedException(
                                          "No such void method",
                                           methodName,
                                           signature,
                                           targetClass.getName());
    
            // Get the modifiers of the method and check for static
            // modifier
            int mod = m.getModifiers();

            if (Modifier.isStatic(mod) == false)
               throw new MethodValidationFailedException(
                                        "No such static method",
                                        methodName,
                                        signature,
                                        targetClass.getName());
         }
      catch (NoSuchMethodException nsme) 
         {
            throw new MethodValidationFailedException(
                                        "Method not found or not public",
                                        methodName,
                                        signature,
                                        targetClass.getName());
         }

    }  // Method verifyMethodSignature() ends

    /**
     * Retrieves, using getDeclaredMethods(), an array of Method objects 
     * reflecting all the methods declared by the class or interface represented
     * by Class tc. This includes public, protected, default (package) access, and 
     * private methods, but excludes inherited methods. The elements in the
     * array returned are not sorted and are not in any particular order.
     *             
     * Method getDeclaredMethods returns an array of length 0 if the class
     * or interface declares no methods, or if this Class object represents 
     * a primitive type, an array class, or void. If no methods are found
     * get the super class and retrieve any methods.
     *
     * Call MatchMethods() to build a list of candidate methods
     * If the returned list is empty return "method not found" error.
     * If the returned list has > 0 elements then return "overloaded" error.
     * If the returned list contains 1 element then examine the method
     * modifiers. 
     * If modifiers are okay pack the signature and return to caller.
     *     
     * @param  targetClass   class
     * @param  methodName    method name to check for
     * @param  signature     signature of the above method
     * @param  numParam      number of parameters in the signature
     * @param  maxResultSets value of DYNAMIC RESULT SETS clause
     * @param  optionalSig   Indicates signature from EXTERNAL NAME
     * @param  retSig        container for returned signature
     * @param  errCode       container for returned error code
     * @param  errDetail     container for any returned error detail text  
     * @throws Exception
     *
     **/
    public static void verifyMethodSignature(Class targetClass,
                                             String methodName,                     
                                             String signature,
                                             int numParam,
                                             int maxResultSets,
                                             int optionalSig,
                                             String [] retSig,
                                             int [] errCode,
                                             String [] errDetail)
        throws Exception
    {
        final String VOID = "void";
        boolean optSig = (optionalSig > 0) ? true : false;    
        Class [] parameterTypes = null;
        Class tc = targetClass;
        
        try
        {
            parameterTypes = getParamArray(signature,numParam,optSig);
        }
        catch (Throwable t)
        {
            String msg = "An internal error occurred in the SQL/MX language manager"
                       + "while trying to retrieve the parameter types.";
            throw new Exception(msg, t);
        }

        while (tc != null)
        {
            Method [] methods = tc.getDeclaredMethods(); 
            if (methods.length == 0)
                tc = tc.getSuperclass(); //No methods found - get superclass
            else
            {
                ArrayList list = matchMethods(methodName,methods,parameterTypes,
                                              maxResultSets,optSig);
                if(list.isEmpty())//No candidate methods found
                   tc = tc.getSuperclass();
                else if (list.size() > 1)// Found multiple candidate methods
                {
                    retSig[0] = signature;
                    errCode[0] = MULTIPLE_COMPATIBLE_METHODS;
                    errDetail[0] = "";
                    return;
                }
                else//Found one
                {
                    Method m = (Method)list.get(0);
                    int mod = m.getModifiers();
                    Class retType = m.getReturnType();
                  
                    if(!(Modifier.isPublic(mod)))//Not Public
                    {
                        retSig[0] = createSignature(m);
                        errCode[0] = METHOD_NOT_PUBLIC;
                        errDetail[0] = "";
                    }
                    else if(!(Modifier.isStatic(mod)))//Not Static
                    {
                        retSig[0] = createSignature(m);
                        errCode[0] = METHOD_NOT_STATIC;
                        errDetail[0] = "";
                    }
                    else if(!(retType.getName().equals(VOID)))//Not Void
                    {
                        retSig[0] = createSignature(m);
                        errCode[0] = METHOD_NOT_VOID;
                        errDetail[0] = "";
                    }
                    else// Valid method 
                    {
                        retSig[0] = createSignature(m);
                        errCode[0] = 0;
                        errDetail[0] = "";
                    }
                    return;
                 }//end else
             }//end else
        }//end while

        retSig[0] = signature;
        errDetail[0] = "";

        if(maxResultSets > 0 &&  optSig == false)
          errCode[0] = NO_COMPATIBLE_METHODS;
        else
          errCode[0] = METHOD_NOT_FOUND;
                                
     }  //method VerifyMethodsSignature ends

    /**
     * Matches the given method name and it's parameter types to each candidate 
     * method in a given list of candidate methods. Any matched methods are stored in a 
     * list to be returned to the caller.
     *
     * The criteria for inclusion of a candidate method into the list are:
     *
     * Candidate method name must match the given method name.
     * Candidate method SQL parameter names must match the given methods parameter names.
     *
     * AND
     * 
     * If the user entered an optional signature in CREATE PROCEDURE EXTERNAL NAME
     * and the candidate method does not have any trailing parameters then add this candidate
     * method to the list.
     * 
     * OR
     *
     * If the user specified the parameters in the CREATE PROCEDURE SQL parameters, the 
     * candidate method has trailing parameters and the user specified DYNAMIC RESULT SETS > 0 
     * then add the method to the list.  
     *
     * OR
     *   
     * If the user specified the parameters in the CREATE PROCEDURE SQL parameters, and the 
     * candidate method does not have any trailing parameters then add the method to the list.  
     *
     * 
     * @param  name    Name of the method 
     * @param  methods Array of Method objects
     * @param  spt     Source parameter list
     * @param  mrs     Max result sets value
     * @param  exact   Exact match flag
     * @returns        A list of candidate method objects
     **/
    private static ArrayList<Method> matchMethods(String name,
    		                                     Method[] methods,
    		                                     Class[] spt,
    		                                     int mrs,
    		                                     boolean exact)
    {
        ArrayList<Method> list = new ArrayList<Method>();
        
        for (int i=0; i < methods.length; i++)
        {
            Method m = methods[i];
            Class [] tpt = m.getParameterTypes();
            
            boolean trailingParams = (tpt.length > spt.length);

            if(name.equals(m.getName()))//Match method names
            {
                if(matchParams(spt,tpt))//Match parameter type names
                {
                    if(exact)//Signature was specified in CREATE PROCEDURE EXTERNAL NAME
                    {
                        if(trailingParams)//Method has trailing params 
                        {
                            //Do nothing - For exact signatures we're not
                            //interested in methods containing trailing params.
                        }
                        else
                        {
                           //We're not concerned with the value of DYNAMIC RESULT SETS
                           //because the signature is an exact match.
                           list.add(m);
                        }
                    }
                    else//Signature was specified in CREATE PROCEDURE SQL parameters
                    {
                        if(trailingParams)//Method has trailing params
                        {
                            if(mrs > 0)//We're interested in methods containing trailing result sets
                            {
                                //Trailing params must all be of type java.sql.ResultSet
                                if(matchTrailingParams(spt.length,tpt.length,tpt)) 
                                   list.add(m);
                            }
                            else
                            {
                                //Do nothing - We're not interested in methods that
                                //contain trailing result sets
                            }
                        }
                        else//Method does not have trailing params
                        {
                            if(mrs > 0)
                            {
                                //Whenever DNR > 0 a java.sql.ResultSet must
                                //be present in the actual Java method signature. 
                            }
                            else
                                list.add(m);
                        }
                    }//endIf exact
                }//endIf match params
            }//end method names match
        }//end for loop

        return list;

    }  //method matchMethods ends

    /**
     * Compares parameter type names of the source array with that
     * of the target array. If the length of the target array 
     * is less than the source array then matching is not performed. 
     * Matching begins with index 0 of each array for the 
     * length of the source array. If all the names of the source 
     * array match those of the target array then return true 
     * otherwise return false. 
     * @param  spt     Array of Method parameter objects 
     * @param  tpt     Array of Method parameter objects
     * @returns        True if names match otherwise false
     **/
    private static boolean matchParams(Class[] spt, 
                                       Class[] tpt)
    {
        if(tpt.length < spt.length) //not enough params in target
            return false;

        for(int i=0; i < spt.length; i++)
        {
            if(!(spt[i].getName()).equals(tpt[i].getName()))
                return false;//SQL param name mismatch
        }
        return true;
    }  //method matchParams ends

   /**
     * Compares remaining parameter type names of the target array with the
     * string "[Ljava.sql.ResultSet". If the length of the target array 
     * is less than or equal to the source array then matching is not performed.
     * Matching begins sptLen deep into the target array. 
     * If all the names match then return true otherwise false.
     * @param  sptLen     Length of the source parameter array 
     * @param  tptLen     Length of the target parameter array
     * @param  tpt        The target parameter array
     * @returns           True if trailing params are of type ResultSet 
     *                    otherwise false
     **/

    private static boolean matchTrailingParams(int sptLen,
                                               int tptLen,
                                               Class[] tpt)
    {
        final String RESULT_SET = "[Ljava.sql.ResultSet;";

        if(tptLen <= sptLen)//No trailing params
            return false;

        for(int i=sptLen; i < tptLen; i++)
        {
            if (!(tpt[i].getName()).equals(RESULT_SET))
                return false;//Trailing param not type ResultSet
        }
        return true;
    }  //method matchTrailingParams ends

   /**
     * Extracts parameters from a given method object and formats
     * the string names of the methods parameters into a compatible signature
     * @param  m          Method object
     * @returns           The created signature
     **/
    private static String createSignature(Method m) throws Exception
    {
        StringBuffer sb = new StringBuffer();
        Class [] params = m.getParameterTypes();
        sb.append("(");
        for(int i=0; i < params.length; i++)
        {
            String str1 = params[i].getName();
	    if(str1.equals("short")) // IN TYPES
                sb.append("S"); 
            else if(str1.equals("int"))
                sb.append("I");
            else if(str1.equals("long"))
                sb.append("J");
            else if(str1.equals("float"))
                sb.append("F");
            else if(str1.equals("double"))
                sb.append("D");
            else if(str1.equals("java.lang.String"))  
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("java.math.BigDecimal"))
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("java.sql.Date"))
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("java.sql.Timestamp"))
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("java.sql.Time"))
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("java.lang.Integer"))
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("java.lang.Long"))
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("java.lang.Float"))
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("java.lang.Double"))
                sb.append("L" + str1.replace('.','/') + ';');
            else if(str1.equals("[S")) // OUT TYPES
                sb.append(str1);
            else if(str1.equals("[I"))
                sb.append(str1);
            else if(str1.equals("[J"))
                sb.append(str1);
            else if(str1.equals("[F"))
                sb.append(str1);
            else if(str1.equals("[D"))
                sb.append(str1);
            else if(str1.equals("[Ljava.lang.String;"))  
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.math.BigDecimal;"))
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.sql.Date;"))
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.sql.Timestamp;"))
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.sql.Time;"))
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.lang.Integer;"))
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.lang.Long;"))
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.lang.Float;"))
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.lang.Double;"))
                sb.append(str1.replace('.','/'));
            else if(str1.equals("[Ljava.sql.ResultSet;"))
                sb.append(str1.replace('.','/'));
            else
            {
                String msg = "An internal error occurred in the SQL/MX language manager. "
                           + "An unknown parameter type name " + "'" + str1 + "' " 
                           + "was encountered while trying to create the packed signature.";
                throw new Exception(msg);
            }
        }
        sb.append(")V");
        return sb.toString();

    }//end createSignature    

    public static void validateMethod(String className,
                                      String methodName,
                                      String externalPath,
                                      String signature,
                                      int numParam,
                                      int maxResultSets,
                                      int optionalSig,
                                      String [] retSig,
                                      int [] errCode,
                                      String [] errDetail)
    throws Exception
    {
      LmClassLoader lmcl;
      Class targetClass;

      // Load the class first using LmClassLoader
      try
      {  
        lmcl = createClassLoader(externalPath, 0);

        // use findResourceInternal first, to validate that
        // the class is actually found in the container
        final String dname = className.replace('.', '/') + ".class";
        File f = lmcl.findResourceInternal(dname);

        if (f != null)
          // then load the class the regular way (note that if the
          // class is also found earlier in the CLASSPATH, we don't
          // load it from the container, but from the jar or class
          // found earlier)
          targetClass = lmcl.loadClass(className);
        else
          {
            retSig[0] = "";
            errCode[0] = CLASS_NOT_FOUND;
            errDetail[0] = "";
            return;
          }
      }
      catch (ClassNotFoundException cnfe)
      {
        retSig[0] = "";
        errCode[0] = CLASS_NOT_FOUND;
        errDetail[0] = "";
        return;
      }
      catch (NoClassDefFoundError cdnfe)
      {
        retSig[0] = "";
        errCode[0] = CLASS_DEF_NOT_FOUND;
        errDetail[0] = "";
        return;
      }
      catch (Throwable t)
      {
            String msg = "An internal error occurred in the SQL/MX language manager. "
                       + "An unexpected exception was caught while trying to load the Java method "
                       + "'" + methodName + signature + "'"
                       + " in Java class " + "'" + className + "'";
            throw new Exception(msg, t);
      }

      verifyMethodSignature(targetClass, methodName, signature,
                            numParam, maxResultSets, optionalSig,
                            retSig, errCode, errDetail);

    }  // Method validateMethod() ends

    /**
     * Returns the generic user id for isolated UDRs. This generic
     * id is used to store files available to all UDRs and it is
     * also used as a sandbox for systems that don't have user ids
     * for isolated UDRs (note that such user ids are not implemented
     * as of March 2017, when this is written).
     *
     * @return: generic isolated UDR user id
     *
     **/
    public static String getPublicUserId()
    {
        return "public";
    }

    /**
     * Returns the directory in which all UDR-related files should reside
     *
     * @return: Root directory for UDR-related files
     *
     **/
    public static Path getSandboxRoot()
    {
        return Paths.get(System.getenv("TRAF_VAR"), "udr");
    }

    /**
     * Returns the directory in which all UDR-related files for a
     * particular isolated UDR user id should reside
     *
     * @param userid user id for which we return the sandbox root. If empty
     *               or null, returns the sandbox root for the public user id.
     * @return: Root directory for UDR-related files for user "user"
     *
     **/
    public static Path getSandboxRootForUser(String userid)
    {
        if (userid == null || userid.length() == 0)
            return getSandboxRootForUser(getPublicUserId());

        return Paths.get(System.getenv("TRAF_VAR"), "udr", userid);
    }

    /**
     * Returns the directory in which all external libraries for a
     * particular isolated UDR user id should reside
     *
     * @param userid user id for which we return the libraries dir. If empty
     *               or null, returns the sandbox root for the public user id.
     * @return: external libs dir for UDR-related files for user "user"
     *
     **/
    public static Path getExternalLibsDirForUser(String userid)
    {
        if (userid == null || userid.length() == 0)
            return getExternalLibsDirForUser(getPublicUserId());

        return Paths.get(System.getenv("TRAF_VAR"), "udr", userid, "external_libs");
    }

    static class JarFilter implements FilenameFilter {
        JarFilter()
        {}

        @Override
        public boolean accept(File dir, String name) {
            return name.endsWith(".jar");
        }
    }

    /**
     * Creates an object of LmClassLoader class.
     * @param  path   external path for CL
     * @param  debug  debug flag (currently ignored)
     * @return: Object of LmClassLoader type
     *
     **/
    public static LmClassLoader createClassLoader(String path, int debug)
        throws Exception
    {
        // We pass a list of URLs to search to the class loader:
        // - The first URL is the path provided as an argument, that
        //   is the actual container name
        // - The second URL is the external libs dir, returned by
        //   getExternalLibsDirForUser()
        // - Following are jars that are available to the UDR,
        //   stored in the external libs dir

        Path extraJarPath = getExternalLibsDirForUser(getPublicUserId());
        File extraJarDir = extraJarPath.toFile();
        final int numStdURLs = 2; // 2 URLs that are always provided
        int numExtraURLs = 0;     // URLs of the jar files in the libs dir
        File[] extraJarFiles = null;
        URL[] extraJarURLs = null;

        if (extraJarDir.isDirectory())
            {
                extraJarFiles = extraJarDir.listFiles(new JarFilter());
                numExtraURLs += extraJarFiles.length;
            }

        extraJarURLs = new URL[numStdURLs+numExtraURLs];

        // add the two standard URLs
        extraJarURLs[0] = new File(path).toURI().toURL();
        extraJarURLs[1] = extraJarDir.toURI().toURL();
        // add any jars that we collected above
        for (int f=0; f<numExtraURLs; f++)
            extraJarURLs[f+numStdURLs] = extraJarFiles[f].toURI().toURL();

        LmClassLoader lmcl = new LmClassLoader(extraJarURLs);
        return lmcl;
    }


    /**
     * Starts Security Manager class in JVM. Uses the given policy file.
     * @param policyFile  policy file to use
     *
     **/
    /*
     * THIS METHOD WAS USED IN R1.8 BUT RETIRED IN R2
     *
    public static void enableSecurity(String policyFile)
    {
      System.setProperty("java.security.policy", policyFile);
      SecurityManager sm = new SecurityManager();
      System.setSecurityManager(sm);
      return;
    }
    */

    /**
     * Goes through the given exception and its children that are accessed
     * by getException()/getNextException() methods.
     * @param  ex instance of Throwable class
     * @return an array of exception message strings
     *
     **/
    /*
     * THIS METHOD WAS USED IN R1.8 BUT RETIRED IN R2
     *
    public static Object[] getExceptionMsgs(Throwable th)
    {
      Vector retVec = new Vector();
      String met;

      // Warning: This method does not return any exceptions. If this
      // method is changed to return any exception, corresponding C++
      // code needs to be updated to check the exception.

      do
      {
        Class c = th.getClass();

        // Store the message string.
        retVec.add(th.toString());

        if (c.getName().startsWith("java.sql"))
          met = "getNextException";
	else
	  met = "getException";

	th = getNextEx(th, met);

        if (th == null)
	  return retVec.toArray();

      } while (true);
    }
    */

    /**
     * Get the next exception in the chain of the exceptions
     * @param  th a throwable object
     * @param  method method to use to get next exception
     * @return Next exception in the chain
     *
     **/
    /*
     * THIS METHOD WAS USED IN R1.8 BUT RETIRED IN R2
     *
    private static Throwable getNextEx(Throwable th, String method)
    {
      try
      {
        Method m = th.getClass().getMethod(method, null);

        return (Throwable) m.invoke(th, null);

      } catch (Exception e)
          {
            return null;
          }
    }
    */

    public static void utils(String command, String[] result)
        throws Exception
    {
        if (command == null)
        {
            return;
        }

        command = command.trim();
        if (command.length() == 0)
        {
            return;
        }

        String upper = command.toUpperCase();

        if (upper.startsWith("THROW ") ||
            upper.compareTo("THROW") == 0)
        {
          if (upper.compareTo("THROW") == 0)
          {
            throw new Exception("*** No message text specified ***");
          }
          else
          {
            int firstChar = command.indexOf(' ');
            String msgText = command.substring(firstChar).trim();
            throw new Exception(msgText);
          }
        }
        else if (upper.startsWith("GETPROP ") ||
                 upper.startsWith("GETPROPERTY "))
        {
            int firstChar = command.indexOf(' ');
            String name = command.substring(firstChar);
            name = name.trim();
            result[0] = System.getProperty(name);
        }
        else if (upper.startsWith("SETPROP ") ||
                 upper.startsWith("SETPROPERTY "))
        {
            int firstSpace = command.indexOf(' ');
            String nameAndVal = command.substring(firstSpace);
            nameAndVal = nameAndVal.trim();

            firstSpace = nameAndVal.indexOf(' ');
            String name = nameAndVal.substring(0, firstSpace);
            String val = nameAndVal.substring(firstSpace);
            name = name.trim();
            val = val.trim();

            System.setProperty(name, val);
            result[0] = val;
        }
        else if (upper.compareTo("SHOWPROP") == 0 ||
                 upper.compareTo("SHOWPROPERTIES") == 0)
        {
            java.util.Properties p = System.getProperties();
            java.util.Enumeration e = p.propertyNames();
            while (e.hasMoreElements())
            {
                String s = e.nextElement().toString();
                System.err.println(s + "=" + p.getProperty(s));
            }
        }
        else if (upper.startsWith("SLEEP "))
        {
            int firstChar = command.indexOf(' ');
            String delay = command.substring(firstChar).trim();
            Thread.sleep(Integer.parseInt(delay) * 1000);
            result[0] = delay;
        }
        else if (upper.startsWith("EXEC "))
        {
            int firstChar = command.indexOf(' ');
            String cmd = command.substring(firstChar).trim();
            Runtime.getRuntime().exec(cmd).waitFor();
            result[0] = "OK";
        }
        else if (upper.startsWith("JDBCEXEC "))
        {
            int firstChar = command.indexOf(' ');
            String cmd = command.substring(firstChar).trim();

            String url = "jdbc:default:connection";
            java.sql.Connection conn =
                java.sql.DriverManager.getConnection(url);

            boolean b = conn.createStatement().execute(cmd);

            if (b)
            {
                result[0] = "TRUE";
            }
            else
            {
                result[0] = "FALSE";
            }
        }
        else if (upper.startsWith("VERIFY "))
        {
            String[] cmd = command.split("\\s");
            //disregard cmd[0]
            String className = cmd[1];
            String methodName = cmd[2];
            String externalPath = cmd[3];
            String signature = cmd[4];
            if(signature.equals("NULL"))
              signature = null;

            String nps = cmd[5];
            Integer np = new Integer(nps);
            int numParams = np.intValue();

            String mrss = cmd[6];
            Integer mrs = new Integer(mrss);
            int maxResultSets = mrs.intValue();

            String oss = cmd[7];
            Integer optSig = new Integer(oss); 
            int optionalSig = optSig.intValue();

            try
            {  
              String [] retSig = new String[1];
              int [] errCode = new int[1];
              String [] errDetail = new String[1];

              retSig[0] = "";
              errCode[0] = 0;
              errDetail[0] = "";
        
              validateMethod(className, methodName, externalPath, signature, 
                             numParams, maxResultSets, optionalSig,
                             retSig, errCode, errDetail);  
              if (errCode[0] != 0)
                result[0] = "errCode=" + errCode[0] + " errDetail=" + errDetail[0]; 
              else             
                result[0] = retSig[0];
            }
            catch (MethodValidationFailedException mvfe)
            {
              result[0] = mvfe.getMessage() + " " + mvfe.getMethodName();
            }
        }
        else
        {
            nativeUtils(command, result);
        }
    }

    // A testware-only method to return one result set
    public static void rsUtils(String command, String[] result,
                               java.sql.ResultSet[] rs)
      throws Exception
    {
      java.sql.Connection connForRSUtils = null;

      if (!isWindows())
      {
        connForRSUtils = 
            java.sql.DriverManager.getConnection("jdbc:default:connection");
        java.sql.PreparedStatement s =
          connForRSUtils.prepareStatement(command);
        rs[0] = s.executeQuery();
      }
      else
      {
        utils("PutEnv RS_SQL_STMT_1=" + command, result);
      }
      result[0] = "OK";
    }

    /**
     * Loads the required Type 4 JDBC/MX classes for result sets processing.
     *
     * @return: Boolean value indicating if the Type 4 JDBC/MX driver
     *          supports SPJ RS.
     *
     **/
    @SuppressWarnings({"unchecked", "cast"})
	public static boolean initT4RS()
       throws Exception
     {
       try
       {
         // Load java.sql.ResultSet class
         rsCls_ = Class.forName("java.sql.ResultSet");
	 getTypeMID_ = rsCls_.getMethod("getType", null);

         // Load org.trafodion.jdbc.t4.TrafT4ResultSet class
         rsT4Cls_ = Class.forName("org.trafodion.jdbc.t4.TrafT4ResultSet");
	 getConnectionMID_ = rsT4Cls_.getMethod("getConnection", null);
	 getProxySyntaxMID_ = rsT4Cls_.getMethod("getProxySyntax", null);
	 getSequenceNumberMID_ = rsT4Cls_.getMethod("getSequenceNumber", null);
	 isClosedMID_ = rsT4Cls_.getMethod("isClosed", null);
	 hasLOBColumnsMID_ = rsT4Cls_.getMethod("hasLOBColumns", null);
       }
       catch(ClassNotFoundException cnfe)
       {
         return false;	// SPJ result set not supported in JDBC/MX
       }
       catch( NoSuchMethodException nsme )
       {
         return false;	// SPJ result set not supported in JDBC/MX
       }
       catch (Throwable t)
       {
         throw new Exception(t);
       }

       return true;
     }

    /**
     * Loads the required Type 2 JDBC/MX classes for result sets processing.
     *
     * @param  rsInfoVer - The linked in JDBC/MX driver's 'ResultSetInfo' 
     *                     class version number.
     * @return: Boolean value indicating if the linked in JDBC/MX driver
     *          supports SPJ RS.
     *
     **/
    @SuppressWarnings("unchecked")
	public static boolean initRS( long[] rsInfoVer )
       throws Exception
     {

       // Load org.apache.trafodion.jdbc.t2.SQLMXResultSet class
       // Get the method ID of SQLMXResultSet::getResultSetInfo()
       try {
         rsT2Cls_ = Class.forName( SQLMXRS_CLASS );
         rsInfoMID_ = rsT2Cls_.getMethod( GETRSINFO_METHOD, null );
       }
       catch(ClassNotFoundException cnfe)
       {
         return false;	// SPJ result set not supported in JDBC/MX
       }
       catch( NoSuchMethodException nsme )
       {
         return false;	// SPJ result set not supported in JDBC/MX
       }
       catch (Throwable t)
       {
         throw new Exception(t);
       }

       // Load org.apache.trafodion.jdbc.t2.ResultSetInfo class
       // Get the public fields in the above class
       try {
         rsInfoCls_ = Class.forName( RSINFO_CLASS );
         rsInfoFlds_ = rsInfoCls_.getFields();
         getVersionMID_ = rsInfoCls_.getMethod( GETVERSION_METHOD, null );
       }
       catch(ClassNotFoundException cnfe)
       {
         return false;	// SPJ result set not supported in JDBC/MX
       }
       catch (Throwable t)
       {
         throw new Exception(t);
       }

       try {
         // Get the version of ResultSetInfo class
         Long ver = (Long) getVersionMID_.invoke( null, null );
         rsInfoVer[0] = ver.longValue();
       }
       catch( Throwable t )
       {
         throw new Exception(t);
       }

       return true;
     }

     public static int getConnectionType(Object rsObj)
     {
       // Is rsObj using T4 result set object?
       if(rsT4Cls_ != null && rsT4Cls_.isInstance(rsObj))
         return JDBC_TYPE4_CONNECTION;

       // Is rsObj using T2 result set object?
       if(rsT2Cls_ != null && rsT2Cls_.isInstance(rsObj))
         return JDBC_TYPE2_CONNECTION;

       // Unknown
       return JDBC_UNKNOWN_CONNECTION;
     }

     public static void getT4RSInfo(Object rsObj,
		                    long[] rsCounter,
				    boolean[] closeStatus,
				    int[] rsType,
				    boolean[] hasLOBColumns,
				    String[] proxySyntax,
				    Object[] conn)
     throws Throwable
     {
       rsCounter[0] = 0;
       closeStatus[0] = false;
       rsType[0] = -1;
       proxySyntax[0] = null;
       conn[0] = null;

       // call TrafT4ResultSet.isClosed()
       closeStatus[0] =
         ((Boolean) (isClosedMID_.invoke(rsObj, null))).booleanValue();

       if (closeStatus[0])
         return;  // RS is closed. No need for further processing

       // call TrafT4ResultSet.getConnection();
       conn[0] = getConnectionMID_.invoke(rsObj, null);

       // call TrafT4ResultSet.getSequenceNumber()
       rsCounter[0] =
         ((Long) (getSequenceNumberMID_.invoke(rsObj, null))).longValue();

       // call TrafT4ResultSet.hasLOBColumns()
       hasLOBColumns[0] =
         ((Boolean) (hasLOBColumnsMID_.invoke(rsObj, null))).booleanValue();

       // call ResultSet.getType()
       int type = ((Integer)(getTypeMID_.invoke(rsObj, null))).intValue();
       if (type == ResultSet.TYPE_FORWARD_ONLY)
         rsType[0] = 0;
       else if (type == ResultSet.TYPE_SCROLL_INSENSITIVE)
         rsType[0] = 1;
       else if (type == ResultSet.TYPE_SCROLL_SENSITIVE)
         rsType[0] = 2;

       // call TrafT4ResultSet.getProxySyntax()
       proxySyntax[0] = (String) getProxySyntaxMID_.invoke(rsObj, null);
     }

    /**
     * Method to get data pertaining to a specific result set object. This
     * method calls the getResultSetInfo() method provided by the JDBC/MX
     * driver. The above method call returns a
     * org.apache.trafodion.jdbc.t2.ResultSetInfo object whose structure is as below.

     * class org.apache.trafodion.jdbc.t2.ResultSetInfo
     * {
     *   public:
     *	  boolean LOBDataDetected;
     *	  Connection connection;
     *    int ctxHandle;
     *    int stmtID;
     *    int firstBufferedRow;
     *    int lastBufferedRow;
     *    int currentRowPosition;
     *    int cursorType;
     *    long RSCounter;
     * }

     * The input (rsObj) to this method is a Java result set object.

     * The following are the outputs of this method:
    
     * rsInfo: An int array with each element populated with data from 
     *         a ResultSetInfo object as represented below.

     * rsInfo[0] = LOBDataDetected  [a value of 1 = true, 0 = false]
     * rsInfo[1] = ctxHandle
     * rsInfo[2] = stmtID
     * rsInfo[3] = firstBufferedRow
     * rsInfo[4] = lastBufferedRow
     * rsInfo[5] = currentRowPosition
     * rsInfo[6] = cursorType
     *             [0 = TYPE_FORWARD_ONLY, 1 = TYPE_SCROLL_INSENSITIVE,
     *              2 = TYPE_SCROLL_SENSITIVE]
     * rsInfo[7] = RSClosed

     * rsCounter: Value representing the order in which the underlying
     *            result set statement was executed.

     * conn: Reference to the java.sql.Connection object of which the
     *       result set is part of.

     * errCode - Container for returned error code
     * errDetail -  Container for any returned error detail text

     * The errCode is set to RS_INFO_ERROR (11235) and errDetail initialized 
     * accordingly whenever an exception is thrown by this method.


     * Note: 
     * This method is called from the LM C++ layer (LmResultSetJava class)
     * via JNI. Any change to the pamaters of this method will impact the 
     * LM C++ layer.
     **/
      
  public static void getRSInfo( Object rsObj,
                                int[] rsInfo,
                                long[] rsCounter, 
                                Object[] conn,
                                int[] errCode,
                                String[] errDetail )
    throws Throwable
  {
    
    errCode[0] = 0;
    errDetail[0] = "";
    
    Object rsInfoObj = null;
    
    // First check if the rsObj is an instance of 
    // org.apache.trafodion.jdbc.t2.SQLMXResultSet class
    if( !(rsT2Cls_.isInstance( rsObj )) ) {
      errCode[0] = RS_INFO_ERROR;
      errDetail[0] =
        "One or more returned result sets are not instances of " +
        "class org.apache.trafodion.jdbc.t2.SQLMXResultSet";
      return;
    }
    
    // Invoke the method
    rsInfoObj = rsInfoMID_.invoke( rsObj, null );
    
    if( rsInfoObj != null ) {
      // Populate the output parameters with the values
      // from ResultSetInfo fields
      
      // rsInfoFlds_[0] = LOBDataDetected - of type boolean
      boolean value = rsInfoFlds_[0].getBoolean( rsInfoObj );
      rsInfo[0] = (value) ? 1 : 0;
      
      // rsInfoFlds_[1] = connection - of type java.sql.Connection object
      conn[0] = rsInfoFlds_[1].get( rsInfoObj );
      
      // rsInfoFlds_[2] = ctxHandle - of type int
      rsInfo[1] = rsInfoFlds_[2].getInt( rsInfoObj );
      
      // rsInfoFlds_[3] = stmtID - of type int
      rsInfo[2] = rsInfoFlds_[3].getInt( rsInfoObj );
      
      // rsInfoFlds_[4] = firstBufferedRow - of type int
      rsInfo[3] = rsInfoFlds_[4].getInt( rsInfoObj );
      
      // rsInfoFlds_[5] = lastBufferedRow - of type int
      rsInfo[4] = rsInfoFlds_[5].getInt( rsInfoObj );
      
      // rsInfoFlds_[6] = currentRowPosition - of type int
      rsInfo[5] = rsInfoFlds_[6].getInt( rsInfoObj );
      
      // rsInfoFlds_[7] = cursorType - of type int
      int ctype = rsInfoFlds_[7].getInt(rsInfoObj);
      if (ctype == ResultSet.TYPE_FORWARD_ONLY)
        rsInfo[6] = 0;
      else if (ctype == ResultSet.TYPE_SCROLL_INSENSITIVE)
        rsInfo[6] = 1;
      else if (ctype == ResultSet.TYPE_SCROLL_SENSITIVE)
        rsInfo[6] = 2;
      
      // rsInfoFlds_[8] = RSCounter - of type long
      rsCounter[0] = rsInfoFlds_[8].getLong( rsInfoObj );
      
      // rsInfoFlds_[9] = RSClosed - of type boolean
      boolean rsIsClosed = rsInfoFlds_[9].getBoolean( rsInfoObj );
      
      // This test shouldn't be necessary and we may eventually decide
      // to remove it. We check to see if the connection is closed and
      // if so, close the RS and set the RSClosed flag to FALSE. JDBC
      // driver should be doing this for us but as of 3/4/2006, is
      // not.
      if (!rsIsClosed)
      {
        try
        {
          java.sql.Connection c = (java.sql.Connection) conn[0];
          if (c.isClosed())
          {
            rsIsClosed = true;
            java.sql.ResultSet rs = (java.sql.ResultSet) rsObj;
            rs.close();
            rs.getStatement().close();
          }
        }
        catch (Throwable t)
        {
        }
      }
      
      rsInfo[7] = rsIsClosed ? 1 : 0;

      // rsInfoFlds_[10] = CLI_stmt_status - of type boolean
      boolean cli_stmt_closed = rsInfoFlds_[10].getBoolean( rsInfoObj );
      rsInfo[8] = cli_stmt_closed ? 1 : 0;
      
    } // if (rsInfoObj)
    
    else
    {
      errCode[0] = RS_INFO_ERROR;
      errDetail[0] = "JDBC/MX returned a NULL ResultSetInfo object";
      return;
    }
  }
  
  //
  // This native method represents a gateway to C where the command
  // string gets processed and an output string gets returned in
  // result[0].
  //
  native public static void nativeUtils(String command, String[] result)
    throws Exception;
  
  // Native method to get the transaction id to be passed to
  // Type 4 JDBC driver
  native public static short[] getTransactionId();

    //
    // This main() method is intended for debugging purposes only
    //
    public static void main(String[] args)
        throws Exception
    {
        System.out.println("This is LmUtility::main()");
        String[] result = new String[1];
        for (int i = 0; i < args.length; i++)
        {
            result[0] = null;
            utils(args[i], result);
            System.out.println(args[i] + " returned " + result[0]);
        }
    }

}
