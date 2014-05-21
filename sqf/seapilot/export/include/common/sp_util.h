// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
//
// $Id$
//

#ifndef __SP_UTIL_
#define __SP_UTIL_

#include <iostream>
#include <string>
#include <sys/time.h>

#include <boost/format.hpp>

using namespace std;

namespace seapilot
{
    using boost::format;

    static pid_t gv_pid = getpid();

    inline string GetTimeStr()
    {
        struct timeval tv;
        struct tm      tm_val;

        if (gettimeofday(&tv, NULL) < 0)
            tv.tv_sec = tv.tv_usec = 0;

        if (localtime_r(&tv.tv_sec, &tm_val))
            return str(format("%d-%02d-%02d %02d:%02d:%02d.%06d(%d)")
                               % (tm_val.tm_year + 1900)
                               % (tm_val.tm_mon  + 1)
                               % tm_val.tm_mday
                               % tm_val.tm_hour
                               % tm_val.tm_min
                               % tm_val.tm_sec
                               % tv.tv_usec
                               % gv_pid);
        else
            return " ";
    }

    class Stdout
    {
    public:
        template <typename T>
        ostream& operator << (const T &value) {
            cout << GetTimeStr() << " " << value;
            return cout;
        }

        // this is specific for endl, since it is not a variable
        ostream& operator << (basic_ostream<char, char_traits<char> >& 
                             (*_Pfn)(basic_ostream<char, char_traits<char> >&) )
        {
            cout << endl;
            return cout;
        }
    };
    
}
       
#endif
