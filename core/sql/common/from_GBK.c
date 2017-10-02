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
 * $Log: from_GBK.c,v $
 * Revision 1.1.12.1  2001/12/07  15:37:40
 *  * Add __UDC_to_gbk() to map UDC to GBK code.
 *
 * Revision 1.1.8.1  2000/12/12  14:16:19
 * 	Add UDC checking code in GBK to UCS conversion routine.
 *
 * Revision 1.1.6.1  2000/01/13  20:25:31
 * 	Remove the old conversion routine in favor of the enhanced
 * 	__<codeset>_index() routine, and add algorithmic UDC conversion
 * 	support. Also add a number of conversion routines to support
 * 	special font charset to Unicode conversion.
 *
 * Revision 1.1.4.1  1999/09/28  21:10:12
 * 	Cosixsteelos to zincos merge.
 *
 * Revision 1.1.2.2  1999/08/18  18:36:12
 * 	Add support for Unicode to GBK/GB2312 conversion.
 * 	[1999/08/18  15:04:47  Long_Man]
 *
 * $EndLog$
 */

#include "fcconv.h"
#include "multi-byte.h"

/*
 * This routine converts one GBK character from the input stream to table
 * index.
 *
 * Return Value:
 *	Table index if no error
 *	-1 - Invalid sequence (EILSEQ)
 *	-2 - Input incomplete
 */
int
__gbk_index(_LC_fcconv_iconv_t * cd, uchar_t **in, int len)
{
    uint   ch1, ch2 ;
    uchar *ip 	    ;

    if (len < 2)
	return(ERR_INPUT_INCOMPLETE) ;

    ip  = *in   ;
    ch1 = *ip++ ;
    ch2 = *ip++ ;

    if (!GBK_VALID1(ch1) || !GBK_VALID2(ch2))
	return(ERR_INVALID_CHAR) ;

    *in = ip ;	/* Adjust input pointer */
    if (GBK_PLANE1(ch1, ch2))
    {
	if (GBK_UDC11(ch1, ch2) || GBK_UDC12(ch1, ch2))
	    return(GBK_IDXU1(ch1, ch2)) ;
	return(GBK_IDX1(ch1, ch2)) ;
    }
    else if (GBK_PLANE2(ch1, ch2))
    {
	return(GBK_IDX2(ch1, ch2)) ;
    }
    else /* GBK_PLANE3(ch1, ch2) */
    {
	if (GBK_UDC3(ch1, ch2))
	    return(GBK_IDXU3(ch1, ch2)) ;
	return(GBK_IDX3(ch1, ch2)) ;
    }
}

/*
 * This routine performs the conversion from a GBK code to GBK:GLGR
 * code.
 */
int
__gbk_to_gbkglgr(WChar_t code)
{
    if (code > 0xff)
	return(code) ;
    return(BAD) ;
}

/*
 * This routine maps UDC characters in Unicode to those in GBK
 */
WChar_t
__UDC_to_gbk(ucs4_t ucs)
{
    int  uidx = UCS_UIDX(ucs) ;
#if 0 /* Unused - and our C++ compiler complains */
    long gb4		      ;
    int  gb4idx		      ;
#endif

    if (uidx < GBK_UCNT)
	return(UIDX_GBK(uidx)) ;
    /*
     * Perform table lookup for other BMP UDCs
     */
    return((uidx >= UCS_UCNT) ? BAD : 0) ;
}

