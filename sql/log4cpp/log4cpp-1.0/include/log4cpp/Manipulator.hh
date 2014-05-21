/*
 * Manipulator.hh
 *
 * Copyright 2005, Francis ANDRE. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef _LOG4CPP_MANIPULATOR_HH
#define _LOG4CPP_MANIPULATOR_HH

#include <iostream>
#include <log4cpp/Portability.hh>
namespace log4cpp {
	class LOG4CPP_EXPORT width {
	private:
		unsigned int size;
	public:
inline	width(unsigned int i) : size(i) {}
#ifdef NA_NSK
friend LOG4CPP_EXPORT ostream& operator<< (ostream& os, const width& w);
#else
friend LOG4CPP_EXPORT std::ostream& operator<< (std::ostream& os, const width& w);
#endif
	};
class LOG4CPP_EXPORT tab {
	private:
		unsigned int size;
	public:
inline	tab(unsigned int i) : size(i) {}
#ifdef NA_NSK
friend LOG4CPP_EXPORT ostream& operator<< (ostream& os, const tab& w);
#else
friend LOG4CPP_EXPORT std::ostream& operator<< (std::ostream& os, const tab& w);
#endif
	};
}
#endif
