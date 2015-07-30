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


#ifndef	FCCONV_H
#define	FCCONV_H	1

#ifndef _KERNEL
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
//#include "iconv_local.h"		   // BRL
#endif

typedef unsigned char   uchar_t ;  // JAC
typedef unsigned char   uchar   ;  // JAC
typedef          char * caddr_t ;  // JAC
typedef unsigned int    uint ;     // JAC
typedef unsigned int    uint_t ;   // JAC
typedef unsigned short  ushort_t ; // JAC

//#include <sys/errno.h> //BRL
#include "errno.h"       //JAC
#include <sys/types.h>

// #ifdef _KERNEL                                   //BRL
/*
 * ICONV status
 */
#define ICONV_DONE 0
#define ICONV_TRUNC     1       /* EINVAL */
#define ICONV_INVAL     2       /* EILSEQ */
#define ICONV_OVER      3       /* E2BIG */
// #endif                                          //BRL

/*
 * Default ROW_SIZE
 */
#ifndef	ROW_SIZE
#define ROW_SIZE 64
#endif

/*
 * Status flags in _LC_fcconv_iconv_t
 */
#define	CONV_BOM_WRITTEN	0x0001 /* Byte order mark written	     */
#define	CONV_REVERSE_INBYTE	0x0002 /* Reverse byte ordering of input     */
#define	CONV_REVERSE_OUTBYTE	0x0004 /* Reverse byte ordering of output    */
#define	CONV_SAVE_FLAGS		0x0008 /* Save flags value in input routine  */
#define	CONV_UTF7_CONVTABLE	0x0010 /* UTF-7 conversion table defined     */
#define	CONV_UTF7_OPTIONAL	0x0020 /* Enable UTF-7 optional direct chars */
#define	CONV_UTF7_VERBOSE	0x0040 /* Always output SHIFT_OUT	     */
#define	CONV_REWIND		0x0080 /* Rewind the conversion due to error */
#define	CONV_ASCII_INOMAP	0x0100 /* No input ASCII character mapping   */
#define	CONV_ASCII_IDMAP2	0x0200 /* Map input ASCII to cell2 directly  */
#define	CONV_ASCII_ONOMAP	0x0100 /* No output ASCII character mapping  */
#define	CONV_ASCII_ODMAP2	0x0200 /* Map output ASCII to cell2 directly */
#define	CONV_INPUT_PROCESSED	0x0400 /* May be set after input processing  */
#define	CONV_FUNC_FIRST		0x0800 /* Try conversion function first      */
#define	CONV_FUNC_NEXT		0x1000 /* Try conversion function next if
					  table lookup fails		     */
#define	CONV_NO_UDC		0x2000 /* Don't map any UCS UDC character    */

/*
 * Other macros
 */
#define	UCS4_BOM		0x0000feff	/* UCS-4 byte order mark */
#define	UCS4_BOM_REVERSE	0xfffe0000	/* Reversed UCS-4 BOM	 */
#define	UCS2_BOM		0xfeff		/* UCS-2 byte order mark */
#define	UCS2_BOM_REVERSE	0xfffe		/* Reversed UCS-2 BOM	 */
#define	BAD			(ucs2_t)-1
#define	IDE			(ucs2_t)-2		/* Code is identical	 */
#define	UCS2_IDE		((ucs2_t)IDE)
#define	UCS2_BAD		((ucs2_t)BAD)
#define	UCS4_BAD		((ucs4_t)BAD)
#define	ARRAY_SIZE(arr)		(sizeof(arr)/sizeof(arr[0]))
#define	UCODE_MASK		0x40000000
#define	IS_UCODE(c)		((c) &  UCODE_MASK)
#define	GET_UCODE(c)		((c) & ~UCODE_MASK)
#ifdef _KERNEL
#define _ISASCII(c)		((c) >= 0x21 && (c) <= 0x7e)
#else
#define _ISASCII(c)     	isascii(c)
#endif
#ifdef _KERNEL
#define _STRCASECMP(f, c)	strcmp(f, c)
#else
#define _STRCASECMP(f, c)	strcasecmp(f, c)
#endif


/*
 * Error return values
 */
#define	ERR_INVALID_CHAR	-1	/* EILSEQ, ICONV_INVAL */
#define	ERR_INPUT_INCOMPLETE	-2	/* EINVAL, ICONV_TRUNC */
#define	ERR_BUFFER_OVERRUN	-3	/* E2BIG , ICONV_OVER  */
#define	ERR_NOINPUT		-4

/*
 * Codeset numbers
 */
typedef enum codeset
{
    UCS		    ,	/* Generic UCS form */
    UCS_4	    ,
    UCS_2	    ,
    UTF_8	    ,
    UTF_7	    ,
    UTF_8_PASCII    ,	/* Pass ASCII directly without table lookup */
    UCS_4_1_1	    ,
    UCS_2_1_1	    ,
    UTF_8_1_1	    ,
    UTF_7_1_1	    ,
    ISO8859_1	    ,
    ISO8859_2	    ,
    ISO8859_3	    ,
    ISO8859_4	    ,
    ISO8859_5	    ,
    ISO8859_6	    ,
    ISO8859_7	    ,
    ISO8859_8	    ,
    ISO8859_9	    ,
    ISO885915	    ,
    CP437	    ,	/* DOS Latin US				*/
    CP737	    ,	/* DOS Greek				*/
    CP775	    ,	/* DOS Balt Rim				*/
    CP850	    ,	/* DOS Latin 1				*/
    CP852	    ,	/* DOS Latin 2				*/
    CP855	    ,	/* DOS Cyrillic				*/
    CP857	    ,	/* DOS Turkish				*/
    CP860	    ,	/* DOS Portuguese			*/
    CP861	    ,	/* DOS Icelandic			*/
    CP862	    ,	/* DOS Hebrew				*/
    CP863	    ,	/* DOS Canadian French			*/
    CP864	    ,	/* DOS Arabic				*/
    CP865	    ,	/* DOS Nordic				*/
    CP866	    ,	/* DOS Cyrillic Russian			*/
    CP869	    ,	/* DOS Greek 2				*/
    CP874	    ,	/* DOS Thai				*/
    CP932	    ,	/* Shift JIS				*/
    CP936	    ,	/* Extended GB				*/
    CP949	    ,	/* Unified Hangul (Extended Wansung)	*/
    CP950	    ,	/* Big 5				*/
    CP1250	    ,	/* Windows Latin 2			*/
    CP1251	    ,	/* Windows Cyrillic			*/
    CP1252	    ,	/* Windows Latin 1			*/
    CP1253	    ,	/* Windows Greek			*/
    CP1254	    ,	/* Windows Turkish			*/
    CP1255	    ,	/* Windows Hebrew			*/
    CP1256	    ,	/* Windows Arabic			*/
    CP1257	    ,	/* Windows Baltic			*/
    CP1258	    ,	/* Windows Vietnamese			*/
    DINGBATS	    ,
    SYMBOL	    ,
    TACTIS	    ,
    DECKANJI	    ,
    SJIS	    ,
    EUCJP	    ,
    SDECKANJI	    ,
    ISO_2022_JP	    ,
    DECHANYU	    ,
    EUCTW	    ,
    BIG5	    ,
    DECHANZI	    ,
    GB2312	    ,
    GBK		    ,
    GB18030	    ,
    DECKOREAN	    ,
    KSC5601	    ,
    EUCKR	    ,
    HKSCS	    ,	/* Hong Kong Supplementary Character Set */
    /*
     * Font mapping character sets
     */
    JISX0201_GR     ,
    JISX0208_GL	    ,
    JISX0208_GR	    ,
    JISX0212_GL	    ,
    JISX0212_GR     ,
    JISX_UDC	    ,
    GB2312_GL	    ,
    GB2312_GR	    ,
    GB2312_UDC	    ,
    GBK_GLGR	    ,
    KSC5601_GL	    ,
    KSC5601_GR	    ,
    CNS11643_1GL    ,
    CNS11643_1GR    ,
    CNS11643_2GL    ,
    CNS11643_2GR    ,
    CNS11643_3GL    ,
    CNS11643_3GR    ,
    CNS11643_4GL    ,
    CNS11643_4GR    ,
    DEC_CNS11643    ,
    DEC_CNS11643_UDC,
    DTSCS	    ,
    BIG5_GLGR	    ,
    BIG5_UDC
} codeset_t ;

/*
 * Row and column manipulation macros
 *
 * Only least significant 11 bits of the row index is used.
 * The other bits are used for status bits.
 */
# define ROW_MASK		0x7ff
# define ROW_CELL4		0x800	/* Row is index to 4-byte cell table */
# define IS_CELL4_ROW(c)	((c) & ROW_CELL4)
# define MASKROW(c)		((c) & ROW_MASK )

#define ROW(c)	  ((c) / ROW_SIZE)
#define COL(c)	  ((c) % ROW_SIZE)

/*
 * The row table is ucs2_t whereas the cell table  can be either ucs2_t or
 * ucs4_t. So the comparison with BAD must match the proper type to avoid
 * sign extension problem.
 */
#ifdef	USE_IDMAP
#define	GET_VAL(cd, idx, val, io)			  \
{							  \
    register ucs2_t _row, _col, _value;			  \
    _row = ROW(idx) ; _col = COL(idx) ;			  \
    if ((_row >= (cd)->##io##row_count) || 		  \
       ((_row = (cd)->##io##row_table[_row]) == UCS2_IDE))\
	val = idx ;					  \
    else if (_row == UCS2_BAD)				  \
	val = BAD ;					  \
    else if (IS_CELL4_ROW(_row))			  \
	val = (cd)->##io##cell4_tab[MASKROW(_row)][_col]; \
    else {						  \
	_value = (cd)->##io##cell2_tab[_row][_col];	  \
	val = (_value == UCS2_BAD) ? BAD : _value ;	  \
    }							  \
}
#else	/* USE_IDMAP */
#define	GET_VAL(cd, idx, val, io)			  \
{							  \
    register ucs2_t _row, _col, _value;			  \
    _row = ROW(idx) ; _col = COL(idx) ;			  \
    if ((_row >= (cd)->##io##row_count) ||		  \
       ((_row = (cd)->##io##row_table[_row]) == UCS2_BAD))\
	val = BAD ;					  \
    else if (IS_CELL4_ROW(_row)) 			  \
	val = (cd)->##io##cell4_tab[MASKROW(_row)][_col]; \
    else {						  \
	_value = (cd)->##io##cell2_tab[_row][_col];	  \
	val = (_value == UCS2_BAD) ? BAD : _value ;	  \
    }							  \
}
#endif	/* USE_IDMAP */

#define	GET_IVAL(cd, idx, val)	GET_VAL(cd, idx, val, i)
#define	GET_OVAL(cd, idx, val)	GET_VAL(cd, idx, val, o)

typedef struct _LC_fcconv_iconv_rec	_LC_fcconv_iconv_t ;
typedef unsigned short			ucs2_t		   ;
typedef unsigned int			ucs4_t		   ;
typedef const ucs2_t			row_t		   ;
typedef const ucs2_t			cell2_t[ROW_SIZE]   ;
typedef const ucs4_t			cell4_t[ROW_SIZE]   ;

/*
 * Need new type because the wchar_t type (used by the DEC source as we
 * received it) is only a 2-byte thing on NT and on Yosemite and the
 * DEC source code was assuming it was a 4-byte thing.
 */
typedef unsigned int WChar_t;

typedef int (*cfunc_t  )(WChar_t) ; /* Font charset conversion function */ // JAC
typedef int (*infunc_t )(_LC_fcconv_iconv_t *, uchar_t **, int) ;
typedef int (*outfunc_t)(_LC_fcconv_iconv_t *, uchar_t * , int, ucs4_t) ;
typedef int (*cnvfunc_t)(_LC_fcconv_iconv_t *, uchar_t **, size_t *,
					       uchar_t **, size_t *) ;
/* UCS to UDC mapping function */
typedef WChar_t (*udcfunc_t)(ucs4_t) ; // JAC

/*
 *	_LC_fcconv_iconv_t
 */
struct _LC_fcconv_iconv_rec {
#ifndef _KERNEL
//    _LC_core_iconv_t core      ; //BRL & JAC -- Don't think we need this.
#endif
    int		     flags     ;	/* Status and control flags	    */
    int		     srccode   ;	/* Source codeset number	    */
    int		     dstcode   ;	/* Destination codeset number	    */
    infunc_t	     infunc    ;	/* Input function		    */
    outfunc_t	     outfunc   ;	/* Output function		    */
    udcfunc_t	     udcfunc   ;	/* UCS to UDC conversion function   */
    cnvfunc_t	     cnvfunc   ;	/* Conversion function		    */
    caddr_t	     convtable ;	/* Conversion table (for UTF-7)	    */

    /* To UCS conversion table */
    cell2_t	    *icell2_tab ;	/* Input 2-byte cell table	    */
    cell4_t	    *icell4_tab ;	/* Input 4-byte cell table	    */
    row_t	    *irow_table ;	/* Input row to cell mapping table  */
    int		     irow_count ;	/* No. of rows in input row_table   */

    /* From UCS conversion table */
    cell2_t	    *ocell2_tab ;	/* Output 2-byte cell table	    */
    cell4_t	    *ocell4_tab ;	/* Output 4-byte cell table	    */
    row_t	    *orow_table ;	/* Output row to cell mapping table */
    int		     orow_count ;	/* No. of rows in output row_table  */

    /* Default conversion character or string if cannot be converted */
    WChar_t	     defucsch   ;	/* Default character in UCS-4	    */ // JAC
    WChar_t	     defchar    ;	/* Default single/multi byte char   */ // JAC
    char	    *defstr     ;	/* Default string		    */
    int		     defstrlen  ;	/* Length of the default string     */
// BRL    wint_t	     maxucs	;	/* Maximum allowable UCS value	    */
} ;

/*
 * Function prototypes
 */
extern int __input_ucs4 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __input_ucs2 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __input_utf7 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __input_utf8 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __output_ucs4(_LC_fcconv_iconv_t *, uchar_t * , int, ucs4_t) ;
extern int __output_ucs2(_LC_fcconv_iconv_t *, uchar_t * , int, ucs4_t) ;
extern int __output_utf7(_LC_fcconv_iconv_t *, uchar_t * , int, ucs4_t) ;
extern int __output_utf8(_LC_fcconv_iconv_t *, uchar_t * , int, ucs4_t) ;

extern int __from_ucs_exec (_LC_fcconv_iconv_t *, uchar_t** , size_t *,
						  uchar_t** , size_t *);
extern int __to_ucs_exec   (_LC_fcconv_iconv_t *, uchar_t** , size_t *,
						  uchar_t** , size_t *);
extern int __cs_to_ucs_exec(_LC_fcconv_iconv_t *, uchar_t** , size_t *,
						  uchar_t** , size_t *);
extern int __sb_to_ucs_exec(_LC_fcconv_iconv_t *, uchar_t** , size_t *,
						  uchar_t** , size_t *);
extern int __sb_to_sb_exec (_LC_fcconv_iconv_t *, uchar_t** , size_t *,
						  uchar_t** , size_t *);
extern int __mb_to_mb_exec (_LC_fcconv_iconv_t *, uchar_t** , size_t *,
						  uchar_t** , size_t *);

extern int __sjis_index       	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __deckanji_index   	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __sdeckanji_index  	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __eucjp_index      	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __jisx0201gr_index 	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __jisx0208gl_index 	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __jisx0208gr_index 	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __jisx0212gl_index 	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __jisx0212gr_index 	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __jisxudc_index    	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;

extern int __dechanzi_index   	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __dechanyu_index   	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __euctw_index      	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __big5_index       	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __hkscs_index       	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __gb2312_index     	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __gb2312gl_index   	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __gbk_index        	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __gb18030_index     	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __cns116431gl_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __cns116431gr_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __cns116432gl_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __cns116432gr_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __deccns11643_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __deccns11643udc_index(_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __dtscs_index      	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __big5glgr_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __big5udc_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __gb2312udc_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;

extern int __deckorean_index  	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __ksc5601gl_index  	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;
extern int __cp949_index	 (_LC_fcconv_iconv_t *, uchar_t **, int) ;

/*
 * Special font character set conversion functions
 */
extern int __eucjp_to_jisx0201gr    	(WChar_t) ; // JAC
extern int __eucjp_to_jisx0208gl    	(WChar_t) ; // JAC
extern int __eucjp_to_jisx0208gr    	(WChar_t) ; // JAC
extern int __eucjp_to_jisx0212gl    	(WChar_t) ; // JAC
extern int __eucjp_to_jisx0212gr    	(WChar_t) ; // JAC
extern int __deckanji_to_jisxudc    	(WChar_t) ; // JAC
extern int __gb2312_to_gb2312gl     	(WChar_t) ; // JAC
extern int __gb2312_to_gb2312gr     	(WChar_t) ; // JAC
extern int __gbk_to_gbkglgr	    	(WChar_t) ; // JAC
extern int __euctw_to_cns116431gl   	(WChar_t) ; // JAC
extern int __euctw_to_cns116431gr   	(WChar_t) ; // JAC
extern int __euctw_to_cns116432gl   	(WChar_t) ; // JAC
extern int __euctw_to_cns116432gr   	(WChar_t) ; // JAC
extern int __euctw_to_cns116433gl   	(WChar_t) ; // JAC
extern int __euctw_to_cns116433gr   	(WChar_t) ; // JAC
extern int __euctw_to_cns116434gl   	(WChar_t) ; // JAC
extern int __euctw_to_cns116434gr   	(WChar_t) ; // JAC
extern int __dechanyu_to_dtscs      	(WChar_t) ; // JAC
extern int __dechanyu_to_deccns11643	(WChar_t) ; // JAC
extern int __dechanyu_to_deccns11643udc	(WChar_t) ; // JAC
extern int __big5_to_big5glgr		(WChar_t) ; // JAC
extern int __big5_to_big5udc		(WChar_t) ; // JAC
extern int __dechanzi_to_gb2312udc	(WChar_t) ; // JAC
extern int __deckorean_to_ksc5601gl	(WChar_t) ; // JAC
extern int __deckorean_to_ksc5601gr	(WChar_t) ; // JAC

/*
 * UCS's UDC mapping functions
 */
extern WChar_t __UDC_to_sjis	 (ucs4_t) ; // JAC
extern WChar_t __UDC_to_big5	 (ucs4_t) ; // JAC
extern WChar_t __UDC_to_dechanyu (ucs4_t) ; // JAC
extern WChar_t __UDC_to_dechanzi (ucs4_t) ; // JAC
extern WChar_t __UDC_to_deckanji (ucs4_t) ; // JAC
extern WChar_t __UDC_to_eucjp	 (ucs4_t) ; // JAC
extern WChar_t __UDC_to_euctw	 (ucs4_t) ; // JAC
extern WChar_t __UDC_to_sdeckanji(ucs4_t) ; // JAC
extern WChar_t __UDC_to_gb18030  (ucs4_t) ; // JAC
extern WChar_t __UDC_to_gb2312   (ucs4_t) ; // JAC
extern WChar_t __UDC_to_gbk	 (ucs4_t) ; // JAC
extern WChar_t __UDC_to_hkscs	 (ucs4_t) ; // JAC
extern WChar_t __UDC_to_deckorean(ucs4_t) ; // JAC
extern WChar_t __UDC_to_cp949    (ucs4_t) ; // JAC


/*
 * Special UCS mapping functions
 */
extern WChar_t __UCS_to_gb18030(ucs4_t) ; // JAC	BRL

/*
 * Function name mappings
 */
#define	__gb2312gr_index	__gb2312_index
#define	__gbkglgr_index		__gbk_index
#define	__ksc5601gr_index	__deckorean_index
#define	__UCS_to_sjis		NULL
#define	__UCS_to_big5		NULL
#define	__UCS_to_dechanyu	NULL
#define	__UCS_to_dechanzi	NULL
#define	__UCS_to_deckanji	NULL
#define	__UCS_to_eucjp		NULL
#define	__UCS_to_sdeckanji	NULL
#define	__UCS_to_gbk		NULL
#define	__UCS_to_euctw		NULL
#define	__UCS_to_deckorean	NULL
#define	__UCS_to_cp949		NULL
#define	__UCS_to_gb2312		NULL
#define	__UCS_to_hkscs		NULL

#endif	/* FCCONV_H */
