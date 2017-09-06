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
/*
 * COPYRIGHT NOTICE
 * 
 * Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 * ALL RIGHTS RESERVED (OSF/1).  See /usr/include/COPYRIGHT.OSF1 .
 */

#include <sys/types.h>

/*
 * Macros
 */
#define MULTIBYTE	0x80
#define SS2		0x8e
#define SS3		0x8f

#define PLANE1		0x00200
#define PLANE2		(PLANE1+PLANE_SIZE)
#define PLANE3		(PLANE2+PLANE_SIZE)
#define PLANE4		(PLANE3+PLANE_SIZE)
#define PLANE5		(PLANE4+PLANE_SIZE)
#define PLANE6		(PLANE5+PLANE_SIZE)
#define PLANE7		(PLANE6+PLANE_SIZE)
#define PLANE8		-1		/* Not used */
#define PLANE9		-1		/* Not used */
#define PLANE10		-1		/* Not used */
#define PLANE11		-1		/* Not used */
#define PLANE12		-1		/* Not used */
#define PLANE13		-1		/* Not used */
#define PLANE14		-1		/* Not used */
#define PLANE15		(PLANE7 +PLANE_SIZE)
#define PLANE16		-1		/* Not used */
#define	PLANEU		(PLANE15+PLANE_SIZE)	/* UDC plane   */
#define	PLANEXU		(PLANEU +UCS_UCNT  )	/* XUDC planes */
#define	PLANE_SIZE	0x02300
#define	MAX_TIDX	(PLANEU+PLANE_SIZE)	/* Maximum table index value */

//#define BAD		-1

/*
 * Check for C1 control characters
 */
#define	ISC1CHAR(c)  ((0x80 <= (c)) && ((c) <= 0x9f))


/*
 * The following macros are used to detect if a certain byte is a valid 
 * first or second byte of a multibyte character. A table lookup mechanism
 * is used for efficient access of valid character information. The cvtab[]
 * table can be found in the mb_iconv.c module.
 *
 * Range bit :
 *	Bit  0 - 0x21 to 0x3f
 *	Bit  1 - 0x40
 *	Bit  2 - 0x41 to 0x5a
 *	Bit  3 - 0x5b to 0x60
 *	Bit  4 - 0x61 to 0x7a
 *	Bit  5 - 0x7b to 0x7d
 *	Bit  6 - 0x7e
 *	Bit  7 - 0x80
 *	Bit  8 - 0x81 to 0x8d
 *	Bit  9 - 0x8e to 0x9d
 *	Bit 10 - 0x9e
 *	Bit 11 - 0x9f
 *	Bit 12 - 0xa0
 *	Bit 13 - 0xa1 to 0xc6
 *	Bit 14 - 0xc7 to 0xdf
 *	Bit 15 - 0xe0 to 0xf9
 *	Bit 16 - 0xfa to 0xfc
 *	Bit 17 - 0xfd
 *	Bit 18 - 0xfe
 *	Bit 19 - 0x30 to 0x39
 */
#define	VBIT_21TO3F		0x00001
#define	VBIT_40			0x00002
#define	VBIT_41TO5A		0x00004
#define	VBIT_5BTO60		0x00008
#define	VBIT_61TO7A		0x00010
#define	VBIT_7BTO7D		0x00020
#define	VBIT_7E			0x00040
#define	VBIT_80			0x00080
#define	VBIT_81TO8D		0x00100
#define	VBIT_8ETO9D		0x00200
#define	VBIT_9E			0x00400
#define	VBIT_9F			0x00800
#define	VBIT_A0			0x01000
#define	VBIT_A1TOC6		0x02000
#define	VBIT_C7TODF		0x04000
#define	VBIT_E0TOF9		0x08000
#define	VBIT_FATOFC		0x10000
#define	VBIT_FD			0x20000
#define	VBIT_FE			0x40000
#define	VBIT_30TO39		0x80000

#define	RANGE_30TO39		(VBIT_30TO39 )
#define	RANGE_81TO8D		(VBIT_81TO8D )
#define	RANGE_A0TOC6		(VBIT_A0     | VBIT_A1TOC6 )
#define	RANGE_A1TODF		(VBIT_A1TOC6 | VBIT_C7TODF)
#define	RANGE_40TO7E		(VBIT_40     | VBIT_41TO5A | VBIT_5BTO60 | \
				 VBIT_61TO7A | VBIT_7BTO7D | VBIT_7E )
#define	RANGE_81TO9F		(VBIT_81TO8D | VBIT_8ETO9D | VBIT_9E | VBIT_9F)
#define	RANGE_8ETOA0		(VBIT_8ETO9D | VBIT_9E     | VBIT_9F | VBIT_A0)
#define	RANGE_21TO7E		(VBIT_21TO3F | RANGE_40TO7E)
#define	RANGE_80TOA0		(RANGE_81TO9F| VBIT_80     | VBIT_A0     )
#define	RANGE_81TOA0		(RANGE_81TO9F| VBIT_A0	   )
#define	RANGE_A1TOFC		(RANGE_A1TODF| VBIT_E0TOF9 | VBIT_FATOFC )
#define	RANGE_A1TOFE		(RANGE_A1TOFC| VBIT_FD     | VBIT_FE     )

#define	RANGE_80TOFC		(VBIT_80     | RANGE_81TOA0| RANGE_A1TOFC)
#define	RANGE_A1TOF9		(RANGE_A1TODF| VBIT_E0TOF9 )
#define	RANGE_E0TOFC		(VBIT_E0TOF9 | VBIT_FATOFC )
#define	RANGE_FATOFE		(VBIT_FATOFC | VBIT_FD     | VBIT_FE     )
#define	RANGE_81TO9F_E0TOFC	(RANGE_81TO9F| VBIT_E0TOF9 | VBIT_FATOFC )
#define	RANGE_21TO7E_A1TOFE	(RANGE_21TO7E| RANGE_A1TOFE)
#define	RANGE_81TOFE		(RANGE_81TOA0| RANGE_A1TOFE)
#define RANGE_40TO7E_80TOA0	(RANGE_40TO7E| RANGE_80TOA0)
#define	RANGE_40TO7E_80TOFC	(RANGE_40TO7E| RANGE_80TOFC)
#define	RANGE_40TO7E_80TOFE	(RANGE_40TO7E| RANGE_80TOFC | VBIT_FD | VBIT_FE)
#define	RANGE_40TO7E_A1TOFE	(RANGE_40TO7E| RANGE_A1TOFE)
#define	RANGE_81TOA0_FATOFE	(RANGE_81TOA0| RANGE_FATOFE)
#define	RANGE_41TO5A_61TO7A_81TOFE	\
				(VBIT_41TO5A | VBIT_61TO7A | RANGE_81TOFE)
#define	RANGE_41TO5A_61TO7A_81TOA0	\
				(VBIT_41TO5A | VBIT_61TO7A | RANGE_81TOA0)

#define	VBITS_EUCTW1		RANGE_A1TOFE
#define	VBITS_EUCTW2		RANGE_A1TOFE
#define	VBITS_DECHANYU1		RANGE_A1TOFE
#define	VBITS_DECHANYU2		RANGE_21TO7E_A1TOFE
#define	VBITS_EDPC1		RANGE_A1TOFE
#define	VBITS_EDPC2		RANGE_A1TOFE
#define	VBITS_DECHANZI1		RANGE_A1TOFE
#define	VBITS_DECHANZI2		RANGE_21TO7E_A1TOFE
#define	VBITS_DECKOREAN1	RANGE_A1TOFE
#define	VBITS_DECKOREAN2	RANGE_21TO7E_A1TOFE
#define	VBITS_BIG5_1		RANGE_81TOFE
#define	VBITS_BIG5_2		RANGE_40TO7E_A1TOFE
#define	VBITS_KANA		RANGE_A1TODF
#define	VBITS_DECKANJI1		RANGE_A1TOFE
#define	VBITS_DECKANJI2		RANGE_21TO7E_A1TOFE
#define	VBITS_EUCJP1		RANGE_A1TOFE
#define	VBITS_EUCJP2		RANGE_A1TOFE
#define	VBITS_SJIS1		RANGE_81TO9F_E0TOFC
#define	VBITS_SJIS2		RANGE_40TO7E_80TOFC
#define	VBITS_JIS1		RANGE_21TO7E
#define	VBITS_JIS2		RANGE_21TO7E
#define	VBITS_GBK1		RANGE_81TOFE
#define	VBITS_GBK2		RANGE_40TO7E_80TOFE

#define	VALID_EUCTW1(c)		(__cvtab[(unsigned)(c)] & VBITS_EUCTW1	  )
#define	VALID_EUCTW2(c)		(__cvtab[(unsigned)(c)] & VBITS_EUCTW2	  )
#define	VALID_DECHANYU1(c)	(__cvtab[(unsigned)(c)] & VBITS_DECHANYU1 )
#define	VALID_DECHANYU2(c)	(__cvtab[(unsigned)(c)] & VBITS_DECHANYU2 )
#define	VALID_EDPC1(c)		(__cvtab[(unsigned)(c)] & VBITS_EDPC1	  )
#define	VALID_EDPC2(c)		(__cvtab[(unsigned)(c)] & VBITS_EDPC2	  )
#define	VALID_DECHANZI1(c)	(__cvtab[(unsigned)(c)] & VBITS_DECHANZI1 )
#define	VALID_DECHANZI2(c)	(__cvtab[(unsigned)(c)] & VBITS_DECHANZI2 )
#define	VALID_DECKOREAN1(c)	(__cvtab[(unsigned)(c)] & VBITS_DECKOREAN1)
#define	VALID_DECKOREAN2(c)	(__cvtab[(unsigned)(c)] & VBITS_DECKOREAN2)
#define	VALID_KANA(c)		(__cvtab[(unsigned)(c)] & VBITS_KANA	  )
#define	VALID_DECKANJI1(c)	(__cvtab[(unsigned)(c)] & VBITS_DECKANJI1 )
#define	VALID_DECKANJI2(c)	(__cvtab[(unsigned)(c)] & VBITS_DECKANJI2 )
#define	VALID_EUCJP1(c)		(__cvtab[(unsigned)(c)] & VBITS_EUCJP1    )
#define	VALID_EUCJP2(c)		(__cvtab[(unsigned)(c)] & VBITS_EUCJP2    )
#define	VALID_JIS1(c)		(__cvtab[(unsigned)(c)] & VBITS_JIS1      )
#define	VALID_JIS2(c)		(__cvtab[(unsigned)(c)] & VBITS_JIS2      )
#define	VALID_GBK1(c)		(__cvtab[(unsigned)(c)] & VBITS_GBK1      )
#define	VALID_GBK2(c)		(__cvtab[(unsigned)(c)] & VBITS_GBK2      )
#define	VALID_EUCTWPLANE(c)	(__cvtab[(unsigned)(c)] & VBIT_EUCTWPLANE )

/*
 * SJIS related macros
 *
 * Valid SJIS first  byte: 0x81 - 0x9f, 0xe0 - 0xfc
 * Valid SJIS second byte: 0x40 - 0x7e, 0x80 - 0xfc
 * 
 * SJIS plane 0: 0xa1 <= 1st byte <= 0xdf	(JIS X 0201 - single byte)
 * SJIS plane 1: 0x81 <= 1st byte <= 0x9f	(JIS X 0208)
 * SJIS plane 2: 0xe0 <= 1st byte <= 0xea	(JIS X 0208)
 * SJIS plane U: 0xf0 <= 1st byte <= 0xfc	(UDC)
 *		 0xeb <= 1st byte <= 0xef
 *
 * The SJIS_ROW*, SJIS_COL & SJIS_IDX* macros are used to compute index
 * for accessing the cell table. The input values must be range checked
 * before using those macros.
 *
 * All unmapped codepoints will now map to the BMP UDC area.
 */
#define	SJIS_VALID1(c)	  (__cvtab[(unsigned)(c)] & VBITS_SJIS1 )
#define	SJIS_VALID2(c)	  (__cvtab[(unsigned)(c)] & VBITS_SJIS2 )
#define	SJIS_PLANE0(c)	  (__cvtab[(unsigned)(c)] & VBITS_KANA  )
#define	SJIS_PLANE1(c)	  (__cvtab[(unsigned)(c)] & RANGE_81TO9F)
#define	SJIS_PLANE2(c)	 ((__cvtab[(unsigned)(c)] & RANGE_E0TOFC) && \
			 ((unsigned)(c) <= 0xea))
#define	SJIS_PLANEU(c)	 ((__cvtab[(unsigned)(c)] & RANGE_E0TOFC) && \
			 ((c) >=  0xeb))
#define	SJIS_ROW1(c)	 ((c) - 0x81)
#define	SJIS_ROW2(c)	 ((c) - 0xe0)
#define	SJIS_ROW3(c)	 (((c) >= 0xf0) ? ((c) - 0xf0) : \
			  ((c)  - 0xeb + (0xfc - 0xf0 + 1)))
#define	SJIS_COL(c)	 sjis_to_idx[c]
#define	SJIS_COLSIZ	 (SJIS_COL(0xfc) + 1)
#define	SJIS_IDX0(c)	 ((c) + 0x100)
#define	SJIS_IDX1(c1,c2) (PLANE1 + SJIS_ROW1(c1) * SJIS_COLSIZ + SJIS_COL(c2))
#define	SJIS_IDX2(c1,c2) (PLANE2 + SJIS_ROW2(c1) * SJIS_COLSIZ + SJIS_COL(c2))
#define	SJIS_IDXU(c1,c2) (PLANEU + SJIS_ROW3(c1) * SJIS_COLSIZ + SJIS_COL(c2))
#define	UIDX1_SJIS(idx)	 ((((idx)/SJIS_COLSIZ + 0xf0) << 8) |\
			 idx_to_sjis[(idx)%SJIS_COLSIZ])
#define	UIDX2_SJIS(idx)	 ((((idx)/SJIS_COLSIZ + 0xeb) << 8) |\
			 idx_to_sjis[(idx)%SJIS_COLSIZ])
#define UIDX_SJIS(idx)	 (((idx)  < SJIS_UCNT1) ? UIDX1_SJIS(idx) : \
			 (((idx) -= SJIS_UCNT1), 		    \
			  ((idx)  < SJIS_UCNT2) ? UIDX2_SJIS(idx) : BAD))
#define	SJIS_UCNT1	 ((0xfc - 0xf0 + 1) * SJIS_COLSIZ)
#define	SJIS_UCNT2	 ((0xef - 0xeb + 1) * SJIS_COLSIZ)
#define	SJIS_UCNT	 (SJIS_UCNT1 + SJIS_UCNT2)

/*
 * BIG-5 related macros
 *
 * Valid BIG-5 first  byte: 0x81 - 0xfe
 * Valid BIG-5 second byte: 0x40 - 0x7e, 0xa1 - 0xfe
 *
 * BIG-5 plane 1: 0xa1 <= 1st byte <= 0xf9
 *		  0x40 <= 2nd byte <= 0x7e
 * BIG-5 plane 2: 0xa1 <= 1st byte <= 0xf9
 *		  0xa1 <= 2nd byte <= 0xfe
 * BIG-5 plane U: 0xfa <= 1st byte <= 0xfe (UDC level 1)
 *		  0x8e <= 1st byte <= 0xa0 (UDC level 2)
 *		  0x81 <= 1st byte <= 0x8d (UDC level 3)
 * 
 * All the non-UDC BIG-5 characters that have no mapping are now mapped
 * to the BMP UDC area after those occupied by the plane U characters.
 */
#define	BIG5_VALID1(c)	    (__cvtab[(unsigned)(c )] & VBITS_BIG5_1)
#define	BIG5_VALID2(c)	    (__cvtab[(unsigned)(c )] & VBITS_BIG5_2)
#define BIG5_PLANE1(c1,c2) ((__cvtab[(unsigned)(c1)] & RANGE_A1TOF9) && \
			    (__cvtab[(unsigned)(c2)] & RANGE_40TO7E))
#define BIG5_PLANE2(c1,c2) ((__cvtab[(unsigned)(c1)] & RANGE_A1TOF9) && \
			    (__cvtab[(unsigned)(c2)] & RANGE_A1TOFE))
#define BIG5_PLANEU(c1,c2)  (__cvtab[(unsigned)(c1)] & RANGE_81TOA0_FATOFE)
#define	BIG5_UDC1(c)	    (__cvtab[(unsigned)(c )] & RANGE_FATOFE)
#define	BIG5_UDC2(c)	    (__cvtab[(unsigned)(c )] & RANGE_8ETOA0)
#define	BIG5_UDC3(c)	    (__cvtab[(unsigned)(c )] & RANGE_81TO8D)
#define	BIG5_COLSIZ	     157
#define	BIG5_UCNT1	   ((0xfe - 0xfa + 1) * BIG5_COLSIZ)
#define	BIG5_UCNT2	   ((0xa0 - 0x8e + 1) * BIG5_COLSIZ)
#define	BIG5_UCNT3	   ((0x8d - 0x81 + 1) * BIG5_COLSIZ)
#define	BIG5_UCNT	    (BIG5_UCNT1 + BIG5_UCNT2 + BIG5_UCNT3)
#define	BIG5_UDC1IDX(c)	  (((c) - 0xfa) * BIG5_COLSIZ)
#define	BIG5_UDC2IDX(c)	  (((c) - 0x8e) * BIG5_COLSIZ + BIG5_UCNT1)
#define	BIG5_UDC3IDX(c)	  (((c) - 0x81) * BIG5_COLSIZ + BIG5_UCNT1 + BIG5_UCNT2)
#define	BIG5_IDX1(c1,c2)    (PLANE1 + ((c1) - 0xa1) * 63 + ((c2) - 0x40))
#define	BIG5_IDX2(c1,c2)    (PLANE2 + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	BIG5_IDXU(c1,c2)    (PLANEU + (BIG5_UDC1(c1) ? BIG5_UDC1IDX(c1)	 \
				    :  BIG5_UDC2(c1) ? BIG5_UDC2IDX(c1)	 \
						     : BIG5_UDC3IDX(c1)) \
				    + big5_to_idx[c2])
#define	BIG5_UDC(c1,c2)	     BIG5_PLANEU(c1,c2)
#define	BIG5_UIDX(c1,c2)    (BIG5_IDXU(c1,c2) - PLANEU)
#define	UIDX1_BIG5(idx)	    ((((idx)/BIG5_COLSIZ + 0xfa) << 8) |\
			    idx_to_big5[(idx)%BIG5_COLSIZ])
#define	UIDX2_BIG5(idx)	    ((((idx)/BIG5_COLSIZ + 0x8e) << 8) |\
			    idx_to_big5[(idx)%BIG5_COLSIZ])
#define	UIDX3_BIG5(idx)	    ((((idx)/BIG5_COLSIZ + 0x81) << 8) |\
			    idx_to_big5[(idx)%BIG5_COLSIZ])
#define	UIDX_BIG5(idx)	    (((idx)  < BIG5_UCNT1) ? UIDX1_BIG5(idx) :	\
			    (((idx) -= BIG5_UCNT1),			\
			     ((idx)  < BIG5_UCNT2) ? UIDX2_BIG5(idx) :	\
			    (((idx) -= BIG5_UCNT2),			\
			     ((idx)  < BIG5_UCNT3) ? UIDX3_BIG5(idx) :	\
			       BAD)))
/* 
 * Number of big-5 characters remapped to the end of private use area.
 */
#define	BIG5_NUMREMAPCHAR   7

/*
 * HKSCS related macros
 *
 * Valid HKSCS first  byte: 0x81 - 0xfe
 * Valid HKSCS second byte: 0x40 - 0x7e, 0xa1 - 0xfe
 *
 * HKSCS is very similar to BIG-5. The major difference is the use of some
 * of the UDC regions to encode Hong Kong specific characters.
 *
 * HKSCS plane 1: 0xa1 <= 1st byte <= 0xfe
 *		  0x40 <= 2nd byte <= 0x7e
 * HKSCS plane 2: 0xa1 <= 1st byte <= 0xfe
 *		  0xa1 <= 2nd byte <= 0xfe
 * HKSCS plane 3: 0x88 <= 1st byte <= 0xa0
 */
#define	HKSCS_VALID1(c)	     (__cvtab[(unsigned)(c )] & VBITS_BIG5_1)
#define	HKSCS_VALID2(c)	     (__cvtab[(unsigned)(c )] & VBITS_BIG5_2)
#define HKSCS_PLANE1(c1,c2) ((__cvtab[(unsigned)(c1)] & RANGE_A1TOFE) && \
			     (__cvtab[(unsigned)(c2)] & RANGE_40TO7E))
#define HKSCS_PLANE2(c1,c2) ((__cvtab[(unsigned)(c1)] & RANGE_A1TOFE) && \
			     (__cvtab[(unsigned)(c2)] & RANGE_A1TOFE))
#define HKSCS_PLANE3(c1,c2) ((__cvtab[(unsigned)(c1)] & RANGE_81TOA0) && \
			    ((c1) >= 0x88))
#define	HKSCS_COLSIZ	     157
#define	HKSCS_IDX1(c1,c2)    (PLANE1 + ((c1) - 0xa1) * 63 + ((c2) - 0x40))
#define	HKSCS_IDX2(c1,c2)    (PLANE2 + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	HKSCS_IDX3(c1,c2)    (PLANE3 + ((c1) - 0x88) * HKSCS_COLSIZ \
				     + big5_to_idx[c2])
#define	HKSCS_UROWSIZE	     32	/* Row size of UDC to HKSCS mapping table */

/*
 * DEC Hanyu related macros
 *
 * Valid DEC Hanyu first  byte: 0xa1 - 0xfe
 * Valid DEC Hanyu second byte: 0x21 - 0x7e, 0xa1 - 0xfe
 * Leading code of DTSCS      : 0xc2 0xcb
 * 
 * DEC Hanyu plane 1: 0xa1 <= 1st byte <= 0xfe
 *		      0xa1 <= 2nd byte <= 0xfe
 * DEC Hanyu plane 2: 0xa1 <= 1st byte <= 0xfe
 *		      0x21 <= 2nd byte <= 0x7e
 * DEC Hanyu plane 3: 1st byte = 0xc2
 *		      2nd byte = 0xcb
 * 		      0xa1 <= 3rd byte <= 0xfe
 *		      0xa1 <= 4th byte <= 0xfe
 * UDC		    : 0xfdcc-0xfefe, 0xaaa1-0xc1fe, 0xf245-0xfe7e
 *
 * The following codepoints will be mapped to XUDC areas:
 *	0xc2cbe5a1 - 0xc2cbfefe
 *	0xc2cba121 - 0xc2cbfe7e
 * The other non-mappable characters will be mapped to UDC areas beyond those
 * used by the three UDC regions listed above.
 */
#define	DECHANYU_VALID1(c)	(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	DECHANYU_VALID2(c)	(__cvtab[(unsigned)(c)] & RANGE_21TO7E_A1TOFE)
#define	DTSCS_VALID1(c)		(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	DTSCS_VALID2(c)		(__cvtab[(unsigned)(c)] & RANGE_21TO7E_A1TOFE)
#define	DTSCS_LEADCODE(c1,c2)	(((c1) == 0xc2) && ((c2) == 0xcb))
#define	DECHANYU_PLANE1(c1,c2)	(__cvtab[(unsigned)(c2)] & RANGE_A1TOFE)
#define	DECHANYU_PLANE2(c1,c2)	(__cvtab[(unsigned)(c2)] & RANGE_21TO7E)
#define	DECHANYU_PLANE3(c1,c2)	DECHANYU_PLANE1(c1,c2)
#define	DECHANYU_PLANE4(c1,c2)	DECHANYU_PLANE2(c1,c2)
#define	DECHANYU_IDX1(c1,c2)	(PLANE1 + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	DECHANYU_IDX2(c1,c2)	(PLANE2 + ((c1) - 0xa1) * 94 + ((c2) - 0x21))
#define	DECHANYU_IDX3(c1,c2)	(PLANE3 + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))

#define	DECHANYU_UCNT11		((0xfe - 0xcc + 1) + 94)
#define	DECHANYU_UCNT12		((0xc1 - 0xaa + 1) * 94)
#define	DECHANYU_UCNT2		((0xfe - 0xf2 + 1) * 94 - (0x45 - 0x21))
#define	DECHANYU_UCNT		(DECHANYU_UCNT11+DECHANYU_UCNT12+DECHANYU_UCNT2)
#define	DECHANYU_START11	0
#define	DECHANYU_START12	DECHANYU_UCNT11
#define	DECHANYU_START2		DECHANYU_UCNT11 + DECHANYU_UCNT12
#define	DECHANYU_IDXU1(c1,c2)	(PLANEU + (((c1) >= 0xfd) \
		? ((((c1) - 0xfd) * 94 + (c2) - 0xcc) + DECHANYU_START11) \
		: ((((c1) - 0xaa) * 94 + (c2) - 0xa1) + DECHANYU_START12)))
#define	DECHANYU_IDXU2(c1,c2)	(PLANEU + (((c1) - 0xf2) * 94 + (c2) - 0x21) + \
				 DECHANYU_START2 - 0x24)

#define	UIDX11_DECHANYU(idx) ((((idx + 0x2b)/94 + 0xfd) << 8) |\
			       ((idx + 0x2b)%94 + 0xa1))
#define	UIDX12_DECHANYU(idx) ((((idx)/94 + 0xaa) << 8) | ((idx)%94 + 0xa1))
#define	UIDX2_DECHANYU(idx)  ((((idx + 0x24)/94 + 0xf2) << 8) |\
			       ((idx + 0x24)%94 + 0x21))
#define	UIDX_DECHANYU(idx)    (((idx)  < DECHANYU_UCNT11)?UIDX11_DECHANYU(idx):\
			      (((idx) -= DECHANYU_UCNT11),		       \
			       ((idx)  < DECHANYU_UCNT12)?UIDX12_DECHANYU(idx):\
			      (((idx) -= DECHANYU_UCNT12),		       \
			       ((idx)  < DECHANYU_UCNT2 )?UIDX2_DECHANYU(idx) :\
				 BAD)))
#define	DECHANYU_UDC11(c)	((0xaaa1 <= (c)) && ((c) <= 0xc1fe))
#define	DECHANYU_UDC12(c)	((0xfdcc <= (c)) && ((c) <= 0xfefe))
#define	DECHANYU_UDC1(c)	(DECHANYU_UDC11(c) || DECHANYU_UDC12(c))
#define	DECHANYU_UDC2(c)	((0xf245 <= (c)) && ((c) <= 0xfe7e))

#define	DECHANYU_ISXUDC3(c1)	 ((c1) >= 0xe5)
#define	DECHANYU_ISXUDC4(c1)	 (1)
#define	DECHANYU_XUDC3_CNT	 ((0xfe - 0xe5 + 1) * 94)
#define	DECHANYU_XUDC4_CNT	 (94 * 94)
#define	DECHANYU_TO_IDXU3(c1,c2) (PLANEXU + ((c1) - 0xe5) * 94 + (c2) - 0xa1)
#define	DECHANYU_TO_IDXU4(c1,c2) (PLANEXU + ((c1) - 0xa1) * 94 + \
				  DECHANYU_XUDC3_CNT + (c2) - 0x21)
#define	DECHANYU_FR_XUDC(ucs)	 (ucs -= UCS_XUDC_START, \
				 ((ucs) < DECHANYU_XUDC3_CNT) \
				 ? (0xc2cb0000 | (((ucs)/94 + 0xe5) << 8) |\
						  ((ucs)%94 + 0xa1)) : \
				 (ucs -= DECHANYU_XUDC3_CNT, \
				 ((ucs) < DECHANYU_XUDC4_CNT) \
				 ? (0xc2cb0000 | (((ucs)/94 + 0xa1) << 8) |\
						  ((ucs)%94 + 0x21)) : BAD))
/*
 * DEC Hanzi related macros
 *
 * Valid DEC Hanzi first  byte: 0xa1 - 0xfe
 * Valid DEC Hanzi second byte: 0x21 - 0x7e, 0xa1 - 0xfe
 * 
 * DEC Hanzi plane 1: 0xa1 <= 1st byte <= 0xfe
 *		      0xa1 <= 2nd byte <= 0xfe
 * DEC Hanzi plane U: 0xa1 <= 1st byte <= 0xfe	(UDC)
 *		      0x21 <= 2nd byte <= 0x7e
 *
 * The plane U is mapped to UDC plane 15. The other unmapped characters
 * are mapped to the BMP UDC area in the same way as GB2312.
 */
#define	DECHANZI_VALID1(c)	(__cvtab[(unsigned)(c )] & RANGE_A1TOFE)
#define	DECHANZI_VALID2(c)	(__cvtab[(unsigned)(c )] & RANGE_21TO7E_A1TOFE)
#define	DECHANZI_PLANE1(c1,c2)	(__cvtab[(unsigned)(c2)] & RANGE_A1TOFE)
#define	DECHANZI_PLANEU(c1,c2)	(__cvtab[(unsigned)(c2)] & RANGE_21TO7E)
#define	DECHANZI_IDX1(c1,c2)	(PLANE1  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	DECHANZI_IDXU(c1,c2)	(PLANEXU + ((c1) - 0xa1) * 94 + ((c2) - 0x21))
#define	UIDX_DECHANZI(idx)	(  (idx) < DECHANZI_UCNT) ? \
				((((idx)/94 + 0xa1) << 8) | (idx)%94 + 0x21) : BAD
#define	DECHANZI_UCNT		(94*94)

/*
 * GB2312 related macros
 *
 * Valid GB2312 first  byte: 0xa1 - 0xfe
 * Valid GB2312 second byte: 0xa1 - 0xfe
 *
 * GB2312 plane 1: 0xa1 <= 1st byte <= 0xfe
 *		   0xa1 <= 2nd byte <= 0xfe
 * UDC		 : 0xaaa1-0xaffe, 0xf8a1-0xfefe
 */
#define	GB2312_VALID1(c)	(__cvtab[(unsigned)(c )] & RANGE_A1TOFE)
#define	GB2312_VALID2(c)	(__cvtab[(unsigned)(c )] & RANGE_A1TOFE)
#define	GB2312_IDX(c1,c2)	(PLANE1 + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	GB2312_UCNT11		((0xaf - 0xaa + 1) * 94)
#define	GB2312_UCNT12		((0xfe - 0xf8 + 1) * 94)
#define	GB2312_UDC11(c1,c2)	((0xaa <= (c1)) && ((c1) <= 0xaf))
#define	GB2312_UDC12(c1,c2)	((0xf8 <= (c1)) && ((c1) <= 0xfe))
#define	GB2312_UDC1(c1,c2)	(GB2312_UDC11(c1,c2) || GB2312_UDC12(c1,c2))
#define	GB2312_START11		0
#define	GB2312_START12		GB2312_UCNT11
#define	GB2312_UCNT		(GB2312_UCNT11 + GB2312_UCNT12)
#define	GB2312_IDXU(c1,c2)	(PLANEU + (((c1) <= 0xaf) \
		? ((((c1) - 0xaa) * 94 + (c2) - 0xa1) + GBK_START11) \
		: ((((c1) - 0xf8) * 94 + (c2) - 0xa1) + GBK_START12)))

#define	UIDX11_GB2312(idx)	((((idx)/94 + 0xaa) << 8) | ((idx)%94 + 0xa1))
#define	UIDX12_GB2312(idx)	((((idx)/94 + 0xf8) << 8) | ((idx)%94 + 0xa1))
#define	UIDX_GB2312(idx)	(((idx)  < GB2312_UCNT11) ? UIDX11_GB2312(idx):\
				(((idx) -= GB2312_UCNT11),		    \
				 ((idx)  < GB2312_UCNT12) ? UIDX12_GB2312(idx):\
				   BAD))

/*
 * GBK related macros
 *
 * Valid GBK first  byte: 0x81 - 0xfe
 * Valid GBK second byte: 0x40 - 0x7e, 0x80 - 0xfe
 *
 * GBK plane 1: 0xa1 <= 1st byte <= 0xfe
 *		0xa1 <= 2nd byte <= 0xfe
 * GBK plane 2: 0x81 <= 1st byte <= 0xa0
 *		0x40 <= 2nd byte <= 0x7e & 0x80 <= 2nd byte <= 0xfe
 * GBK plane 3: 0xa1 <= 1st byte <= 0xfe
 *		0x40 <= 2nd byte <= 0x7e & 0x80 <= 2nd byte <= 0xa0
 * UDC	      : 0xaaa1-0xaffe, 0xf8a1-0xfefe, 0xa140-0xa7a0
 */

#define	GBK_VALID1(c)	   (__cvtab[(unsigned)(c )] & VBITS_GBK1)
#define	GBK_VALID2(c)	   (__cvtab[(unsigned)(c )] & VBITS_GBK2)
#define GBK_PLANE1(c1,c2) ((__cvtab[(unsigned)(c1)] & RANGE_A1TOFE) && \
			   (__cvtab[(unsigned)(c2)] & RANGE_A1TOFE))
#define GBK_PLANE2(c1,c2) ((__cvtab[(unsigned)(c1)] & RANGE_81TOA0) && \
			   (__cvtab[(unsigned)(c2)] & RANGE_40TO7E_80TOFE))
#define GBK_PLANE3(c1,c2) ((__cvtab[(unsigned)(c1)] & RANGE_A1TOFE) && \
			   (__cvtab[(unsigned)(c2)] & RANGE_40TO7E_80TOA0))
#define	GBK_IDX1(c1,c2)    (PLANE1 + ((c1) - 0xa1) *  94 + ((c2) - 0xa1))
#define	GBK_IDX2(c1,c2)    (PLANE2 + ((c1) - 0x81) * 190 + __gbk_2bidx[c2])
#define	GBK_IDX3(c1,c2)    (PLANE3 + ((c1) - 0xa1) *  96 + __gbk_2bidx[c2])

#define	GBK_UCNT11	  GB2312_UCNT11
#define	GBK_UCNT12	  GB2312_UCNT12
#define	GBK_UCNT3	  ((0xa7 - 0xa1 + 1) * 96)
#define	GBK_UCNT	  (GBK_UCNT11+GBK_UCNT12+GBK_UCNT3)
#define	GBK_START11	  0
#define	GBK_START12	  GBK_UCNT11
#define	GBK_START3	  GBK_UCNT11 + GBK_UCNT12
#define	GBK_IDXU1(c1,c2)  (PLANEU + (((c1) <= 0xaf) \
		? ((((c1) - 0xaa) * 94 + (c2) - 0xa1) + GBK_START11) \
		: ((((c1) - 0xf8) * 94 + (c2) - 0xa1) + GBK_START12)))
#define	GBK_IDXU3(c1,c2)  (PLANEU + ((c1) - 0xa1) * 96 + __gbk_2bidx[c2] + \
			   GBK_START3)

#define	UIDX11_GBK(idx)   ((((idx)/94 + 0xaa) << 8) | ((idx)%94 + 0xa1))
#define	UIDX12_GBK(idx)   ((((idx)/94 + 0xf8) << 8) | ((idx)%94 + 0xa1))
#define	UIDX3_GBK(idx)    ((((idx)/96 + 0xa1) << 8) | __gbk_idx2b[(idx)%96])
#define	UIDX_GBK(idx)      (((idx)  < GBK_UCNT11) ? UIDX11_GBK(idx):\
			   (((idx) -= GBK_UCNT11),		    \
			    ((idx)  < GBK_UCNT12) ? UIDX12_GBK(idx):\
			   (((idx) -= GBK_UCNT12),		    \
			    ((idx)  < GBK_UCNT3 ) ? UIDX3_GBK(idx) :\
			      BAD)))
#define	GBK_UDC11(c1,c2)  (GBK_PLANE1(c1,c2) && GB2312_UDC11(c1,c2))
#define	GBK_UDC12(c1,c2)  (GBK_PLANE1(c1,c2) && GB2312_UDC12(c1,c2))
#define	GBK_UDC3(c1,c2)	  (GBK_PLANE3(c1,c2) && (0xa1 <= (c1)) && ((c1) <= 0xa7))

/*
 * GB18030-2000 related macros
 *
 * The 2-byte form of GB18030 is a superset of GBK.
 *
 * Valid 2-byte GB18030 - first  byte: 0x81 - 0xfe
 * Valid 2-byte GB18030 - second byte: 0x40 - 0x7e, 0x80 - 0xfe
 *
 * GB18030 plane 1: 0xa1 <= 1st byte <= 0xfe
 *		    0xa1 <= 2nd byte <= 0xfe
 * GB18030 plane 2: 0x81 <= 1st byte <= 0xa0
 *		    0x40 <= 2nd byte <= 0x7e & 0x80 <= 2nd byte <= 0xfe
 * GB18030 plane 3: 0xa1 <= 1st byte <= 0xfe
 *		    0x40 <= 2nd byte <= 0x7e & 0x80 <= 2nd byte <= 0xa0
 *
 * There are also some UDC character mapping distributed in the non-allocated
 * spaces elsewhere in plane 1,2,3.
 *
 * Valid 4-byte GB18030 - 1st byte: 0x81-0xfe
 *			- 2nd byte: 0x30-0x39
 *			- 3rd byte: 0x81-0xfe
 *			- 4th byte: 0x30-0x39
 *
 * Please see from_GB18030.c for details about converting these 4-byte
 * code to Unicode and vice versa
 *
 * 2-byte GB18030 UDC mapping:
 * 	0xaaa1-0xaffe -> U+E000-U+E233
 *	0xf8a1-0xfefe -> U+E234-U+E4C5
 *	0xa140-0xa7a0 -> U+E4C6-U+E765
 *	0xa2ab-0xa2b0 -> U+E766-U+E76B
 *	0xa2e3-0xa2e4 -> U+E76C-U+E76D
 *	0xa2ef-0xa2f0 -> U+E76E-U+E76F
 *	0xa2fd-0xa2ff -> U+E770-U+E771
 *	0xa4f4-0xa4fe -> U+E772-U+E77C
 *	0xa5f7-0xa5fe -> U+E77D-U+E784
 *		     ....
 *
 * 4-byte GB18030 UDC mapping:
 *	0x8336d030-0x84308130 -> U+E865-U+F8FF
 *
 * The mapping between UDC and GB18030 will be done algorithmically if
 * 	UDC	= U+E000-U+E765 or U+E865-U+F8FF
 *	GB18030 = 0xaaa1-0xaffe, 0xf8a1-0xfefe, 0xa140-0xa7a0,
 *		  0x8336d030-0x84308130
 * Other will be done by mapping tables.
 *
 * Plane mapping:
 *	0x81308130-0x8439fe39 (50400   codepoints) -> BMP (plane 0)
 *	0x90308130-0xe339fe39 (1058400 codepoints) -> Plane 1-16
 */
#define	GB18030_1BYTE(c)      ((unsigned)(c) < 0x80)
#define	GB18030_2BYTE(c1,c2)  (GBK_VALID1(c1) && GBK_VALID2(c2))
#define	GB18030_4BYTE(c1,c2)  (GBK_VALID1(c1) && \
			      (__cvtab[(unsigned)(c2)] & VBIT_30TO39))
#define GB18030_PLANE1(c1,c2) GBK_PLANE1(c1,c2)
#define GB18030_PLANE2(c1,c2) GBK_PLANE2(c1,c2)
#define GB18030_PLANE3(c1,c2) GBK_PLANE3(c1,c2)
#define	GB18030_IDX1(c1,c2)   GBK_IDX1  (c1,c2)
#define	GB18030_IDX2(c1,c2)   GBK_IDX2  (c1,c2)
#define	GB18030_IDX3(c1,c2)   GBK_IDX3  (c1,c2)

#define	GB18030_UCNT11	      GBK_UCNT11
#define	GB18030_UCNT12	      GBK_UCNT12
#define	GB18030_UCNT3	      GBK_UCNT3
#define	GB18030_START11	      GBK_START11
#define	GB18030_START12	      GBK_START12
#define	GB18030_START3	      GBK_START3
#define	GB18030_IDXU1(c1,c2)  GBK_IDXU1(c1,c2)
#define	GB18030_IDXU3(c1,c2)  GBK_IDXU3(c1,c2)
#define	GB18030_UDC11(c1,c2)  GBK_UDC11(c1,c2)
#define	GB18030_UDC12(c1,c2)  GBK_UDC12(c1,c2)
#define	GB18030_UDC3(c1,c2)   GBK_UDC3 (c1,c2)
#define	UIDX_GB18030(idx)     UIDX_GBK(idx)

/*
 * DEC Kanji related macros
 *
 * Valid DEC Kanji first  byte: 0xa1 - 0xfe
 * Valid DEC Kanji second byte: 0x21 - 0x7e, 0xa1 - 0xfe
 * 
 * DEC Kanji plane 1: 0xa1 <= 1st byte <= 0xfe
 *		      0xa1 <= 2nd byte <= 0xfe
 * DEC Kanji plane U: 0xa1 <= 1st byte <= 0xfe (UDC)
 *		      0x21 <= 2nd byte <= 0x7e
 *
 * The plane U of DEC Kanji are mapped to plane 15 UDC area.
 * The other unmapped codepoints are mapped to BMP UDC area in the same
 * way as eucJP.
 */
#define	DECKANJI_VALID1(c)	(__cvtab[(unsigned)(c )] & RANGE_A1TOFE)
#define	DECKANJI_VALID2(c)	(__cvtab[(unsigned)(c )] & RANGE_21TO7E_A1TOFE)
#define	DECKANJI_PLANE1(c1,c2)	(__cvtab[(unsigned)(c2)] & RANGE_A1TOFE)
#define	DECKANJI_PLANEU(c1,c2)	(__cvtab[(unsigned)(c2)] & RANGE_21TO7E)
#define	DECKANJI_IDX1(c1,c2)	(PLANE1  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	DECKANJI_IDXU(c1,c2)	(PLANEXU + ((c1) - 0xa1) * 94 + ((c2) - 0x21))
#define	UIDX_DECKANJI(idx)	(  (idx) < DECKANJI_UCNT) ? \
				((((idx)/94 + 0xa1) << 8) | (idx)%94 + 0x21) : BAD
#define	DECKANJI_UCNT		(94*94)

/*
 * JISX-UDC macros
 *
 * Plane U: 0xa1 <= 1st byte <= 0xfe
 *	    0x21 <= 2nd byte <= 0x7e
 */
#define	JISXUDC_VALID1(c)	(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	JISXUDC_VALID2(c)	(__cvtab[(unsigned)(c)] & RANGE_21TO7E)
#define	JISXUDC_IDX(c1,c2)	DECKANJI_IDXU(c1,c2)

/*
 * DEC Korean related macros
 *
 * Valid DEC Korean first  byte: 0xa1 - 0xfe
 * Valid DEC Korean second byte: 0xa1 - 0xfe
 * 
 * DEC Korean plane 2: 0xa1 <= 1st byte <= 0xfe
 *		       0xa1 <= 2nd byte <= 0xfe
 */
#define	DECKOREAN_VALID1(c)	(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	DECKOREAN_VALID2(c)	(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	DECKOREAN_IDX1(c1,c2)	(PLANE1 + ((c1) - 0xa1) * KSC5601_WIDTH1 + ((c2) - 0xa1))

/*
 * Taiwanese EUC related macros
 *
 * Valid eucTW 2nd last byte: 0xa1 - 0xfe
 * Valid eucTW     last	byte: 0xa1 - 0xfe
 * 
 * eucTW plane 1 : 0xa1 <= 1st byte <= 0xfe
 *		   0xa1 <= 2nd byte <= 0xfe
 * eucTW plane 2 : 1st byte = 0x8e
 *		   2nd byte = 0xa2
 * 		   0xa1 <= 3rd byte <= 0xfe
 *		   0xa1 <= 4th byte <= 0xfe
 * eucTW plane 3 : 1st byte = 0x8e
 *		   2nd byte = 0xa3
 * 		   0xa1 <= 3rd byte <= 0xfe
 *		   0xa1 <= 4th byte <= 0xfe
 * eucTW plane 4 : 1st byte = 0x8e
 *		   2nd byte = 0xa4
 * 		   0xa1 <= 3rd byte <= 0xfe
 *		   0xa1 <= 4th byte <= 0xfe
 * eucTW plane 5 : 1st byte = 0x8e
 *		   2nd byte = 0xa5
 * 		   0xa1 <= 3rd byte <= 0xfe
 *		   0xa1 <= 4th byte <= 0xfe
 * eucTW plane 6 : 1st byte = 0x8e
 *		   2nd byte = 0xa6
 * 		   0xa1 <= 3rd byte <= 0xfe
 *		   0xa1 <= 4th byte <= 0xfe
 * eucTW plane 7 : 1st byte = 0x8e
 *		   2nd byte = 0xa7
 * 		   0xa1 <= 3rd byte <= 0xfe
 *		   0xa1 <= 4th byte <= 0xfe
 * eucTW plane 15: 1st byte = 0x8e
 *		   2nd byte = 0xaf
 * 		   0xa1 <= 3rd byte <= 0xfe
 *		   0xa1 <= 4th byte <= 0xfe
 *
 * All the unmappable codepoints in plane 1-4 will be mapped to the characters
 * in the private use areas for preserving codepoints during roundtrip 
 * conversion. As the UDC area in BMP is not large enough, the following
 * codepoints will be mapped to Plane 15 UDC:
 *	0x8ea3e8a1 - 0x8ea3fefe
 *	0x8ea4efa1 - 0x8ea4fefe
 *
 * Codepoints that belong to the UDC area of DEC Hanyu will be mapped to 
 * UDC in the same way.
 */
#define	EUCTW_VALID1(c)	   (__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	EUCTW_VALID2(c)	   (__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	EUCTW_IDX1(c1,c2)  (PLANE1  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCTW_IDX2(c1,c2)  (PLANE2  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCTW_IDX3(c1,c2)  (PLANE3  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCTW_IDX4(c1,c2)  (PLANE4  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCTW_IDX5(c1,c2)  (PLANE5  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCTW_IDX6(c1,c2)  (PLANE6  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCTW_IDX7(c1,c2)  (PLANE7  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCTW_IDXF(c1,c2)  (PLANE15 + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCTW_ISXUDC3(c1)  ((c1) >= 0xe8)
#define	EUCTW_ISXUDC4(c1)  ((c1) >= 0xef)
#define	EUCTW_XUDC3_CNT	   ((0xfe - 0xe8 + 1) * 94)
#define	EUCTW_XUDC4_CNT	   ((0xfe - 0xef + 1) * 94)

#define	EUCTW_TO_IDXU3(c1,c2)	(PLANEXU + ((c1) - 0xe8) * 94 + (c2) - 0xa1)
#define	EUCTW_TO_IDXU4(c1,c2)	(PLANEXU + ((c1) - 0xef) * 94 + \
				EUCTW_XUDC3_CNT +  (c2) - 0xa1)
#define	EUCTW_FR_XUDC(ucs)	(ucs -= UCS_XUDC_START, \
				((ucs) < EUCTW_XUDC3_CNT) \
				? (0x8ea30000 | (((ucs)/94 + 0xe8) << 8) |\
						 ((ucs)%94 + 0xa1)) : \
				(ucs -= EUCTW_XUDC3_CNT, \
				((ucs) < EUCTW_XUDC4_CNT) \
				? (0x8ea40000 | (((ucs)/94 + 0xef) << 8) |\
						 ((ucs)%94 + 0xa1)) : BAD))
#define	EUCTW_ISUDC1(c)		DECHANYU_UDC1(c)
#define	EUCTW_ISUDC2(c)		((0xf2c5 <= (c)) && ((c) <= 0xfefe))
#define	EUCTW_UCNT		DECHANYU_UCNT
#define	EUCTW_TO_IDXU1(c1,c2)	DECHANYU_IDXU1(c1, c2)
#define	EUCTW_TO_IDXU2(c1,c2)	DECHANYU_IDXU2(c1, (c2 & 0x7f))
#define	UIDX2_TO_EUCTW(idx)	(UIDX2_DECHANYU(idx) | 0x8ea20080)
#define	UDC_TO_EUCTW(ucs)      ((ucs) -= UCS_UDC_START,\
			      (((ucs)  < DECHANYU_UCNT11)?UIDX11_DECHANYU(ucs):\
			      (((ucs) -= DECHANYU_UCNT11),		       \
			       ((ucs)  < DECHANYU_UCNT12)?UIDX12_DECHANYU(ucs):\
			      (((ucs) -= DECHANYU_UCNT12),		       \
			       ((ucs)  < DECHANYU_UCNT2 )?UIDX2_TO_EUCTW(ucs) :\
				 BAD))))

/*
 * Japanese EUC related macros
 *
 * Valid eucJP first  byte: 0xa1 - 0xfe
 * Valid eucJP second byte: 0xa1 - 0xfe
 * 
 * eucJP plane 0 : 1st byte = 0x8e
 *		   0xa1 <= 2nd byte <= 0xfe
 * eucJP plane 1 : 0xa1 <= 1st byte <= 0xfe
 *		   0xa1 <= 2nd byte <= 0xfe
 * eucJP plane 2 : 1st byte = 0x8f
 * 		   0xa1 <= 2nd byte <= 0xfe
 *		   0xa1 <= 3rd byte <= 0xfe
 * eucJP UDC	 : 0xf5a1-0xfefe, 0x8ff5a1-0x8ffefe, 0x8feea1-0x8ff4fe
 *
 * All unmapped codepoints will now map to the BMP UDC area.
 */
#define	EUCJP_VALID1(c)	   (__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	EUCJP_VALID2(c)	   (__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	EUCJP_IDX0(c)      ((c) + 0x100)
#define	EUCJP_IDX1(c1,c2)  (PLANE1  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))
#define	EUCJP_IDX2(c1,c2)  (PLANE2  + ((c1) - 0xa1) * 94 + ((c2) - 0xa1))

#define	EUCJP_UCNT1	   ((0xfe - 0xf5 + 1) * 94)
#define	EUCJP_UCNT21	   ((0xfe - 0xf5 + 1) * 94)
#define	EUCJP_UCNT22	   ((0xf4 - 0xee + 1) * 94)
#define	EUCJP_UCNT	   (EUCJP_UCNT1 + EUCJP_UCNT21 + EUCJP_UCNT22)
#define	EUCJP_UDC1(c1,c2)  ((c1) >= 0xf5)
#define	EUCJP_UDC2(c1,c2)  ((c1) >= 0xee)
#define	EUCJP_IDXU1(c1,c2) (PLANEU +  ((c1) - 0xf5) * 94 + ((c2) - 0xa1))
#define	EUCJP_IDXU2(c1,c2) (PLANEU + (((c1) >= 0xf5) \
		? ((((c1) - 0xf5) * 94 + ((c2) - 0xa1)) + EUCJP_UCNT1) \
		: ((((c1) - 0xee) * 94 + ((c2) - 0xa1)) + EUCJP_UCNT1 + EUCJP_UCNT21)))

#define	UIDX1_EUCJP(idx)   ((((idx)/94 + 0xf5) << 8) | ((idx)%94 + 0xa1))
#define	UIDX21_EUCJP(idx)  ((((idx)/94 + 0xf5) << 8) | ((idx)%94 + 0xa1) | 0x8f0000)
#define	UIDX22_EUCJP(idx)  ((((idx)/94 + 0xee) << 8) | ((idx)%94 + 0xa1) | 0x8f0000)
#define	UIDX_EUCJP(idx)	   (((idx)  < EUCJP_UCNT1 ) ? UIDX1_EUCJP(idx) :\
			   (((idx) -= EUCJP_UCNT1 ), 		        \
			    ((idx)  < EUCJP_UCNT21) ? UIDX21_EUCJP(idx):\
			   (((idx) -= EUCJP_UCNT21),		        \
			    ((idx)  < EUCJP_UCNT22) ? UIDX22_EUCJP(idx):\
			      BAD)))

/*
 * Super DEC Kanji related macros
 *
 * Valid sdeckanji first  byte: 0xa1 - 0xfe
 * Valid sdeckanji second byte: 0xa1 - 0xfe, 0x21 - 0x7e
 * 
 * sdeckanji plane 0 : 1st byte = 0x8e
 *		       0xa1 <= 2nd byte <= 0xfe
 * sdeckanji plane 1 : 0xa1 <= 1st byte <= 0xfe
 *		       0xa1 <= 2nd byte <= 0xfe
 * sdeckanji plane 2 : 1st byte = 0x8f
 * 		       0xa1 <= 2nd byte <= 0xfe
 *		       0xa1 <= 3rd byte <= 0xfe
 * sdeckanji plane U : 0xa1 <= 1st byte <= 0xfe
 *		       0x21 <= 2nd byte <= 0x7e
 *
 * The plane U of Super DEC Kanji are mapped to plane 15 UDC area.
 * The other unmapped codepoints are mapped to BMP UDC area in the same
 * way as eucJP.
 */
#define	SDECKANJI_VALID1(c)	(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	SDECKANJI_VALID2(c)	(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	SDECKANJI_UDC2(c)	(__cvtab[(unsigned)(c)] & RANGE_21TO7E)
#define	SDECKANJI_IDX0(c)	EUCJP_IDX0(c)
#define	SDECKANJI_IDX1(c1,c2)	EUCJP_IDX1(c1,c2)
#define	SDECKANJI_IDX2(c1,c2)	EUCJP_IDX2(c1,c2)
#define	SDECKANJI_IDXU(c1,c2)	DECKANJI_IDXU(c1,c2)
#define	UIDX_SDECKANJI(idx)	UIDX_DECKANJI(idx)

/*
 * Unified Hangul (KSC 5601-1992)
 *
 * It includes 2,350 characters same as Wansung (KSC 5601-1987)
 *	1st Byte = 0xa1-0xfe with 2nd Byte = 0xa1-0xfe
 * plus 8,822 characters :
 *	1st Byte = 0x81-0xa0 with 2nd Byte = 0x41-0x5a, 0x61-0x7a, 0x81-0xfe
 *	1st Byte = 0xa1-0xc6 with 2nd Byte = 0x41-0x5a, 0x61-0x7a, 0x81-0xa0
 *
 * KSC5601 plane 1 : 1st byte = 0xa1 - 0xfe
 *		     2nd byte = 0xa1 - 0xfe
 * KSC5601 plane 2 : 1st Byte = 0x81 - 0xa0
 *		     2nd Byte = 0x41 - 0x5a, 0x61 - 0x7a, 0x81 - 0xfe
 * KSC5601 plane 3 : 1st Byte = 0xa1 - 0xc6
 *		     2nd Byte = 0x41 - 0x5a, 0x61 - 0x7a, 0x81 - 0xa0
 */
#define	KSC5601_VALID1(c)	(__cvtab[(unsigned)(c)] & RANGE_81TOFE)
#define	KSC5601_VALID2(c)	(__cvtab[(unsigned)(c)] & RANGE_41TO5A_61TO7A_81TOFE)
#define	KSC5601_1_VALID1(c)	(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	KSC5601_1_VALID2(c)	(__cvtab[(unsigned)(c)] & RANGE_A1TOFE)
#define	KSC5601_2_VALID1(c)	(__cvtab[(unsigned)(c)] & RANGE_81TOA0)
#define	KSC5601_2_VALID2(c)	(__cvtab[(unsigned)(c)] & RANGE_41TO5A_61TO7A_81TOFE)
#define	KSC5601_3_VALID1(c)	(__cvtab[(unsigned)(c)] & RANGE_A0TOC6)
#define	KSC5601_3_VALID2(c)	(__cvtab[(unsigned)(c)] & RANGE_41TO5A_61TO7A_81TOA0)
#define	KSC5601_IDX1(c1, c2)	(PLANE1 + ((c1) - 0xa1) * KSC5601_WIDTH1 + ((c2) - 0xa1))
#define	KSC5601_IDX2(c1, c2)	(PLANE2 + ((c1) - 0x81) * KSC5601_WIDTH2 + ((c2) - 0x41))
#define	KSC5601_IDX3(c1, c2)	(PLANE3 + ((c1) - 0xa1) * KSC5601_WIDTH3 + ((c2) - 0x41))
#define	KSC5601_WIDTH1		(0xfe - 0xa1 + 1)
#define	KSC5601_WIDTH2		(0xfe - 0x41 + 1)
#define	KSC5601_WIDTH3		(0xa0 - 0x41 + 1)

/*
 * UDC Mappings
 * ------------
 *
 * Japanese:
 *  - eucJP	0xf5a1-0xfefe, 0x8ff5a1-0x8ffefe, 0x8feea1-0x8ff4fe
 *  - deckanji	0xa121-0xfe7e
 *  - sdeckanji	0xa121-0xfe7e + eucJP UDC
 *  - SJIS	0xf040-0xfcfc
 *
 * Chinese:
 *  - dechanzi	0xa121-0xfe7e
 *  - dechanyu	0xf321-0xfe7e, 0xaaa1-0xc1fe, 0xfdcc-0xfefe
 *  - big5	0xfa40-0xfefe, 0x8e40-0xa0fe
 *
 * Unicode:	U+E000-U+F8FF, U-000F0000-U-0010FFFF
 *
 * IDXU = UIDX + PLANEU
 */
#define	UCS_UDC_START	0xe000
#define	UCS_UDC_END	0xf8ff
#define	UCS_XUDC_START	0x0f0000 /* Plane 15 & 16 are addtional UDC planes */
#define	UCS_XUDC_END	0x10ffff
#define	UCS_UCNT	(0xf8ff - 0xe000 + 1)

#define	UCS_UIDX(ucs)	(((ucs) >= UCS_XUDC_START) ? \
			 ((ucs)  - UCS_XUDC_START+UCS_UCNT) : ((ucs) - 0xe000))
#define	UCS_UDC(ucs)	(((0xe000 <= (ucs)) && ((ucs) <= 0xf8ff)) ||\
			 ((ucs)   >= UCS_XUDC_START))
#define	IDXU_UCS(idx)	(((idx) >= PLANEXU) ? \
			 ((idx)  + UCS_XUDC_START - PLANEXU) : \
			 ((idx)  + UCS_UDC_START  - PLANEU))
#define	ISIDXU(idx)	((idx) >= PLANEU)

/*
 * Macros for the mb*towc*() and wc*tomb*() routines to be used by the
 * @ucs4 locales.
 */


/*
 * External reference
 */
extern const unsigned int  __cvtab[]	;  //BRL
extern const short __gbk_2bidx[];
extern const short __gbk_idx2b[];
