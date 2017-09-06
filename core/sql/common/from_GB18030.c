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
 * $Log: from_GB18030.c,v $
 * Revision 1.1.8.1  2001/12/07  15:37:38
 *  * Remove __UDC_to_gbk.
 *  * Add UCS plane 1-16 character mapping support.
 *
 * Revision 1.1.4.3  2000/12/12  14:16:18
 * 	Add UDC checking code in GB18030 to UCS conversion routine.
 *
 * Revision 1.1.4.2  2000/12/11  19:41:16
 * 	Incorporate update due to the new GB18030 to UCS conversion table.
 *
 * Revision 1.1.4.1  2000/10/16  18:44:45
 * 	COSIX.Zulu to Yankee merge for GB18030 support.
 *
 * Revision 1.1.2.1  2000/08/07  14:33:44
 * 	Add support for Unicode to GB18030-2000 conversion.
 * 	[2000/08/01  17:40:14  Waiman_Long]
 *
 * $EndLog$
 */

#include "fcconv.h"
#include "multi-byte.h"

/*
 * Valid 4-byte GB18030 - 1st byte: 0x81-0xfe
 *			- 2nd byte: 0x30-0x39
 *			- 3rd byte: 0x81-0xfe
 *			- 4th byte: 0x30-0x39
 *
 * Macro to compute the index of the given 4-byte GB18030 character.
 */
#define	GB18030_4NROWS  (0xfe - 0x81 + 1)
#define	GB18030_4NCOLS	(0x39 - 0x30 + 1)
#define	GB18030_4SIZE_4	 GB18030_4NCOLS
#define	GB18030_4SIZE_3	(GB18030_4NROWS * GB18030_4SIZE_4)
#define	GB18030_4SIZE_2	(GB18030_4NCOLS * GB18030_4SIZE_3)
#define	GB18030_4IDX(c1,c2,c3,c4)	   \
	(((c1) - 0x81) * GB18030_4SIZE_2 + \
	 ((c2) - 0x30) * GB18030_4SIZE_3 + \
	 ((c3) - 0x81) * GB18030_4SIZE_4 + \
	 ((c4) - 0x30))
#define	GB18030_GBIDX(c)	GB18030_4IDX((c >> 24) & 0xff, \
					     (c >> 16) & 0xff, \
					     (c >>  8) & 0xff, \
					      c        & 0xff)
#define	GBIDX_GB18030(idx, gb)					   \
	{							   \
	    (gb )  =  (int )(idx) % GB18030_4NCOLS + 0x30	 ; \
	    (idx) /= GB18030_4NCOLS	 			 ; \
	    (gb ) |= ((int )(idx) % GB18030_4NROWS + 0x81) <<  8 ; \
	    (idx) /= GB18030_4NROWS			 	 ; \
	    (gb ) |= ((int )(idx) % GB18030_4NCOLS + 0x30) << 16 ; \
	    (gb ) |= ((int )(idx) / GB18030_4NCOLS + 0x81) << 24 ; \
	}

/*
 * 4-byte UDC checking macros (0x8336d030-0x84308130 -> U+E865-U+F8FF)
 */
#define	GB4_UIDXLO	      GB18030_4IDX(0x83, 0x36, 0xd0, 0x30)
#define	GB4_UIDXHI	      GB18030_4IDX(0x84, 0x30, 0x81, 0x30)
#define	GB4_UDCLO	      0xE865
#define	GB4_UDCHI	      0xF8FF
#define	GB18030_UIDX4(idx)    ((GB4_UIDXLO <= (idx)) && ((idx) <= GB4_UIDXHI))
#define	GB18030_IDX4UDC4(idx) (idx - GB4_UIDXLO + GB4_UDCLO )
#define	GB18030_UDC4IDX4(udc) (udc - GB4_UDCLO  + GB4_UIDXLO)
#define	GB18030_4IDX_MAX      GB18030_4IDX(0x84, 0x31, 0xa4, 0x37)

/*
 * UDC range U+E766-U+E864 will be mapped by table lookup
 */
#define	GB18030_TUDCLO	  0xE766
#define	GB18030_TUDCHI	  0xE864
#define	GB18030_TUDCLEN	  (GB18030_TUDCHI - GB18030_TUDCLO + 1)
#define	GB18030_TUDC(udc) ((GB18030_TUDCLO <= (udc)) && ((udc) <= GB18030_TUDCHI))

/*
 * UCS plane 1-16 mapping
 * 0x90308130-0xe339fe39 (1058400 codepoints) -> Plane 1-16
 */
#define	GB18030E_4IDX_MIN	GB18030_4IDX(0x90, 0x30, 0x81, 0x30)
#define	GB18030E_4IDX_MAX	GB18030_4IDX(0Xe3, 0x39, 0xfe, 0x39)
#define	IS_4IDXE(idx)		((GB18030E_4IDX_MIN <= (idx)) && \
				((idx) <= GB18030E_4IDX_MAX))
#define	UCS_TO_4IDXE(ucs)	((ucs) - 0x10000 + GB18030E_4IDX_MIN)
#define	UCS_FR_4IDXE(idx)	((idx) + 0x10000 - GB18030E_4IDX_MIN)

/*
 * Miscellaneous macros
 */
#define	ARRSIZE(arr)	(sizeof(arr)/sizeof(arr[0]))
#define	SFUNC_CAST	(int (*)(const void *, const void *))

/*
 * Assuming that 4-byte GB18030 code is rare. We can use a slower but
 * more memory efficient method of mapping those 4-byte codes to Unicode
 * and vice versa.
 *
 * The gbidx_ucs_t structure is used to map 4-byte GB18030 index to the
 * corresponding UCS value. For all index value >= gbidx and < gbidx of
 * the next entry, the offset value is added to produce the final UCS
 * result.
 *
 * The ucs_gbidx_t structure is for mapping UCS value back to 4-byte
 * GB18030 index.
 */
typedef struct
{
    int gbidx  ;	/* 4-byte GB18030 index of first one in a range	  */
    int offset ;	/* Offset that adds to index to produce UCS value */
} gb4idx_ucs_t ;

typedef struct
{
    int lo_ucs ;	/* Low limit of UCS range			*/
    int hi_ucs ;	/* High limit of UCS range			*/
    int offset ;	/* Offset to be added to produce index value	*/
} ucs_gb4idx_t ;

#ifndef	NODATA
#include "gb18030_data.c"
#else
static const ushort_t     udc_gb_table    [] = { 0	 } ;
static const gb4idx_ucs_t gb4idx_ucs_table[] = { 0, 0    } ;
static const ucs_gb4idx_t ucs_gb4idx_table[] = { 0, 0, 0 } ;
#endif

static int
compare_gb4idx_ucs(gb4idx_ucs_t *ptr1, gb4idx_ucs_t *ptr2)
{
    int		 factor = 1 ;
    gb4idx_ucs_t *ptr	    ;

    if (ptr2->offset == INT_MAX)
    {
	/*
	 * Swap ptr1 and ptr2 & set factor to -1
	 */
	ptr    = ptr1 ;
	ptr1   = ptr2 ;
	ptr2   = ptr  ;
	factor = -1   ;
    }
    if (ptr1->gbidx < ptr2->gbidx)
	return(-1 * factor) ;
    if ((ptr1->gbidx >= ptr2->gbidx) && (ptr1->gbidx < ptr2[1].gbidx))
	return(0) ;	/* A match */
    else
	return(factor) ;
}

static int
compare_gb4idx_ucs_C(const void *ptr1, const void *ptr2) //JAC - CAST FUNCTION needed to make our NT compiler happy
{
  return compare_gb4idx_ucs( (gb4idx_ucs_t *) ptr1, (gb4idx_ucs_t *) ptr2 );
}

static int
compare_ucs_gb4idx(ucs_gb4idx_t *ptr1, ucs_gb4idx_t *ptr2)
{
    int		 factor = 1 ;
    ucs_gb4idx_t *ptr	    ;

    if (ptr2->offset == INT_MAX)
    {
	/*
	 * Swap ptr1 and ptr2 & set factor to -1
	 */
	ptr    = ptr1 ;
	ptr1   = ptr2 ;
	ptr2   = ptr  ;
	factor = -1   ;
    }
    if (ptr1->lo_ucs < ptr2->lo_ucs)
	return(-1 * factor) ;
    if (ptr1->lo_ucs > ptr2->hi_ucs)
	return(factor) ;
    return(0) ;	/* A match */
}

static int
compare_ucs_gb4idx_C(const void *ptr1, const void *ptr2) //JAC - CAST FUNCTION needed to make our NT compiler happy
{
  return compare_ucs_gb4idx( (ucs_gb4idx_t *) ptr1, (ucs_gb4idx_t *) ptr2 );
}


/*
 * This routine maps a UCS UDC value in the table mapping range back to
 * a 2-byte GB18030 code if the value >= 0xa1a1. Otherwise, it is assumed
 * to be a 4-byte index and converted to a 4-byte GB18030 code.
 */
static uint_t
tudc_to_gb(int udc)
{
    uint_t gb = udc_gb_table[udc - GB18030_TUDCLO] ;
    if (gb < 0xa1a1)
    {
	int idx = gb ;
	GBIDX_GB18030(idx, gb) ;	/* UDC area 4 */
    }
    return(gb) ;
}

/*
 * This routine converts one GB18030 character from the input stream to table
 * index or directly to Unicode value.
 *
 * Return Value:
 *	Table index if no error
 *	-1 - Invalid sequence (EILSEQ)
 *	-2 - Input incomplete
 */
int
__gb18030_index(_LC_fcconv_iconv_t * cd, uchar_t **in, int len)
{
    uint   ch1, ch2 ;
    uint   ch3, ch4 ;
    uint   idx      ;
    uchar *ip 	    ;

    ip = *in   ;
    if (GB18030_1BYTE(*ip))
    {
	ch1 = *ip++ ;
	*in = ip    ;
	return(ch1) ;
    }

    if (len < 2)
	return(ERR_INPUT_INCOMPLETE) ;

    ch1 = *ip++ ;
    ch2 = *ip++ ;

    if (GB18030_2BYTE(ch1, ch2))
    {
	*in = ip ;	/* Adjust input pointer */
	if (GB18030_PLANE1(ch1, ch2))
	{
	    if (GB18030_UDC11(ch1, ch2) || GB18030_UDC12(ch1, ch2))
		return(GB18030_IDXU1(ch1, ch2)) ;
	    return(GB18030_IDX1(ch1, ch2)) ;
	}
	else if (GB18030_PLANE2(ch1, ch2))
	{
	    return(GB18030_IDX2(ch1, ch2)) ;
	}
	else /* GB18030_PLANE3(ch1, ch2) */
	{
	    if (GB18030_UDC3(ch1, ch2))
		return(GB18030_IDXU3(ch1, ch2)) ;
	    return(GB18030_IDX3(ch1, ch2)) ;
	}
    }
    if (GB18030_4BYTE(ch1, ch2))
    {
	gb4idx_ucs_t  key    ;
	gb4idx_ucs_t *result ;

	if (len < 4)
	    return(ERR_INPUT_INCOMPLETE) ;
	ch3 = *ip++ ;
	ch4 = *ip++ ;
	if (!GB18030_4BYTE(ch3, ch4))
	    return(ERR_INVALID_CHAR) ;
	if ((idx = GB18030_4IDX(ch1, ch2, ch3, ch4)) > GB18030_4IDX_MAX)
	{
	    if (IS_4IDXE(idx)) {
		*in = ip ;      /* Adjust input pointer */
		return(UCS_FR_4IDXE(idx) | UCODE_MASK) ;
	    }
	    return(ERR_INVALID_CHAR) ;	/* Invalid character */
	}
	/*
	 * Search the mapping table for a match entry
	 */
	key.gbidx  = idx     ;
	key.offset = INT_MAX ;
	result = (gb4idx_ucs_t *)bsearch(&key, gb4idx_ucs_table, ARRSIZE(gb4idx_ucs_table), //JAC
		         sizeof(gb4idx_ucs_table[0]),
			 compare_gb4idx_ucs_C) ; //JAC
	*in = ip ;	/* Adjust input pointer */
	/* Return plain Unicode value */
	return((idx + result->offset) | UCODE_MASK) ;
    }
    return(ERR_INVALID_CHAR) ;
}

/*
 * This routine maps UDC characters in Unicode to those in GB18030
 */
WChar_t	  // JAC
__UDC_to_gb18030(ucs4_t ucs)
{
    int  uidx = UCS_UIDX(ucs) ;
    int  gb4		      ;
    int  gb4idx		      ;

    if (ucs < GB18030_TUDCLO)
	return(UIDX_GB18030(uidx)) ;	/* UDC areas 1, 2, 3	*/
    if (ucs <= GB18030_TUDCHI)
	return(tudc_to_gb(ucs)) ;	/* Table lookup UDC	*/
    gb4idx = (ucs <= UCS_UDC_END) ? GB18030_UDC4IDX4(ucs)
				  : UCS_TO_4IDXE    (ucs) ;
    GBIDX_GB18030(gb4idx, gb4)     ;	/* UDC area 4		*/
    return((WChar_t)gb4)	   ;    // JAC
}

/*
 * This routine maps UCS characters to 4-byte GB18030 characters
 */
WChar_t	  // JAC
__UCS_to_gb18030(ucs4_t ucs)
{
    ucs_gb4idx_t key, *result ;
    int		 gb4idx	      ;
    int 	 gb4	      ;

    if (ucs < 0x10000)
    {
	key.hi_ucs = key.lo_ucs = ucs ;
	key.offset = INT_MAX	  ;
	if ((result = (ucs_gb4idx_t *)bsearch(&key, ucs_gb4idx_table, ARRSIZE(ucs_gb4idx_table), //JAC
		      sizeof(ucs_gb4idx_table[0]),
		      compare_ucs_gb4idx_C)) == NULL) //JAC
	    return(BAD) ;
	gb4idx = result->offset + ucs;
    }
    else if (ucs < 0x110000)
	gb4idx = UCS_TO_4IDXE(ucs) ;	/* Map plane 1-16 UCS character */
    else
	return(BAD) ;
    GBIDX_GB18030(gb4idx, gb4)   ;
    return((WChar_t)gb4)	 ;    // JAC
}
