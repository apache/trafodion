/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/

#ifndef EX_RLE_H
#define EX_RLE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExRLE.cpp
 * Description:  methods to encode/decode using RLE algorithm.
 *
 * Simple RLE encoder/decoder.
 * Provided by Venkat R.
 * This file need to be kept in sync with the corresponding
 * file maintained by NVT.
 *               
 * Created:      
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

/**
* Flag definitions
*/

#define COMP_ALGO_RLE_BASE        0x00000001
#define COMP_ALGO_RLE_PACKBITS    0x00000002

/**
* Whether single byte, unicode or integer comparison for runs
**/

#define COMP_COMPARE_BYTE         0x40000000
#define COMP_COMPARE_SHORT        0x20000000

/*
* Whether checksum is valid
*/
#define COMP_CHECKSUM_VALID       0x80000000


#define EYE_CATCHER_1   0x434F4D50
#define EYE_CATCHER_2   0x524C4500

/*
* The lengths are always in bytes in the
* header
*/

typedef struct buf_hdr {
	Int32 compressed_len;   
	Int32 eye_catcher1;
	Int32 eye_catcher2;
	Int32 checksum;
	Int32 exploded_len;
	Int32 flags;
} buf_hdr;

#define HDR_SIZE  sizeof(buf_hdr)
typedef struct comp_buf {
	union u {
		buf_hdr hdr;
		char byte[HDR_SIZE];
	} u;
	char data[1];
} comp_buf;



/*
* Simple RLE encoder/decoder
*/

/*
* Max number of counts we can express per char.  Longer runs will have
* multiple (char, count) pairs as needed.  Essentiall maximum count we
* can represent in a byte.
*/
#define MAXCOUNT         0x000000FF
#define MAXUCOUNT        0x0000FFFF
#define EOFCHAR          -1


#define ADLER32_MODULUS  65521


/* minimum run length to encode */
#define RLE_PACKBITS_MINRUN          3

/* Max copy size */
#define RLE_PACKBITS_MAXCOPY_BYTE    128
#define RLE_PACKBITS_MAXCOPY_SHORT   32768

/* maximum run length to encode */
#define RLE_PACKBITS_MAXRUN_BYTE    (RLE_PACKBITS_MAXCOPY_BYTE + RLE_PACKBITS_MINRUN - 1)
#define RLE_PACKBITS_MAXRUN_SHORT    (RLE_PACKBITS_MAXCOPY_SHORT + RLE_PACKBITS_MINRUN - 1)

#define RLE_PACKBITS_MAXBUF_BYTE    (RLE_PACKBITS_MAXCOPY_BYTE + RLE_PACKBITS_MINRUN - 1)
#define RLE_PACKBITS_MAXBUF_SHORT    (RLE_PACKBITS_MAXCOPY_SHORT + RLE_PACKBITS_MINRUN - 1)

static Int32 encode_byte(unsigned char *dbuf, Int32 dlen, unsigned char *ebuf, Int32 *elen);
static Int32 pb_encode_byte(unsigned char *dbuf, Int32 dlen, unsigned char *ebuf, Int32 *elen);
static Int32 encode_short(unsigned short *dbuf, Int32 dlen_bytes, unsigned short *ebuf, Int32 *elen_bytes);
static Int32 pb_encode_short(unsigned short *dbuf, Int32 dlen_bytes, unsigned short *ebuf, Int32 *elen_bytes);
static Int32 decode_byte(unsigned char *ebuf, Int32 elen, unsigned char *dbuf, Int32 *dlen);
static Int32 pb_decode_byte(unsigned char *ebuf, Int32 elen, unsigned char *dbuf, Int32 *dlen);
static Int32 decode_short(unsigned short *ebuf, Int32 elen_bytes, unsigned short *dbuf, Int32 *dlen_bytes);
static Int32 pb_decode_short(unsigned short *ebuf, Int32 elen_bytes, unsigned short *dbuf, Int32 *dlen_bytes);

Int32 ExEncode(unsigned char *dbuf, Int32 dlen, unsigned char *ebuf, Int32 *elen, Int32 flags);
Int32 ExDecode(unsigned char *ebuf, Int32 elen, unsigned char *dbuf, Int32 *dlen,
	     Lng32 &param1, Lng32 &param2);


#endif


