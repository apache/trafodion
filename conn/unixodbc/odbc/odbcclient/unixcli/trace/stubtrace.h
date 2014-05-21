#ifndef __STUBTRACE_H__
#define __STUBTRACE_H__

#include "windows.h"
#include <sql.h>


#define UNALIGNED

class CEnvironment {

public: 
	CEnvironment();
	~CEnvironment();
	void SetEnvironment();
public :
	short	cpu;
	short	pin;
	short	trace_pin;
	long	nodenumber;
	char	nodename[17];
	char	volume[17];
	char	subvolume[17];
	char	progname[17];
};

extern char* versionString;

extern CEnvironment g_Environment;

// TRACE FLAGS
#define TR_ODBC_RANGE_MIN	0x00000000
#define TR_ODBC_ERROR       0x00000002
#define TR_ODBC_WARN        0x00000004
#define TR_ODBC_CONFIG      0x00000008
#define TR_ODBC_INFO        0x00000010
#define TR_ODBC_DEBUG       0x00000020
#define TR_ODBC_TRANSPORT	0x00000040
#define TR_ODBC_RANGE_MAX	0x0FFFFFFF


#endif
