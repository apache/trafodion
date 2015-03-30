/*************************************************************************
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**************************************************************************/

/*
 * mreadline - v1.1.0 
 * vim:hls:ru:scs:si:sm:sw=4:sta:ts=4:tw=0
 *
 * Perfection is achieved, not when there is nothing more to add, but when there is nothing left to take away.
 * -- Antoine de Saint Exupéry
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#define LINE_CHUNK	256					/* line buffer chunks */
#define PRMP_CHUNK	64					/* prompt chunks */
#ifdef __CYGWIN__
	#undef WORD
	#include <windows.h>
	#undef _WIN32
#endif 	
#ifdef _WIN32
	#include <windows.h>
	CONSOLE_SCREEN_BUFFER_INFO _wsz;	/* used to get terminal size */
#else
	#include <termios.h>
	#include <unistd.h>
	#include <sys/ioctl.h>
	struct winsize _wsz;				/* used to get terminal size */
	struct termios _oattr;				/* old terminal attr */
	char *_tmpfil = "/tmp/mrXXXXXX";	/* tmp file name template */
#endif
	char **_mrhist = 0;					/* history records array */
	size_t *_mrhll = 0;					/* history records length array */
	size_t *_mrhms = 0;					/* history records allocated array */
	unsigned int _mrhsize = 200,		/* default history size */
				 _mrcy = 0,				/* Current Y cursor position */
				 _mrhfirst = 0,			/* First (older) history element index */
				 _mrhcount = 0,			/* Number of history records */
				 _mrchg = 0;			/* Change flag: 1=yes, 0=no */
unsigned int mrcol = 80;				/* number of screen columns */
unsigned int _mrp = PRMP_CHUNK;			/* mrprompt buffer size */
char *_mrprompt = 0;					/* mreadline prompt string */
char *_mredit = 0;
char _mrinit = 0;						/* init flag is set to 1 during initialization */

/* mrend: end mreadline
 *
 * input: none
 * return: void
 */
void mrend ( void )
{
	unsigned int i = 0,					/* loop variable */
				 mx = 0;				/* history index */

	for ( i = 0 ; i < _mrhcount ; i++ ) {
		mx = ( _mrhfirst + i ) % _mrhsize ;
		if ( _mrhist[mx] )
			free ( _mrhist[mx] ) ;
	}
	if ( _mrhist[_mrhsize] )
		free ( _mrhist[_mrhsize] );
	if ( _mrhist[_mrhsize + 1] )
		free ( _mrhist[_mrhsize + 1] );
	if ( _mrhist )
		free ( _mrhist );
	if ( _mrhms )
		free ( _mrhms );
	if ( _mrhll )
		free ( _mrhll );
	if ( _mrprompt )
		free ( _mrprompt );				/* free prompt memory */
#ifndef _WIN32
	(void)tcsetattr (0, TCSANOW, &_oattr);
#endif
}

/* mrgetcol: Get nummber of screen columns
 *
 * input: none
 * return: void
 */
void mrgetcol( void )
{
#ifdef _WIN32
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &_wsz))
		mrcol = (unsigned int)(_wsz.srWindow.Right - _wsz.srWindow.Left) - 1;
#else
	(void)ioctl(0, TIOCGWINSZ, &_wsz);
	if ( _wsz.ws_col > 0 )
		mrcol = (unsigned int)_wsz.ws_col;
#endif
}

/* mrinit: initialize mreadline
 *
 * input: none
 * return: 0 = status ok, 1 errors
 */
int mrinit ( void )
{
#ifndef _WIN32
	struct termios tattr;

 	/* Make sure stdin is a terminal. */
 	if (!isatty (0)) {
		fprintf (stderr, "[mrinit (%d)] Not a terminal.\n", __LINE__);
		exit ( 2 );
    }

	/* Get nummber of columns */
	mrgetcol();

	/* Get current terminal attributes */
	(void)tcgetattr (0, &_oattr);

	/* Set terminal modes. */
	tattr = _oattr;						/* dup current settings */
	tattr.c_lflag &= ~ICANON;			/* No Buffering */
	tattr.c_lflag &= ~ISIG;				/* Ignore Ctrl-C */
	tattr.c_lflag &= ~ECHO;				/* No Echo */
	tattr.c_cc[VMIN] = 1;				/* min number of read chars */
	tattr.c_cc[VTIME] = 0;				/* no delay after key press */
	(void)tcsetattr (0, TCSAFLUSH, &tattr);	/* set new attrs */

#endif
	/* Allocate initial prompt memory chunk */
	if ( ( _mrprompt = malloc ( PRMP_CHUNK ) ) == (void *) NULL ) {
		fprintf(stderr, "[mreadline (%d)] Error %d allocating _mrprompt memory:%s\n",
			__LINE__, errno, strerror(errno));
		return ( 1 );
	}

	/* Get editor */
	_mredit = getenv("_mredit");
	if ( !_mredit )
		_mredit = getenv("EDITOR");

	return ( 0 );
}

#ifndef _WIN32
/* _mrefresh: refresh input line
 *
 * prompt: prompt string. If prompt is Null move the cursor without re-printing current line
 * pl: prompt length
 * line: string to display
 * ps: cursor position in line
 * return: void
 */
void _mrefresh ( char *prompt, unsigned int pl, char *line, unsigned int ps )
{
	unsigned int i = 0,		/* loop variable */
				 j = 0,		/* loop variable */
				 cx = 0,	/* new Cursor position X (column from line start) */
				 cy = 0,	/* new Cursor position Y (lines from bottom) */
				 n = 0;		/* number of lines in line */

	for ( i = 0, j = 0 ; line[i]; i++, j++) {
		if ( line[i] == '\n' || j >= mrcol ||
			 ( i + pl < mrcol && j + pl >= mrcol - 1 ) ) {
			n++;
			j = 0;
		}
		if ( ps - 1 == i ) {
			cy = n ;
			cx = j ;
		}
	}
	if ( prompt ) {							/* if we have to re-print the line */
		if ( _mrcy ) {						/* previous line cy */
			if ( !_mrchg || ( _mrchg && n ) )
				printf("\033[%uF", _mrcy);	/* go up _mrcy lines to col #0 (beginning of the line) */
		} else {							/* everything on one row */
			(void)fputs ("\033[0G", stdout );	/* cursor to the left */
		}
		(void)fputs ("\033[J", stdout);		/* erase below */
		(void)fputs ( prompt, stdout );		/* write prompt */
		(void)fputs ( line, stdout );		/* write line */
		if ( n > cy )
			printf("\033[%uF", n - cy);		/* go up to the beginning of the line */
	} else {
		if ( _mrcy > cy ) 
			printf("\033[%uF", _mrcy - cy);	/* go up to the beginning of the line */
		else if ( _mrcy < cy )
			printf("\033[%uE", cy - _mrcy);
		(void)fputs ("\033[0G", stdout );	/* cursor to the left */
	}
	if ( cy ) {
		if ( cx )
   			printf("\033[0G\033[%uC", cx );
		else if ( ps == i )
			fputc('\n', stdout);
	} else {
    	printf("\033[0G\033[%uC", ps + pl ); 
	}
	(void)fflush(stdout);					/* flush stdout */
	_mrcy = cy;								/* save current cursor y position */
}
#endif
/* mrhadd: add history record
 *
 * hrec: history record to add
 * hl: history record length
 * return: 0 OK, -1 errors
 */
int mrhadd(char *hrec, size_t hl)
{
#ifndef _WIN32
	unsigned int next = 0,	/* History record element to use */
				 last = 0;	/* Previous history record */

	if ( ! _mrinit )	{	/* Not during the initialization */
		/* do not store empty commands */
		if ( !hl )
			return ( 0 );

		/* do not store consecutive entry of the same command */
		last = ( _mrhfirst + _mrhcount - 1 ) % _mrhsize ;
		if ( hl == _mrhll[last] )
			if ( !strcmp(hrec, _mrhist[last]) )
				return ( 0 ) ;
	}

	next = ( _mrhfirst + _mrhcount ) % _mrhsize ;
	
	if ( hl >= _mrhms[next] ) {		/* new memory to be allocated for this element */
		if ( ( _mrhist[next] = realloc ( _mrhist[next], hl + 1 ) ) == (void *)NULL ) {
			fprintf(stderr, "[mreadline (%d)] - Error %d allocating _mrhist memory element %d: %s\n",
				__LINE__, errno, next, strerror(errno));
			return(-1);
		}
		_mrhms[next] = hl + 1 ;		/* Update history memory array */
	}
	memcpy(_mrhist[next], hrec, hl);	/* Copy history element */
	_mrhist[next][hl] = '\0';		/* Put a NULL at the end */
	_mrhll[next] = hl;				/* Update history length array */
	if ( _mrhcount < _mrhsize ) 		/* If history array is NOT full... */
		_mrhcount++;					/* Increase count (first element unchanged) */
	else							/* If history array is full... */
		_mrhfirst = next + 1;		/* Increase first element (count unchanged) */
#endif
	return(0);
}

/* mrhinit: history init
 *
 * hsize: history size. If zero use default _mrhsize
 * hfile: history file. If NULL use ./._mrhistory
 * return: 0=OK, 1=problems with history file != No such file, 2/3/4 = memory allocation error
 */
int mrhinit(size_t hsize, char *hfile)
{
#ifndef _WIN32
	FILE *fh;						/* history file pointer */
	int ch;							/* char read from history file */
	char *hrec;						/* history record buffer */
#endif
	size_t hl = 0;					/* history record length */
	size_t hrl = LINE_CHUNK;		/* history buffer size */

	_mrinit = 1;					/* switch on initialization flag */

	if ( hsize )
		_mrhsize = (unsigned int)hsize;

	/* history buffer memory allocation (extra '_mrhsize' elements to hold & save current line when browsing hist */
	if ( ( _mrhist = (char **)calloc (_mrhsize+2, sizeof(char *))) == (void *)NULL ) {
		fprintf(stderr, "[mrhinit (%d)] - Error %d allocating history record array: %s\n",
			__LINE__, errno, strerror(errno));
		return (2);
	}
	if ( ( _mrhist[_mrhsize] = malloc (LINE_CHUNK)) == (void *)NULL ) {
		fprintf(stderr, "[mrhinit (%d)] - Error %d allocating _mrhist[mrsize]: %s\n",
			__LINE__, errno, strerror(errno));
		return (2);
	}
	if ( ( _mrhll = (size_t *)calloc (_mrhsize+2, sizeof(size_t))) == (void *)NULL ) {
		fprintf(stderr, "[mrhinit (%d)] - Error %d allocating history lenght array: %s\n",
			__LINE__, errno, strerror(errno));
		return (3);
	}
	if ( ( _mrhms = (size_t *)calloc (_mrhsize+2, sizeof(size_t))) == (void *)NULL ) {
		fprintf(stderr, "[mrhinit (%d)] - Error %d allocating history  memorylenght array: %s\n",
			__LINE__, errno, strerror(errno));
		return (3);
	}
	_mrhms[_mrhsize]=LINE_CHUNK;
#ifndef _WIN32
	if ( ( fh = fopen(hfile ? hfile : "._mrhistory" , "r" ) ) == (FILE *)NULL ) {
		if ( errno == ENOENT ) {	/* history file does not exists */
			fprintf(stderr, "[mrhinit (%d)] - Warning history file %s does not exist\n",
				__LINE__, hfile ? hfile : "._mrhistory" );
			return ( 0 );
		}
		fprintf(stderr, "[mrhinit (%d)] - Error %d opening history file %s: %s\n",
				__LINE__, errno, hfile ? hfile : "._mrhistory", strerror(errno) );
		return ( 1 );
	}
	if ( ( hrec = malloc ( hrl ) ) == (void *) NULL ) {
		fprintf(stderr, "[mrhinit (%d)] Error %d allocating memory: %s\n",
			__LINE__, errno, strerror(errno));
		return (4);
	}
	while ( ( ch = fgetc(fh) ) ) {
		if ( ch == 1 ) {				/* End of record  (CTRL-A) */
			if ( mrhadd ( hrec, hl ) )
				fprintf(stderr, "[mrhinit (%d)] Error setting history\n", __LINE__);
			hl = 0;
		} else if ( feof ( fh ) ) {		/* End of history file */
			if ( hl )
				if( mrhadd ( hrec, hl ) )
					fprintf(stderr, "[mrhinit (%d)] Error setting history\n", __LINE__);
			break;
		} else {
			if ( ( hl + 1 ) == hrl ) {	/* history record needs more memory */
				hrl += LINE_CHUNK ;
				if ( ( hrec = realloc(hrec, hrl)) == (void *)NULL ) {
					fprintf(stderr,
						"[mrhinit (%d)] - Error %d reallocating hrec memory: %s\n",
							__LINE__, errno, strerror(errno));
					return(5);
				}
			}
			hrec[hl++] = (char)ch;
		}
	}
	free ( hrec );
	(void)fclose ( fh );
#endif
	_mrinit = 0;					/* switch off initialization flag */
	return(0);
}

/* mrhsave: save history record
 *
 * hfile: history file. If NULL use ./._mrhistory
 * return: void
 */
int mrhsave(char *hfile)
{
#ifndef _WIN32
	FILE *fh;				/* history file pointer */
	unsigned int mx = 0,	/* history index */
				 i = 0;		/* loop variable */

	if ( ( fh = fopen(hfile ? hfile : "._mrhistory" , "w" ) ) == (FILE *)NULL ) {
		fprintf(stderr, "[mrhinit (%d)] - Error %d opening history file %s: %s\n",
				__LINE__, errno, hfile ? hfile : "._mrhistory", strerror(errno) );
		return (3);
	}
	for ( i = 0 ; i < _mrhcount ; i++ ) {
		mx = ( _mrhfirst + i ) % _mrhsize ;
		(void)fputs ( _mrhist[mx] , fh );	/* write history record */
		(void)fputc ( 1 , fh );				/* write history record separator */
	}
	(void)fclose ( fh );
#endif
	return ( 0 );
}

/* _mrcpbuff: copy history buffer elements
 *
 * src: source history buffer index
 * tgt: target history buffer index
 * return: 0=OK, 1=ERROR
 */
unsigned int _mrcpbuff(unsigned int src, unsigned int tgt)
{
	if ( _mrhms[tgt] <= _mrhll[src] ) {	/* tgt need more memory */
		_mrhms[tgt] = _mrhll[src] + 1 ;
		if ( (_mrhist[tgt]=realloc(_mrhist[tgt], _mrhms[tgt])) == (void *)NULL ) {
			fprintf(stderr, "[mreadline (%d)] - Error %d reallocating _mrhist[%d] memory: %s\n",
				__LINE__, errno, tgt, strerror(errno));
			return(1);
		}
	}
	if ( _mrhll[src] ) {
		memcpy(_mrhist[tgt], _mrhist[src], _mrhll[src] + 1 );	/*  +1 is to copy the ending NULL */
	} else {
		_mrhist[tgt][0] = '\0';
	}
	_mrhll[tgt] = _mrhll[src];
	return ( 0 );
}

/* mreadline: read next line
 *
 * input: prompt 
 * return: line read
 */
char *mreadline ( char *prompt, unsigned int *length )
{
	char *mrline = _mrhist[_mrhsize];	/* local pointer to current buffer */
#ifndef _WIN32
	int	fd = 0;					/* tmp file descriptor */
	char buff[128];				/* edit buffer */
	char tfile[14];				/* temporary file name */
	ssize_t ret = 0;			/* read return value */
	unsigned int ps = 0,		/* cursor position in mrline */
				 pss = 0,		/* cursor position in mrline save */
				 nx = 0,		/* to store temporary prev/next index */
				 mx = 0;		/* history index delta respect current buffer */
#endif
	unsigned int i = 0,			/* loop variable */
				 q = 0;			/* Quoted Text Flag */
	size_t ll = 0,				/* current buffer (_mrhist[mrsize]) length */
		   pl = 0,				/* prompt length (visible characters) */
		   j = 0;				/* loop variable */
	int c;						/* read char */

	/* Initialize edit flag */
	_mrchg = 0 ;

	/* Print Prompt */
	(void)fflush ( stdout );
	for ( i = 0, pl = 0, q = 0 ; prompt[i] ; i++ ) {
		if ( prompt[i] == 1 ) {
			q = q ? 0 : 1;
		} else {
			if ( j > _mrp - 1 ) {			/* _mrprompt needs more memory */
				_mrp += PRMP_CHUNK;
				if ( (_mrprompt=realloc(_mrprompt, (size_t)_mrp)) == (void *)NULL ) {
					fprintf(stderr, "[mreadline (%d)] - Error %d reallocating _mrprompt memory: %s\n",
						__LINE__, errno, strerror(errno));
					return((char *)NULL);
				}
			}
			_mrprompt[j++] = prompt[i];
			if ( !q )
				pl++;
		}
	}
	_mrprompt[j]='\0';
	(void)fputs ( _mrprompt, stdout );

	/* Main loop */
	while ( ( c = fgetc(stdin) ) ) {;
#ifdef _WIN32
		if ( c != 3 )
			*(mrline + ll++) = c;
#else
		switch ( c ) {
			case 1:		/* CTRL-A: go to line start */
				ps = 0;
				_mrefresh( (char *)NULL, pl, mrline, ps );
				break;
			case 2:		/* CTRL-B: cursor back */
				back:
				if ( ps ) {
					ps--;
					_mrefresh( (char *)NULL, pl, mrline, ps );
				}
				break;
			case 4:		/* CTRL-D: EOF */
				if ( ll == 0 ) {	 			/* if is the first char... */
					return ( (char *) EOF );	/* ...return EOF */
				} else if ( ps < ll ) {			/* remove to the right */
					memmove(mrline+ps, mrline+ps+1, ll - ps);
					mrline[--ll] = '\0';
					_mrefresh( _mrprompt, pl, mrline, ps );
				}
				break;
			case 5:		/* CTRL-E: go to line end */
				ps = ll;
				_mrefresh( (char *)NULL, pl, mrline, ps );
				break;
			case 6:		/* CTRL-F: cursor forward */
				forward:
				if ( ps < ll ) {
					ps++;
					_mrefresh( (char *)NULL, pl, mrline, ps );
				}
				break;
			case 7:		/* CTRL-G: go to history record */
				mx = (unsigned int) atoi(mrline);
				if ( _mrhist[mx] ) {
					memcpy ( (void *)mrline, (void *)_mrhist[mx], (size_t)_mrhll[mx] );
					ll = ps = _mrhll[mx];
					mrline[ll] = '\0';
				} else {
					fprintf(stderr, "[mrhinit (%d)] - Error. Cannot access history entry %d\n",
							__LINE__, mx );
				}
				_mrefresh( _mrprompt, pl, mrline, ps );
				break;
			case 8:		/* CTRL-H: Back Space */
			case 127:	/* Delete */
				if ( ps ) {
					memmove( mrline+ps-1, mrline+ps, ll - ps );
					ps--;
					mrline[--ll] = '\0';
				}
				_mrchg = 1 ;
				_mrefresh( _mrprompt, pl, mrline, ps );
				break;
			case 9:		/* Horizontal Tab */
				break;		/* do nothing */
			case 10:	/* New Line: just echo */
				if ( ll )	/* move to buffer end if any */
					_mrefresh( (char *)NULL, pl, mrline, ll );
				(void)fputc ( c, stdout );
				_mrcy = 0;	/* reset previous line Y */
				break;
			case 12:	/* CTRL-L: to lower */
				for ( i = 0, q = 0 ; i < (unsigned int)ll ; i++ )
					if ( mrline[i] == '\'' )
						q = q ? 0 : 1 ;
					else if ( !q )
						mrline[i] = (char)tolower ( (int)mrline[i] );
				_mrefresh( _mrprompt, pl, mrline, ps );
				break;
			case 14:	/* CTRL-N: history next */
				histnext:
				if ( mx ) 
					mx--;
				if ( mx ) {
					nx = ( _mrhcount + _mrhfirst - mx ) % _mrhsize ;
					mrline = _mrhist[ nx ]; 
					ll = ps = _mrhll[ nx ];
				} else {	/* restore current buffer */
					_mrcpbuff(_mrhsize+1, _mrhsize);	/* copy saved buffer into current buffer */
					ll = _mrhll[ _mrhsize ] ;
					ps = pss ;
					mrline = _mrhist[_mrhsize];
				}
				_mrefresh( _mrprompt, pl, mrline, ps );
				break;
			case 16:	/* CTRL-P: history previous */
				histprev:
				if ( !mx ) {
					_mrhll[ _mrhsize ] = ll ;
					if ( ll ) {
						_mrcpbuff(_mrhsize, _mrhsize+1);/* save current buffer */
						pss = ps;						/* save current buffer position */
					}
				}
				if ( mx < _mrhcount )
					mx++;
				nx = ( _mrhcount + _mrhfirst - mx ) % _mrhsize ;
				_mrcpbuff(nx, _mrhsize);				/* copy buffer nx in current buffer */
				ll = ps = _mrhll[ nx ];
				mrline = _mrhist[_mrhsize];
				_mrefresh( _mrprompt, pl, mrline, ps );
				break;
			case 23:	/* CTRL-W: List History */
				(void)fputc ( '\n' , stdout );
				for ( i = 0 ; i < _mrhcount ; i++ ) {
					nx = ( _mrhfirst + i ) % _mrhsize ;
					fprintf(stdout, "%5d:  %s\n", nx, _mrhist[nx]);
				}
				mrline[ll] = '\0';
				_mrefresh( _mrprompt, pl, mrline, ps  );
				break;
			case 18:	/* CTRL-R: redraw */
				mrgetcol();
				_mrefresh( _mrprompt, pl, mrline, ps );
				break;
			case 21:	/* CTRL-U: to upper */
				for ( i = 0, q = 0 ; i < (unsigned int)ll ; i++ )
					if ( mrline[i] == '\'' )
						q = q ? 0 : 1 ;
					else if ( !q )
						mrline[i] = (char)toupper ( (int)mrline[i] );
				_mrefresh( _mrprompt, pl, mrline, ps );
				break;
			case 22:	/* CTRL-V: edit current line */
				if ( !_mredit ) {
					fprintf(stderr, "[mreadline (%d)] - Error: neither _mredit nor EDITOR are set(_mredit=>%s<\n",
						__LINE__, _mredit);
					break;
				}
				strncpy ( tfile, _tmpfil, sizeof(tfile) ) ;			/* tmp file name */
				if ( ( fd = mkstemp(tfile) ) == ( -1 ) ) {
					fprintf(stderr, "[mreadline (%d)] - Error %d creating tmp file: %s\n",
						__LINE__, errno, strerror(errno));
				} else {
					if ( (unsigned int)( ret = write ( fd, mrline, (size_t)ll ) ) != ll )
						fprintf(stderr, "[mreadline (%d)] - Error %d writing tmp file: %s\n",
							__LINE__, errno, strerror(errno));
					(void)fsync ( fd );
					(void)snprintf(buff, sizeof(buff), "%s %s", _mredit, tfile);
					if ( system(buff) < 0 )
						fprintf(stderr, "[mreadline (%d)] - Error running %s\n", __LINE__, buff);
					mrline[0] = '\0';
					ll = 0;
					(void)lseek ( fd, (off_t)0, SEEK_SET ) ;
					while ( ( ret = read ( fd, buff, sizeof(buff) ) ) ) {
						if ( ret == ( -1 ) ) {
							fprintf(stderr, "[mreadline (%d)] - Error %d reading tmp file: %s\n",
								__LINE__, errno, strerror(errno));
							break;
						}
						while ( ( ll + (unsigned int)ret ) >= _mrhms[_mrhsize] ) {				/* mrline needs more memory */
							_mrhms[_mrhsize] += LINE_CHUNK;
							if ( (_mrhist[_mrhsize]=realloc(_mrhist[_mrhsize], _mrhms[_mrhsize])) == (void *)NULL ) {
								fprintf(stderr, "[mreadline (%d)] - Error %d reallocating mrline memory: %s\n",
									__LINE__, errno, strerror(errno));
								return((char *)NULL);
							}
						}
						mrline = _mrhist[_mrhsize];
						strncat ( mrline, buff, (size_t) ( ret + 1 ) ) ;
						ll += ret;
					}
					(void)close ( fd );			/* close temporary file */
					(void)unlink ( tfile );		/* remove temporary file */
					mrline[--ll] = '\0';		/* exclude last \n added by vi */
					ps = ll;					/* set cursor position to the end */
					_mrefresh( _mrprompt, pl, mrline, ps );
				}
				break;
			case 24:	/* CTRL-X: Clear line */
				ll = 0;
				(void)fputc ( '\n' , stdout );
				return ((char *)(-2));
			case 26:	/* CTRL-Z: Return last history entry (repeat command) */
				nx = ( _mrhcount + _mrhfirst - 1 ) % _mrhsize ;
				*length = ps = _mrhll[nx];
				_mrcpbuff(nx, _mrhsize);	/* copy buffer nx in current buffer */
				_mrefresh( _mrprompt, pl, mrline, ps );
				(void)fputc ( '\n' , stdout );
				_mrcy = 0;					/* reset previous line Y */
				return (mrline);
			case 27:	/* Escape: */
				if ( ( c = fgetc(stdin) ) == 91 ) {
					switch ( ( c = fgetc(stdin) ) ) {
						case 65:	/* Up Arrow */
							goto histprev;
						case 66:	/* Down Arrow */
							goto histnext;
						case 67:	/* Right Arrow */
							goto forward;
						case 68:	/* Left Arrow */
							goto back;
					}
				}
				break;
			default:	/* Edit current buffer */
				if ( ps != ll ) {
					memmove(mrline+ps+1, mrline+ps, ll - ps);
					mrline[ps++] = (char)c;	
					mrline[++ll] = '\0';
					_mrefresh( _mrprompt, pl, mrline, ps );
				} else {
					mrline[ps++] = (char)c;
					mrline[++ll] = '\0';
					(void)fputc ( c, stdout );
					if ( ( ps + pl ) % mrcol == 0 )
						(void)fputc ( '\n', stdout );
					_mrcy = ( ps + pl ) / mrcol ;
				}
		}
#endif
		if( ll > _mrhms[_mrhsize] ) {				/* mrline needs more memory */
			_mrhms[_mrhsize] += LINE_CHUNK;
			if ( (_mrhist[_mrhsize]=realloc(_mrhist[_mrhsize], _mrhms[_mrhsize])) == (void *)NULL ) {
				fprintf(stderr, "[mreadline (%d)] - Error %d reallocating _mrhist[_mrhsize] memory: %s\n",
					__LINE__, errno, strerror(errno));
					return((char *)NULL);
			}
			mrline = _mrhist[_mrhsize] ;
		}
		if ( (char)c == '\n' ) {
			break;
		}
	}
	/* End */
#ifdef _WIN32
	if ( mrline[ll-1] == '\n' )
		ll--;
#endif
	*length = (unsigned int) ll ;			/* Write line length */
	mrline[ll] = '\0';
	return ( mrline );		/* return */
}

/* mresize: check if mreadline buffer should be resized
 *
 * size: new mreadline size
 * return: pointer to new (or old if not resized) _mrhist[_mrhsize]
 */
char *mresize(size_t size)
{
	if ( size > _mrhms[_mrhsize] ) {
		if ( (_mrhist[_mrhsize]=realloc(_mrhist[_mrhsize], size)) == (void *)NULL ) {
			fprintf(stderr, "[mreadline (%d)] - Error %d reallocating _mrhist[_mrhsize] memory: %s\n",
				__LINE__, errno, strerror(errno));
			return((char *)NULL);
		}
		_mrhms[_mrhsize] = size;
	}
	return ( _mrhist[_mrhsize] );
}
