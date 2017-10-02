/* -*-C++-*-
 * $OpenBSD: sprintf.c,v 1.13 2005/10/10 12:00:52 espie Exp $ 
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
 * File:         swsprintf.h
 * Description:  SQL/MX wide-char swsprintf() function, adapted from NetBSD vfprintf.c
 *               found at http://ftp.stu.edu.tw/BSD/OpenBSD/src/lib/libc/stdio/sprintf.c 
 * Created:      2/13/2002
 * Language:     C++
 * Limitation:   Floating point numbers are not supported. 
 *
 *
 *
 *
 *****************************************************************************
 */

/* <- Back to the Main Page 
 */
/* commented out because of static data structure */
//#if defined(LIBC_SCCS) && !defined(lint)
///*static char *sccsid = "from: @(#)vfprintf.c	5.50 (Berkeley) 12/16/92";*/
//static char *rcsid = "$NetBSD: vfprintf.c,v 1.18 1997/04/02 12:50:25 kleink Exp $";
//#endif /* LIBC_SCCS and not lint */


/*
 * Actual printf innards.
 *
 * This code is large and complicated...
 */

#include  <sys/types.h> 


#include  <stdlib.h>
#include  <string.h>
#include <limits.h>

#include  <stdarg.h> 

#include "Platform.h"
#include "wc_scanf_sprintf.h"

#ifdef FLOATING_POINT
#undef FLOATING_POINT
#endif

//#define FLOATING_POINT 1

Int32
vfprintf(SPRINTF_BUF* fp, const NAWchar* fmt0, va_list ap);

Int32
na_wsprintf(NAWchar *str, NAWchar const *fmt, ...)
{
	Int32 ret;
	va_list ap;
	SPRINTF_BUF f;

	f._p = (NAWchar *)str;
	f._w = INT_MAX;

	va_start(ap, fmt);

	ret = vfprintf(&f, fmt, ap);
	va_end(ap);
	*f._p = 0;
	return (ret);
}


#ifdef FLOATING_POINT
#include  <math.h> 
#include <float.h>

//#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */
#define BUF DBL_DIG+1 /* MSDEV 4.0 */
#define	DEFPREC		6

#define __P(x) x

static char *cvt __P((double, Int32, Int32, char *, Int32 *, Int32, Int32 *));
static Int32 exponent __P((char *, Int32, Int32));

#else /* no FLOATING_POINT */
#undef  BUF
#define	BUF		40

#endif /* FLOATING_POINT */


/*
 * Macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - L'0')
#define is_digit(c)	((UInt32)to_digit(c) <= 9)
#define	to_char(n)	((n) + L'0')

/*
 * Flags used during conversion.
 */
/*  */
/* vfprintf */
#define	ALT		0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#undef  LONGDBL
#define	LONGDBL		0x008		/* long double; unimplemented */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SHORTINT	0x040		/* short integer */
#define	ZEROPAD		0x080		/* zero (as opposed to blank) pad */
#define FPT		0x100		/* Floating point number */
Int32
vfprintf(SPRINTF_BUF* fp, const NAWchar* fmt0, va_list ap)
{
	register NAWchar *fmt;	/* format string */
	register Int32 ch;	/* character from fmt */
	register Int32 n, m;	/* handy integers (short term usage) */
	register NAWchar *cp;	/* handy char pointer (short term usage) */
	//register struct __siov *iovp;/* for PRINT macro */
	register Int32 flags;	/* flags as above */
	Int32 ret;		/* return value accumulator */
	Int32 width;		/* width from format (%8d), or 0 */
	Int32 prec;		/* precision from format (%.3d), or -1 */
	NAWchar sign;		/* sign prefix (' ', '+', '-', or \0) */
	NAWchar wc;
#ifdef FLOATING_POINT
	NAWchar *decimal_point = /*localeconv()->decimal_point*/ WIDE_(".");
	NAWchar softsign;		/* temporary negative sign for floats */
	double _double;		/* double precision arguments %[eEfgG] */
	Int32 expt;		/* integer value of exponent */
	Int32 expsize;		/* character count for expstr */
	Int32 ndig;		/* actual number of digits returned by cvt */
	NAWchar expstr[7];		/* buffer for exponent string */
#endif

#ifdef __GNUC__			/* gcc has builtin quad type (long long) SOS */
#define	quad_t	  Int64
#define	u_quad_t  UInt64
#endif

	u_quad_t _uquad;	/* integer arguments %[diouxX] */
	enum { OCT, DEC, HEX } base;/* base for [diouxX] conversion */
	Int32 dprec;		/* a copy of prec if [diouxX], 0 otherwise */
	Int32 realsz;		/* field size expanded by dprec */
	Int32 size;		/* size of converted field or string */
	NAWchar *xdigs = NULL;		/* digits for [xX] conversion */
#define NIOV 8
	//struct __suio uio;	/* output information: summary */
	//struct __siov iov[NIOV];/* ... and individual io vectors */
	NAWchar buf[BUF];		/* space for %c, %[diouxX], %[eEfgG] */
	NAWchar ox[2];		/* space for 0x hex-prefix */

	/*
	 * Choose PADSIZE to trade efficiency vs. size.  If larger printf
	 * fields occur frequently, increase PADSIZE and make the initialisers
	 * below longer.
	 */
#define	PADSIZE	16		/* pad chunk size */
	/* static */ NAWchar blanks[PADSIZE] =
	 {L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' ',L' '};
	/* static */ NAWchar zeroes[PADSIZE] =
	 {L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0',L'0'};

	/*
	 * BEWARE, these `goto error' on error, and PAD uses `n'.
	 */
#define	PRINT(ptr, len) { \
	  if ( fp->_w >= len ) { \
		  na_wcsncpy(fp->_p, ptr, len); \
		  fp->_w -= len; \
		  fp->_p += len; \
	  }  else { goto error;} \
}
#define	PAD(howmany, with) { \
	if ((n = (howmany)) > 0) { \
		while (n > PADSIZE) { \
			PRINT(with, PADSIZE); \
			n -= PADSIZE; \
		} \
		PRINT(with, n); \
	} \
}
#define	FLUSH() 

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() \
	(flags&QUADINT ? va_arg(ap, quad_t) : \
	    flags&LONGINT ? va_arg(ap, Lng32) : \
	    flags&SHORTINT ? (Lng32)(short)va_arg(ap, Int32) : \
	    (Lng32)va_arg(ap, Int32))
#define	UARG() \
	(flags&QUADINT ? va_arg(ap, u_quad_t) : \
	    flags&LONGINT ? va_arg(ap, u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, Int32) : \
	    (u_long)va_arg(ap, u_int))

	/* sorry, fprintf(read_only_SPRINTF_BUF, "") returns EOF, not 0 */
	//if (cantwrite(fp))
	//	return (EOF);

	/* optimise fprintf(stderr) (and other unbuffered Unix SPRINTF_BUFs) */
	//if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) &&
	//    fp->_SPRINTF_BUF >= 0)
	//	return (__sbprintf(fp, fmt0, ap));

	fmt = (NAWchar *)fmt0;
	//uio.uio_iov = iovp = iov;
	//uio.uio_resid = 0;
	//uio.uio_iovcnt = 0;
	ret = 0;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		cp = fmt;
		//while ((n = mbtowc(&wc, fmt, MB_CUR_MAX)) > 0) 
      
      do {
         wc = *fmt;
         
         n = (wc == 0) ? 0 : 1;
           
         fmt += n;
			if (wc == L'%') {
				fmt--;
				break;
			}
      } while (n > 0);

		if ((m = fmt - cp) != 0) {
			PRINT(cp, m);
			ret += m;
		}
		if (n <= 0)
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = L'\0';

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case L' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = L' ';
			goto rflag;
		case L'#':
			flags |= ALT;
			goto rflag;
		case L'*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(ap, Int32)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case L'-':
			flags |= LADJUST;
			goto rflag;
		case L'+':
			sign = L'+';
			goto rflag;
		case L'.':
			if ((ch = *fmt++) == L'*') {
				n = va_arg(ap, Int32);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case L'0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case L'1': case L'2': case L'3': case L'4':
		case L'5': case L'6': case L'7': case L'8': case L'9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case L'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case L'h':
			flags |= SHORTINT;
			goto rflag;
		case L'l':
			if (*fmt == L'l') {
				fmt++;
				flags |= QUADINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case L'q':
			flags |= QUADINT;
			goto rflag;
		case L'c':
			*(cp = buf) = va_arg(ap, Int32);
			size = 1;
			sign = L'\0';
			break;
		case L'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case L'd':
		case L'i':
			_uquad = SARG();
			if ((quad_t)_uquad < 0) {
				_uquad = -_uquad;
				sign = L'-';
			}
			base = DEC;
			goto number;
#ifdef FLOATING_POINT
		case L'e':
		case L'E':
		case L'f':
		case L'g':
		case L'G':
			if (prec == -1) {
				prec = DEFPREC;
			} else if ((ch == L'g' || ch == L'G') && prec == 0) {
				prec = 1;
			}

			if (flags & LONGDBL) {
				_double = (double) va_arg(ap, Lng32 double);
			} else {
				_double = va_arg(ap, double);
			}

			/* do this before tricky precision changes */
			if (! _finite(_double)) {
				if (_double < 0)
					sign = L'-';
				cp = WIDE_("Inf");
				size = 3;
				break;
			}
			if (_isnan(_double)) {
				cp = WIDE_("NaN");
				size = 3;
				break;
			}

			flags |= FPT;
			cp = cvt(_double, prec, flags, &softsign,
				&expt, ch, &ndig);
			if (ch == L'g' || ch == L'G') {
				if (expt <= -4 || expt > prec)
					ch = (ch == 'g') ? L'e' : L'E';
				else
					ch = L'g';
			} 
			if (ch <= L'e') {	/* 'e' or 'E' fmt */
				--expt;
				expsize = exponent(expstr, expt, ch);
				size = expsize + ndig;
				if (ndig > 1 || flags & ALT)
					++size;
			} else if (ch == L'f') {		/* f fmt */
				if (expt > 0) {
					size = expt;
					if (prec || flags & ALT)
						size += prec + 1;
				} else	/* "0.X" */
					size = prec + 2;
			} else if (expt >= ndig) {	/* fixed g fmt */
				size = expt;
				if (flags & ALT)
					++size;
			} else
				size = ndig + (expt > 0 ?
					1 : 2 - expt);

			if (softsign)
				sign = L'-';
			break;
#endif /* FLOATING_POINT */
		case L'n':
			if (flags & QUADINT)
				*va_arg(ap, quad_t *) = ret;
			else if (flags & LONGINT)
				*va_arg(ap, Lng32 *) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short *) = ret;
			else
				*va_arg(ap, Int32 *) = ret;
			continue;	/* no output */
		case L'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case L'o':
			_uquad = UARG();
			base = OCT;
			goto nosign;
		case L'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			/* NOSTRICT */
			_uquad = (u_long)va_arg(ap, void *);
			base = HEX;
			xdigs = (NAWchar *)WIDE_("0123456789abcdef");
			flags |= HEXPREFIX;
			ch = NAWchar('x');
			goto nosign;
		case L's':
			if ((cp = va_arg(ap, NAWchar *)) == NULL)
				cp = (NAWchar *)WIDE_("(null)");
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				NAWchar *p = (NAWchar*)na_wmemchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = na_wcslen(cp);
			sign = L'\0';
			break;
		case L'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case L'u':
			_uquad = UARG();
			base = DEC;
			goto nosign;
		case L'X':
			xdigs = (NAWchar *)WIDE_("0123456789ABCDEF");
			goto hex;
		case L'x':
			xdigs = (NAWchar *)WIDE_("0123456789abcdef");
hex:			_uquad = UARG();
			base = HEX;
			/* leading 0x/X only if non-zero */
			if (flags & ALT && _uquad != 0)
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign:			sign = L'\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + BUF;
			if (_uquad != 0 || prec != 0) {
				/*
				 * Unsigned mod is hard, and unsigned mod
				 * by a constant is easier than that by
				 * a variable; hence this switch.
				 */
				switch (base) {
				case OCT:
					do {
						*--cp = to_char(_uquad & 7);
						_uquad >>= 3;
					} while (_uquad);
					/* handle octal leading 0 */
					if (flags & ALT && *cp != L'0')
						*--cp = L'0';
					break;

				case DEC:
					/* many numbers are 1 digit */
					while (_uquad >= 10) {
						*--cp = to_char(_uquad % 10);
						_uquad /= 10;
					}
					*--cp = to_char(_uquad);
					break;

				case HEX:
					do {
						*--cp = xdigs[_uquad & 15];
						_uquad >>= 4;
					} while (_uquad);
					break;

				default:
					cp = (NAWchar *) WIDE_("bug in vfprintf: bad base");
					size = na_wcslen(cp);
					goto skipsize;
				}
			}
			size = buf + BUF - cp;
		skipsize:
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == L'\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = L'\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * size excludes decimal prec; realsz includes it.
		 */
		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		else if (flags & HEXPREFIX)
			realsz+= 2;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST|ZEROPAD)) == 0)
			PAD(width - realsz, blanks);

		/* prefix */
		if (sign) {
			PRINT(&sign, 1);
		} else if (flags & HEXPREFIX) {
			ox[0] = L'0';
			ox[1] = ch;
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
			PAD(width - realsz, zeroes);

		/* leading zeroes from decimal precision */
		PAD(dprec - size, zeroes);

		/* the string or number proper */
#ifdef FLOATING_POINT
		if ((flags & FPT) == 0) {
			PRINT(cp, size);
		} else {	/* glue together f_p fragments */
			if (ch >= L'f') {	/* 'f' or 'g' */
				if (_double == 0) {
					/* kludge for __dtoa irregularity */
					PRINT(WIDE_("0"), 1);
					if (expt < ndig || (flags & ALT) != 0) {
						PRINT(decimal_point, 1);
						PAD(ndig - 1, zeroes);
					}
				} else if (expt <= 0) {
					PRINT(WIDE_("0"), 1);
					PRINT(decimal_point, 1);
					PAD(-expt, zeroes);
					PRINT(cp, ndig);
				} else if (expt >= ndig) {
					PRINT(cp, ndig);
					PAD(expt - ndig, zeroes);
					if (flags & ALT)
						PRINT(WIDE_("."), 1);
				} else {
					PRINT(cp, expt);
					cp += expt;
					PRINT(WIDE_("."), 1);
					PRINT(cp, ndig-expt);
				}
			} else {	/* 'e' or 'E' */
				if (ndig > 1 || flags & ALT) {
					ox[0] = *cp++;
					ox[1] = L'.';
					PRINT(ox, 2);
					if (_double || flags & ALT == 0) {
						PRINT(cp, ndig-1);
					} else	/* 0.[0..] */
						/* __dtoa irregularity */
						PAD(ndig - 1, zeroes);
				} else	/* XeYYY */
					PRINT(cp, 1);
				PRINT(expstr, expsize);
			}
		}
#else
		PRINT(cp, size);
#endif
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		ret += width > realsz ? width : realsz;

		FLUSH();	/* copy out the I/O vectors */
	}
done:
	FLUSH();
error:
	//return (__sferror(fp) ? EOF : ret);
	return ret;
	/* NOTREACHED */
}

#ifdef FLOATING_POINT

extern "C" char *__dtoa (double, Int32, Int32, Int32 *, Int32 *, char **, char**);

static NAWchar *
cvt(double value, Int32 ndigits, Int32 flags, NAWchar* sign, Int32* decpt, Int32 ch, Int32* length)
	//double value;
	//int ndigits, flags, *decpt, ch, *length;
	//char *sign;
{
	Int32 mode, dsgn;
	NAWchar *digits, *bp, *rve;

	if (ch == L'f') {
		mode = 3;		/* ndigits after the decimal point */
	} else {
		/* To obtain ndigits after the decimal point for the 'e' 
		 * and 'E' formats, round to ndigits + 1 significant 
		 * figures.
		 */
		if (ch == L'e' || ch == L'E') {
			ndigits++;
		}
		mode = 2;		/* ndigits significant digits */
	}

	if (value < 0) {
		value = -value;
		*sign = L'-';
	} else
		*sign = L'\000';
	char* resultp;
	digits = __dtoa(value, mode, ndigits, decpt, &dsgn, &rve, &resultp);
	if ((ch != L'g' && ch != L'G') || flags & ALT) {	/* Print trailing zeros */
		bp = digits + ndigits;
		if (ch == L'f') {
			if (*digits == L'0' && value)
				*decpt = -ndigits + 1;
			bp += *decpt;
		}
		if (value == 0)	/* kludge for __dtoa irregularity */
			rve = bp;
		while (rve < bp)
			*rve++ = L'0';
	}
	*length = rve - digits;
	return (digits);
}

static Int32
exponent(NAWchar* p0, Int32 exp, Int32 fmtch)
	//char *p0;
	//int exp, fmtch;
{
	register NAWchar *p, *t;
	//char expbuf[MAXEXP];
	NAWchar expbuf[DBL_DIG]; /* MSDEV 4.0 */

	p = p0;
	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = L'-';
	}
	else
		*p++ = L'+';
	t = expbuf + sizeof(expbuf) /*MAXEXP*/;
	if (exp > 9) {
		do {
			*--t = to_char(exp % 10);
		} while ((exp /= 10) > 9);
		*--t = to_char(exp);
		for (; t < expbuf + sizeof(expbuf) /*MAXEXP*/; *p++ = *t++);
	}
	else {
		*p++ = L'0';
		*p++ = to_char(exp);
	}
	return (p - p0);
}

#endif
