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
********************************************************************/
/**************************************************************************
**************************************************************************/
//
// MODULE: NSKIEEE.cpp
//
//
//	Purpose: Routines to convert between t16 float and ieee float.
//
//	Compaq 64-bit float format :                            
//     +--------------------------------------------------+
//     |s|            mantissa                 |  exp     |
//     +--------------------------------------------------+
//      1                54                         9      
//                                                         
//	Compaq 32-bit float format :                            
//     +--------------------------------------+            
//     |s|      mantissa           |  exp     |            
//     +--------------------------------------+            
//      1          22                   9                  
//                                                         
//	IEEE   64-bit float format :                            
//     +--------------------------------------------------+
//     |s|   exp      |             mantissa              |
//     +--------------------------------------------------+
//      1     11                       52                  
//                                                         
//IEEE   32-bit float format :                            
//     +--------------------------------------+            
//     |s|   exp      |       mantissa        |            
//     +--------------------------------------+            
//      1     8                  23                        
//                                                                 
// Compaq float exponent is expressed as an excessed-256 value     
// (biased by 256).                                                
// The exponent range is between -256 to +255.                     
//                                                                 
// IEEE float exponent is also biased. The exponent bias is 1023   
// for double (64 bits) format and 127 for single (32 bits) format.
//                                                                 
//-----------------------------------------------------------------
#include <windows.h>
#include <sqltypes.h>
#include "sqlcli.h"
#include "nskieee.h"
#include "DrvrSrvr.h"
#ifdef NSK_PLATFORM
#include "kfpconv.h"
#endif

#define	IEEE_EXPONENT_BIAS		1023
#define T16_EXPONENT_BIAS			256
#define	BIAS_DIFFERENCE				767		// 1023 - 256 = 767
#define	IEEE_EXPONENT_BIAS_S	127		// single format
#define	BIAS_DIFFERENCE_S			-129	// 127 - 256 = -129

//----------------------------------------------------------------
// If the source length is not an even number, no swapping will
// be performed. No error is generated in this function.       
// IN/OUT : source/target, IN : swap length
//----------------------------------------------------------------
void ODBC::byte_swap(BYTE *source, long source_len)
{                                                              
#ifndef NSK_PLATFORM
#ifdef unixcli
	BYTE   *target;                                              
	BYTE    temp;                                                
	short   i;                                                   

	if (!(source_len % 2))
	{
		for (i = 1, target = source; i <= source_len /2; i++)
		{                                                 
			temp = source[i-1];                               
			target[i-1] = source[source_len - i];             
			source[source_len -i] = temp;                     
		}                                                 
	}
#endif	
#endif
}   // byte_swap

#ifndef unixcli
// The following byte order inversion routines were written by Dale Rempert.  They are
// specific to the 80x86 processors.  Even though the external representation is little endian
// the internal representation is big endian.  Here is how these routines work.  Take a short
// in big endian say 0xCDAB.  When read by the processor it "sees" the value 0xABCD. We rotate
// this value by 8 bits yealding 0xCDAB. Then written out to memory we get 0xABCD.  For 4 & 8
// byte values I read two byes at a time and build one or two 4 byte "longs" then write out the 
// longs.  This minimizes the reads/writes to slower memory, and if the compiler is nice will
// keep the temps in registers.  Though some compilers will not do this if a routine contains
// inline assembly statements.  

// Note the only reasonable sizes for endian inversion are 2, 4, and 8 bytes.

// byte_swap a 16 bit field (i.e. 2 bytes)
void ODBC::byte_swap_2(BYTE *source)
{
#ifndef BIGE
#ifndef NSK_PLATFORM
#ifdef unixcli
	unsigned short tmp;

			tmp = *(unsigned short *)source;
#ifndef unixcli
			_asm rol tmp,8
#else
			asm("rolw %1,%0" : "=g" (tmp) : "cI" (8), "0" (tmp));
			//asm ("rol tmp,8");
#endif						
			*(unsigned short *)source = tmp; 
#endif			
#endif
#endif
}   


// byte_swap a 32 bit field (i.e. 4 bytes)
void ODBC::byte_swap_4(BYTE *source)
{
#ifndef BIGE
#ifndef NSK_PLATFORM
#ifdef unixcli
	unsigned long t32;
	unsigned short t16;

			t16 = *(unsigned short *)source;
#ifndef unixcli			
			_asm rol t16,8
#else
			asm("roll %1,%0" : "=g" (t16) : "cI" (8), "0" (source));
//			asm ("rol t16,8");
#endif			
			t32 = ((unsigned long)t16) << 16;

			t16 = *(short *)(source+2);
#ifndef unixcli			
			_asm rol t16,8
#else
			asm("roll %1,%0" : "=g" (t16) : "cI" (8), "0" (source));
//			asm ("rol t16,8");
#endif
			
			t32 |= (unsigned long)t16;
			*(unsigned long *)source = t32; 
#endif			
#endif
#endif
}   

// byte_swap a 64 bit field (i.e. 8 bytes)
// 0x1122334455667788 => 0x8877665544332211

void ODBC::byte_swap_8(BYTE *source)
{
#ifndef BIGE
#ifndef NSK_PLATFORM
#ifdef unixcli
	unsigned long t32a, t32b;
	unsigned short t16;

			t16 = *(unsigned short *)source;		// 0x2211
#ifndef unixcli
			_asm rol t16,8				// 0x1122
#else
			asm("roll %1,%0" : "=g" (t16) : "cI" (8), "0" (source));
//			asm ("rol t16, 8");
#endif			
			t32a = ((unsigned long)t16) << 16;	// 0x11220000

			t16 = *(unsigned short *)(source+2);	// 0x4433
#ifndef unixcli			
			_asm rol t16,8				// 0x3344
#else
			asm("roll %1,%0" : "=g" (t16) : "cI" (8), "0" (source));
//			asm("rol t16,8");
#endif
			t32a |= (unsigned long)t16;			// 0x11223344
			
			t16 = *(unsigned short *)(source+4);	// 0x6655
#ifndef unixcli			
			_asm rol t16,8				// 0x5566
#else
			asm("roll %1,%0" : "=g" (t16) : "cI" (8), "0" (source));
//			asm("rol t16,8");
#endif
			t32b = ((unsigned long)t16) << 16;	// 0x55660000

			t16 = *(unsigned short *)(source+6);	// 0x8877
#ifndef unixcli			
			_asm rol t16,8				// 0x7788
#else
			asm("roll %1,%0" : "=g" (t16) : "cI" (8), "0" (source));
//			asm("rol t16,8");
#endif			
			t32b |= (unsigned long)t16;			// 0x55667788

			*(unsigned long *)source = t32b;		// 0x88776655
			*(unsigned long *)(source+4) = t32a; // 0x44332211 
#endif			
#endif
#endif
}   

#endif /* unixcli */

// byte swap a string of chars in unicode UCS2 or any other chars in 
// fixed two byte representation.

void ODBC::byte_swap_string(BYTE *source, long source_len)
{
#ifdef unixcli
	unsigned short   *target;                                                                                              
	int   i; 
	unsigned short tmp;

	target = (unsigned short *)source;

	for (i = source_len /2; i > 0; i--)
	{ 
		tmp = *target;
#ifdef MXLINUX
		asm("rolw %1,%0" : "=g" (tmp) : "cI" (8), "0" (tmp));
#else
		byte_swap((BYTE*)&tmp,2);
#endif
		*target++ = tmp;                   
	}     
#endif
}   // byte_swap_string


//-------------------------------------------------------------------- 
// Convert T16 64-bit float to IEEE float.                             
//	IN : T16 floating number, OUT: IEEE float
//  For alignment reason, this is not declared as long long *.          
//-------------------------------------------------------------------- 
short ODBC::conv_float64_to_ieee_d (BYTE *op)
{                                                                       
	unsigned int	exp;          // exponent
	BYTE rop[8];
	BYTE *exp1;
	BYTE *exp2;

	// Get exponent and sign.
	exp = ((op[6] & 0x01) << 8) | op[7];

	if (exp == 0)
		return (STATUS_OK);                                                   
	exp = exp + BIAS_DIFFERENCE;	//1023 - 256
	exp1 = (BYTE *)&exp;
	exp2 = exp1 + 1;

	rop[0] = (BYTE)((op[0] & 0x80) | (*exp2 << 4) | (*exp1 >> 4));
	rop[1] = (BYTE)((*exp1 << 4) | ((op[0] & 0x7F) >> 3));	// RS has to remove the sign
	rop[2] = (BYTE)((op[0] << 5) | (op[1] >> 3));
	rop[3] = (BYTE)((op[1] << 5) | (op[2] >> 3));
	rop[4] = (BYTE)((op[2] << 5) | (op[3] >> 3));
	rop[5] = (BYTE)((op[3] << 5) | (op[4] >> 3));
	rop[6] = (BYTE)((op[4] << 5) | (op[5] >> 3));
	rop[7] = (BYTE)((op[5] << 5) | (op[6] >> 3));

	byte_swap(rop, 8);
	memcpy(op, rop, 8);
                                                                         
	return (STATUS_OK);                                                   
} // conv_float64_to_ieee_d

//--------------------------------------------------------------------
// Convert IEEE float to T16 64-bit float.
//--------------------------------------------------------------------
short ODBC::conv_ieee_d_to_float64 (BYTE *op)
{                                                                       
	unsigned int	exp;          // exponent
	BYTE rop[8];
	BYTE *exp1;
	BYTE *exp2;

	byte_swap(op, 8);
	// Get exponent and sign.
	exp = (((op[0] & 0x7F) << 8 ) | (op[1] & 0xF0)) >> 4;

	if (exp == 0)
		return (STATUS_OK);                                                   
	if ((int)exp < (1023 - 256))
		return( STATUS_UNDERFLOW);
	if ((int)exp > (1023 + 255))
		return ( STATUS_OVERFLOW);
	
	exp = exp - BIAS_DIFFERENCE;	//1023 - 256
	exp1 = (BYTE *)&exp;
	exp2 = exp1 + 1;

	rop[0] = (BYTE)((op[0] & 0x80) | ((op[1] & 0x0F) << 3) | ((op[2] & 0xE0) >> 5));
	rop[1] = (BYTE)((op[2] << 3) | (op[3] >> 5));
	rop[2] = (BYTE)((op[3] << 3) | (op[4] >> 5));
	rop[3] = (BYTE)((op[4] << 3) | (op[5] >> 5));
	rop[4] = (BYTE)((op[5] << 3) | (op[6] >> 5));
	rop[5] = (BYTE)((op[6] << 3) | (op[7] >> 5));
	rop[6] = (BYTE)((op[7] << 3) | (*exp2));
	rop[7] = (BYTE)(*exp1);

	memcpy(op, rop, 8);
                                                                         
	return (STATUS_OK);                                                   
} // conv_ieee_d_to_float64

//--------------------------------------------------------------------
// Convert T16 32-bit float to IEEE float (single format)             
// IN : T16 floating number, OUT: IEEE float ,single format.
//--------------------------------------------------------------------
short ODBC::conv_float32_to_ieee_s (BYTE *op )
{                                                                       
	unsigned int	exp;          // exponent
	BYTE rop[4];

	// Get exponent.
	exp = ((op[2] & 0x01) << 8) | op[3];

	if (exp == 0)
		return (STATUS_OK);                                                   
	// Check underflow or overflow, since Tandem has 9-bits exponents
	// IEEE has 8-bits exponents.
	if ( (int)exp < (127 - 256))
		return ( STATUS_UNDERFLOW);
	if ( (int)exp > (127 + 255))
		return ( STATUS_OVERFLOW);

	exp = exp + BIAS_DIFFERENCE_S;	//	127 - 256

	rop[0] = (BYTE)((op[0] & 0x80) | (exp >> 1));
	rop[1] = (BYTE)(((exp & 0x01) << 7) | (op[0] & 0x7F));
	rop[2] = op[1];
	rop[3] = (BYTE)(op[2] & 0xFE);

	byte_swap(rop, 4);
	memcpy(op, rop, 4);
                                                                         
	return (STATUS_OK);                                                   
} // conv_float32_to_ieee

//--------------------------------------------------------------------
// Convert IEEE float to T16 64-bit float.                            
//--------------------------------------------------------------------
short ODBC::conv_ieee_s_to_float32 (BYTE *op )
{                                                                       
	unsigned int	exp;          // exponent
	BYTE rop[4];

	byte_swap(op, 4);
	// Get exponent.
	exp = ((op[0] & 0x7F) << 1) | ((op[1] & 0x80) >> 7);
	if (exp == 0)
		return (STATUS_OK);                                                   
	exp = exp - BIAS_DIFFERENCE_S;	//	127 - 256

	rop[0] = (BYTE)((op[0] & 0x80) | (op[1] & 0x7F));
	rop[1] = (BYTE)(op[2]);
	rop[2] = (BYTE)((op[3] & 0xFE) | (exp >> 8));
	rop[3] = (BYTE)(exp);

	memcpy(op, rop, 4);
                                                                         
	return (STATUS_OK);                                                   
} // conv_ieee_s_to_float32

//--------------------------------------------------------------------
// Convert depending on datatype.
//--------------------------------------------------------------------
short ODBC::Datatype_Dependent_Swap(BYTE *source, long dataType, SQLINTEGER charSet, long source_len, BOOL tandem_ieee)
{

	short rst = STATUS_OK;

	switch (dataType)
	{
		case SQLTYPECODE_CHAR:
		case SQLTYPECODE_INTERVAL:
			if (charSet == SQLCHARSETCODE_UCS2)
				byte_swap_string(source, source_len);
//		case SQLTYPECODE_CHAR_UP:
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		case SQLTYPECODE_VARCHAR:
//		case SQLTYPECODE_VARCHAR_UP:
		case SQLTYPECODE_VARCHAR_LONG:
#ifndef unixcli
			byte_swap_2(source);
#endif
			if (charSet == SQLCHARSETCODE_UCS2)
				byte_swap_string(source+2,*(short *)source);
			byte_swap(source,2);
			break;
		case SQLTYPECODE_SMALLINT:
		case SQLTYPECODE_SMALLINT_UNSIGNED:
#ifndef unixcli
			byte_swap_2(source);
#else
			byte_swap(source,2);
#endif
			break;
		case SQLTYPECODE_INTEGER:
		case SQLTYPECODE_INTEGER_UNSIGNED:
#ifndef unixcli
			byte_swap_4(source);
#else
			byte_swap(source,4);
#endif
			break;
		case SQLTYPECODE_LARGEINT:
#ifndef unixcli
			byte_swap_8(source);
#else
			byte_swap(source, 8);
#endif
			break;
		case SQLTYPECODE_IEEE_REAL:
/*	Commented the following code since SQL/MX from 2.0 started supporting IEEE float.
			if (tandem_ieee == TANDEM_TO_IEEE)
				rst = conv_float32_to_ieee_s ((BYTE *)source);
			else
				rst = conv_ieee_s_to_float32 ((BYTE *)source);
*/
#ifndef unixcli
			byte_swap_4(source);
#else
			byte_swap(source,4);
#endif
			break;
    case SQLTYPECODE_IEEE_FLOAT:
	case SQLTYPECODE_IEEE_DOUBLE:
/*	Commented the following code since SQL/MX from 2.0 started supporting IEEE float.
			if (tandem_ieee == TANDEM_TO_IEEE)
				rst = conv_float64_to_ieee_d ((BYTE *)source);
			else
				rst = conv_ieee_d_to_float64 ((BYTE *)source);
*/
#ifndef unixcli
			byte_swap_8(source);
#else
			byte_swap(source,8);
#endif
			break;

		case SQLTYPECODE_DATETIME:
			break;
		case SQLTYPECODE_DECIMAL_UNSIGNED:
		case SQLTYPECODE_DECIMAL:
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
		case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
			break;
		case SQLTYPECODE_NUMERIC:              //2
		case SQLTYPECODE_NUMERIC_UNSIGNED:    //-201
			byte_swap_bignum(source, source_len);
			break;
		default:
			byte_swap(source, source_len);
			break;
	}
	return (rst);
}

#ifdef NSK_PLATFORM
short ODBC::Datatype_Dependent_Convert(BYTE *source, long dataType, long source_len, BOOL tandem_ieee)
{
	uint32 rst = NSK_FLOAT_OK;

/*	Commented the following code since SQL/MX from 2.0 started supporting IEEE float.

	NSK_float_ieee32	in_ieee32;
	NSK_float_ieee32	out_ieee32;
	NSK_float_ieee64	in_ieee64;
	NSK_float_ieee64	out_ieee64;

	NSK_float_tns32		in_tns32;
	NSK_float_tns32		out_tns32;
	NSK_float_tns64		in_tns64;
	NSK_float_tns64		out_tns64;

	switch (dataType)
	{
		case SQLTYPECODE_IEEE_REAL:
			if (tandem_ieee == TANDEM_TO_IEEE)
			{
// float32_to_ieee_s
				in_tns32 = *(NSK_float_tns32*)source;
				rst = NSK_FLOAT_TNS32_TO_IEEE32_((const NSK_float_tns32*)&in_tns32, &out_ieee32);
				*(NSK_float_ieee32*)source = out_ieee32;
			}
			else
			{
// ieee_s_to_float32
				in_ieee32 = *(NSK_float_ieee32*)source;
				rst = NSK_FLOAT_IEEE32_TO_TNS32_((const NSK_float_ieee32*)&in_ieee32, &out_tns32);
				*(NSK_float_tns32*)source = out_tns32;
			}
			break;
		case SQLTYPECODE_IEEE_FLOAT:
		case SQLTYPECODE_IEEE_DOUBLE:
			if (tandem_ieee == TANDEM_TO_IEEE)
			{
// float64_to_ieee_d
				in_tns64 = *(NSK_float_tns64*)source;
				rst = NSK_FLOAT_TNS64_TO_IEEE64_((const NSK_float_tns64*)&in_tns64, &out_ieee64);
				*(NSK_float_ieee64*)source = out_ieee64;
			}
			else
			{
// ieee_d_to_float64
				in_ieee64 = *(NSK_float_ieee64*)source;
				rst = NSK_FLOAT_IEEE64_TO_TNS64_((const NSK_float_ieee64*)&in_ieee64, &out_tns64);
				*(NSK_float_tns64*)source = out_tns64;
			}
			break;
		default:
			break;
	}
	if (rst & NSK_FLOAT_IEEE_OVERFLOW)
		rst = -1;
	else
		rst = 0;
*/
	return (rst);
}
#endif

void ODBC::SQLDatatype_Dependent_Swap(BYTE *source, long dataType, SQLINTEGER charSet, long dataLength, long DateTimeCode)
{

	switch (dataType)
	{
		case SQLTYPECODE_INTERVAL:
			break;
		case SQLTYPECODE_CHAR:
			if (charSet == SQLCHARSETCODE_UCS2)
				byte_swap_string(source,dataLength);
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		case SQLTYPECODE_VARCHAR:
		case SQLTYPECODE_VARCHAR_LONG:
#ifndef unixcli
			byte_swap_2(source);
#else
			byte_swap(source,2);
#endif
			if (charSet == SQLCHARSETCODE_UCS2)
				byte_swap_string(source+2,*(short *)source);
			break;
		case SQLTYPECODE_SMALLINT:
		case SQLTYPECODE_SMALLINT_UNSIGNED:
#ifndef unixcli
			byte_swap_2(source);
#else
			byte_swap(source,2);
#endif
			break;
		case SQLTYPECODE_INTEGER:
		case SQLTYPECODE_INTEGER_UNSIGNED:
#ifndef unixcli
			byte_swap_4(source);
#else
			byte_swap(source, 4);
#endif
			break;
		case SQLTYPECODE_LARGEINT:
#ifndef unixcli
			byte_swap_8(source);
#else
			byte_swap(source,8);
#endif
			break;
		case SQLTYPECODE_IEEE_REAL:
#ifndef unixcli
			byte_swap_4(source);
#else
			byte_swap(source,4);
#endif
			break;
		case SQLTYPECODE_IEEE_FLOAT:
		case SQLTYPECODE_IEEE_DOUBLE:
#ifndef unixcli
			byte_swap_8(source);
#else
			byte_swap(source,8);
#endif
			break;
		case SQLTYPECODE_DATETIME:
			switch (DateTimeCode)
			{
			case SQLDTCODE_DATE:
				byte_swap((BYTE *)&((DATE_TYPES *)source)->year, 2);
				break;
			case SQLDTCODE_TIME:
				if (dataLength == 7)
					byte_swap((BYTE *)&((TIME_TYPES *)source)->fraction, 4);
				break;
			case SQLDTCODE_TIMESTAMP:
				byte_swap((BYTE *)&((TIMESTAMP_TYPES *)source)->year, 2);
				if (dataLength == 11)
					byte_swap((BYTE *)&((TIMESTAMP_TYPES *)source)->fraction, 4);
				break;
			}
			break;
		case SQLTYPECODE_DECIMAL_UNSIGNED:
		case SQLTYPECODE_DECIMAL:
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
		case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
			break;
		case SQLTYPECODE_NUMERIC:              //2
		case SQLTYPECODE_NUMERIC_UNSIGNED:    //-201
			byte_swap_bignum(source, dataLength);
			break;
		default:
			break;
	}
}

#ifdef NSK_PLATFORM
short ODBC::SQLDatatype_Dependent_Convert(BYTE *source, long dataType, long DateTimeCode, BOOL tandem_ieee)
{

	uint32 rst = NSK_FLOAT_OK;

/*	Commented the following code since SQL/MX from 2.0 started supporting IEEE float.
	NSK_float_ieee32	in_ieee32;
	NSK_float_ieee32	out_ieee32;
	NSK_float_ieee64	in_ieee64;
	NSK_float_ieee64	out_ieee64;

	NSK_float_tns32		in_tns32;
	NSK_float_tns32		out_tns32;
	NSK_float_tns64		in_tns64;
	NSK_float_tns64		out_tns64;

	switch (dataType)
	{
		case SQLTYPECODE_IEEE_REAL:
			if (tandem_ieee == TANDEM_TO_IEEE)
			{
// float32_to_ieee_s
				in_tns32 = *(NSK_float_tns32*)source;
				rst = NSK_FLOAT_TNS32_TO_IEEE32_((const NSK_float_tns32*)&in_tns32, &out_ieee32);
				*(NSK_float_ieee32*)source = out_ieee32;
			}
			else
			{
// ieee_s_to_float32
				in_ieee32 = *(NSK_float_ieee32*)source;
				rst = NSK_FLOAT_IEEE32_TO_TNS32_((const NSK_float_ieee32*)&in_ieee32, &out_tns32);
				*(NSK_float_tns32*)source = out_tns32;
			}
			break;
		case SQLTYPECODE_IEEE_FLOAT:
		case SQLTYPECODE_IEEE_DOUBLE:
			if (tandem_ieee == TANDEM_TO_IEEE)
			{
// float64_to_ieee_d
				in_tns64 = *(NSK_float_tns64*)source;
				rst = NSK_FLOAT_TNS64_TO_IEEE64_((const NSK_float_tns64*)&in_tns64, &out_ieee64);
				*(NSK_float_ieee64*)source = out_ieee64;
			}
			else
			{
// ieee_d_to_float64
				in_ieee64 = *(NSK_float_ieee64*)source;
				rst = NSK_FLOAT_IEEE64_TO_TNS64_((const NSK_float_ieee64*)&in_ieee64, &out_tns64);
				*(NSK_float_tns64*)source = out_tns64;
			}
			break;
		default:
			break;
	}
	if (rst & NSK_FLOAT_IEEE_OVERFLOW)
		rst = -1;
	else
		rst = 0;
*/
	return (rst);
}
#endif

long ODBC::dataLength(SQLSMALLINT SQLDataType, SQLINTEGER SQLOctetLength, unsigned char* buffer)
{
	long allocLength = 0;
	switch (SQLDataType)
	{
	case SQLTYPECODE_CHAR:
	case SQLTYPECODE_BIT:
	case SQLTYPECODE_VARCHAR:
	case SQLTYPECODE_INTERVAL:
		allocLength = SQLOctetLength;
		break;
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
	case SQLTYPECODE_BITVAR:
		allocLength = *(USHORT *)buffer + 3;
		break;
	case SQLTYPECODE_SMALLINT:
	case SQLTYPECODE_SMALLINT_UNSIGNED:
	case SQLTYPECODE_INTEGER:
	case SQLTYPECODE_INTEGER_UNSIGNED:
	case SQLTYPECODE_LARGEINT:
	case SQLTYPECODE_IEEE_REAL:
	case SQLTYPECODE_IEEE_FLOAT:
	case SQLTYPECODE_IEEE_DOUBLE:
	case SQLTYPECODE_DATETIME:
	case SQLTYPECODE_DECIMAL_UNSIGNED:
	case SQLTYPECODE_DECIMAL:
	case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
	case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
	default:
		allocLength = SQLOctetLength;
		break;
	}
	return allocLength;
}

long ODBC::dataLength(SQLSMALLINT SQLDataType, SQLINTEGER SQLOctetLength, unsigned char* buffer, BOOL out)
{
	long allocLength = 0;
	switch (SQLDataType)
	{
	case SQLTYPECODE_CHAR:
	case SQLTYPECODE_BIT:
	case SQLTYPECODE_VARCHAR:
	case SQLTYPECODE_INTERVAL:
		allocLength = SQLOctetLength;
		break;
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
	case SQLTYPECODE_BITVAR:
		if (out == FALSE)
#ifdef MXHPUX
		{
			short length;
			memcpy(&length, buffer, sizeof(USHORT));
			allocLength = length + 2;
		}
#else
		{
			allocLength = *(USHORT *)buffer + 2;
		}
#endif
		else
		{
			short length;
#ifdef MXHPUX
			memcpy(&length, buffer, sizeof(USHORT));
#else
			length = *(USHORT *)buffer;
#endif

#ifndef unixcli
			byte_swap_2((unsigned char*)&length);
			allocLength = length + 2;
#else
			byte_swap((unsigned char*)&length, 2);
			//allocLength = SQLOctetLength + 2;
			allocLength = length + 2;
#endif
		}
		break;
	case SQLTYPECODE_SMALLINT:
	case SQLTYPECODE_SMALLINT_UNSIGNED:
	case SQLTYPECODE_INTEGER:
	case SQLTYPECODE_INTEGER_UNSIGNED:
	case SQLTYPECODE_LARGEINT:
	case SQLTYPECODE_IEEE_REAL:
	case SQLTYPECODE_IEEE_FLOAT:
	case SQLTYPECODE_IEEE_DOUBLE:
	case SQLTYPECODE_DATETIME:
	case SQLTYPECODE_DECIMAL_UNSIGNED:
	case SQLTYPECODE_DECIMAL:
	case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
	case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
	default:
		allocLength = SQLOctetLength;
		break;
	}
	return allocLength;
}

long ODBC::dataLengthFetchRowset(SQLSMALLINT SQLDataType, SQLINTEGER SQLOctetLength, SQLINTEGER maxRowLen, unsigned char* buffer)
{
	long allocLength = 0;
	long maxRowLength = maxRowLen;
	switch (SQLDataType)
	{
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
	case SQLTYPECODE_BITVAR:
#ifdef MXHPUX
		short temp;
		memcpy(&temp, buffer, sizeof(USHORT));
		allocLength = temp + 2;
#else
		allocLength = *(USHORT *)buffer + 2;
#endif
		break;
	case SQLTYPECODE_CHAR:
	case SQLTYPECODE_BIT:
	case SQLTYPECODE_VARCHAR:
		allocLength = SQLOctetLength - 1;
		if (maxRowLength > 0)
			allocLength = (allocLength>maxRowLength)?maxRowLength:allocLength;
		break;
	default:
		allocLength = SQLOctetLength;
		break;
	}
	return allocLength;
}

long ODBC::dataLengthFetchPerf(SQLSMALLINT SQLDataType, SQLINTEGER SQLOctetLength, SQLINTEGER maxRowLen, unsigned char* buffer)
{
	long allocLength = 0;
	long maxRowLength = maxRowLen;
	switch (SQLDataType)
	{
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
	case SQLTYPECODE_BITVAR:
		allocLength = *(USHORT *)buffer + 3;
		break;
	case SQLTYPECODE_CHAR:
	case SQLTYPECODE_BIT:
	case SQLTYPECODE_VARCHAR:
		allocLength = SQLOctetLength;
		if (maxRowLength > 0)
			allocLength = (allocLength>maxRowLength)?maxRowLength + 1:allocLength;
		break;
	default:
		allocLength = SQLOctetLength;
		break;
	}
	return allocLength;
}

long ODBC::adjustIndexForBulkFetch(SQLSMALLINT SQLDataType, long index)
{
	long totalLength = index;
	switch (SQLDataType)
	{
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
	case SQLTYPECODE_VARCHAR_LONG:
		totalLength = ((totalLength + 2 - 1) >> 1) << 1; 
		break;
	case SQLTYPECODE_SMALLINT:
	case SQLTYPECODE_SMALLINT_UNSIGNED:
		totalLength = ((totalLength + 2 - 1) >> 1) << 1; 
		break;
	case SQLTYPECODE_INTEGER:
	case SQLTYPECODE_INTEGER_UNSIGNED:
		totalLength = ((totalLength + 4 - 1) >> 2) << 2; 
		break;
	case SQLTYPECODE_LARGEINT:
	case SQLTYPECODE_IEEE_REAL:
	case SQLTYPECODE_IEEE_FLOAT:
	case SQLTYPECODE_IEEE_DOUBLE:
	case SQLTYPECODE_DATETIME:
		totalLength = ((totalLength + 8 - 1) >> 3) << 3; 
		break;
	case SQLTYPECODE_DECIMAL_UNSIGNED:
	case SQLTYPECODE_DECIMAL:
	case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
	case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
	case SQLTYPECODE_INTERVAL:		// Treating as CHAR
		break;
	default:
		break;
	}
	return totalLength;
}

void ODBC::byte_swap_bignum(BYTE *source, long source_len)
{
#if !defined (NSK_PLATFORM)  || !defined (BIGE) 
  unsigned short   *target;                                                                                              
	int   i; 
	unsigned short *tmp;

	target = (unsigned short *)source;

	for (i = source_len /2; i > 0; i--)
	{ 
		tmp = target;
		
		if (*tmp != 0 && *tmp != 0xFFFF)
		{
			union
			{
				char c[2];
				unsigned short j;
			} u;
			char hold;
			u.j = *tmp;
			hold = u.c[0];
			u.c[0] = u.c[1];
			u.c[1] = hold;
			*tmp = u.j;
		}
		*target++ = *tmp;                   
	} 
#endif
}
