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
 * HISTORY
 * $Log: from_ksc5601.c,v $
 * Revision 1.1.10.1  2001/12/07  15:37:50
 *  * Enable mapping of all unassigned codepoints to UDC.
 *
 * Revision 1.1.6.1  2000/01/13  20:25:42
 * 	Remove the old conversion routine in favor of the enhanced
 * 	__<codeset>_index() routine, and add algorithmic UDC conversion
 * 	support. Also add a number of conversion routines to support
 * 	special font charset to Unicode conversion.
 *
 * Revision 1.1.4.2  1998/03/26  18:56:18
 * 	Prepend __ to global functions to prevent namespace pollution
 * 	and add __<codeset>__index function for use by UCS-4 locale method.
 * 	[1998/03/23  14:42:31  Long_Man]
 *
 * Revision 1.1.2.2  1997/03/07  18:54:07
 * 	New multibyte to table index converter for KSC 5601-1992.
 * 	[1997/03/06  20:19:34  Long_Man]
 *
 * $EndLog$
 */

#include "fcconv.h"
#include "multi-byte.h"

/*
 * This routine converts one ksc 5601-1992 (cp949) character from the input
 * stream to table index.
 *
 * Return Value:
 *	Table index if no error
 *	-1 - Invalid sequence (EILSEQ)
 *	-2 - Input incomplete
 */
int
__cp949_index(_LC_fcconv_iconv_t *cd, uchar_t **in, int len)
{
    uint   ch1, ch2 ;
    uchar *ip = *in ;
    int	   index    ;

    if (len < 2)
	return(ERR_INPUT_INCOMPLETE) ;

    ch1 = *ip++ ;
    ch2 = *ip++ ;
    if (KSC5601_1_VALID1(ch1) && KSC5601_1_VALID2(ch2))
	index = KSC5601_IDX1(ch1, ch2) ;
    else if (KSC5601_2_VALID1(ch1) && KSC5601_2_VALID2(ch2))
	index = KSC5601_IDX2(ch1, ch2) ;
    else if (KSC5601_3_VALID1(ch1) && KSC5601_3_VALID2(ch2))
	index = KSC5601_IDX3(ch1, ch2) ;
    else
	return(ERR_INVALID_CHAR) ;

    *in = ip	  ;
    return(index) ;
}

/*
 * This routine enables table lookup of BMP UDC characters to cp949
 */
WChar_t  // JAC
__UDC_to_cp949(ucs4_t ucs)
{
    if (ucs <= UCS_UDC_END)
	return(0) ;
    return(BAD) ;
}

