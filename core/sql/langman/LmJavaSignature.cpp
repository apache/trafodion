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
/* -*-C++-*-
******************************************************************************
*
* File:         LmJavaSignature.cpp
* Description:  Java Signature
*
* Created:      06/17/2003
* Language:     C++
*
*
******************************************************************************
*/

#include "LmParameter.h"
#include "LmJavaType.h"
#include "LmJavaSignature.h"
#include "LmJavaSignatureHelpers.h"

//
// Constructor
//
// This class uses the heap parameter if provided. Other wise
// uses global heap to allocate memory. This is to facilitate
// DDOL code calls into this class. DDOL code does not use any
// classes from /common directory.
//
LmJavaSignature::LmJavaSignature(const char* encodedSignature, void* heap)
 : heap_(NULL),
   encodedSignature_(NULL),
   unpackedSignatureSize_(-1),
   numParams_(-1)
{
  if (heap != NULL)
    heap_ = (NAMemory *) heap;

  if (encodedSignature == NULL)
  {
    encodedSignature_ = NULL;
  }
  else
  {
    UInt32 siglen = strlen(encodedSignature);

    if (heap_ == NULL)
      encodedSignature_ = new char[siglen + 1];
    else
      encodedSignature_ = new (heap_) char[siglen + 1];

    strcpy(encodedSignature_, encodedSignature);

    // Compute the numParams_ value
    ::getUnpackedSignatureSize(encodedSignature_, &numParams_);
  }
}


// Destructor
LmJavaSignature::~LmJavaSignature()
{
  if (encodedSignature_ != NULL)
  {
    if (heap_)
      NADELETEBASIC(encodedSignature_, heap_);
    else
      delete encodedSignature_;
  }
}

//
// createSig() : Creates a Java signature for the specifed SQL attributes.
// If optionalSig is specified, it contains the string entered in the
// EXTERNAL NAME field of the CREATE PROCEDURE command, and is validated
// in this function. The returned signature is built in the sigBuf parameter.
//
// Returns:  LM_OK   on success
//           LM_ERR  on failure
//
LmResult
LmJavaSignature::createSig(ComFSDataType paramType[],
                           ComUInt32     paramSubType[],
                           ComColumnDirection direction[],
                           ComUInt32     numParam,
                           ComFSDataType resultType,
                           ComUInt32     resultSubType,
                           ComUInt32     numResultSets,
                           const char    *optionalSig,
                           ComBoolean    isUdrForJavaMain,
                           char          *sigBuf,
                           ComUInt32     sigLen,
                           ComDiagsArea  *da)
{
  static const char *resultSet = "[Ljava/sql/ResultSet;";
  static const Int32 resultSetLen = str_len(resultSet);

  ComUInt32 len = 0;
  ComUInt32 idx = 0;
  ComBoolean lookForTrailingParams = FALSE;
  const char *type;
  char *optc = NULL, *optsig = NULL, *optype = NULL;

  if (optionalSig != NULL)
  {
    len = (ComUInt32) str_len(optionalSig);
    optype = optsig = new (heap_) char[len+1];

    str_cpy_all(optsig, optionalSig, (Lng32) len);
    optsig[len] = '\0';

    if (*optype != '(')
    {
      // Error: Missing parentheses.
      *da << DgSqlCode(-LME_SIGNATURE_INVALID1);
      if (optsig) NADELETEBASIC(optsig, heap_);
      return LM_ERR;
    }
    optype++;
  } //if (optionalSig...)

  //
  // Build the Java signature in the sig buffer.
  // Start of method's parameter list.
  //
  // In this method we check for space in sigBuf everytime
  // we write something into it.
  //
  if (idx + 1 > sigLen)
  {
    *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
    if (optsig) NADELETEBASIC(optsig, heap_);
    return LM_ERR;
  }

  sigBuf[idx++] = '(';

  //
  // Special treatment for UDR-MAIN
  //
  // Check if optional signature is java.lang.String[]
  // Then write "[Ljava/lang/String;" to sigBuf
  //
  if (isUdrForJavaMain)
  {
    if (optionalSig != NULL)
    {
      char * endOfOptSig= strchr(optype, ')');
      if (endOfOptSig == NULL)
      {
        // Error: Missing parantheses
        *da << DgSqlCode(-LME_SIGNATURE_INVALID1);
        if (optsig) NADELETEBASIC(optsig, heap_);
        return LM_ERR;
      }

      *endOfOptSig = '\0';
      optype = strip_spaces(optype);

      len = (ComUInt32) str_len(optype);
      if (len >= 2)
      {
        if (optype[len-2] != '[' || optype[len-1] != ']')
        {
          *da << DgSqlCode(-LME_SIGNATURE_INVALID8);
          if (optsig) NADELETEBASIC(optsig, heap_);
          return LM_ERR;
        }
        else
        {
          optype[len-2] = '\0';
          optype = strip_spaces(optype);
        }
      }

      if (str_cmp_ne(optype, "java.lang.String") != 0)
      {
        *da << DgSqlCode(-LME_SIGNATURE_INVALID8);
        if (optsig) NADELETEBASIC(optsig, heap_);
        return LM_ERR;
      }

      // Move optype to point to next of ')' in optional signature
      optype = endOfOptSig + 1;
    }

    if (idx + 19 > sigLen)
    {
      *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
      if (optsig) NADELETEBASIC(optsig, heap_);
      return LM_ERR;
    }

    str_cpy_all(&sigBuf[idx], "[Ljava/lang/String;", 19);
    idx += 19;
  }
  else if (((Int32)numParam == 0) && optype && (numResultSets > 0))
  {
    lookForTrailingParams = true;
  }
  else if (((Int32)numParam == 0) && optype && (numResultSets == 0))
  {
    //
    // Case: numParam is 0. '()', '(void)' in optsig are valid
    //
    optc = strchr(optype, ')');
    if (optc == NULL)
    {
      // Error: Missing parantheses
      *da << DgSqlCode(-LME_SIGNATURE_INVALID1);
      if (optsig) NADELETEBASIC(optsig, heap_);
      return LM_ERR;
    }
    *optc = '\0';
    optype = strip_spaces(optype);

    if ((!* optype) || (str_cmp_ne(optype, "void") == 0))
      optype = optc + 1;
    else
    {
      // Case: Params specified in opt sig are more than expected
      *da << DgSqlCode(-LME_SIGNATURE_INVALID2)
          << DgInt0((Lng32) numParam);
      if (optsig) NADELETEBASIC(optsig, heap_);
      return LM_ERR;
    }
  } // if ((int)numParam...)

   //
  // Travel through the parameters. The following steps are performed
  // in the 'for' loop.
  // 1. For UDr-MAIN, check all types are CHAR/VARCHAR and mode is IN
  // 2. Get the type from the optional signature if there is one.
  // 3. Enfore '[]' for OUT/INOUT types in optional signature.
  // 4. Call getJavaTypeName() to get java name of the type.
  // 5. Write the type name into sigBuf
  //
  for (Int32 i = 1; i <= (Int32)numParam; i++)
  {
    //
    // Special treatment for UDR-MAIN
    //
    // All the parameters should be IN.
    //
    // We don't need to check here if the correct type is
    // specified in the optional signature. This is already
    // done above.
    //
    // Just check if the parameter is String type. We allow
    // only String types to main method.
    //
    // We don't need to write the type name to sigBuf. We can
    // directly write ')' after this 'for' loop
    //
    if (isUdrForJavaMain)
    {
      if (direction[i-1] != COM_INPUT_COLUMN)
      {
        *da << DgSqlCode(-LME_SIGNATURE_INVALID9);
        if (optsig) NADELETEBASIC(optsig, heap_);
        return LM_ERR;
      }

      LmParameter lmParam((paramType[i-1]), (short) paramSubType[i-1]);
      LmJavaType jType(&lmParam);
      type = jType.getJavaTypeName(len);
      if (str_cmp_ne(type, "Ljava/lang/String;") != 0)
      {
        *da << DgSqlCode(-LME_SIGNATURE_INVALID10);
        if (optsig) NADELETEBASIC(optsig, heap_);
        return LM_ERR;
      }

      continue;
    }
    if (optionalSig)
    {
      optc = strchr(optype, ',');
      if (optc != NULL)
      {
        *optc = '\0';
        optype = strip_spaces(optype);
        if ((i == (Int32)numParam) && (numResultSets == 0))
        {
          // Case: Params specified in opt sig are more than expected
          *da << DgSqlCode(-LME_SIGNATURE_INVALID2)
              << DgInt0((Lng32) numParam);
          if (optsig) NADELETEBASIC(optsig, heap_);
          return LM_ERR;
        }
        lookForTrailingParams = true;
      }
      else // first if (optc != NULL)
      {
        optc = strchr(optype, ')');
        if (optc != NULL)
        {
          *optc = '\0';
          optype = strip_spaces(optype);
          if ((i < (Int32)numParam) || ((numParam ==1)&& !(*optype)))
          {
            // Case: Params specified in opt sig are less than expected
            // Case: sig is () but numParam is 1
            *da << DgSqlCode(-LME_SIGNATURE_INVALID2)
                << DgInt0((Lng32) numParam);
            if (optsig) NADELETEBASIC(optsig, heap_);
            return LM_ERR;
          }
          lookForTrailingParams = FALSE;
        }
        else // second if (optc != NULL)
        {
          // Error: Missing parentheses.
          *da << DgSqlCode(-LME_SIGNATURE_INVALID1);
          if (optsig) NADELETEBASIC(optsig, heap_);
          return LM_ERR;
        }
      } // first if (optc != NULL)
    } // if (optionalSig)

    // For OUT/INOUT params, [] in optional signature is mandatory.
    if (optype)
    {
      ComUInt32 optlen = (ComUInt32) str_len(optype);
      if (direction[i-1] == COM_INOUT_COLUMN ||
          direction[i-1] == COM_OUTPUT_COLUMN)
      {
        if (optlen > 2 && optype[optlen-2] == '[' && optype[optlen-1] == ']')
        {
          optype[optlen-2] = '\0';
          optype = strip_spaces(optype); // Allow spaces btw TYPE and []
        }
        else
        {
          // Error: Missing [] for OUT/INOUT param
          *da << DgSqlCode(-LME_SIGNATURE_INVALID3)
              << DgInt0(i);
          if (optsig) NADELETEBASIC(optsig, heap_);
          return LM_ERR;
        }
      }
    }

    //
    // Get the Java type for the param's SQL <type,subtype>
    // First check if the type is base type. Next check if it
    // is Object type. For object types optional type should
    // be there.
    //
    LmParameter lmParam(paramType[i-1], (short) paramSubType[i-1]);
    LmJavaType baseType(&lmParam);
    LmJavaType::TypeElement *e = baseType.getTypeElement();

    if (e != NULL)
    {
      type = e->javaTypeName;
      len = e->javaTypeNameLen;
    }

    if (e == NULL || (optype && (str_cmp_ne(e->javaText, optype) != 0)))
    {
      // User wants the object type. So look for Object type
      // in type table only if optional signature is specified
      if (optype)
      {
        lmParam.setObjMapping();
        LmJavaType objType(&lmParam);
        if ((e = objType.getTypeElement()) != NULL)
        {
          type = e->javaTypeName;
          len = e->javaTypeNameLen;
        }
      }
    }

    if (e == NULL || (optype && (str_cmp_ne(e->javaText, optype) != 0)))
    {
      // Error: Unknown parameter type used.
      *da << DgSqlCode(-LME_SIGNATURE_INVALID4)
          << DgInt0(i);
      if (optsig) NADELETEBASIC(optsig, heap_);
      return LM_ERR;
    }

    // Out mode params are specified using an array for the given type.
    if (direction[i-1] == COM_INOUT_COLUMN ||
        direction[i-1] == COM_OUTPUT_COLUMN)
    {
      if ((idx + 1) > sigLen)
      {
        *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
        if (optsig != NULL) NADELETEBASIC(optsig, heap_);
        return LM_ERR;
      }

      sigBuf[idx++] = '[';
    }

    if ((idx + len) > sigLen)
    {
      *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
      if (optsig != NULL) NADELETEBASIC(optsig, heap_);
      return LM_ERR;
    }

    // Add the Java type to the sig buffer.
    str_cpy_all(&sigBuf[idx], type, (Lng32) len);
    idx += len;

    // move optype to point to next type in optional sig
    if (optype) optype = optc + 1;

  } // for()

  if(lookForTrailingParams)
  {
    Int32 numTrailingParams = 0;

    while(true)
    {
      numTrailingParams++;
      optc = strchr(optype, ',');
      if (optc != NULL)
      {
        *optc = '\0';
        optype = strip_spaces(optype); // Allow spaces btw TYPE and comma
        ComUInt32 optlen = (ComUInt32) str_len(optype);
        if (optlen > 2 && optype[optlen-2] == '[' && optype[optlen-1] == ']')
        {
          optype[optlen-2] = '\0';
          optype = strip_spaces(optype); // Allow spaces btw TYPE and []

          if (str_cmp_ne(optype, "java.sql.ResultSet") == 0)
          {
            // Ensure buffer accomodates ResultSet
            if (idx + resultSetLen > sigLen)
            {
              *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
              if (optsig) NADELETEBASIC(optsig, heap_);
                return LM_ERR;
            }
            str_cpy_all(&sigBuf[idx], resultSet, resultSetLen);
            idx += resultSetLen;	    
            // move optype to point to next type in optional sig
            if (optype) optype = optc + 1;
          }
          else
          {
            //Error: Not a java.sql.ResultSet[]
            *da << DgSqlCode(-LME_SIGNATURE_INVALID11)
                << DgInt0((Lng32)numParam + numTrailingParams);
            if (optsig) NADELETEBASIC(optsig, heap_);
              return LM_ERR;
          }
        }
        else
        {
          // Error: Not a java.sql.ResultSet[]
          *da << DgSqlCode(-LME_SIGNATURE_INVALID11)
              << DgInt0((Lng32)numParam + numTrailingParams);
          if (optsig) NADELETEBASIC(optsig, heap_);
          return LM_ERR;
        } 
      }
      else  
      {
        optc = strchr(optype, ')');
        if (optc != NULL)
        {
          *optc = '\0';
          optype = strip_spaces(optype); // Allow spaces btw TYPE and comma
          ComUInt32 optlen = (ComUInt32) str_len(optype);
          if (optlen > 2 && optype[optlen-2] == '[' && optype[optlen-1] == ']')
          {
            optype[optlen-2] = '\0';
            optype = strip_spaces(optype); // Allow spaces btw TYPE and []

            if (str_cmp_ne(optype, "java.sql.ResultSet") == 0)
            {
              // Ensure buffer accomodates ResultSet
              if (idx + resultSetLen > sigLen)
              {
                *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
                if (optsig) NADELETEBASIC(optsig, heap_);
                  return LM_ERR;
              }
              str_cpy_all(&sigBuf[idx], resultSet, resultSetLen);
              idx += resultSetLen;	    
              // move optype to point to next type in optional sig
              if (optype) optype = optc + 1;
                break;
            }
            else
            {
              //Error: not a java.sql.ResultSet[]
              *da << DgSqlCode(-LME_SIGNATURE_INVALID11)
                  << DgInt0((Lng32)numParam + numTrailingParams);
              if (optsig) NADELETEBASIC(optsig, heap_);
                return LM_ERR;
            }
          }
          else
          {
            // Error: not a java.sql.ResultSet[]
            *da << DgSqlCode(-LME_SIGNATURE_INVALID11)
                << DgInt0((Lng32)numParam + numTrailingParams);
            if (optsig) NADELETEBASIC(optsig, heap_);
            return LM_ERR;
          } 
        }
        else  
        {
          // Error: Missing parentheses.
          *da << DgSqlCode(-LME_SIGNATURE_INVALID1);
          if (optsig) NADELETEBASIC(optsig, heap_);
            return LM_ERR;
        }
      }//end else
    }//end while
  }//end if lookForTrailingPrams

  if ((idx + 1) > sigLen)
  {
    *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
    if (optsig != NULL) NADELETEBASIC(optsig, heap_);
    return LM_ERR;
  }
  for(int i = 0; i < numResultSets; i++) 
  {
    if (idx + resultSetLen > sigLen)
    {
        *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
        if (optsig) NADELETEBASIC(optsig, heap_);
        return LM_ERR;
    }
    str_cpy_all(&sigBuf[idx], resultSet, resultSetLen);
    idx += resultSetLen;
  }

  // End of method's parameter list.
  sigBuf[idx++] = ')';

  //
  // Now write the signature element for Return type
  //

  // If optional sig, ensure no return type is specified.
  if ((optsig != NULL) && (*optype))
  {
    *da << DgSqlCode(-LME_SIGNATURE_INVALID6);
    if (optsig) NADELETEBASIC(optsig, heap_);
    return LM_ERR;
  }

  // Get the Java type for the return type's SQL <type,subtype>.
  LmParameter lmParam(resultType, (short) resultSubType);
  LmJavaType jType(&lmParam);
  type = jType.getJavaTypeName(len);
  if (type == NULL)
  {
    // Error: Unknown or unsupported type used as a return type.
    *da << DgSqlCode(-LME_SIGNATURE_INVALID7);
    if (optsig) NADELETEBASIC(optsig, heap_);
    return LM_ERR;
  }

  // Ensure buffer accomodates return type and '\0'.
  if ((idx + len + 1) > sigLen)
  {
    *da << DgSqlCode(-LME_SIGNATURE_INVALID5);
    if (optsig) NADELETEBASIC(optsig, heap_);
    return LM_ERR;
  }

  // Add the method's return type to the sig Buffer.
  str_cpy_all(&sigBuf[idx], type, (Lng32) len);
  sigBuf[idx+len] = '\0';

  if (optsig) NADELETEBASIC(optsig, heap_);

  return LM_OK;
}

//
// unpackSignature():  Unpacks a Java signature.
// Callers has to call getUnpackedSignatureSize() first and then allocate
// so many bytes before calling unpackSignature().
//
// Example: For procedure proc(IN int, IN NUMERIC (9,3), INOUT char(50)),
// packed signature is (ILjava/math/BigDecimal;[Ljava/lang/String;)V
// unpacked signature is (int,java.math.BigDecimal,java.lang.String[])
//
// Note: This method does not do extensive error checking. For example,
// there is no checks for improper placement of '(', '[' etc. We assume
// that this method is called with proper unpackedSignature, that was
// generated by us.
//
Int32
LmJavaSignature::unpackSignature(char *unpackedSignature)
{
  return ::unpackSignature(encodedSignature_, unpackedSignature);
}

//
// getUnpackedSignatureSize() : Calculates the bytes required if
// the given (Java) signture is unpacked.
// returns size of unpacked signature string
//         -1 if unrecognized type is in signature
//
Int32
LmJavaSignature::getUnpackedSignatureSize()
{
  if (unpackedSignatureSize_ == -1)
    unpackedSignatureSize_ = ::getUnpackedSignatureSize(encodedSignature_);

  return unpackedSignatureSize_;
}
