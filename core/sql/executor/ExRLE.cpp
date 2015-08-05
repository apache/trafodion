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

#include "ExRLE.h"

Int32
ExEncode(unsigned char *dbuf, Int32 dlen, unsigned char *ebuf, Int32 *elen, Int32 flags) {
  Int32 c = EOFCHAR, p = EOFCHAR;
  Int32 i = 0;
  UInt32 j = 0;
  Int32 cnt = 0;
  UInt32 cs_lo = 1, cs_hi = 0;
  Int32 compare_size = 1;
  Int32 ret = 0;
  comp_buf *b = (comp_buf *)ebuf;



  if (flags & COMP_COMPARE_BYTE)  {
    compare_size = 1;
  }
  else if (flags & COMP_COMPARE_SHORT)  {
    compare_size = 2;
  }


  b->u.hdr.compressed_len = 0;
  b->u.hdr.flags = flags;

  b->u.hdr.eye_catcher1 = EYE_CATCHER_1;
  b->u.hdr.eye_catcher2 = EYE_CATCHER_2;
  b->u.hdr.exploded_len = dlen;
  b->u.hdr.checksum = 0;


  switch(compare_size) {
  case 1:
    if (flags & COMP_ALGO_RLE_PACKBITS) {
      ret = pb_encode_byte(dbuf, dlen, ebuf + HDR_SIZE, elen);
    }
    else {
      ret = encode_byte(dbuf, dlen, ebuf + HDR_SIZE, elen);
    }

    break;
  case 2:
    if (flags & COMP_ALGO_RLE_PACKBITS) {
      ret = pb_encode_short((unsigned short *)dbuf, dlen, (unsigned short *)(ebuf + HDR_SIZE), elen);
    }
    else {
      ret = encode_short((unsigned short *)dbuf, dlen, (unsigned short *)(ebuf + HDR_SIZE), elen);
    }
    break;
  default:
    ret = -1;
    return ret;
  }

  b->u.hdr.compressed_len = *elen + HDR_SIZE - sizeof(Int32);

  *elen += HDR_SIZE;

  /* Compute checksum assuming the checksum field is currently 0 */
  if (flags & COMP_CHECKSUM_VALID) {
    for (j = 0; j <  (b->u.hdr.compressed_len + sizeof(Int32)); ++j) {
      cs_lo += ebuf[j];
      cs_lo %= ADLER32_MODULUS;
      cs_hi += cs_lo;
      cs_hi %= ADLER32_MODULUS;
    }

    b->u.hdr.checksum = (cs_hi << 16) | cs_lo;
  }

  return ret;
}

/**
 *  Encodes using RLE encoding the input buffer (clear or decoded
 *     buffer) dbuf of length dlen bytes to encoded buffer ebuf and 
 *     length elen bytes
 *     Note that ebuf should be atleast as large as the dbuf.
 *     dbuf - decoded buffer (clear buffer)
 *     dlen - buffer length to encode
 *     ebuf - encoded buffer
 *     elen - pointer to store encoded buffer len
 */
Int32
encode_byte(unsigned char *dbuf, Int32 dlen, unsigned char *ebuf, Int32 *elen) {
  Int32 c = EOFCHAR, p = EOFCHAR;
  Int32 i = 0, j = 0;
  Int32 cnt = 0;

  while ( i < dlen) {
    /* Get the next byte */
    c = dbuf[i++];
    /* write it to the encoded buffer */
    ebuf[j++] = c;
    cnt = 0;
    /* Check for repetition */
    if ( c == p) {
      while (i < dlen) {
	if ((c = dbuf[i++]) == p) {
	  /* if next char is the same as previous char */
	  cnt++;
	  if (cnt == MAXCOUNT) {
	    /*
	     * If we hit a count of UCHAR_MAX (representable in a byte)
	     * repetitions, and force new pair of char,count
	     */
	    ebuf[j++] = cnt;
	    /* Force new char count */
	    p = EOFCHAR;
	    break;
	  }
	}
	else {
	  /*
	   * The current run of the char ended.  Write out the count and start
	   * for next
	   */
	  ebuf[j++] = cnt;
	  ebuf[j++] = c;
	  /*
	   * Reset prev to current
	   */
	  p = c;
	  break;
	}
      }
      if (i == dlen) {
	/*
	 * run was stopped because we exhausted the buffer
	 */
	c = EOFCHAR;
      }
    }
    else {
      /*
       * Reset prev to current char
       */
      p = c;
    }
    /*
     * If we hit the end of buffer, write out the end count
     */
    if (c == EOFCHAR) {
      ebuf[j++] = cnt;
    }
  }
  /*
   * Return encoded buffer length
   */
  *elen = j;

  return 0;
}



/**
 *  Encodes using modified RLE encoding the input buffer (clear or decoded buffer) dbuf
 *     of length dlen bytes to encoded buffer ebuf and length elen bytes
 *     Note that ebuf should be atleast as large as the dbuf.
 *     dbuf - decoded buffer (clear buffer)
 *     dlen - buffer length to encode
 *     ebuf - encoded buffer
 *     elen - pointer to store encoded buffer len
 *
 */
Int32
pb_encode_byte(unsigned char *dbuf, Int32 dlen, unsigned char *ebuf, Int32 *elen) {
  Int32 c = EOFCHAR, n = EOFCHAR;
  Int32 i = 0, j = 0;
  Int32 cnt = 0;
  Int32 k = 0;
  Int32 t = 0;
  unsigned char match_buf[RLE_PACKBITS_MAXBUF_BYTE]; 


  cnt = 0;

  while (i < dlen) {
    c = dbuf[i++];
    /* Get the next char */
    match_buf[cnt] = c;

    ++cnt;

    if (cnt >= RLE_PACKBITS_MINRUN) {

      /* check for run */
      for (k = 2; k <= RLE_PACKBITS_MINRUN; k++) {
	if (c != match_buf[cnt - k]){
	  /* no run */
	  k = 0;
	  break;
	}
      }
      if (k != 0) {
	/* we have a run - let us write current ouput buffer */
	if (cnt > RLE_PACKBITS_MINRUN) {
	  ebuf[j++] = (unsigned char)(cnt - RLE_PACKBITS_MINRUN - 1);
	  for (t = 0; t < (cnt - RLE_PACKBITS_MINRUN); t++) {
	    ebuf[j++] = match_buf[t];
	  }
	}

	/* Get run length */
	cnt = RLE_PACKBITS_MINRUN;
	while (i < dlen && ((n = dbuf[i++]) == c)) {
	  cnt++;
	  if (cnt == RLE_PACKBITS_MAXRUN_BYTE) {
	    /* run is at max length */
	    break;
	  }
	}
	ebuf[j++] = (unsigned char)(RLE_PACKBITS_MINRUN - 1 - cnt);
	ebuf[j++] = (unsigned char)c;

	if ((i < dlen) && (cnt != RLE_PACKBITS_MAXRUN_BYTE)) {
	  /* make run breaker start of next buffer */
	  match_buf[0] = n;
	  cnt = 1;
	}
	else {
	  /* end of buffer or max run hit */
	  cnt = 0;
	}
      }
    }

    if (cnt == RLE_PACKBITS_MAXBUF_BYTE) {
      ebuf[j++] = RLE_PACKBITS_MAXBUF_BYTE - 1;
      for (k = 0; k < RLE_PACKBITS_MAXCOPY_BYTE; k++) {
	ebuf[j++] = match_buf[k];
      }

      /* start a new buffer */

      cnt = RLE_PACKBITS_MAXBUF_BYTE - RLE_PACKBITS_MAXCOPY_BYTE;


      /* copy excess to front of buffer */
      for (k = 0; k < cnt; k++) {
	match_buf[k] = match_buf[RLE_PACKBITS_MAXCOPY_BYTE + k];
      }
    }
  }

  /* write out the last match buffer */

  if (cnt != 0 ) {
    if (cnt <= RLE_PACKBITS_MAXCOPY_BYTE) {
      /* write the entire buffer out */
      ebuf[j++] = cnt - 1;
      for (k = 0; k < cnt; k++) {
	ebuf[j++] = match_buf[k];
      }
    }
    else {
      /* we read more - write out the copy buffer */
      ebuf[j++] = RLE_PACKBITS_MAXCOPY_BYTE - 1;
      for (k = 0; k < RLE_PACKBITS_MAXCOPY_BYTE; k++) {
	ebuf[j++] = match_buf[k];
      }
      /* and the rest */
      cnt -= RLE_PACKBITS_MAXCOPY_BYTE;
      ebuf[j++] = cnt - 1;
      for (k = 0; k < cnt; k++) {
	ebuf[j++] = match_buf[k + RLE_PACKBITS_MAXCOPY_BYTE];
      }
    }
  }
  *elen = j;
  return 0;
}

/**
 *  Encodes using RLE encoding the input buffer (clear or decoded
 *     buffer) dbuf of length dlen shorts to encoded buffer ebuf and 
 *     length elen shorts
 *     Note that ebuf should be atleast as large as the dbuf.
 *     dbuf - decoded buffer (clear buffer)
 *     dlen - buffer length to encode in chars
 *     ebuf - encoded buffer
 *     elen - pointer to store encoded buffer len
 */
Int32
encode_short(unsigned short *dbuf, Int32 dlen_bytes, unsigned short *ebuf, Int32 *elen_bytes) {

  Int32 c = EOFCHAR, p = EOFCHAR;
  Int32 i = 0, j = 0;
  Int32 cnt = 0;
  Int32 dlen = dlen_bytes/2;

  while ( i < dlen) {
    /* Get the next short */
    c = dbuf[i++];
    /* write it to the encoded buffer */
    ebuf[j++] = c;
    cnt = 0;
    /* Check for repetition */
    if ( c == p) {
      while (i < dlen) {
	if ((c = dbuf[i++]) == p) {
	  /* if next char is the same as previous char */
	  cnt++;
	  if (cnt == MAXUCOUNT) {
	    /*
	     * If we hit a count of UCHAR_MAX (representable in a byte)
	     * repetitions, and force new pair of char,count
	     */
	    ebuf[j++] = cnt;
	    /* Force new char count */
	    p = EOFCHAR;
	    break;
	  }
	}
	else {
	  /*
	   * The current run of the char ended.  Write out the count and start
	   * for next
	   */
	  ebuf[j++] = cnt;
	  ebuf[j++] = c;
	  /*
	   * Reset prev to current
	   */
	  p = c;
	  break;
	}
      }
      if (i == dlen) {
	/*
	 * run was stopped because we exhausted the buffer
	 */
	c = EOFCHAR;
      }
    }
    else {
      /*
       * Reset prev to current char
       */
      p = c;
    }
    /*
     * If we hit the end of buffer, write out the end count
     */
    if (c == EOFCHAR) {
      ebuf[j++] = cnt;
    }
  }
  /*
   * Return encoded buffer length
   */
  *elen_bytes = j * sizeof(short);

  return 0;
}

Int32
pb_encode_short(unsigned short *dbuf, Int32 dlen_bytes, unsigned short *ebuf, Int32 *elen_bytes) {
  Int32 c = EOFCHAR, n = EOFCHAR;
  Int32 i = 0, j = 0;
  Int32 cnt = 0;
  Int32 k = 0;
  Int32 t = 0;
  Int32 dlen = dlen_bytes/sizeof(short);

  unsigned short match_buf[RLE_PACKBITS_MAXBUF_SHORT]; 

  cnt = 0;

  while (i < dlen) {
    c = dbuf[i++];
    /* Get the next char */
    match_buf[cnt] = c;

    ++cnt;

    if (cnt >= RLE_PACKBITS_MINRUN) {

      /* check for run */
      for (k = 2; k <= RLE_PACKBITS_MINRUN; k++) {
	if (c != match_buf[cnt - k]){
	  /* no run */
	  k = 0;
	  break;
	}
      }
      if (k != 0) {
	/* we have a run - let us write current ouput buffer */
	if (cnt > RLE_PACKBITS_MINRUN) {
	  ebuf[j++] = (unsigned short)(cnt - RLE_PACKBITS_MINRUN - 1);
	  for (t = 0; t < (cnt - RLE_PACKBITS_MINRUN); t++) {
	    ebuf[j++] = match_buf[t];
	  }
	}

	/* Get run length */
	cnt = RLE_PACKBITS_MINRUN;
	while (i < dlen && ((n = dbuf[i++]) == c)) {
	  cnt++;
	  if (cnt == RLE_PACKBITS_MAXRUN_SHORT) {
	    /* run is at max length */
	    break;
	  }
	}
	ebuf[j++] = (unsigned short)(RLE_PACKBITS_MINRUN - 1 - cnt);
	ebuf[j++] = (unsigned short)c;

	if ((i < dlen) && (cnt != RLE_PACKBITS_MAXRUN_SHORT)) {
	  /* make run breaker start of next buffer */
	  match_buf[0] = n;
	  cnt = 1;
	}
	else {
	  /* end of buffer or max run hit */
	  cnt = 0;
	}
      }
    }

    if (cnt == RLE_PACKBITS_MAXBUF_SHORT) {
      ebuf[j++] = RLE_PACKBITS_MAXBUF_SHORT - 1;
      for (k = 0; k < RLE_PACKBITS_MAXCOPY_SHORT; k++) {
	ebuf[j++] = match_buf[k];
      }

      /* start a new buffer */

      cnt = RLE_PACKBITS_MAXBUF_SHORT - RLE_PACKBITS_MAXCOPY_SHORT;


      /* copy excess to front of buffer */
      for (k = 0; k < cnt; k++) {
	match_buf[k] = match_buf[RLE_PACKBITS_MAXCOPY_SHORT + k];
      }
    }
  }

  /* write out the last match buffer */

  if (cnt != 0 ) {
    if (cnt <= RLE_PACKBITS_MAXCOPY_SHORT) {
      /* write the entire buffer out */
      ebuf[j++] = cnt - 1;
      for (k = 0; k < cnt; k++) {
	ebuf[j++] = match_buf[k];
      }
    }
    else {
      /* we read more - write out the copy buffer */
      ebuf[j++] = RLE_PACKBITS_MAXCOPY_SHORT - 1;
      for (k = 0; k < RLE_PACKBITS_MAXCOPY_SHORT; k++) {
	ebuf[j++] = match_buf[k];
      }
      /* and the rest */
      cnt -= RLE_PACKBITS_MAXCOPY_SHORT;
      ebuf[j++] = cnt - 1;
      for (k = 0; k < cnt; k++) {
	ebuf[j++] = match_buf[k + RLE_PACKBITS_MAXCOPY_SHORT];
      }
    }
  }
  /*
   * Return encoded buffer length
   */
  *elen_bytes = j * sizeof(short);

  return 0;
}

Int32
ExDecode(unsigned char *ebuf, Int32 elen, unsigned char *dbuf, Int32 *dlen,
	 Lng32 &param1, Lng32 &param2) {
  comp_buf *b = (comp_buf *)ebuf;
  Int32 saved_checksum = b->u.hdr.checksum;
  Int32 checksum = 0;
  Int32 flags = b->u.hdr.flags;
  UInt32 j = 0;
  UInt32 cs_lo = 1, cs_hi = 0;
  Int32 compare_size = 1;
  Int32 ret = 0;

  param1 = 0;
  param2 = 0;

  /* check eye catcher */
  if (b->u.hdr.eye_catcher1 != EYE_CATCHER_1 || b->u.hdr.eye_catcher2 != EYE_CATCHER_2) {
    if (b->u.hdr.eye_catcher1 != EYE_CATCHER_1)
      {
	param1 = b->u.hdr.eye_catcher1;
      }
    else
      {
	param1 = b->u.hdr.eye_catcher2;
      }
      
    return -2;
  }

  if (flags & COMP_COMPARE_BYTE)  {
    compare_size = 1;
  }
  else if (flags & COMP_COMPARE_SHORT)  {
    compare_size = 2;
  }


  /* check length.  elen should be ebuf->compressed_len + 4 */


  if (flags & COMP_CHECKSUM_VALID) {
    /* check the checksum */
    b->u.hdr.checksum = 0;

    for (j = 0; j <  (b->u.hdr.compressed_len + sizeof(Int32)); ++j) {
      cs_lo += ebuf[j];
      cs_lo %= ADLER32_MODULUS;
      cs_hi += cs_lo;
      cs_hi %= ADLER32_MODULUS;
    }


    b->u.hdr.checksum = saved_checksum;

    checksum = (cs_hi << 16) | cs_lo;

    if (checksum != saved_checksum) {
      //      fprintf(stderr, "computed checksum = %d\n real checksum = %d\n", checksum, saved_checksum);
      param1 = checksum;
      param2 = saved_checksum;
      return -4;
    }
  } 

  /*
   *  OK all clear - do the decoded now based on flags
   */
  switch(compare_size) {
  case 1:
    if (flags & COMP_ALGO_RLE_PACKBITS) {
      ret = pb_decode_byte(ebuf + HDR_SIZE, elen - HDR_SIZE, dbuf, dlen);
    }
    else {
      ret = decode_byte(ebuf + HDR_SIZE, elen - HDR_SIZE, dbuf, dlen);
    }

    break;
  case 2:
    if (flags & COMP_ALGO_RLE_PACKBITS) {
      ret = pb_decode_short((unsigned short *)(ebuf + HDR_SIZE), elen - HDR_SIZE, (unsigned short *)dbuf, dlen);
    }
    else {
      ret = decode_short((unsigned short *)(ebuf + HDR_SIZE), elen - HDR_SIZE, (unsigned short *)dbuf, dlen);
    }
    break;
  default:
    ret = -1;
    /* Compute checksum assuming the checksum field is currently 0 */
  }
  return ret;
}



/**
 *  Decodes using RLE encoding the input buffer (encoded buffer) ebuf
 *     of length elen bytes to decoded buffer dbuf and length dlen bytes
 *     Note that dbuf should be atleast as large as the ebuf but potentially
 *     much bigger.  The assumption is dbuf is big enough to hold
 *     the decoded buffer
 *     ebuf - encoded buffer
 *     elen - pointer to store encoded buffer len
 *     dbuf - decoded buffer (clear buffer)
 *     dlen - buffer length to encode
 *       
 */
Int32
decode_byte(unsigned char *ebuf, Int32 elen, unsigned char *dbuf, Int32 *dlen) {
  Int32 c = EOFCHAR, p = EOFCHAR;
  Int32 i = 0, j = 0, k = 0;
  Int32 cnt = 0;

  /*
   * We need to get the checksum and validate the buffer.  If valid, we will
   * call the corresponding decode function 
   */

  while (i < elen) {
    c = ebuf[i++];
    dbuf[j++] = c;
    /*
     * If current char equals prev char
     * print the char and the count following it
     */
    if (c == p) {
      cnt = ebuf[i++];
      for(k = 0; k < cnt; ++k) {
	dbuf[j++] = c;
      }
      /*
       * Reset prev char to force new char to be written
       */
      p = EOFCHAR;
    }
    else {
      /*
       * Set current char as previous
       */
      p = c;
    }
  }
  /*
   * Return decoded length
   */
  *dlen = j;
  return 0;
}


Int32
pb_decode_byte(unsigned char *ebuf, Int32 elen, unsigned char *dbuf, Int32 *dlen) {
  Int32 c = EOFCHAR, p = EOFCHAR;
  Int32 i = 0, j = 0, k = 0;
  Int32 cnt = 0;

  /*
   * We need to get the checksum and validate the buffer.  If valid, we will
   * call the corresponding decode function 
   */

  while (i < elen) {
    cnt = (char)ebuf[i++];
    if (cnt < 0) {
      cnt = (RLE_PACKBITS_MINRUN - 1) - cnt;

      if (i < elen) {
	c = ebuf[i++];
	for (k = 0; k < cnt; ++k) {
	  dbuf[j++] = c;
	}
      }
      else {
	cnt = 0;
      }
    }
    else {
      for (cnt++; cnt > 0; --cnt) {
	if (i < elen) {
	  dbuf[j++] = ebuf[i++];
	}
      }
    }
  }
  /*
   * Return decoded length
   */
  *dlen = j;
  return 0;
}

/**
 *  Decodes using RLE encoding the input buffer (encoded buffer) ebuf
 *     of length elen chars to decoded buffer dbuf and length dlen chars
 *     Note that dbuf should be atleast as large as the ebuf but potentially
 *     much bigger.  The assumption is dbuf is big enough to hold
 *     the decoded buffer
 *     ebuf - encoded buffer
 *     elen - pointer to store encoded buffer len
 *     dbuf - decoded buffer (clear buffer)
 *     dlen - buffer length to encode
 *       
 */
Int32
decode_short(unsigned short *ebuf, Int32 elen_bytes, unsigned short *dbuf, Int32 *dlen_bytes) {
  Int32 c = EOFCHAR, p = EOFCHAR;
  Int32 i = 0, j = 0, k = 0;
  Int32 cnt = 0;
  Int32 elen = elen_bytes/sizeof(short);

  while (i < elen) {
    c = ebuf[i++];
    dbuf[j++] = c;
    /*
     * If current char equals prev char
     * print the char and the count following it
     */
    if (c == p) {
      cnt = ebuf[i++];
      for(k = 0; k < cnt; ++k) {
	dbuf[j++] = c;
      }
      /*
       * Reset prev char to force new char to be written
       */
      p = EOFCHAR;
    }
    else {
      /*
       * Set current char as previous
       */
      p = c;
    }
  }
  /*
   * Return decoded length
   */
  *dlen_bytes = j * sizeof(short);
  return 0;
}

Int32
pb_decode_short(unsigned short *ebuf, Int32 elen_bytes, unsigned short *dbuf, Int32 *dlen_bytes) {
  Int32 c = EOFCHAR, p = EOFCHAR;
  Int32 i = 0, j = 0, k = 0;
  Int32 cnt = 0;
  Int32 elen = elen_bytes/sizeof(short);
  /*
   * We need to get the checksum and validate the buffer.  If valid, we will
   * call the corresponding decode function 
   */

  while (i < elen) {
    cnt = (short)ebuf[i++];
    if (cnt < 0) {
      cnt = (RLE_PACKBITS_MINRUN - 1) - cnt;

      if (i < elen) {
	c = ebuf[i++];
	for (k = 0; k < cnt; ++k) {
	  dbuf[j++] = c;
	}
      }
      else {
	cnt = 0;
      }
    }
    else {
      for (cnt++; cnt > 0; --cnt) {
	if (i < elen) {
	  dbuf[j++] = ebuf[i++];
	}
      }
    }
  }
  /*
   * Return decoded length
   */
  *dlen_bytes = j * sizeof(short);
  return 0;
}


