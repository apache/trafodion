/**********************************************************************
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
********************************************************************/

#include "WmsEvents.h"

//check the input Julian timestamp in valid range current_timestamp +/- one year
int64 getValidJulianTimestamp(const int64 usec_timestamp)
{
	if (usec_timestamp > 0)
	{
		const int USECS_PER_SEC = 1000000;  						// u-secs per sec
		const int SECS_PER_YEAR = 31536000; 						// secs per year
		int64 sec_timestamp = usec_timestamp / USECS_PER_SEC;		// input time in seconds
		int64 cur_timestamp = JULIANTIMESTAMP() / USECS_PER_SEC;	// current time in seconds

		if (sec_timestamp >= (cur_timestamp - SECS_PER_YEAR) && sec_timestamp <= (cur_timestamp + SECS_PER_YEAR))
			return usec_timestamp;
	}

	return -1;
}

//convert UCT to Local Time in u-seconds
int64 utc2lct_useconds(const int64 utcTimestamp)
{
	int64 lctTimestamp = 0;
	const int U_SECS = 1000000;

	if (utcTimestamp > 0)
	{
		int64 utcTS = utcTimestamp / U_SECS;
		int64 utcTS_usec = utcTimestamp % U_SECS;
		lctTimestamp = (int64)utc2lct_seconds(utcTS) * U_SECS + utcTS_usec;
	}

	return lctTimestamp;
}

//convert UCT to Local Time in seconds
time_t utc2lct_seconds(const time_t utcTimestamp)
{
	time_t lctTimestamp = 0;

	if (utcTimestamp > 0)
	{
		tm localTime;
		localtime_r(&utcTimestamp, &localTime);
		lctTimestamp = timegm(&localTime);
	}

	return lctTimestamp;
}

//convert UCT to Local Time in u-seconds using Julian timestamp
int64 utc2lct_useconds_jts(const int64 utcTimestamp)
{
#if 1
	short error;
	return CONVERTTIMESTAMP(utcTimestamp, 0, -1, &error);
#else
	return utc2lct_useconds(utcTimestamp - JULIANTIME_DIFF) + JULIANTIME_DIFF;
#endif
}

//convert Local Time to UCT in u-seconds
int64  lct2utc_useconds(const int64 lctTimestamp)
{
	int64 utcTimestamp = 0;
	const int U_SECS = 1000000;

	if (lctTimestamp > 0)
	{
		int64 lctTS = lctTimestamp / U_SECS;
		int64 lctTS_usec = lctTimestamp % U_SECS;
		utcTimestamp = (int64)lct2utc_seconds(lctTS) * U_SECS + lctTS_usec;
	}

	return utcTimestamp;
}

//convert Local Time to UCT in seconds
time_t lct2utc_seconds(const time_t lctTimestamp)
{
	time_t utcTimestamp = 0;

	if (lctTimestamp > 0)
	{
		tm gmTime;
		gmtime_r(&lctTimestamp, &gmTime);
		gmTime.tm_isdst = -1;
		utcTimestamp = mktime(&gmTime);
	}

	return utcTimestamp;
}

//convert Local Time to UCT in u-seconds using Julian timestamp
int64  lct2utc_useconds_jts(const int64 lctTimestamp)
{
#if 1
	short error;
	return CONVERTTIMESTAMP(lctTimestamp, 2, -1, &error);
#else
	return lct2utc_useconds(lctTimestamp - JULIANTIME_DIFF) + JULIANTIME_DIFF;
#endif
}

bool findString(char *p, int *idx, string msg)
{
   *idx = msg.find(p);
   if (*idx == string::npos)
   {
      return false;
   }
   return true;
   ;
}

int getComponentId()
{
	return isWms ? SQEVL_WMS : SQEVL_NDCS;
}

long getLongEventId(int type, int component_id, int event_id, int max_len)
{
   char *buff = new char[max_len+1];

   if (buff == 0)
      return -1;

   int len = sprintf(buff, "%01d%02d%06d", type, component_id, event_id);
   len = (len > max_len) ? max_len : len;
   buff[len] = '\0';
   long long_event_id = atol(buff);

   delete []buff;

   return long_event_id;
}

char * _itoa(int n, char *buff, int base) {

   char t[100], *c=t, *f=buff;
   int d;
   char sign;
   int bit;
   unsigned int un;

   if (base == 10) {
		if (n < 0) {
			*(f++) = '-';
			un = -n;
		} else
			un = n;

	   while ( un > 0) {
			d = un % base;
			*(c++) = d + '0';
			un = un / base;
	   }
   } else {
	  if (base == 2) bit = 1;
      else if (base == 8) bit = 3;
      else if (base == 16) bit = 4;
      else
		  return "";

	  while (n != 0) {
		 d = (n  & (base-1));
		 *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
		 n = (unsigned int) n >> bit;
	  }

   }

   c--;

   while (c >= t) *(f++) = *(c--);

   if (buff == f)
	   *(f++) = '0';
   *f = '\0';
   return buff;
}



