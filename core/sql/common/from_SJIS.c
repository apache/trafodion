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
 * $Log: from_SJIS.c,v $
 * Revision 1.1.14.1  2001/12/07  15:37:41
 *  * Enable __UDC_to_sjis() to return 0 for conversion table lookup.
 *  * Merge codes from kernel/bsd.
 *
 * Revision 1.1.10.1  2000/01/13  20:25:32
 * 	Remove the old conversion routine in favor of the enhanced
 * 	__<codeset>_index() routine, and add algorithmic UDC conversion
 * 	support. Also add a number of conversion routines to support
 * 	special font charset to Unicode conversion.
 *
 * Revision 1.1.8.2  1998/03/26  18:55:46
 * 	Prepend __ to global functions to prevent namespace pollution
 * 	and add __<codeset>__index function for use by UCS-4 locale method.
 * 	[1998/03/23  14:42:08  Long_Man]
 *
 * Revision 1.1.6.2  1996/11/22  17:02:14
 * 	Improve execution speed & other minor bug fixes.
 * 	[1996/11/12  17:18:45  Long_Man]
 *
 * 	Consolidate UCS iconv converter binaries & support UCS-2.
 * 	[1996/10/28  21:10:56  Long_Man]
 *
 * Revision 1.1.2.3  1995/06/30  14:44:48
 * 	Fix pointer and length settings for E2BIG errors
 * 	[1995/06/30  13:31:15  Bill_Fountas]
 *
 * Revision 1.1.2.2  1995/06/07  16:12:16
 * 	Initial iconv Unicode support check in
 * 	[1995/06/02  14:58:14  Kelly_Mulheren]
 *
 * $EndLog$
 */

#include "fcconv.h"
#include "multi-byte.h"

/*
 * Mapping tables between SJIS 2nd byte code and index code
 */
static const short sjis_to_idx[] =
{
   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  /* 0x00-0x07 */
   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  /* 0x08-0x0f */
   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  /* 0x10-0x17 */
   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  /* 0x18-0x1f */
   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  /* 0x20-0x27 */
   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  /* 0x28-0x2f */
   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  /* 0x30-0x37 */
   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  /* 0x38-0x3f */
    0,   1,   2,   3,   4,   5,   6,   7,  /* 0x40-0x47 */
    8,   9,  10,  11,  12,  13,  14,  15,  /* 0x48-0x4f */
   16,  17,  18,  19,  20,  21,  22,  23,  /* 0x50-0x57 */
   24,  25,  26,  27,  28,  29,  30,  31,  /* 0x58-0x5f */
   32,  33,  34,  35,  36,  37,  38,  39,  /* 0x60-0x67 */
   40,  41,  42,  43,  44,  45,  46,  47,  /* 0x68-0x6f */
   48,  49,  50,  51,  52,  53,  54,  55,  /* 0x70-0x77 */
   56,  57,  58,  59,  60,  61,  62,  -1,  /* 0x78-0x7f */
   63,  64,  65,  66,  67,  68,  69,  70,  /* 0x80-0x87 */
   71,  72,  73,  74,  75,  76,  77,  78,  /* 0x88-0x8f */
   79,  80,  81,  82,  83,  84,  85,  86,  /* 0x90-0x97 */
   87,  88,  89,  90,  91,  92,  93,  94,  /* 0x98-0x9f */
   95,  96,  97,  98,  99, 100, 101, 102,  /* 0xa0-0xa7 */
  103, 104, 105, 106, 107, 108, 109, 110,  /* 0xa8-0xaf */
  111, 112, 113, 114, 115, 116, 117, 118,  /* 0xb0-0xb7 */
  119, 120, 121, 122, 123, 124, 125, 126,  /* 0xb8-0xbf */
  127, 128, 129, 130, 131, 132, 133, 134,  /* 0xc0-0xc7 */
  135, 136, 137, 138, 139, 140, 141, 142,  /* 0xc8-0xcf */
  143, 144, 145, 146, 147, 148, 149, 150,  /* 0xd0-0xd7 */
  151, 152, 153, 154, 155, 156, 157, 158,  /* 0xd8-0xdf */
  159, 160, 161, 162, 163, 164, 165, 166,  /* 0xe0-0xe7 */
  167, 168, 169, 170, 171, 172, 173, 174,  /* 0xe8-0xef */
  175, 176, 177, 178, 179, 180, 181, 182,  /* 0xf0-0xf7 */
  183, 184, 185, 186, 187,  -1,  -1,  -1   /* 0xf8-0xff */
} ;

static const uchar_t idx_to_sjis[] =
{
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
  0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x80,
  0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
  0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90,
  0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
  0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0,
  0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
  0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0,
  0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8,
  0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0,
  0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8,
  0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
  0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
  0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0,
  0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
  0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa, 0xfb, 0xfc,
} ;

/*
 * This routine converts one SJIS character from the input stream to table
 * index.
 *
 * Return Value:
 *	Table index if no error
 *	-1 - Invalid sequence (EILSEQ)
 *	-2 - Input incomplete
 */
int
__sjis_index(_LC_fcconv_iconv_t *cd, uchar_t **in, int len)
{
    uint   ch1, ch2 ;
    uchar *ip 	    ;

    ip  = *in   ;
    ch1 = *ip++ ;
    if (SJIS_PLANE0(ch1))
    {
	*in = ip    ;
	return(SJIS_IDX0(ch1)) ;
    }

    if (!SJIS_VALID1(ch1))
	return(ERR_INVALID_CHAR) ;
    if (len < 2)
	return(ERR_INPUT_INCOMPLETE) ;
    ch2 = *ip++ ;
    if (!SJIS_VALID2(ch2))
	return(ERR_INVALID_CHAR) ;

    *in = ip ;	/* Adjust input pointer */
    return(SJIS_PLANE1(ch1) ?  SJIS_IDX1(ch1, ch2) :
	   SJIS_PLANE2(ch1) ?  SJIS_IDX2(ch1, ch2) :
	/* SJIS_PLANEU(ch1) */ SJIS_IDXU(ch1, ch2)) ;
}

/*
 * This routine map UDC characters in Unicode to those in SJIS
 */
WChar_t // JAC
__UDC_to_sjis(ucs4_t ucs)
{
    int uidx = UCS_UIDX(ucs) ;
    return((uidx < SJIS_UCNT) ? UIDX_SJIS(uidx): 0)  ;
}

#ifdef	GEN_TABLE
const uint __cvtab[] = { 0 } ;
/*
 * The program below is used to generate the mapping tables between
 * SJIS code and index value.
 */
main()
{
    int idx ;
    int val ;
    int cnt ;

    /*
     * SJIS second byte code to index mapping table
     * 2nd byte : 0x40-0x7e, 0x80-0xfc
     */
    printf("static const short sjis_to_idx[] =\n{\n") ;
    for (idx = cnt = 0 ; idx <= 0xff ; idx++)
    {
	if (((0x40 <= idx) && (idx <= 0x7e)) ||
	    ((0x80 <= idx) && (idx <= 0xfc)))
	    val = cnt++ ;
	else
	    val = -1 ;
	if ((idx & 0x7) == 0)
	    printf("  ") ;
	printf((idx != 0xff) ? "%3d, " : "%3d  ", val) ;
	if ((idx & 0x7) == 0x7)
	    printf(" /* 0x%02x-0x%02x */\n", idx & ~7, idx) ;
    }
    printf("} ;\n\n") ;

    /*
     * SJIS index to second byte code mapping table
     */
    printf("static const uchar_t idx_to_sjis[] =\n{\n") ;
    for (idx = cnt = 0 ; idx <= 0xff ; idx++)
    {
	if (!(((0x40 <= idx) && (idx <= 0x7e)) ||
	      ((0x80 <= idx) && (idx <= 0xfc))))
	    continue ;
	if ((cnt & 0x07) == 0)
	    printf("  ") ;
	printf("0x%02x, ", idx) ;
	if ((cnt & 0x7) == 0x7)
	    printf("\n") ;
	cnt++ ;
    }
    printf("\n} ;\n") ;
}
#endif	/* GEN_TABLE */
