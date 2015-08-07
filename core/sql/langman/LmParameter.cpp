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
/*
******************************************************************************
*
* File:         LmParameter.cpp
* Description:  LmParameter Class
* Created:      08/22/2003
* Language:     C++
*
******************************************************************************
*/

#include "LmParameter.h"
#include "wstr.h"

#ifdef LANGMAN
#include "LmAssert.h"
#include "exp_expr.h"
#include "exp_clause_derived.h"
#endif

//////////////////////////////////////////////////////////////////////
// setOut<type>: The following setOut<type> methods set the contents 
// of the outAddr attribute to the respective SQL <type>. Note that
// SQL types such as CHAR and DATE are treated as null-terminated
// strings (COM_VANSI_FSDT) from the LM's perspective.
//////////////////////////////////////////////////////////////////////
LmResult LmParameter::setOutSmallInt(void *dataPtr, short a)
{
  if (sizeof(a) > outSize_ )
    return LM_PARAM_OVERFLOW;

  *(short*) ((char*)dataPtr + outDataOffset_) = a;
  return LM_OK;
}

LmResult LmParameter::setOutInteger(void *dataPtr, Int32 a)
{
  if (sizeof(a) > outSize_)
    return LM_PARAM_OVERFLOW;

  *(Int32*) ((char*)dataPtr + outDataOffset_) = a;
  return LM_OK;
}

LmResult LmParameter::setOutLargeInt(void *dataPtr, Int64 a)
{
  if (sizeof(a) > outSize_)
    return LM_PARAM_OVERFLOW;

  *(Int64*) ((char*)dataPtr + outDataOffset_) = a;
  return LM_OK;
}

LmResult LmParameter::setOutReal(void *dataPtr, float a)
{
  if (sizeof(a) > outSize_)
    return LM_PARAM_OVERFLOW;

  *(float*) ((char*)dataPtr + outDataOffset_) = a;
  return LM_OK;
}

LmResult LmParameter::setOutFloat(void *dataPtr, double a)
{
  if (sizeof(a) > outSize_)
    return LM_PARAM_OVERFLOW;

  *(double*) ((char*)dataPtr + outDataOffset_) = a;
  return LM_OK;
}

LmResult LmParameter::setOutDouble(void *dataPtr, double a)
{
  if (sizeof(a) > outSize_)
    return LM_PARAM_OVERFLOW;

  *(double*) ((char*)dataPtr + outDataOffset_) = a;
  return LM_OK;
}

LmResult LmParameter::setOutNumeric(void *dataPtr,
	                     	    const char *source,
	             	            ComBoolean copyBinary,
	             	            CollHeap *heap,
	             	            ComDiagsArea *diags)
{
  LmResult result = LM_OK;

  // Note that this file also gets linked as lmcomp library
  // which is used in utilities. But this function gets called
  // only when this file is linked into UDR server.
#ifdef LANGMAN
  if (copyBinary)
  {
    NABoolean callerWantsDiags = (diags ? TRUE : FALSE);

    ex_expr::exp_return_type expRetcode =
      convDoIt((char*)source, str_len(source), REC_BYTE_F_ASCII, 0, 0,
               ((char*)dataPtr + outDataOffset_), (Lng32)outSize_,
               fsType_, prec_, scale_,
               NULL, 0, heap, &diags, CONV_ASCII_BIGNUM, NULL, 0);
    
    if (expRetcode == ex_expr::EXPR_ERROR)
      result = LM_ERR;
    
    // If the caller did not pass in a diags area, but convDoIt
    // generated a new one, we need to release the new one.
    if (!callerWantsDiags && diags)
      diags->decrRefCount();
  }
  else
  {
    result = setOutChar(dataPtr, source, str_len(source));
  }
#endif

  return result;
}

// Copy the return string without null terminator into outAddr_.
LmResult LmParameter::setOutChar(void *dataPtr,
	                	 const char *src,
				 ComUInt32 lengthInBytes)
{
  LmResult result = LM_OK;

#ifdef LANGMAN
  if (lengthInBytes > outSize_)
  {
    lengthInBytes = outSize_;
    result = LM_PARAM_OVERFLOW;
  }

  str_cpy_all((char*)dataPtr + outDataOffset(), src, (Lng32)lengthInBytes);

  // Copy length into varchar length indicator
  switch (outVCLenIndSize())
  {
    case 0:
      break;

    case 2:
      {
        ComUInt16 tempLen = (ComUInt16) lengthInBytes;
        memcpy((char*)dataPtr + outVCLenIndOffset(), &tempLen, sizeof(short));
        break;
      }
    case 4:
      memcpy((char*)dataPtr + outVCLenIndOffset(), &lengthInBytes,
             sizeof(Int32));
      break;

    default:
      LM_ASSERT1(0, "Unknown varchar indicator length.");
      break;
  }

  // Blank-pad FIXED char types.
  if (DFS2REC::isSQLFixedChar(fsType()))
  {
     if (outSize_ > lengthInBytes)
     {
       switch (encodingCharSet_)
       {
         case CharInfo::ISO88591 :
         case CharInfo::SJIS :
         case CharInfo::UTF8 :
           str_pad((char*)dataPtr + outDataOffset_ + lengthInBytes,
                   (Lng32)(outSize_ - lengthInBytes),
		   ' ');
           break;

         case CharInfo::UNICODE :
           wc_str_pad((NAWchar*)((char*)dataPtr + outDataOffset_ +
                                 lengthInBytes),
                      (Lng32)(outSize_ - lengthInBytes)/2,
                      unicode_char_set::space_char());
           break;

         default :
           char msg[256];
           sprintf(msg, "Unknown Character set value: %d", encodingCharSet_);
	   LM_ASSERT1(0, msg);
           break;
       }
     }
  }

#endif

  return result;
}

LmResult LmParameter::setOutDecimal(void *dataPtr,
	           	            const char *source,
                                    CollHeap *heap,
                                    ComDiagsArea *diags)
{
  LmResult result = LM_OK;

  // Note that this file also gets linked as lmcomp library
  // which is used in utilities. But this function gets called
  // only when this file is linked into UDR server.
#ifdef LANGMAN
  NABoolean callerWantsDiags = (diags ? TRUE : FALSE);

  ex_expr::exp_return_type expRetcode =
    convDoIt((char*)source, str_len(source), REC_BYTE_F_ASCII, 0, 0,
             (char*)dataPtr + outDataOffset() , (Lng32)outSize_, fsType_, prec_,
	     scale_, NULL, 0, heap, &diags, CONV_ASCII_DEC, NULL, 0);

  if (expRetcode == ex_expr::EXPR_ERROR)
    result = LM_ERR;

  // If the caller did not pass in a diags area, but convDoIt
  // generated a new one, we need to release the new one.
  if (!callerWantsDiags && diags)
    diags->decrRefCount();
#endif

  return result;
}

LmResult LmParameter::setOutDate(void *dataPtr, const char *a)
{
  return setOutChar(dataPtr, a, str_len(a));
}

LmResult LmParameter::setOutTime(void *dataPtr, const char *val)
{
  ComUInt32 valLen = str_len(val);
  LmResult result = setOutChar(dataPtr, val, valLen);
  if (result != LM_OK)
    return result;

  // JDBC's Time format is always hh:mm:ss. We need to pad
  // .msssss to this format if needed.
  // If the target type is Time(0), no padding is needed.

  if (outSize_ > valLen)
    *((char*)dataPtr + outDataOffset() + valLen) = '.';

  for (ComUInt32 index=valLen+1; index<outSize_; index++)
    *((char*)dataPtr + outDataOffset() + index) = '0';

  return LM_OK;
}

LmResult LmParameter::setOutTimestamp(void *dataPtr, const char *a)
{
  // JDBC's Timestamp format is always yyyy-mm-dd hh:mm:ss.fffffffff
  // We need to truncate or pad .fffffffff if needed.
  // Target Type         Action
  // +++++++++++         ++++++
  // Timestamp(0)        Truncate .fffffffff (JDBC returns ".0". We
  //                      raise warning if .fffffffff is not .0)
  // Timestamp(n)        Pad value if there are fewer than 'n' digits
  //                      after "."
  // Timestamp(n)        Trucate value(raise warning) if there are more than
  //                      'n' digits after "." (This is possible becuase
  //                      JDBC precision is greater than SQL precision)
  ComUInt32 valLen = str_len(a);
  if (valLen > outSize_) 
  {
    // value needs to be truncated.
    if (a[valLen-2] == '.' && a[valLen-1] == '0')
    {
      // The value ends with ".0" No need for warning
      return setOutChar(dataPtr, a, valLen-2);
    }
    else
    {
      // The returned value does not fit, raise warning
      return setOutChar(dataPtr, a, valLen);
    }
  }
  else
  {
    LmResult result = setOutChar(dataPtr, a, valLen);
    if (result != LM_OK)
      return result;

    // If value needs padding, pad with character '0'.
    for (ComUInt32 index=valLen; index<outSize_; index++)
       *((char*)dataPtr + outDataOffset() + index) = '0';

    return LM_OK;
  }

}

LmResult LmParameter::setOutInterval(void *dataPtr,
	                	     const char *rawBytes,
			             ComUInt32 len)
{
  LmResult result = LM_OK;

  if (len > outSize_)
  {
    len = outSize_;
    result = LM_PARAM_OVERFLOW;
  }

  Lng32 numOfBlanks = (Lng32) (outSize_ - len);
  char *temp = (char*)dataPtr + outDataOffset();
  for (Lng32 i = 0; i < numOfBlanks; i++)
  {
    *temp = ' ';
    temp++;
  }

  str_cpy_all(temp, rawBytes, (Lng32)len);
  return result;
}

// This private method should not be called for NOT NULL
// parameters. indOffset is expected to be >= 0.
ComBoolean LmParameter::isNullValue(char *dataRow, ComUInt32 indOffset) const
{
#ifdef LANGMAN
  if (((char*)dataRow)[indOffset] & NEG_BIT_MASK)
    return TRUE;
#endif
  return FALSE;
}

// This private method should not be called for NOT NULL
// parameters. indOffset is expected to be >= 0 and indSize is
// expected to be > 0.
void LmParameter::setNullValue(char *dataRow,
                               ComUInt32 indOffset, ComUInt32 indSize,
                               ComBoolean valueIsNull) const
{
#ifdef LANGMAN
  str_pad(dataRow + indOffset, indSize, (valueIsNull ? '\377' : '\0'));
#endif
}

ComUInt32
LmParameter::actualInDataSize(void *data) const
{
  if (inVCLenIndSize_ == 0)
    return inSize();
  else
    return vcDataSize((char*)data, inVCLenIndOffset_, inVCLenIndSize_);
}

ComUInt32
LmParameter::actualOutDataSize(void *data) const
{
  if (outVCLenIndSize_ == 0)
    return outSize();
  else
    return vcDataSize((char*)data, outVCLenIndOffset_, outVCLenIndSize_);
}

ComUInt32
LmParameter::vcDataSize(char *data,
	          	ComSInt32 lenIndOffset,
		       	ComSInt16 lenIndSize) const
{
  ComUInt32 len = 0;
#ifdef LANGMAN
  switch (lenIndSize)
  {
    case 0:
      break;

    case 2: short templen;
            memcpy(&templen, data + lenIndOffset, sizeof(short));
            len = (ComUInt32) templen;
            break;

    case 4: memcpy(&len, data + lenIndOffset, sizeof(Int32));
            break;

    default :
	    LM_ASSERT1(0, "Unknown varchar indicator length.");
  }
#endif

  return len;
}

void LmParameter::setParamName(const char *name)
{
  if (paramName_)
  {
    free(paramName_);
    paramName_ = NULL;
  }

  if (name)
  {
    ComUInt32 len = strlen(name);
    paramName_ = (char *) malloc(len + 1);
    memcpy(paramName_, name, len + 1);
  }
}
