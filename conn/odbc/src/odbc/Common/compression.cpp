// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
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

#include "compression.h"
#include <zlib.h>

CCompression::CCompression()
{
}

CCompression::~CCompression()
{
}

bool CCompression::compress(unsigned char *input, unsigned long input_size, int level, unsigned char **output, unsigned long& output_size)
{
	int ret, flush;
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, level);
	if (ret != Z_OK)
		return ret;

	strm.next_in = input;
	strm.avail_in = input_size;
	strm.data_type = Z_BINARY;

	output_size = deflateBound(&strm,input_size);
	*output = new unsigned char[output_size];
	if(!*output)
	{
		deflateEnd(&strm);
		return false;
	}

	strm.avail_out = output_size;
	strm.next_out = (Bytef*)*output;
	flush = Z_FINISH;

	ret = deflate(&strm, flush);
	deflateEnd(&strm);
	if(ret == Z_STREAM_END)  /* have to be Z_STREAM_END */
	{
		if((output_size-strm.avail_out)>=input_size)
		{
			return false;
		}
		output_size-=strm.avail_out;
		return true;
	}
	else
		return false;
}

bool CCompression::expand(unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long& output_size, int& error)
{
	int ret;
    z_stream strm;

	if(!input_size)
	{
		output_size = 0;
		return false;
	}

	strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
	if (ret != Z_OK)
        return ret;

	strm.next_in = input;
	strm.avail_in = input_size;
	strm.next_out = (Bytef*)*output;
	strm.avail_out=output_size;		 
    ret = inflate(&strm, Z_FINISH);

	inflateEnd(&strm);

	if(ret == Z_STREAM_END && strm.avail_out == 0)
	{
		error=0;
		return true;
	}
   	
    error=ret;
	output_size = 0;
	return false;
}
