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
 * $Log: from_GB2312.c,v $
 * Revision 1.1.10.1  2001/12/07  15:37:39
 *  * Check for UDC & add __UDC_to_gb2312().
 *
 * Revision 1.1.6.1  2000/01/13  20:25:30
 * 	Remove the old conversion routine in favor of the enhanced
 * 	__<codeset>_index() routine, and add algorithmic UDC conversion
 * 	support. Also add a number of conversion routines to support
 * 	special font charset to Unicode conversion.
 *
 * Revision 1.1.4.1  1999/09/28  21:10:10
 * 	Cosixsteelos to zincos merge.
 *
 * Revision 1.1.2.2  1999/08/18  18:36:08
 * 	Add support for Unicode to GBK/GB2312 conversion.
 * 	[1999/08/18  15:04:43  Long_Man]
 *
 * $EndLog$
 */

/*
 * COPYRIGHT NOTICE
 *
 * Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 * ALL RIGHTS RESERVED (OSF/1).  See /usr/include/COPYRIGHT.OSF1 .
 */

#include "fcconv.h"
#include "multi-byte.h"

/*
 * This routine converts one GB2312 character from the input stream to table
 * index.
 *
 * Return Value:
 *	Table index if no error
 *	-1 - Invalid sequence (EILSEQ)
 *	-2 - Input incomplete
 */
#if 0 // Don't need these since we chose to support GBK (a superset of GB2312) instead
NA_EIDPROC
int
__gb2312_index(_LC_fcconv_iconv_t *cd, uchar_t **in, int len)
{
    uint   ch1, ch2 ;
    uchar *ip = *in ;

    if (len < 2)
	return(ERR_INPUT_INCOMPLETE) ;

    ch1 = *ip++ ;
    ch2 = *ip++ ;

    if (!GB2312_VALID1(ch1) || !GB2312_VALID2(ch2))
	return(ERR_INVALID_CHAR) ;

    *in = ip ;	/* Adjust input pointer */
    return((GB2312_UDC11(ch1, ch2) || GB2312_UDC12(ch1, ch2))
	   ? GB2312_IDXU(ch1, ch2) : GB2312_IDX(ch1, ch2)) ;
}

/*
 * This routine convert a GB2312-GL character to table index
 */
NA_EIDPROC
int
__gb2312gl_index(_LC_fcconv_iconv_t *cd, uchar_t **in, int len)
{
    uint   ch1, ch2 ;
    uchar *ip = *in ;

    if (len < 2)
	return(ERR_INPUT_INCOMPLETE) ;

    ch1 = *ip++ ;
    ch2 = *ip++ ;

    if ((ch1 >= 0x80) || (ch2 >= 0x80))
	return(ERR_INVALID_CHAR) ;
    ch1 |= 0x80 ;
    ch2 |= 0x80 ;

    if (!GB2312_VALID1(ch1) || !GB2312_VALID2(ch2))
	return(ERR_INVALID_CHAR) ;

    *in = ip ;	/* Adjust input pointer */
    return(GB2312_IDX(ch1, ch2)) ;
}

/*
 * This routine maps the UCS UDC codepoint to GB2312
 */
NA_EIDPROC
WChar_t __UDC_to_gb2312(ucs4_t udc) //JAC
{
    if (udc <= UCS_UDC_START + GB2312_UCNT) {
	int uidx = UCS_UIDX(udc)  ;
	return(UIDX_GB2312(uidx)) ;
    }
    /* Lookup table for mapping for BMP UDC */
    return((udc <= UCS_UDC_END) ? 0 : BAD) ;
}

/*---------------------------------------------------------------------------*/

/*
 * This routine performs the conversion from a GB2312 code to GB2312-GL
 * code.
 */
NA_EIDPROC
int
__gb2312_to_gb2312gl(WChar_t code) //JAC
{
    if (code > 0xff)
	return(code & 0x7f7f) ;
    return(BAD) ;
}

/*
 * This routine performs the conversion from a GB2312 code to GB2312-GR
 * code.
 */
NA_EIDPROC
int
__gb2312_to_gb2312gr(WChar_t code) //JAC
{
    if (code > 0xff)
	return(code) ;
    return(BAD) ;
}
#endif // Don't need these since we chose to support GBK (a superset of GB2312) instead
