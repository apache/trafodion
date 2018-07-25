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
 * $Log: iconv_gen.c,v $
 * Revision 1.1.19.1  2001/12/07  15:37:53
 *  * Add checking for the CONV_NO_UDC flag to disable mapping to UDC.
 *  * Merge codes from kernel/bsd.
 *
 * Revision 1.1.15.2  2000/12/11  19:41:22
 * 	Enable table mapping for private use area character if 0 is returned
 * 	from the UDC function.
 *
 * Revision 1.1.15.1  2000/10/16  18:44:52
 * 	COSIX.Zulu to Yankee merge for GB18030 support.
 *
 * Revision 1.1.13.1  2000/08/07  14:33:48
 * 	Support GB18030 and map UTF-8 surrogate pair to the right UCS code.
 *
 * Revision 1.1.11.2  2000/01/19  19:56:15
 * 	Fix cut & paste error in input_ucs2().
 * 	[2000/01/14  14:38:48  Long_Man]
 *
 * Revision 1.1.11.1  2000/01/13  20:25:46
 * 	Remove the old conversion routine in favor of the enhanced
 * 	__<codeset>_index() routine, and add algorithmic UDC conversion
 * 	support. Also add a number of conversion routines to support
 * 	special font charset to Unicode conversion.
 *
 * Revision 1.1.9.4  1998/08/21  17:51:25
 * 	Fix iconv dump problem when converting ISO8859-1 to UCS-2.
 * 	[1998/08/20  18:50:33  Long_Man]
 *
 * Revision 1.1.9.3  1998/05/04  15:21:21
 * 	Fix typo error in output_ucs2().
 * 	[1998/05/04  14:54:26  Long_Man]
 * 
 * Revision 1.1.9.2  1998/03/26  18:56:28
 * 	Prepend __ to global functions to prevent namespace pollution
 * 	and fix problem in UCS-2 and UCS-4 routines.
 * 	[1998/03/23  14:43:44  Long_Man]
 * 
 * Revision 1.1.5.6  1997/06/17  21:29:46
 * 	Fix QAR 53565 by guarding against the boundary case.
 * 	[1997/06/16  21:37:44  Long_Man]
 * 
 * Revision 1.1.5.5  1997/04/07  19:04:57
 * 	Fix QAR 52035 by adjusting input pointer to failure location.
 * 	[1997/04/03  22:40:55  Long_Man]
 * 
 * Revision 1.1.5.4  1997/02/24  21:01:51
 * 	Fix UCS locale build by not writing BOM if it causes E2BIG error.
 * 	[1997/02/21  21:36:41  Long_Man]
 * 
 * 	Fix QAR 51653: Missing first character in UCS-2 output if BOM enabled.
 * 	[1997/02/21  14:58:45  Long_Man]
 * 
 * Revision 1.1.5.3  1997/01/07  15:58:47
 * 	Add new converters for cp437 and cp850 codeset support.
 * 	[1996/12/24  16:15:44  Long_Man]
 * 
 * Revision 1.1.5.2  1996/11/22  17:02:32
 * 	Improve execution speed & other minor bug fixes.
 * 	[1996/11/12  17:19:26  Long_Man]
 * 
 * 	Consolidate UCS iconv converter binaries & support UCS-2.
 * 	[1996/10/28  21:11:39  Long_Man]
 * 
 * Revision 1.1.2.4  1995/07/31  14:37:37
 * 	QAR 35010 - Verify the input buffer contains a full
 * 	wchar_t for processing from UCS-4/UTF-8 to a target
 * 	codeset. If inlen < wchar_t, set errno=EINVAL and
 * 	return ICONV_TRUNC.
 * 	[1995/07/12  14:41:34  Bill_Fountas]
 * 
 * 	Fix pointer and length settings for E2BIG errors
 * 	[1995/06/27  13:17:11  Bill_Fountas]
 * 
 * Revision 1.1.2.3  1995/06/30  14:45:11
 * 	Fix pointer and length settings for E2BIG errors
 * 	[1995/06/30  13:31:45  Bill_Fountas]
 * 
 * Revision 1.1.2.2  1995/06/07  16:12:48
 * 	Initial iconv Unicode support check in
 * 	[1995/06/02  14:59:11  Kelly_Mulheren]
 * 
 * $EndLog$
 */

#ifndef _KERNEL
#include <string.h>
#else
#include <stddef.h>
#endif
/*#include <sys/malloc.h> //BRL & JAC */
#include "fcconv.h"
#include "multi-byte.h"
/*
 * Macros to reverse the byte ordering
 */
#define	REVERSE_UCS4_BYTE(ucs4)	((((ucs4) & 0x000000ff) << 24) | \
				 (((ucs4) & 0x0000ff00) <<  8) | \
				 (((ucs4) & 0x00ff0000) >>  8) | \
				 (((ucs4) & 0xff000000) >> 24))
#define	REVERSE_UCS2_BYTE(ucs2)	((((ucs2) & 0x00ff) << 8) | \
				 (((ucs2) & 0xff00) >> 8))

/*
 * Macro to set error status and returned value
 */
#ifdef DONT_NEED_THIS /* JAC */
#ifndef _KERNEL
#define	SET_ERR_RETURN(error) \
    {								\
	switch (error) 						\
	{							\
	    case ERR_INVALID_CHAR    :				\
		error = EILSEQ ; retval = ICONV_INVAL ; break ;	\
	    case ERR_INPUT_INCOMPLETE:				\
		error = EINVAL ; retval = ICONV_TRUNC ; break ;	\
	    case ERR_BUFFER_OVERRUN  :				\
		error = E2BIG  ; retval = ICONV_OVER  ; break ;	\
	}							\
	_Seterrno(error ) ;					\
	return   (retval) ;					\
    }
#else
#define	SET_ERR_RETURN(error) \
    {								\
	switch (error) 						\
	{							\
	    case ERR_INVALID_CHAR    :				\
		error = EILSEQ ; retval = ICONV_INVAL ; break ;	\
	    case ERR_INPUT_INCOMPLETE:				\
		error = EINVAL ; retval = ICONV_TRUNC ; break ;	\
	    case ERR_BUFFER_OVERRUN  :				\
		error = E2BIG  ; retval = ICONV_OVER  ; break ;	\
	}							\
	return   (retval) ;					\
    }
#endif
#endif /* DONT_NEED_THIS - JAC */

/**********
 * __from_ucs_exec
 *
 * Driver routine for converting from UCS/UTF.
 **********/
#ifdef USING_OPEN_SOURCE_from_ucs_exec /* JAC */
int 
__from_ucs_exec (_LC_fcconv_iconv_t *cd,
		 uchar_t** in_buff , size_t *in_bytes_left ,
		 uchar_t** out_buff, size_t *out_bytes_left)
{
    uchar_t	*outptr	  ;	/* Pointer to output buffer	*/
    uchar_t	*inptr	  ;	/* Pointer to input buffer	*/
    uchar_t	*inptr2	  ;	/* Pointer to input buffer	*/
    size_t	outlen    ;	/* Number of outbytes left	*/
    size_t	inlen	  ;	/* Number of inbytes left	*/
    WChar_t	new_char  ;	/* converted character		*/ /*JAC */
    int		inword	  ;	/* low word of input		*/
    unsigned int char_size ;	/* Output size of MB chars	*/ /*BRL & JAC */
    int		error	  ;	/* Error code			*/
    int		retval	  ;	/* Return value			*/
    int		idx	  ;
    int		nomap	  ;	/* No ASCII mapping		*/
    int		d2map	  ;	/* ASCII direct map		*/
    int		cfirst	  ;	/* Conversion function first	*/
    int		cnext	  ;	/* Conversion function next	*/
    cfunc_t	conv	  ;

    if (!in_buff) 
	return(ICONV_DONE) ;

    outptr = *out_buff			   ;
    outlen = *out_bytes_left		   ;
    inptr  = *in_buff			   ;
    inlen  = *in_bytes_left		   ;
    inptr2 = inptr			   ;
    error  = 0				   ;
    nomap  = cd->flags & CONV_ASCII_ONOMAP ;
    d2map  = cd->flags & CONV_ASCII_ODMAP2 ;
    cfirst = cd->flags & CONV_FUNC_FIRST   ;
    cnext  = cd->flags & CONV_FUNC_NEXT    ;
    conv   = (cfunc_t)cd->outfunc	   ;

    /**********
     * perform conversion
     **********/
    for ( ; inlen > 0 ; inlen -= inptr2 - inptr, inptr = inptr2) {
	inword = (*cd->infunc)(cd, &inptr2, inlen) ;

	if (inword < 0) {
	    error = inword ;
	    break	   ;
	}
	/**********
	 * Translate input word to a new character
	 **********/
	if ((nomap || d2map) && _ISASCII(inword))
	{
	    new_char  = nomap ? inword : cd->ocell2_tab[0][inword] ;
	}
	else if (UCS_UDC(inword)) {
	    /*
	     * Map UCS UDC character to the corresponding multibyte 
	     * character, if applicable, and CONV_NO_UDC flag not defined.
	     */
	    new_char = (cd->udcfunc && !(cd->flags & CONV_NO_UDC))
		     ? (*cd->udcfunc)(inword) : BAD ;
	    /*
	     * Do a table mapping if the UDC function returns 0
	     */
	    if (new_char == 0)
	    {
		GET_OVAL(cd, inword, new_char) ;
	    }
	}
	else if (cfirst && conv)
	{
	    /*
	     * Invoke the conversion function first before doing a table
	     * lookup.
	     */
	    if ((new_char = (*conv)(inword)) == BAD)
	    {
		if (cd->maxucs)
		    if (inword > cd->maxucs)
		    {
			error = ERR_INVALID_CHAR ;	/* Invalid sequence */
			break			 ;
		    }
		GET_OVAL(cd, inword, new_char) ;
	    }
	}
	else
	{
	    if (cd->maxucs)
		if (inword > cd->maxucs)
		{
		    error = ERR_INVALID_CHAR ;	/* Invalid sequence */
		    break		     ;
		}
	    GET_OVAL(cd, inword, new_char) ;

	    /**********
	     * invoke special converion function if defined
	     **********/
	    if (conv)
	    {
		/*
		 * If cnext is defined, pass input word through conv function
		 * if table lookup fails. Otherwise, pass the new character
		 * to conversion function to modify it.
		 */
		if (cnext)
	    	{
		    if (new_char == BAD)
			new_char = (*conv)(inword) ;
	    	}
		else if (new_char != BAD)
		    new_char = (*conv)(new_char) ;
	    }
	}
	/**********
	 * valid character?
	 **********/
	if (new_char == BAD) {
	    if (cd->defchar != 0)
		new_char = cd->defchar ;
	    else if (cd->defstr) {
		if (cd->defstrlen == 0)
		    continue ;	/* Skip the invalid character */
		if ((size_t)cd->defstrlen > outlen) {                      /*BRL & JAC */
		    error = ERR_BUFFER_OVERRUN ; /* Output buf overflow */
		    break			   ;
		}
		memcpy(outptr, cd->defstr, cd->defstrlen) ;
		outptr += cd->defstrlen ;
		outlen -= cd->defstrlen ;
		continue		;
	    }
	    else if (inword == 0xFFFD) {	/* Replacement character ? */
		/*
		 * Skip replacement character, if found
		 */
		continue ;
	    }
	    else {
		error = ERR_INVALID_CHAR ;
		break			 ;
	    }
	}

	/**********
	 * calculate the character size in byte
	 **********/
	if ((new_char & 0xffffff00) == 0)
	    char_size = 1 ;
	else if ((new_char & 0xffff0000) == 0)
	    char_size = 2 ;
	else if ((new_char & 0xff000000) == 0)
	    char_size = 3 ;
	else
	    char_size = 4 ;

	    /**********
	     * have we exceeded size of output buffer?
	     **********/
	if (outlen < char_size) {
	    error = ERR_BUFFER_OVERRUN ;
	    break		       ;
	}

	/**********
	 * Output the bytes
	 **********/
	switch (char_size) {
	    case 4: *outptr++ = (new_char >> 24) & 0xff;
	    case 3: *outptr++ = (new_char >> 16) & 0xff;
	    case 2: *outptr++ = (new_char >>  8) & 0xff;
	    case 1: *outptr++ =  new_char	 & 0xff;
		    break ;
	}
	outlen -= char_size ;
    }


    /**********
     * set output parameters
     **********/
    *in_buff	    = inptr  ;
    *out_buff	    = outptr ;
    *in_bytes_left  = inlen  ;
    *out_bytes_left = outlen ;
    if (!error)
	return(ICONV_DONE) ;
    SET_ERR_RETURN(error) ;
}   /* __from_ucs_exec */
#endif /* USING_OPEN_SOURCE_from_ucs_exec // JAC */

/**********
 * __sb_to_ucs_exec
 *
 * Driver routine for converting from single-byte to UCS/UTF.
 **********/

#ifdef USING_OPEN_SOURCE_sb_to_ucs_exec /* JAC */
int 
__sb_to_ucs_exec(_LC_fcconv_iconv_t *cd,
		 uchar_t** in_buff , size_t *in_bytes_left ,
		 uchar_t** out_buff, size_t *out_bytes_left)
{
    uchar_t	*outptr	 ;	/* Pointer to output buffer	*/
    uchar_t	*inptr	 ;	/* Pointer to input buffer	*/
    size_t	outlen   ;	/* Number of outbytes left	*/
    size_t	inlen	 ;	/* Number of inbytes left	*/
    WChar_t	new_char ;	/* converted character		*/ /*JAC */
    WChar_t	input_ch ;	/* converted character	   */ /*JAC */
    int		retval	 ;
    int		error	 ;	/* Error code		   */
    int		nomap	 ;	/* No ASCII mapping	   */
    int		d2map	 ;	/* ASCII direct map	   */

    if (!in_buff) 
	return ICONV_DONE;

    inptr  = *in_buff		;
    inlen  = *in_bytes_left	;
    outptr = *out_buff		;
    outlen = *out_bytes_left 	;
    error  = 0			;
    nomap  = cd->flags & CONV_ASCII_INOMAP ;
    d2map  = cd->flags & CONV_ASCII_IDMAP2 ;

    /**********
     * perform conversion
     **********/
    for ( ; inlen > 0 ; inptr++, inlen--) {
	input_ch = *inptr ;
	if ((nomap || d2map) && _ISASCII(input_ch))
	    new_char = nomap ? input_ch : cd->icell2_tab[0][input_ch] ;
	else
	{
	    GET_IVAL(cd, input_ch, new_char) ;
	    /*
	     * Restrict output to less than ICONV_MAXUCS, if defined
	     */
	    if (cd->maxucs && (new_char != BAD))
		if (new_char > cd->maxucs)
		    new_char = BAD ;

	    /**********
	     * valid character?
	     **********/
	    if (new_char == BAD) {
		if (cd->defucsch) 
		    new_char = cd->defucsch ;
		else if (cd->defstr && (cd->defstrlen == 0))
		    continue ;	/* Skip that character */
		else {
		    error = ERR_INVALID_CHAR	;
		    break			;
		}
	    }
	}
	retval = (*cd->outfunc)(cd, outptr, outlen, new_char) ;
	if (retval < 0)
	{
	    error = retval ;
	    break	   ;
	}
	outptr += retval ;
	outlen -= retval ;
    }

    /**********
     * set output parameters
     **********/
    *in_buff	    = inptr	;
    *out_buff	    = outptr	;
    *in_bytes_left  = inlen 	;
    *out_bytes_left = outlen	;
    if (!error)
	return(ICONV_DONE) ;

    SET_ERR_RETURN(error) ;
}   /* __sb_to_ucs_exec */
#endif /* USING_OPEN_SOURCE_sb_to_ucs_exec // JAC */

/**********
 * __sb_to_sb_exec
 *
 * Driver routine for converting from single-byte to single-byte via UCS.
 **********/

#ifdef USING_OPEN_SOURCE_sb_to_sb_exec /* JAC */
int 
__sb_to_sb_exec(_LC_fcconv_iconv_t *cd,
		uchar_t** in_buff , size_t *in_bytes_left ,
		uchar_t** out_buff, size_t *out_bytes_left)
{
    uchar_t	*inptr   ;	/* Pointer to input buffer  */
    uchar_t	*outptr  ;	/* Pointer to output buffer */
    size_t	inlen    ;	/* Number of inbytes left   */
    size_t	outlen   ;	/* Number of outbytes left  */
    WChar_t	new_char ;	/* converted character	    */ /*JAC */
    WChar_t	input_ch ;	/* Input character	    */ /*JAC */
    int		retval	 ;	/* Function return value    */
    int		error	 ;	/* Error code		    */
    int		inomap	 ;	/* No input ASCII mapping   */
    int		id2map	 ;	/* INput ASCII direct map   */
    int		onomap	 ;	/* No output ASCII mapping  */
    int		od2map	 ;	/* Output ASCII direct map  */

    if (!in_buff) 
	return ICONV_DONE;

    inptr  = *in_buff			   ;
    inlen  = *in_bytes_left		   ;
    outptr = *out_buff			   ;
    outlen = *out_bytes_left		   ;
    error  = 0				   ;
    inomap = cd->flags & CONV_ASCII_INOMAP ;
    id2map = cd->flags & CONV_ASCII_IDMAP2 ;
    onomap = cd->flags & CONV_ASCII_ONOMAP ;
    od2map = cd->flags & CONV_ASCII_ODMAP2 ;

    /**********
     * perform conversion
     **********/
    while (inlen > 0) {

	input_ch = *inptr ;
	if ((inomap || id2map) && _ISASCII(input_ch))
	    new_char = inomap ? input_ch : cd->icell2_tab[0][input_ch] ;
	else
	{
	    GET_IVAL(cd, input_ch, new_char) ;

	    /**********
	     * valid character?
	     **********/
	    if (new_char == BAD) {
		if (cd->defucsch) 
		    new_char = cd->defucsch ;
		else if (cd->defstr && (cd->defstrlen == 0)) {
		    inptr++, inlen-- ;
		    continue ;	/* Skip that character */
		}
		else if (cd->defstrlen > 0) {
		    if (outlen < cd->defstrlen) {
			/* Not enough output buffer */
			error = ERR_BUFFER_OVERRUN ;
			break			   ;
		    }
		    /* Copy default string to output */
		    bcopy(cd->defstr, (char *)outptr, cd->defstrlen) ;
		    inptr++, inlen--	    ;
		    outptr += cd->defstrlen ;
		    outlen -= cd->defstrlen ;
		    continue		    ;
		}
		else {
		    error = ERR_INVALID_CHAR ;
		    break		     ;
		}
	    }
	}

	if (((ssize_t)outlen) <= 0) {
	    error = ERR_BUFFER_OVERRUN	;
	    break			;
	}

	input_ch = new_char ;
	if ((onomap || od2map) && _ISASCII(input_ch))
	    new_char = onomap ? input_ch : cd->ocell2_tab[0][input_ch] ;
	else
	{
	    GET_OVAL(cd, input_ch, new_char) ;

	    /**********
	     * valid character?
	     **********/
	    if (new_char == BAD) {
		if (cd->defchar)
		    new_char = cd->defchar ;
		else if (cd->defstr && (cd->defstrlen == 0)) {
		    inptr++, inlen-- ;
		    continue ;	/* Skip that charcter */
		}
		else if (cd->defstrlen > 0) {
		    if (outlen < cd->defstrlen) {
			/* Not enough output buffer */
			error = ERR_BUFFER_OVERRUN ;
			break			   ;
		    }
		    /* Copy default string to output */
		    bcopy(cd->defstr, (char *)outptr, cd->defstrlen) ;
		    inptr++, inlen--	    ;
		    outptr += cd->defstrlen ;
		    outlen -= cd->defstrlen ;
		    continue		    ;
		}
		else {
		    error = ERR_INVALID_CHAR ;
		    break		     ;
		}
	    }
	}
	*outptr = new_char ;
	inptr ++, inlen -- ;
	outptr++, outlen-- ;
    }

    /**********
     * set output parameters
     **********/
    *in_buff	    = inptr  ;
    *out_buff	    = outptr ;
    *in_bytes_left  = inlen  ;
    *out_bytes_left = outlen ;
    if (!error)
	return(ICONV_DONE) ;

    SET_ERR_RETURN(error) ;
}   /* __sb_to_sb_exec */
#endif /* USING_OPEN_SOURCE_sb_to_sb_exec // JAC */

/**********
 * __to_ucs_exec
 *
 * Generic driver routine for converting from any character set to UCS.
 * It is assumed that ASCII is a proper subset of the character set except
 * for UCS characters.
 **********/

#ifdef USING_OPEN_SOURCE_to_ucs_exec /* JAC */
int 
__to_ucs_exec(_LC_fcconv_iconv_t *cd,
	      uchar_t** in_buff , size_t *in_bytes_left ,
	      uchar_t** out_buff, size_t *out_bytes_left)
{
    uchar_t    *inptr	;
    uchar_t    *inptr2	;
    uchar_t    *outptr	;
    size_t	inlen	;
    size_t	outlen	;
    int		inword  ;
    WChar_t	outword ; /*JAC */
    int		retval  ;
    int		error	;	/* Error code			*/
    int		chkasc  ;	/* Check for ASCII		*/
    int		inomap  ;	/* No mapping needed for ASCII	*/

    if (!in_buff) 
	return ICONV_DONE;

    inptr  = *in_buff		;
    inlen  = *in_bytes_left	;
    outptr = *out_buff		;
    outlen = *out_bytes_left	;
    inptr2 = inptr		;
    error  = 0			;
    chkasc = cd->flags & (CONV_ASCII_INOMAP|CONV_ASCII_IDMAP2)	;
    inomap = cd->flags &  CONV_ASCII_INOMAP			;

    /**********
     * Perform conversion
     **********/
    if (cd->srccode == UCS) while (inlen > 0) {
	/*
	 * infunc will return UCS-4 value
	 */
	if ((inword = (*cd->infunc)(cd, &inptr2, inlen)) < 0)
	{
	    error = inword ;
	    break	   ;
	}
	retval = (*cd->outfunc)(cd, outptr, outlen, inword) ;
	if (retval < 0)
	{
	    error = retval ;
	    break	   ;
	}
	inlen  -= inptr2 - inptr ;
	inptr   = inptr2	 ;
	outlen -= retval	 ;
	outptr += retval	 ;
    }
    else for ( ; inlen > 0 ; inlen  -= inptr2 - inptr, inptr = inptr2) {
	if (chkasc && _ISASCII(inword = *inptr2))
	{
	    inptr2++ ;
	    outword = inomap ? inword : cd->icell2_tab[0][inword] ;
	}
	else
	{
	    /*
	     * infunc will return UCS-4 table index
	     */
	    if ((inword = (*cd->infunc)(cd, &inptr2, inlen)) < 0)
	    {
		error = inword	;
		break		;
	    }
	    /*
	     * Convert table index into UCS-4
	     */
	    if (inword == BAD)
		outword = BAD ;
	    else if (IS_UCODE(inword))
		outword = GET_UCODE(inword) ;
	    else
	    {
		/*
		 * Check for UDC
		 */
		if (ISIDXU(inword))
		    outword = IDXU_UCS(inword) ;
		else
		{
		    GET_IVAL(cd, inword, outword)
		}
		/*
		 * Restrict output to less than ICONV_MAXUCS, if defined
		 */
		if (cd->maxucs && (outword != BAD))
		    if (outword > cd->maxucs)
			outword = BAD ;
	    }

	    if ((outword == BAD) || 
	       ((cd->flags & CONV_NO_UDC) && UCS_UDC(outword))) {
		if (cd->defucsch != 0)
		    outword = cd->defucsch ;
		else if (cd->defstr && (cd->defstrlen == 0))
		    continue ;	/* Ignore this character */
		else
		/*
		 * Invalid character
		 * Setup indata & adjust error position as
		 * there may be a E2BIG error before that.
		 */
		{
		    error = ERR_INVALID_CHAR ;
		    break		     ;
		}
	    }
	}

	retval = (*cd->outfunc)(cd, outptr, outlen, outword) ;
	if (retval < 0)
	{
	    error = retval ;
	    break 	   ;
	}
	outlen -= retval ;
	outptr += retval ;
    }

    /**********
     * set output parameters
     **********/
    *in_buff	    = inptr  ;
    *out_buff	    = outptr ;
    *in_bytes_left  = inlen  ;
    *out_bytes_left = outlen ;
    if (!error)
	return(ICONV_DONE) ;

    SET_ERR_RETURN(error) ;
}   /* __to_ucs_exec */
#endif /* USING_OPEN_SOURCE_to_ucs_exec // JAC */

/**********
 * __cs_to_ucs_exec
 *
 * Special driver routine for converting from character set which may
 * not contain ASCII to UCS.
 **********/

#ifdef USING_OPEN_SOURCE_cs_to_ucs_exec /* JAC */
int 
__cs_to_ucs_exec(_LC_fcconv_iconv_t *cd,
	         uchar_t** in_buff , size_t *in_bytes_left ,
	         uchar_t** out_buff, size_t *out_bytes_left)
{
    uchar_t    *inptr	;
    uchar_t    *inptr2	;
    uchar_t    *outptr	;
    size_t	inlen	;
    size_t	outlen	;
    int		inword  ;
    WChar_t	outword ; /*JAC */
    int		error	;	/* Error code */
    int		retval  ;

    if (!in_buff) 
	return ICONV_DONE;

    inptr  = *in_buff		;
    inlen  = *in_bytes_left	;
    outptr = *out_buff		;
    outlen = *out_bytes_left	;
    inptr2 = inptr		;
    error  = 0			;

    /**********
     * Perform conversion
     **********/
    for ( ; inlen > 0 ; inlen -= inptr2 - inptr, inptr = inptr2) {
	/*
	 * infunc will return UCS-4 table index
	 */
	 if ((inword = (*cd->infunc)(cd, &inptr2, inlen)) < 0)
	 {
	    error = inword ;
	    break	   ;
	 }
	 /*
	  * Convert table index into UCS-4
	  */
	 if (inword != BAD)
	 {
	    /*
	     * Check for UDC
	     */
	    if (ISIDXU(inword))
		outword = IDXU_UCS(inword) ;
	    else
	    {
		GET_IVAL(cd, inword, outword)
	    }
	    /*
	     * Restrict output to less than ICONV_MAXUCS, if defined
	     */
	    if (cd->maxucs && (outword != BAD))
		if (outword > cd->maxucs)
		    outword = BAD ;
	}
	else
	    outword = BAD ;

	 if ((outword == BAD) || 
	    ((cd->flags & CONV_NO_UDC) && UCS_UDC(outword))) {
	    if (cd->defucsch != 0)
		    outword = cd->defucsch ;
	    else if (cd->defstr && (cd->defstrlen == 0))
		    continue ;	/* Ignore this character */
	    else
	    /*
	     * Invalid character
	     * Setup indata & adjust error position as
	     * there may be a E2BIG error before that.
	     */
	    {
		error = ERR_INVALID_CHAR ;
		break		         ;
	    }
	}

	retval = (*cd->outfunc)(cd, outptr, outlen, outword) ;
	if (retval < 0)
	{
	    error = retval ;
	    break 	   ;
	}
	outlen -= retval ;
	outptr += retval ;
    }

    /**********
     * set output parameters
     **********/
    *in_buff	    = inptr  ;
    *out_buff	    = outptr ;
    *in_bytes_left  = inlen  ;
    *out_bytes_left = outlen ;
    if (!error)
	return(ICONV_DONE) ;

    SET_ERR_RETURN(error) ;
}   /* __cs_to_ucs_exec */
#endif /* USING_OPEN_SOURCE_cs_to_ucs_exec // JAC */

/**********
 * __mb_to_mb_exec
 *
 * Driver routine for converting from multi-byte to multi-byte via UCS.
 **********/

#ifdef USING_OPEN_SOURCE_mb_to_mb_exec /* JAC */
int 
__mb_to_mb_exec(_LC_fcconv_iconv_t *cd,
		uchar_t** in_buff , size_t *in_bytes_left ,
		uchar_t** out_buff, size_t *out_bytes_left)
{
    uchar_t    *inptr	 ;
    uchar_t    *inptr2	 ;
    uchar_t    *outptr	 ;
    size_t	inlen	 ;
    size_t	outlen	 ;
    int		inword   ;
    WChar_t	outword  ; /*JAC */
    int		char_size;	/* Multi-byte char size     */
    int		error	 ;	/* Error code		    */
    int		retval	 ;
    int		inomap	 ;	/* No input ASCII mapping   */
    int		id2map	 ;	/* Input ASCII direct map   */
    int		onomap	 ;	/* No output ASCII mapping  */
    int		od2map	 ;	/* Output ASCII direct map  */
    int		idx	 ;

    if (!in_buff) 
	return ICONV_DONE;

    inptr  = *in_buff		;
    inptr2 = inptr		;
    inlen  = *in_bytes_left	;
    outptr = *out_buff		;
    outlen = *out_bytes_left	;
    inomap = cd->flags & CONV_ASCII_INOMAP ;
    id2map = cd->flags & CONV_ASCII_IDMAP2 ;
    onomap = cd->flags & CONV_ASCII_ONOMAP ;
    od2map = cd->flags & CONV_ASCII_ODMAP2 ;

    /**********
     * perform conversion
     **********/
    for ( ; inlen > 0 ; inlen -= inptr2 - inptr, inptr = inptr2) {
	/*
	 * infunc will return UCS-4 table index
	 */
	if ((inword = (*cd->infunc)(cd, &inptr2, inlen)) < 0)
	{
	    error = inword ;
	    break	   ;
	}

	/*
	 * Convert table index into UCS-4
	 */
	if ((inomap || id2map) && _ISASCII(inword))
	    outword = inomap ? inword : cd->icell2_tab[0][inword] ;
	else
	{
	    if (inword == BAD)
		outword = BAD ;
	    else if (IS_UCODE(inword))
		outword = GET_UCODE(inword) ;
	    else
	    {
		/* 
		 * Check for UDC
		 */
		if (ISIDXU(inword))
		    outword = IDXU_UCS(inword) ;
		else
		{
		    GET_IVAL(cd, inword, outword)
		}
		/*
		 * Restrict output to less than ICONV_MAXUCS, if defined
		 */
		if (cd->maxucs && (outword != BAD))
		    if (outword > cd->maxucs)
			outword = BAD ;
	    }

	    if (outword == BAD) {
		if (cd->defucsch != 0)
		    outword = cd->defucsch ;
		else if (cd->defstr && (cd->defstrlen == 0))
		    continue ;	/* Skip this character */
		else if (cd->defstrlen > 0) {
		    if (outlen < cd->defstrlen) {
			error = ERR_BUFFER_OVERRUN ;
			break			   ;
		    }
		    bcopy(cd->defstr, (char *)outptr, cd->defstrlen) ;
		    outptr += cd->defstrlen ;
		    outlen -= cd->defstrlen ;
		    continue		    ;
		}
		else
		/*
		 * Invalid character
		 */
		{
		    error = ERR_INVALID_CHAR ;
		    break		     ;
		}
	    }
	}

	/*
	 * Convert UCS-4 into output multibyte character
	 */
	inword = outword ;
	if ((onomap || od2map) && _ISASCII(inword))
	    outword = onomap ? inword : cd->ocell2_tab[0][inword] ;
	else if (UCS_UDC(inword))
	{
	    /*
	     * Map UCS UDC character to the corresponding multiple UDC 
	     * character, if applicable
	     */
	    outword = cd->udcfunc ? (*cd->udcfunc)(inword) : BAD ;
	}
	else
	{
	    GET_OVAL(cd, inword, outword) ;
	    if (outword == BAD) {
		if (cd->defchar)
			outword = cd->defchar ;
		else if ((cd->defstr) && (cd->defstrlen == 0))
			continue ;	/* Skip this character */
		else if (cd->defstrlen > 0)
		{
		    if (outlen < cd->defstrlen) {
			    error = ERR_BUFFER_OVERRUN ;
			    break		       ;
		    }
		    bcopy(cd->defstr, (char *)outptr, cd->defstrlen) ;
		    outptr += cd->defstrlen ;
		    outlen -= cd->defstrlen ;
		    continue		    ;
		}
		else
		/*
		 * Invalid character
		 */
		{
		    error = ERR_INVALID_CHAR ;
		    break		     ;
		}
	    }
	}

	/*
	 * calculate the character size in byte
	 */
	if ((outword & 0xffffff00) == 0)
	    char_size = 1 ;
	else if ((outword & 0xffff0000) == 0)
	    char_size = 2 ;
	else if ((outword & 0xff000000) == 0)
	    char_size = 3 ;
	else
	    char_size = 4 ;

	/*
	 * Check for output buffer overflow
	 */
	if (outlen < char_size) {
	    error = ERR_BUFFER_OVERRUN ;
	    break		       ;
	}

	/*
	 * Output the bytes
	 */
	switch (char_size) {
	    case 4: *outptr++ = (outword >> 24) & 0xff;
	    case 3: *outptr++ = (outword >> 16) & 0xff;
	    case 2: *outptr++ = (outword >>  8) & 0xff;
	    case 1: *outptr++ =  outword	& 0xff;
		    break ;
	}
	outlen -= char_size ;
    }

    /**********
     * set output parameters
     **********/
    *in_buff	    = inptr  ;
    *out_buff	    = outptr ;
    *in_bytes_left  = inlen  ;
    *out_bytes_left = outlen ;
    if (!error)
	return(ICONV_DONE) ;

    SET_ERR_RETURN(error) ;
}   /* __mb_to_mb_exec */
#endif /* USING_OPEN_SOURCE_mb_to_mb_exec // JAC */

/************************************************************************/
/*									*/
/*		Macros, constants & type definitions for		*/
/*	      UCS-2/UCS-4/UTF-7/UTF-8 input/output routines		*/
/*									*/
/************************************************************************/

/*
 * Surrogates related macros
 *
 * The surrogates Area consists of 1024 low-half surrogates and 1024 high-half
 * surrogates which are interpreted in pairs to access over a million codes.
 * The high surrogate characters are encoded in the range U+D800 -> U+DBFF.
 * The high surrogate character is always the first element of a
 * surrogate-pair.
 * The low surrogate characters are encoded in the range U+DC00 -> U+DFFF.
 * The low surrogate character is always the second element of a
 * surrogate-pair.
 * These code values are drawn from planes 1-16 of group 0 of UCS-4, that is,
 * the range of UCS-4 code values 0x010000 - 0x10ffff. The last two planes
 & (15 & 16) are reserved for private use.
 */
#define	MAX_UCS4_VALUE		0x10ffff /* Max UCS-4 value for surrogates */
#define	SURROGATE_MASK		0x03ff
#define	HSURROGATE_PL		0xd800
#define	LSURROGATE_PL		0xdc00
#define	IS_HSURROGATE(ucs2)  (((ucs2) &~SURROGATE_MASK) == HSURROGATE_PL )
#define	IS_LSURROGATE(ucs2)  (((ucs2) &~SURROGATE_MASK) == LSURROGATE_PL )
#define	GET_LSURROGATE(ucs4) (((ucs4) & SURROGATE_MASK)  | LSURROGATE_PL )
#define	GET_HSURROGATE(ucs4) (((((ucs4)-0x10000) >> 10)  & SURROGATE_MASK) \
							 | HSURROGATE_PL )
#define	SURROGATES_TO_UCS4(high, low)	((((low) & SURROGATE_MASK)	    | \
					 (((high ) & SURROGATE_MASK) << 10)) + \
					 0x10000)
#define	NEED_SURROGATE(ucs4)	((0x10000 <= (ucs4)) && ((ucs4) < 0x110000))

/*
 * UTF-8 related macros
 *
 * UTF-8 is a variable length encoding of Unicode using 8-bit sequences,
 * where the high bits indicate which part of the sequence a byte belongs to.
 * The following table shows how the bits in a Unicode value (or surrogate
 * pair) are distributed among the bytes in the UTF-8 encoding.
 *
 * Unicode value	1st byte    2nd byte	3rd byte    4th byte
 * -------------	--------    --------	--------    --------
 * 000000000gfedcba	0gfedcba
 *
 * 00000kjihgfedcba	110kjihg    10fedcba
 *
 * ponmlkjihgfedcba	1110ponm    10lkjihg	10fedcba
 *
 * 110110jihgfedcba	11110UTS    10RQponm	10lkjihg    10fedcba
 * 110111tsrqponmlk
 * where UTSRQ = tsrq + 1
 *
 * The following table shows the format of the first octet of a coded
 * character; the free bits available for coding the character are
 * indicated by an x. [Note 2]
 *
 *     Octets	       Binary	  Bits Free   Max. UCS-4
 *     1st of 1        0xxxxxxx        7       0000 007F
 *     1st of 2        110xxxxx        5       0000 07FF
 *     1st of 3        1110xxxx        4       0000 FFFF
 *     1st of 4        11110xxx        3       001F FFFF
 *     1st of 5        111110xx        2       03FF FFFF
 *     1st of 6        1111110x        1       7FFF FFFF
 *     2nd .. nth      10xxxxxx        6
 */
#define	MIN_2BYTE_UTF8		0x0000080
#define	MIN_3BYTE_UTF8		0x0000800
#define	MIN_4BYTE_UTF8		0x0010000
#define	MIN_5BYTE_UTF8		0x0200000
#define	MIN_6BYTE_UTF8		0x4000000
#define	IS_1BYTE_UTF8(ch)	( (ch) < 0x80)
#define	IS_2BYTE_UTF8(ch)	(((ch) & 0xe0) == 0xc0)
#define	IS_3BYTE_UTF8(ch)	(((ch) & 0xf0) == 0xe0)
#define	IS_4BYTE_UTF8(ch)	(((ch) & 0xf8) == 0xf0)
#define	IS_5BYTE_UTF8(ch)	(((ch) & 0xfc) == 0xf8)
#define	IS_6BYTE_UTF8(ch)	(((ch) & 0xfe) == 0xfc)
#define	IS_UCS_TO_1B_UTF8(ucs)	((ucs) < 0x80	    ) /* <= 1 byte  UTF-8 */
#define	IS_UCS_TO_2B_UTF8(ucs)	((ucs) < 0x800	    ) /* <= 2 bytes UTF-8 */
#define	IS_UCS_TO_3B_UTF8(ucs)	((ucs) < 0x10000    ) /* <= 3 bytes UTF-8 */
#define	IS_UCS_TO_4B_UTF8(ucs)	((ucs) < 0x200000   ) /* <= 4 bytes UTF-8 */
#define	IS_UCS_TO_5B_UTF8(ucs)	((ucs) < 0x4000000  ) /* <= 5 bytes UTF-8 */
#define	IS_UCS_TO_6B_UTF8(ucs)	((ucs) < 0x80000000L) /* <= 6 bytes UTF-8 */
#define	RD_1BYTE_UTF8(in)	  (in)[0]
#define	RD_2BYTE_UTF8(in)	(((in)[1] & 0x3f) | (((in)[0] & 0x1f) << 6 ))
#define	RD_3BYTE_UTF8(in)	(((in)[2] & 0x3f) | (((in)[1] & 0x3f) << 6 ) \
						  | (((in)[0] & 0x0f) << 12))
#define	RD_4BYTE_UTF8(in)	(((in)[3] & 0x3f) | (((in)[2] & 0x3f) << 6 ) \
						  | (((in)[1] & 0x3f) << 12) \
						  | (((in)[0] & 0x07) << 18))
#define	RD_5BYTE_UTF8(in)	(((in)[4] & 0x3f) | (((in)[3] & 0x3f) << 6 ) \
						  | (((in)[2] & 0x3f) << 12) \
						  | (((in)[1] & 0x3f) << 18) \
						  | (((in)[0] & 0x03) << 24))
#define	RD_6BYTE_UTF8(in)	(((in)[5] & 0x3f) | (((in)[4] & 0x3f) << 6 ) \
						  | (((in)[3] & 0x3f) << 12) \
						  | (((in)[2] & 0x3f) << 18) \
						  | (((in)[1] & 0x3f) << 24) \
						  | (((in)[0] & 0x01) << 30))
#define	WR_1BYTE_UTF8(out, ucs)	{ *(out)++ = (ucs) & 0x7f ; }
#define	WR_2BYTE_UTF8(out, ucs)	{ *(out)++ = (((ucs) >>  6) & 0x1f) | 0xc0 ; \
				  *(out)++ = ( (ucs)	    & 0x3f) | 0x80 ; }
#define	WR_3BYTE_UTF8(out, ucs)	{ *(out)++ = (((ucs) >> 12) & 0x0f) | 0xe0 ; \
				  *(out)++ = (((ucs) >>  6) & 0x3f) | 0x80 ; \
				  *(out)++ = ( (ucs)	    & 0x3f) | 0x80 ; }
#define	WR_4BYTE_UTF8(out, ucs)	{ *(out)++ = (((ucs) >> 18) & 0x07) | 0xf0 ; \
				  *(out)++ = (((ucs) >> 12) & 0x3f) | 0x80 ; \
				  *(out)++ = (((ucs) >>  6) & 0x3f) | 0x80 ; \
				  *(out)++ = ( (ucs)	    & 0x3f) | 0x80 ; }
#define	WR_5BYTE_UTF8(out, ucs)	{ *(out)++ = (((ucs) >> 24) & 0x03) | 0xf8 ; \
				  *(out)++ = (((ucs) >> 18) & 0x3f) | 0x80 ; \
				  *(out)++ = (((ucs) >> 12) & 0x3f) | 0x80 ; \
				  *(out)++ = (((ucs) >>  6) & 0x3f) | 0x80 ; \
				  *(out)++ = ( (ucs)	    & 0x3f) | 0x80 ; }
#define	WR_6BYTE_UTF8(out, ucs)	{ *(out)++ = (((ucs) >> 30) & 0x01) | 0xfc ; \
				  *(out)++ = (((ucs) >> 24) & 0x3f) | 0x80 ; \
				  *(out)++ = (((ucs) >> 18) & 0x3f) | 0x80 ; \
				  *(out)++ = (((ucs) >> 12) & 0x3f) | 0x80 ; \
				  *(out)++ = (((ucs) >>  6) & 0x3f) | 0x80 ; \
				  *(out)++ = ( (ucs)	    & 0x3f) | 0x80 ; }
/*
 * Check if the trailing bytes of a UTF-8 byte sequence is a valid one
 * The trailing bytes are valid only if it is of the form 10xxxxxx.
 * The macro will return ERR_INVALID_CHAR if an invalid byte is found.
 */
#define	CHECK_UTF8(in,len)			\
    {						\
	if (len > 1) {				\
	    register uchar_t *ptr = in  + 1 ;	\
	    register int      cnt = len - 1 ;	\
	    while (cnt-- > 0)			\
		if ((*ptr++ & 0xc0) != 0x80)	\
		    return(ERR_INVALID_CHAR) ;	\
	}					\
    }

#ifdef USING_OPEN_SOURCE_UTF7 /* JAC */
/*
 * UTF-7 related macros and constants
 *
 * UTF-7 is a 7-bit form of UCS Transformation Format. UTF-7 depends on some
 * definition of US-ASCII character subsets:
 *
 * Set D (directly encoded characters, derived from RFC 1521) consists of
 * the following characters:
 *	A-Z, a-z, 0-9, "'(),-./:?"
 *
 * Set O (optional direct characters) consists of the following 20 characters:
 *	"!\"#$%&*;<=>@[]^_`{|}"
 *
 * Set B (Modified Base 64) is the set of characters in the Base64 alphabet
 * defined in RFC 1521, excluding the pad character "=". 
 *
 */
#define	SHIFT_IN	'+'
#define	SHIFT_OUT	'-'
#define	SETD_SIZE	(sizeof(direct	) - 1)
#define	SETO_SIZE	(sizeof(optional) - 1)
#define	SETS_SIZE	(sizeof(spaces	) - 1)
#define	SETB_SIZE	(sizeof(base64	) - 1)

#define	READ_N_BITS(n)	  ((buffertemp = (table->bitbuffer >> (32-(n)))),  \
			   (table->bitbuffer	 <<= (n)),		   \
			   (table->bits_in_buffer -= (n)), buffertemp)
#define	WRITE_N_BITS(x,n) ((table->bitbuffer |=				   \
		(((x) & ~(-1L << (n))) << (32-(n)-table->bits_in_buffer))),\
		table->bits_in_buffer += (n))

static const char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" ;
static const char direct[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'(),-./:?" ;
static const char optional[] =
    "!\"#$%&*;<=>@[]^_`{|}" ;
static const char spaces[] =
    " \011\015\012" ;	/* Space, tab, return, line feed */
#endif /* USING_OPEN_SOURCE_UTF7 // JAC */

#ifdef USING_OPEN_SOURCE_UTF7 /* JAC */
/*
 * UTF-7 conversion structure
 */
typedef struct utf7_conv
{
    char   mustshift[128] ;
    int    invbase64[128] ;
    int	   shifted	  ;	/* Set if in shifted state		  */
    int	   first	  ;	/* Set for first character after SHIFT_IN */
    int	   wroteone	  ;	/* Set if any least one UCS written	  */
    int	   bits_in_buffer ;	/* Number of valid bits in buffer	  */
    int    bitbuffer	  ;	/* Buffer for base64 bits		  */
    ucs4_t high_surrogate ;	/* Contain high surrogate		  */
    ucs4_t last_ucs4	  ;	/* Record last converted UCS-4 character  */
    int    last_cnvlen	  ;	/* Record last conversion length	  */

} utf7_conv_t ;

/*
 * UTF-7 conversion table initialization routine
 */
static void
utf7_convtable_init(_LC_fcconv_iconv_t *cd)
{
    int		 idx	;
    utf7_conv_t *table	;

    if (cd->flags & CONV_UTF7_CONVTABLE)
	return ;
#ifdef _KERNEL
    MALLOC(table, utf7_conv_t *, sizeof(utf7_conv_t),
	   M_KERN, M_WAITOK) ;
#else
    table = (utf7_conv_t *)malloc(sizeof(utf7_conv_t)) ;
#endif
    if (table == NULL) {
#ifndef _KERNEL
	perror("Memory allocation fails") ;
	exit  (1) ;
#else
	return ;
#endif

    }

    bzero ((char *)table, sizeof(utf7_conv_t))		   ;
#ifndef _KERNEL
    memset(table->mustshift,  1, sizeof(table->mustshift)) ;
    memset(table->invbase64, -1, sizeof(table->invbase64)) ;
#endif

    for (idx = 0 ; idx < SETD_SIZE ; idx++)
	table->mustshift[direct[idx]] = 0 ;

    for (idx = 0 ; idx < SETS_SIZE ; idx++)
	table->mustshift[spaces[idx]] = 0 ;

    if (cd->flags & CONV_UTF7_OPTIONAL)
	for (idx = 0 ; idx < SETO_SIZE ; idx++)
	    table->mustshift[optional[idx]] = 0 ;

    for (idx = 0 ; idx < SETB_SIZE ; idx++)
	table->invbase64[base64[idx]] = idx ;

    cd->convtable = (caddr_t) table	;
    cd->flags	 |= CONV_UTF7_CONVTABLE ;
}
#endif /* USING_OPEN_SOURCE_UTF7 // JAC */

/************************************************************************/
/*									*/
/*		UCS-2/UCS-4/UTF-7/UTF-8 input routines			*/
/* 		Return: The UCS-4 character or an error code	        */
/*			The in string reference is updated		*/
/*									*/
/************************************************************************/
//LCOV_EXCL_START :cnu -- As of 8/30/2011, not used on SQ platform, but may be used on Clients
int __input_ucs4(_LC_fcconv_iconv_t *cd, uchar_t **in, int len)
{
    ucs4_t *inptr = (ucs4_t *)*in ;
    int	    first = !(cd->flags & CONV_INPUT_PROCESSED) ;
    WChar_t word ; /*JAC */

    do {
	if (len < sizeof(ucs4_t))
	    return(ERR_INPUT_INCOMPLETE) ;

	word       = *inptr++		  ;
	len	  -= sizeof(ucs4_t)	  ;
	cd->flags |= CONV_INPUT_PROCESSED ;
	if (word == UCS4_BOM) {	/* Skip over initial byte order mark */
	    if (first) {
	       cd->flags &= ~CONV_REVERSE_INBYTE ;
	       continue ;
	    }
	}
	if (word == UCS4_BOM_REVERSE) {
	    if (first) {
	       cd->flags |= CONV_REVERSE_INBYTE ;
	       continue ;
	    }
	}
	break   ;
    } while (1) ;

    if (cd->flags & CONV_REVERSE_INBYTE)
	word = REVERSE_UCS4_BYTE(word) ;
    *in = (uchar_t *)inptr ;
    return((int)word)	 ;
}
//LCOV_EXCL_STOP
/*#endif /* USING_OPEN_SOURCE_input_ucs4 // JAC */

int __input_ucs2(_LC_fcconv_iconv_t *cd, uchar_t **in, int len)
{
    ucs2_t *inptr = (ucs2_t *)*in ;
    int	    first = !(cd->flags & CONV_INPUT_PROCESSED) ;
    WChar_t word ; /*JAC */

    do {
	if (len < sizeof(ucs2_t))
	    return(ERR_INPUT_INCOMPLETE) ;

	word       = *inptr++	  ;
	len	  -= sizeof(ucs2_t)	  ;
	cd->flags |= CONV_INPUT_PROCESSED ;
	if (word == UCS2_BOM) {	/* Skip over initial byte order mark */
	    if (first) {
	       cd->flags &= ~CONV_REVERSE_INBYTE ;
	       continue ;
	    }
	}
	if (word == UCS2_BOM_REVERSE) {
	    if (first) {
	       cd->flags |= CONV_REVERSE_INBYTE ;
	       continue ;
	    }
	}
	break   ;
    } while (1) ;

    if (cd->flags & CONV_REVERSE_INBYTE)
	word = REVERSE_UCS2_BYTE(word)	;
    /*
     * convert surrogates pair (<high-surrogate> <low-surrogate>) to UCS-4
     */
    if (IS_LSURROGATE(word)) /* Low surrogate without high surrogate */
	return(ERR_INVALID_CHAR) ;

    else if (IS_HSURROGATE(word)) {
	WChar_t low_surrogate ; /*JAC */

	if (len < sizeof(ucs2_t))
	    return(ERR_INPUT_INCOMPLETE) ;	/* Not enough input */

	low_surrogate = *inptr++  ;
	if (cd->flags & CONV_REVERSE_INBYTE)
	    low_surrogate = REVERSE_UCS2_BYTE(low_surrogate) ;
	if (!IS_LSURROGATE(low_surrogate))
	    return(ERR_INVALID_CHAR) ;

	word = SURROGATES_TO_UCS4(word, low_surrogate) ;
    }
    *in = (uchar_t *)inptr ;
    return((int)word)	   ;
}

int __input_utf8(_LC_fcconv_iconv_t *cd, uchar_t **in, int len)
{
    int	     first	= !(cd->flags & CONV_INPUT_PROCESSED) ;
    uchar_t *inptr = *in	;
    int	     char_size 		;
    WChar_t  word		; /*JAC */

    word = *inptr ;

    if (IS_1BYTE_UTF8(word))
	char_size = 1 ;
    else if (IS_2BYTE_UTF8(word))
	char_size = 2 ;
    else if (IS_3BYTE_UTF8(word))
	char_size = 3 ;
    else if (IS_4BYTE_UTF8(word))
	char_size = 4 ;
//LCOV_EXCL_START : cnu - We don't claim support for 5 or 6 byte long UTF8 chars yet.
    else if (IS_5BYTE_UTF8(word))
	char_size = 5 ;
    else if (IS_6BYTE_UTF8(word))
	char_size = 6 ;
//LCOV_EXCL_STOP
    else
	return(ERR_INVALID_CHAR) ;

    if (len < char_size)
	return(ERR_INPUT_INCOMPLETE) ;

    CHECK_UTF8(inptr, char_size) ;
    switch (char_size)
    {
	/*
	 * Over-long UTF-8 sequences are rejected for better security
	 */
	case 1: word = RD_1BYTE_UTF8(inptr) ; break ;
	case 2: word = RD_2BYTE_UTF8(inptr) ;
		if (word < MIN_2BYTE_UTF8)
		    return(ERR_INVALID_CHAR);
		break ;
	case 3: word = RD_3BYTE_UTF8(inptr) ;
		if (word < MIN_3BYTE_UTF8)
		    return(ERR_INVALID_CHAR);
		break ;
	case 4: word = RD_4BYTE_UTF8(inptr) ;
		if (word < MIN_4BYTE_UTF8)
		    return(ERR_INVALID_CHAR);
		break ;
//LCOV_EXCL_START : cnu - We don't claim support for 5 or 6 byte long UTF8 chars yet.
	case 5: word = RD_5BYTE_UTF8(inptr) ;
		if (word < MIN_5BYTE_UTF8)
		    return(ERR_INVALID_CHAR);
		break ;
	case 6: word = RD_6BYTE_UTF8(inptr) ;
		if (word < MIN_6BYTE_UTF8)
		    return(ERR_INVALID_CHAR);
		break ;
//LCOV_EXCL_STOP
    }

    inptr += char_size ;
    /*
     * Properly handle non-BMP characters formed by UTF-8 surrogate pairs
     */
    if ((char_size == 3) && (IS_HSURROGATE(word)))
    {
	WChar_t low_surrogate ; /*JAC */

	if (len == 3)
	    return(ERR_INPUT_INCOMPLETE) ;
	if (!IS_3BYTE_UTF8(*inptr))
	    return(ERR_INVALID_CHAR) ;
	low_surrogate = RD_3BYTE_UTF8(inptr) ;
	if (!IS_LSURROGATE(low_surrogate))
	    return(ERR_INVALID_CHAR) ;
	word = SURROGATES_TO_UCS4(word, low_surrogate) ;
    }
    cd->flags |= CONV_INPUT_PROCESSED ;
    *in	       = inptr 		      ;
    /*
     * Skip BOM if at the beginning of the file
     */
    if (first && (word == UCS4_BOM))
	return(__input_utf8(cd, in, len - char_size)) ;
    return((int)word) ;
}

/*
 * Due to the complex nature of UTF-7 to UCS conversion, it is done on a
 * character-by-character basis.
 */
#ifdef USING_OPEN_SOURCE_UTF7 /* JAC */
int __input_utf7(_LC_fcconv_iconv_t *cd, uchar_t **in, int len)
{
    uchar_t	*inptr = *in	;
    utf7_conv_t *table		;
    ucs4_t	 ch, word	;
    int		 base64value	;
    int		 buffertemp	;

    if (!(cd->flags & CONV_UTF7_CONVTABLE))
	utf7_convtable_init(cd) ;
    table = (utf7_conv_t *)cd->convtable	;

    /*
     * If inptr is NULL, it indicates the end of conversion and
     * hence it is necessary to check the current states of conversion
     * for error, e.g. partially converted character.
     */
    if ((inptr == NULL) || (len <= 0)) {
	return(ERR_NOINPUT) ;
    }

    while (1)
    {
	/*
	 * Incomplete input buffer sequence
	 */
	if (len < 1)
	    return(ERR_INPUT_INCOMPLETE) ;

	ch = *inptr++ ;
	len--	      ;

	if (ch > 0x7f)
	    return(ERR_INPUT_INCOMPLETE) ;

	if (table->shifted) {
	    base64value = table->invbase64[ch] ;

	    if (base64value < 0) {
		table->shifted = 0 ;
		/*
		 * If SHIFT_IN is immediately followed by SHIFT_OUT,
		 * this is a special case for the SHIFT_OUT character.
		 */
		if (table->first && (ch == SHIFT_OUT)) {
		    word	 = SHIFT_IN ;
		    table->first = 0	    ;
		    break		    ;
		}
		if (!table->wroteone || 
		   (READ_N_BITS(table->bits_in_buffer) != 0))
		    return(ERR_INVALID_CHAR) ;

		if (ch == SHIFT_OUT)
		    continue ;	/* The SHIFT_OUT character is discarded */
		/* Fall through to the non-shifted state */
	    }
	    else {
		/* Add another 6 bits of base64 value to the buffer */
		WRITE_N_BITS(base64value, 6) ;
		table->first = 0 ;
		if (table->bits_in_buffer >= 16) {
		    word	    = READ_N_BITS(16) ;
		    table->wroteone = 1		      ;
		    if (table->high_surrogate) {
			if (!IS_LSURROGATE(word))
			    return(ERR_INVALID_CHAR) ;
			word = SURROGATES_TO_UCS4(table->high_surrogate, word) ;
			table->high_surrogate = 0 ;
		    }
		    else if (IS_HSURROGATE(word)) {
			table->high_surrogate = word ;
			continue		     ;
		    }
		    break ;
		}
		continue ;	/* Read next byte */
	    }
	}
	if (ch == SHIFT_IN) {
	    table->first	  = 1 ;
	    table->shifted	  = 1 ;
	    table->wroteone	  = 0 ;
	    table->bits_in_buffer = 0 ;
	    continue ;	/* Read next byte */
	}
	/* It must be a directly encoded character */
	word = ch ;
	break	  ;
    }

    *in = (uchar_t *)inptr ;
    return((int)word)	   ;
}
#endif /* USING_OPEN_SOURCE_UTF7 // JAC */

/************************************************************************/
/*									*/
/*		UCS-2/UCS-4/UTF-7/UTF-8 output routines			*/
/*		Return: Number of bytes written to output buffer	*/
/*									*/
/************************************************************************/
//LCOV_EXCL_START :cnu -- As of 8/30/2011, not used on SQ platform, but may be used on Clients
int __output_ucs4(_LC_fcconv_iconv_t *cd, uchar_t *out, int len, ucs4_t word)
{
    ucs4_t *outptr = (ucs4_t *)out	;

    /*
     * For the special case that len == sizeof(ucs4_t)
     * and BOM has not been sent out, don't send out the BOM to avoid 
     * sending out E2BIG error unnecessarily.
     */
    if (!(cd->flags & CONV_BOM_WRITTEN)) {
	if (len == sizeof(ucs4_t))
	    cd->flags |= CONV_BOM_WRITTEN ;	/* Don't send BOM */
	else if (len > sizeof(ucs4_t)) {
	    *outptr++  = (cd->flags & CONV_REVERSE_OUTBYTE)
		       ? UCS4_BOM_REVERSE : UCS4_BOM ;
	    len	      -= sizeof(ucs4_t)   ;
	    cd->flags |= CONV_BOM_WRITTEN ;
	}
    }

    if (len < sizeof(ucs4_t))
	return(ERR_BUFFER_OVERRUN) ;
    if (cd->flags & CONV_REVERSE_OUTBYTE)
	word = REVERSE_UCS4_BYTE(word) ;
    *outptr++ = word	 ;
    return((uchar_t *)outptr - out) ;
}
//LCOV_EXCL_STOP
/*#endif /* USING_OPEN_SOURCE_output_ucs4 // JAC */

int __output_ucs2(_LC_fcconv_iconv_t *cd, uchar_t *out, int len, ucs4_t word)
{
    ucs2_t *outptr = (ucs2_t *)out ;

    /*
     * For the special case that len = sizeof(ucs2_t)
     * and BOM has not been sent out, don't send out the BOM to avoid 
     * sending out E2BIG error unnecessarily.
     */
    if (!(cd->flags & CONV_BOM_WRITTEN)) {
	if (len == sizeof(ucs2_t))
	    cd->flags |= CONV_BOM_WRITTEN ;	/* Don't send BOM */
	else if (len > sizeof(ucs2_t)) {
	    *outptr++  = (cd->flags & CONV_REVERSE_OUTBYTE)
		       ? UCS2_BOM_REVERSE : UCS2_BOM ;
	    len	      -= sizeof(ucs2_t)   ;
	    cd->flags |= CONV_BOM_WRITTEN ;
	}
    }

    /*
     * Not enough output buffer space?
     */
    if (len < sizeof(ucs2_t))
	return(ERR_BUFFER_OVERRUN) ;

    if (word > MAX_UCS4_VALUE)
	return(ERR_INVALID_CHAR) ;

    if (NEED_SURROGATE(word)) {
	ucs4_t high_surrogate ;

	if (len < 2 * sizeof(ucs2_t))
	    return(ERR_BUFFER_OVERRUN) ;

	high_surrogate = GET_HSURROGATE(word) ;
	word	       = GET_LSURROGATE(word) ;
	if (cd->flags & CONV_REVERSE_OUTBYTE)
	    high_surrogate = REVERSE_UCS2_BYTE(high_surrogate) ;
	*outptr++ = high_surrogate ;
    }
    if (cd->flags & CONV_REVERSE_OUTBYTE)
	word = REVERSE_UCS2_BYTE(word) ;

    *outptr++ = word     ;
    return((uchar_t *)outptr - out) ;
}

int __output_utf8(_LC_fcconv_iconv_t *cd, uchar_t *out, int len, ucs4_t word)
{
    uchar_t *outptr = out  ;
    int	     char_size	   ;

    if (IS_UCS_TO_1B_UTF8(word))
	char_size = 1 ;
    else if (IS_UCS_TO_2B_UTF8(word))
	char_size = 2 ;
    else if (IS_UCS_TO_3B_UTF8(word))
	char_size = 3 ;
    else if (IS_UCS_TO_4B_UTF8(word))
	char_size = 4 ;
//LCOV_EXCL_START : cnu - We don't claim support for 5 or 6 byte long UTF8 chars yet.
    else if (IS_UCS_TO_5B_UTF8(word))
	char_size = 5 ;
    else if (IS_UCS_TO_6B_UTF8(word))
	char_size = 6 ;
//LCOV_EXCL_STOP
    else
	return(ERR_INVALID_CHAR) ;

    /*
     * Not enough output buffer space?
     */
    if (len < char_size)
	return(ERR_BUFFER_OVERRUN) ;

    switch (char_size) 
    {
	case 1: WR_1BYTE_UTF8(outptr, word) ; break ;
	case 2: WR_2BYTE_UTF8(outptr, word) ; break ;
	case 3: WR_3BYTE_UTF8(outptr, word) ; break ;
	case 4: WR_4BYTE_UTF8(outptr, word) ; break ;
//LCOV_EXCL_START : cnu - We don't claim support for 5 or 6 byte long UTF8 chars yet.
	case 5: WR_5BYTE_UTF8(outptr, word) ; break ;
	case 6: WR_6BYTE_UTF8(outptr, word) ; break ;
//LCOV_EXCL_STOP
    }
    return(char_size) ;
}

#ifdef USING_OPEN_SOURCE_output_utf7 /* JAC */
int __output_utf7(_LC_fcconv_iconv_t *cd, uchar_t *out, int len, ucs4_t word)
{
    uchar_t	*outptr  = out	;
    utf7_conv_t *table		;
    uchar_t	*orig_outptr	;
    int		 buffertemp	;
    int		 needshift	;
    int		 char_size	;

    if (!(cd->flags & CONV_UTF7_CONVTABLE))
	utf7_convtable_init(cd) ;
    table = (utf7_conv_t *)cd->convtable ;

    orig_outptr = outptr	;
    needshift	= (word > 0x7f) || table->mustshift[word] ;

    /*
     * Estimate required size of output buffer
     */
    if (!needshift)
	char_size = table->shifted ? 2 : 1 ;
    else if (word < 0x10000)
	char_size = table->shifted ? 3 : 4 ;
    else if (NEED_SURROGATE(word))
	char_size = table->shifted ? 6 : 7 ;
    else
	return(ERR_INVALID_CHAR) ;

    /*
     * Not enough output buffer space?
     */
    if (len < char_size)
	return(ERR_BUFFER_OVERRUN) ;

    /*
     * Write output bytes
     */
    if (needshift && !table->shifted) {
	*outptr++ = SHIFT_IN ;
	/*
	 * Check for the special case of SHIFT_IN character
	 */
	if (word == SHIFT_IN) {
	    *outptr++ = SHIFT_OUT ;
	    return(outptr - out)  ;
	}
	else
	    table->shifted = 1 ;
    }

    if (table->shifted) {
	/*
	 * Either write the character to the bit buffer,
	 * or pad the bit buffer out to a full base64 character.
	 */
	if (needshift) {
	    if (NEED_SURROGATE(word)) {
		register WChar_t hiword ; /*JAC */

		hiword = GET_HSURROGATE(word) ;
		word   = GET_LSURROGATE(word) ;
		WRITE_N_BITS(hiword, 16)      ;
		while (table->bits_in_buffer > 6)
		*outptr++ = base64[READ_N_BITS(6)] ;
	    }
	    WRITE_N_BITS(word, 16) ;
	}
	else
	    WRITE_N_BITS(0, (6 - (table->bits_in_buffer%6)) % 6) ;

	/*
	 * Flush out as many full base64 characters as possible from the
	 * bit buffer.
	 */
	while (table->bits_in_buffer > 6)
	    *outptr++ = base64[READ_N_BITS(6)] ;

	if (!needshift) {
	    /*
	     * Write the explicit shift out character if 
	     * 1) The caller has requested we always do it, or
	     * 2) The directly encoded character is in the base64 set.
	     */
	    if ((cd->flags & CONV_UTF7_VERBOSE ) ||
		(table->invbase64[word] >= 0))
		*outptr++ = SHIFT_OUT ;
	    table->shifted = 0 ;
	}
    }

    /*
     * The character can be directly encoded as ASCII
     */
    if (!needshift) {
	/*
	 * Check buffer space again as the byte size estimate 
	 * may not be correct.
	 */
	if (len < 1)
	    return(ERR_BUFFER_OVERRUN) ;
	*outptr++ = word ;
    }

    return(outptr - out) ;
}
#endif /* USING_OPEN_SOURCE_output_utf7 // JAC */
