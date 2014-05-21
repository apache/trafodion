// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef _MAL_UTF8_
#define _MAL_UTF8_

#include <stdint.h>
#include <string.h>

using namespace std; 

namespace Trinity 
{
    enum { MAX_BYTES_PER_UTF8_CHARACTER = 1};
    //! Determines if a UTF-8 character has been truncated at the end of a buffer,
    //! and if so returns the length of the truncated character. As an optimization,
    //! we expect the caller already knows the string length so we do not look for
    //! a null terminator ourselves.
    //!
    //! \param[in]  buffer              The buffer to be checked
    //! \param[in]  bufferLength        The number of bytes in the buffer; checking starts at the last byte
    //!
    //! \return     0 if no truncated character is found; the number of bytes in
    //!             the truncated character otherwise (that is, if a 3-byte character
    //!             was truncated to 2 bytes, we return 2).
    int32_t truncatedUTF8CharLength(const char * buffer, int32_t bufferLength);

    //! Checks a UTF-8 string for invalid characters (most likely some subsystem has mistakenly
    //! stuck SJIS or UCS2 characters in there instead). If any are found, assume the string
    //! is not UTF-8, and replace any bytes that are not ASCII with ?'s.
    //!
    //! \param[in]  buffer              The buffer to be checked; assumed to be a null-terminated string
    void scrubUTF8String(char * buffer);
    
    
    int32_t getUTF8byteLength(const char * buffer, int32_t numOfChar);
}

#endif  // _MAL_UTF8_
