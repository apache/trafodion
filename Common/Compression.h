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

#ifndef COMPRESSION_H
#define COMPRESSION_H

#define MIN_LENGTH_FOR_COMPRESSION 1000

class CCompression 
{
public:
	CCompression();
	~CCompression();
	bool compress(unsigned char *input, 
		          unsigned long input_size, 
				  int level,
		          unsigned char **output, 
		          unsigned long& output_size);

	bool expand(unsigned char *input,
		        unsigned long input_size, 
		        unsigned char **output, 
		        unsigned long& output_size,
				int& error);
private:


};


#endif
