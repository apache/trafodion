//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

void myprintf(const char *format, ...) {
    va_list         ap;
    int             len;
    char            line[1000];
#ifdef TS
    int             ms;
    struct timeval  t;
    struct tm       tmbuf;
    struct tm      *tx;
    int             us;
#endif

#ifdef TS
    util_time_gtod(&t, NULL);
    tx = localtime_r(&t.tv_sec, tmbuf);
    ms = t.tv_usec / 1000;
    us = t.tv_usec - ms * 1000;
    sprintf(line, "%02d:%02d:%02d.%03d.%03d [%s] ",
            tx->tm_hour, tx->tm_min, tx->tm_sec, ms, us,
            gdisplay_name);
#else
    sprintf(line, "[%s] ", gdisplay_name);
#endif
    len = (int) strlen(line); // cast
    va_start(ap, format);
    vsprintf(&line[len], format, ap);
    va_end(ap);
    printf(line);
    fflush(stdout);
}

