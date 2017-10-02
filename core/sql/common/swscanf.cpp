/* -*-C++-*-
 * $OpenBSD: vfscanf.c,v 1.21 2006/01/13 21:33:28 millert Exp $ 
 *-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

 /*****************************************************************************
  *
  * File:         swscanf.h
  * Description:  SQL/MX wide-char swscanf() function, modified based on 
  *               NetBSD __svfscanf.c found at 
  *               http://ftp.stu.edu.tw/BSD/OpenBSD/src/lib/libc/stdio/vfscanf.c
  * Created:      2/13/2002
  * Language:     C++
  * Limitation:   
  *
  *
  *
  *
  *****************************************************************************
 */

/* commented out the useless static data structure.*/
//#if defined(LIBC_SCCS) && !defined(lint)
//#if 0
//static char sccsid[] = "@(#)vfscanf.c	8.1 (Berkeley) 6/4/93";
//#endif
//static char rcsid[] = "$NetBSD: vfscanf.c,v 1.15 1996/03/29 23:29:28 jtc Exp $";
//#endif /* LIBC_SCCS and not lint */


#include <stdlib.h> 
#include <ctype.h> 
#include <stdarg.h> 


#define FLOATING_POINT 1

#ifdef FLOATING_POINT
#include "float.h"
#endif

#define	BUF		513	/* Maximum length of numeric string. */

/*
 * Flags used during conversion.
 */
#define	LONG		0x01	/* l: long or double */
#define	LONGDBL		0x02	/* L: long double; unimplemented */
#define	SHORT		0x04	/* h: short */
#define  QUAD		0x08	/* q: quad */
#define	SUPPRESS	0x10	/* suppress assignment */
#define	POINTER		0x20	/* weird %p pointer (`fake hex') */
#define	NOSKIP		0x40	/* do not skip blanks */

/*
 * The following are used in numeric conversions only:
 * SIGNOK, NDIGITS, DPTOK, and EXPOK are for floating point;
 * SIGNOK, NDIGITS, PFXOK, and NZDIGITS are for integral.
 */
#define	SIGNOK		0x080	/* +/- is (still) legal */
#define	NDIGITS		0x100	/* no digits detected */

#define	DPTOK		0x200	/* (float) decimal point is still legal */
#define	EXPOK		0x400	/* (float) exponent (e+3, etc) still legal */

#define	PFXOK		0x200	/* 0x prefix is (still) legal */
#define	NZDIGITS	0x400	/* no zero digits detected */

/*
 * Conversion types.
 */
#define	CT_CHAR		0	/* %c conversion */
#define	CT_CCL		1	/* %[...] conversion */
#define	CT_STRING	2	/* %s conversion */
#define	CT_INT		3	/* integer, i.e., strtoq or strtouq */
#define	CT_FLOAT	4	/* floating, i.e., strtod */

#include "wc_scanf_sprintf.h"



typedef quad_t (*ccfnT)(const NAWchar*);


#include <string.h>


Int32
__srefill(SCANBUF* fp)
	
{
   return 1;
}

void ungetc(NAWchar c, SCANBUF* fp)
{
   fp->_p--;
   *(fp->_p) = c; 
   fp->_r++;
}
/*
 * Fill in the given table from the scanset at the given format
 * (just after `[').  Return a pointer to the character past the
 * closing `]'.  The table has a 1 wherever characters should be
 * considered part of the scanset.
 */
static NAWchar *
__sccl(NAWchar* tab, NAWchar* fmt)
{
	register Int32 c, n, v;

	/* first `clear' the whole table */
	c = *fmt++;		/* first char hat => negated scanset */
	if (c == L'^') {
		v = 1;		/* default => accept */
		c = *fmt++;	/* get new first char */
	} else
		v = 0;		/* default => reject */
	/* should probably use memset here */
	for (n = 0; n < 256; n++)
		tab[n] = v;
	if (c == 0)
		return (fmt - 1);/* format ended before closing ] */

	/*
	 * Now set the entries corresponding to the actual scanset
	 * to the opposite of the above.
	 *
	 * The first character may be ']' (or '-') without being special;
	 * the last character may be '-'.
	 */
	v = 1 - v;
	for (;;) {
		tab[c] = v;		/* take character c */
doswitch:
		n = *fmt++;		/* and examine the next */
		switch (n) {

		case 0:			/* format ended too soon */
			return (fmt - 1);

		case L'-':
			/*
			 * A scanset of the form
			 *	[01+-]
			 * is defined as `the digit 0, the digit 1,
			 * the character +, the character -', but
			 * the effect of a scanset such as
			 *	[a-zA-Z0-9]
			 * is implementation defined.  The V7 Unix
			 * scanf treats `a-z' as `the letters a through
			 * z', but treats `a-a' as `the letter a, the
			 * character -, and the letter a'.
			 *
			 * For compatibility, the `-' is not considerd
			 * to define a range if the character following
			 * it is either a close bracket (required by ANSI)
			 * or is not numerically greater than the character
			 * we just stored in the table (c).
			 */
			n = *fmt;
			if (n == L']' || n < c) {
				c = L'-';
				break;	/* resume the for(;;) */
			}
			fmt++;
			do {		/* fill in the range */
				tab[++c] = v;
			} while (c < n);
#if 1	/* XXX another disgusting compatibility hack */
			/*
			 * Alas, the V7 Unix scanf also treats formats
			 * such as [a-c-e] as `the letters a through e'.
			 * This too is permitted by the standard....
			 */
			goto doswitch;
#else
			c = *fmt++;
			if (c == 0)
				return (fmt - 1);
			if (c == L']')
				return (fmt);
#endif
			break;

		case L']':		/* end of scanset */
			return (fmt);

		default:		/* just another character */
			c = n;
			break;
		}
	}
	/* NOTREACHED */
}


/*
 * vfscanf
 */
Int32
__svfscanf(SCANBUF* fp, NAWchar const* fmt0, va_list ap)
{
	register NAWchar *fmt = (NAWchar *)fmt0;
	register Int32 c;		/* character from format, or conversion */
	register UInt32 width;	/* field width, or 0 */
	register NAWchar *p;	/* points into all kinds of strings */
	register UInt32 n;		/* handy integer */
	register Int32 flags;	/* flags as defined above */
	register NAWchar *p0;	/* saves original value of p when necessary */
	Int32 nassigned;		/* number of fields assigned */
	Int32 nread;		/* number of characters consumed from fp */
	Int32 base;		/* base argument to strtoq/strtouq */
	ccfnT ccfn; /* conversion function (strtoq/strtouq) */
   
   NAWchar ccltab[256];	/* character class table for %[...] */
	NAWchar buf[BUF];		/* buffer for numeric conversions */
	char sb_buf[BUF];		/* single byte buffer for numeric conversions */

	/* `basefix' is used to avoid `if' tests in the integer scanner */
	/* static */ short basefix[17] =
		{ 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

	nassigned = 0;
	nread = 0;
	base = 0;		/* XXX just to keep gcc happy */
   ccfn = NULL;
	
	for (;;) {
		c = *fmt++;
		if (c == 0)
			return (nassigned);
		if (na_iswspace(c)) {
			for (;;) {
				if (fp->_r <= 0 && __srefill(fp))
					return (nassigned);
				if (!na_iswspace(*fp->_p))
					break;
				nread++, fp->_r--, fp->_p++;
			}
			continue;
		}
		if (c != L'%')
			goto literal;
		width = 0;
		flags = 0;
		/*
		 * switch on the format.  continue if done;
		 * break once format type is derived.
		 */
again:		c = *fmt++;
		switch (c) {
		case L'%':
literal:
			if (fp->_r <= 0 && __srefill(fp))
				goto input_failure;
			if (*fp->_p != c)
				goto match_failure;
			fp->_r--, fp->_p++;
			nread++;
			continue;

		case L'*':
			flags |= SUPPRESS;
			goto again;
		case L'L':
			flags |= LONGDBL;
			goto again;
		case L'h':
			flags |= SHORT;
			goto again;
		case L'l':
			if (*fmt == L'l') {
				fmt++;
				flags |= QUAD;
			} else {
				flags |= LONG;
			}
			goto again;
		case L'q':
			flags |= QUAD;
			goto again;

		case L'0': case L'1': case L'2': case L'3': case L'4':
		case L'5': case L'6': case L'7': case L'8': case L'9':
			width = width * 10 + c - L'0';
			goto again;

		/*
		 * Conversions.
		 * Those marked `compat' are for 4.[123]BSD compatibility.
		 *
		 * (According to ANSI, E and X formats are supposed
		 * to the same as e and x.  Sorry about that.)
		 */
		case L'D':	/* compat */
			flags |= LONG;
			/* FALLTHROUGH */
		case L'd':
			c = CT_INT;
			if (flags & QUAD)
				ccfn = (ccfnT)na_wcstoll;
			else
				ccfn = (ccfnT)na_wcstol;
			base = 10;
			break;

		case L'i':
			c = CT_INT;
			ccfn = (ccfnT)na_wcstol;
			base = 0;
			break;

		case L'O':	/* compat */
			flags |= LONG;
			/* FALLTHROUGH */
		case L'o':
			c = CT_INT;
			ccfn = (ccfnT)na_wcstol;
			base = 8;
			break;

		case L'u':
			c = CT_INT;
			ccfn = (ccfnT)na_wcstol;
			base = 10;
			break;

		case L'X':
		case L'x':
			flags |= PFXOK;	/* enable 0x prefixing */
			c = CT_INT;
			ccfn = (ccfnT)na_wcstol;
			base = 16;
			break;

#ifdef FLOATING_POINT
		case L'E':
		case L'G':
		case L'e': 
		case L'f': 
		case L'g':
			c = CT_FLOAT;
         break;
#endif

		case L's':
			c = CT_STRING;
			break;

		case L'[':
			fmt = __sccl(ccltab, fmt);
			flags |= NOSKIP;
			c = CT_CCL;
			break;

		case L'c':
			flags |= NOSKIP;
			c = CT_CHAR;
			break;

		case L'p':	/* pointer format is like hex */
			flags |= POINTER | PFXOK;
			c = CT_INT;
			ccfn = (ccfnT)na_wcstol;
			base = 16;
			break;

		case L'n':
			if (flags & SUPPRESS)	/* ??? */
				continue;
			if (flags & SHORT)
				*va_arg(ap, short *) = nread;
			else if (flags & LONG)
				*va_arg(ap, Lng32 *) = nread;
			else
				*va_arg(ap, Int32 *) = nread;
			continue;

		/*
		 * Disgusting backwards compatibility hacks.	XXX
		 */
		case L'\0':	/* compat */
			//return (EOF);
         return -1;

		default:	/* compat */
			if ( L'A' <= c && c <= L'Z' )
				flags |= LONG;
			c = CT_INT;
			ccfn = (ccfnT)na_wcstol;
			base = 10;
			break;
		}

		/*
		 * We have a conversion that requires input.
		 */
		if (fp->_r <= 0 && __srefill(fp))
			goto input_failure;

		/*
		 * Consume leading white space, except for formats
		 * that suppress this.
		 */
		if ((flags & NOSKIP) == 0) {
			while (na_iswspace(*fp->_p)) {
				nread++;
				if (--fp->_r > 0)
					fp->_p++;
				else if (__srefill(fp))
					goto input_failure;
			}
			/*
			 * Note that there is at least one character in
			 * the buffer, so conversions that do not set NOSKIP
			 * ca no longer result in an input failure.
			 */
		}

		/*
		 * Do the conversion.
		 */
		switch (c) {

		case CT_CHAR:
			/* scan arbitrary characters (sets NOSKIP) */
			if (width == 0)
				width = 1;
			if (flags & SUPPRESS) {
				size_t sum = 0;
				for (;;) {
					if ((n = fp->_r) < width) {
						sum += n;
						width -= n;
						fp->_p += n;
						if (__srefill(fp)) {
							if (sum == 0)
							    goto input_failure;
							break;
						}
					} else {
						sum += width;
						fp->_r -= width;
						fp->_p += width;
						break;
					}
				}
				nread += sum;
			} else {
				//size_t r = fread((void *)va_arg(ap, char *), 1,
				//    width, fp);
            size_t r;
            if ( fp->_r >= width ) {
               na_wcsncpy((NAWchar*)va_arg(ap, NAWchar *), (NAWchar*)fp->_ptr, width);
               r = width;
            } else
               r = 0;

				if (r == 0)
					goto input_failure;
				nread += r;
				nassigned++;
			}
			break;

		case CT_CCL:
			/* scan a (nonempty) character class (sets NOSKIP) */
			if (width == 0)
				width = ~0;	/* `infinity' */
			/* take only those things in the class */
			if (flags & SUPPRESS) {
				n = 0;
				while (ccltab[*fp->_p]) {
					n++, fp->_r--, fp->_p++;
					if (--width == 0)
						break;
					if (fp->_r <= 0 && __srefill(fp)) {
						if (n == 0)
							goto input_failure;
						break;
					}
				}
				if (n == 0)
					goto match_failure;
			} else {
				p0 = p = va_arg(ap, NAWchar *);
				while (ccltab[*fp->_p]) {
					fp->_r--;
					*p++ = *fp->_p++;
					if (--width == 0)
						break;
					if (fp->_r <= 0 && __srefill(fp)) {
						if (p == p0)
							goto input_failure;
						break;
					}
				}
				n = p - p0;
				if (n == 0)
					goto match_failure;
				*p = 0;
				nassigned++;
			}
			nread += n;
			break;

		case CT_STRING:
			/* like CCL, but zero-length string OK, & no NOSKIP */
			if (width == 0)
				width = ~0;
			if (flags & SUPPRESS) {
				n = 0;
				while (!na_iswspace(*fp->_p)) {
					n++, fp->_r--, fp->_p++;
					if (--width == 0)
						break;
					if (fp->_r <= 0 && __srefill(fp))
						break;
				}
				nread += n;
			} else {
				p0 = p = va_arg(ap, NAWchar *);
				while (!na_iswspace(*fp->_p)) {
					fp->_r--;
					*p++ = *fp->_p++;
					if (--width == 0)
						break;
					if (fp->_r <= 0 && __srefill(fp))
						break;
				}
				*p = 0;
				nread += p - p0;
				nassigned++;
			}
			continue;

		case CT_INT:
			/* scan an integer as if by strtoq/strtouq */
#ifdef hardway
			if (width == 0 || width > sizeof(buf) - 1)
				width = sizeof(buf) - 1;
#else
			/* size_t is unsigned, hence this optimisation */
			if (--width > sizeof(buf) - 2)
				width = sizeof(buf) - 2;
			width++;
#endif
			flags |= SIGNOK | NDIGITS | NZDIGITS;
			for (p = buf; width; width--) {
				c = *fp->_p;
				/*
				 * Switch on the character; `goto ok'
				 * if we accept it as a part of number.
				 */
				switch (c) {

				/*
				 * The digit 0 is always legal, but is
				 * special.  For %i conversions, if no
				 * digits (zero or nonzero) have been
				 * scanned (only signs), we will have
				 * base==0.  In that case, we should set
				 * it to 8 and enable 0x prefixing.
				 * Also, if we have not scanned zero digits
				 * before this, do not turn off prefixing
				 * (someone else will turn it off if we
				 * have scanned any nonzero digits).
				 */
				case L'0':
					if (base == 0) {
						base = 8;
						flags |= PFXOK;
					}
					if (flags & NZDIGITS)
					    flags &= ~(SIGNOK|NZDIGITS|NDIGITS);
					else
					    flags &= ~(SIGNOK|PFXOK|NDIGITS);
					goto ok;

				/* 1 through 7 always legal */
				case L'1': case L'2': case L'3':
				case L'4': case L'5': case L'6': case L'7':
					base = basefix[base];
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				/* digits 8 and 9 ok iff decimal or hex */
				case L'8': case L'9':
					base = basefix[base];
					if (base <= 8)
						break;	/* not legal here */
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				/* letters ok iff hex */
				case L'A': case L'B': case L'C':
				case L'D': case L'E': case L'F':
				case L'a': case L'b': case L'c':
				case L'd': case L'e': case L'f':
					/* no need to fix base here */
					if (base <= 10)
						break;	/* not legal here */
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				/* sign ok only as first character */
				case L'+': case L'-':
					if (flags & SIGNOK) {
						flags &= ~SIGNOK;
						goto ok;
					}
					break;

				/* x ok iff flag still set & 2nd char */
				case L'x': case L'X':
					if (flags & PFXOK && p == buf + 1) {
						base = 16;	/* if %i */
						flags &= ~PFXOK;
						goto ok;
					}
					break;
				}

				/*
				 * If we got here, c is not a legal character
				 * for a number.  Stop accumulating digits.
				 */
				break;
		ok:
				/*
				 * c is legal: store it and look at the next.
				 */
				*p++ = c;
				if (--fp->_r > 0)
					fp->_p++;
				else if (__srefill(fp))
					break;		/* EOF */
			}
			/*
			 * If we had only a sign, it is no good; push
			 * back the sign.  If the number ends in `x',
			 * it was [sign] '0' 'x', so push back the x
			 * and treat it as [sign] '0'.
			 */
			if (flags & NDIGITS) {
				if (p > buf)
					(void) ungetc(*(NAWchar *)--p, fp);
				   goto match_failure;
			}
			c = ((NAWchar *)p)[-1];
			if (c == L'x' || c == L'X') {
				--p;
				(void) ungetc(c, fp);
            
			}
			if ((flags & SUPPRESS) == 0) {
				u_quad_t res = NULL;

				*p = 0;
            if ( ccfn )
				   res = (*ccfn)(buf); // only 10 based
               //res = (*ccfn)(buf);
				if (flags & POINTER)
					*va_arg(ap, void **) =
					    (void *)(Lng32)res;
				else if (flags & QUAD)
					*va_arg(ap, quad_t *) = res;
				else if (flags & LONG)
					*va_arg(ap, Lng32 *) = (Lng32) res;
				else if (flags & SHORT)
					*va_arg(ap, Int16 *) = (Int16) res;
				else
					*va_arg(ap, Int32 *) = (Int32) res;
				nassigned++;
			}
			nread += p - buf;
			break;

#ifdef FLOATING_POINT
		case CT_FLOAT:
			/* scan a floating point number as if by strtod */
#ifdef hardway
			if (width == 0 || width > sizeof(buf) - 1)
				width = sizeof(buf) - 1;
#else
			/* size_t is unsigned, hence this optimisation */
			if (--width > sizeof(buf) - 2)
				width = sizeof(buf) - 2;
			width++;
#endif
			flags |= SIGNOK | NDIGITS | DPTOK | EXPOK;

                        char* q = sb_buf;

			for (p = buf; width; width--) {
				c = *fp->_p;
				/*
				 * This code mimicks the integer conversion
				 * code, but is much simpler.
				 */
				switch (c) {

				case L'0': case L'1': case L'2': case L'3':
				case L'4': case L'5': case L'6': case L'7':
				case L'8': case L'9':
					flags &= ~(SIGNOK | NDIGITS);
					goto fok;

				case L'+': case L'-':
					if (flags & SIGNOK) {
						flags &= ~SIGNOK;
						goto fok;
					}
					break;
				case L'.':
					if (flags & DPTOK) {
						flags &= ~(SIGNOK | DPTOK);
						goto fok;
					}
					break;
				case L'e': case L'E':
					/* no exponent without some digits */
					if ((flags&(NDIGITS|EXPOK)) == EXPOK) {
						flags =
						    (flags & ~(EXPOK|DPTOK)) |
						    SIGNOK | NDIGITS;
						goto fok;
					}
					break;
				}
				break;
		fok:
				*p++ = c;
				*q++ = (char)c;
				if (--fp->_r > 0)
					fp->_p++;
				else if (__srefill(fp))
					break;	/* EOF */
			}
			/*
			 * If no digits, might be missing exponent digits
			 * (just give back the exponent) or might be missing
			 * regular digits, but had sign and/or decimal point.
			 */
			if (flags & NDIGITS) {
				if (flags & EXPOK) {
					/* no digits at all */
					while (p > buf)
						ungetc(*(NAWchar *)--p, fp);
					goto match_failure;
				}
				/* just a bad exponent (e and maybe sign) */
				c = *(NAWchar *)--p;
				if (c != L'e' && c != L'E') {
					(void) ungetc(c, fp);/* sign */
               c = *(NAWchar *)--p;
				}
				(void) ungetc(c, fp);
			}
			if ((flags & SUPPRESS) == 0) {
				double res;

				*p = 0;
				//res = na_wcstod(buf, (NAWchar **) NULL);
				res = strtod(sb_buf, NULL);
				if (flags & LONGDBL)
					*va_arg(ap, long double *) = res;
				else if (flags & LONG)
					*va_arg(ap, double *) = res;
				else
					*va_arg(ap, float *) = (float) res;
				nassigned++;
			}
			nread += p - buf;
			break;
#endif /* FLOATING_POINT */
		}
	}
input_failure:
	return (nassigned ? nassigned : -1);
match_failure:
	return (nassigned);
}

Int32 na_swscanf(const NAWchar *str, NAWchar const *fmt, ...)
{
	Int32 ret;
	va_list ap;
	SCANBUF f;

	f._p = ( NAWchar *)str;
	f._r = na_wcslen(str);
	
	va_start(ap, fmt);

	ret = __svfscanf(&f, fmt, ap);
	va_end(ap);
	return (ret);
}

