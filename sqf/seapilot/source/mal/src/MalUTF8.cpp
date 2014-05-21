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

#include "MalUTF8.h"

using namespace Trinity;


//given the buffer contains string in UTF-8 and the number of Character
//return the length in bytes that make up the first numOfChar charaters in the buffer
int32_t Trinity::getUTF8byteLength(const char * buffer, int32_t numOfChar)
{
    int32_t rc = 0;  // assume no truncation found
     int32_t i =0, bufferLen = strlen(buffer);    
    if(numOfChar>0)
    {
       
        for(i=0; i < bufferLen ; i++)
        {
            if( rc == numOfChar )
                break;
            if((buffer[i] & 0x80 ) == 0x00)  // 1-byte characters
            {
                rc++;
            }
            else if((buffer[i] & 0xE0 ) == 0xC0)  // 2-byte characters
            {
                rc++; i++;
            }
            else if((buffer[i] & 0xF0 ) == 0xE0)  // 3-byte characters
            {
                rc++; i+=2;
            }
            else if((buffer[i] & 0xF8 ) == 0xF0)  // 4-byte characters
            {
                rc++; i+=3;
            }
            else if((buffer[i] & 0xFC ) == 0xF8)  // 5-byte characters
            {
                rc++; i+=4;
            }
            else if((buffer[i] & 0xFE ) == 0xFC)  // 6-byte characters
            {
                rc++; i+=5;
            }
            else
            {
                //invalid char detected, return current, interrupt?
                return bufferLen;
            }
        }
    }
    return i;
}

int32_t Trinity::truncatedUTF8CharLength(const char * buffer, int32_t bufferLength)
{
    int32_t rc = 0;  // assume no truncation found

    if (bufferLength > 0)  // logic is simpler below if we are assured string is non-empty 
    {
        // Some notes about UTF-8 characters:
        //
        // 1-byte characters are of the form 0x00 - 0x7f, that is, bit 0 is zero
        // 2-byte characters are of the form binary 110xxxxx 10xxxxxx
        // 3-byte characters are of the form binary 1110xxxx 10xxxxxx 10xxxxxx
        // 4-byte characters are of the form binary 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        // 5-byte characters are of the form binary 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        // 6-byte characters are of the form binary 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        //
        // (source: Wikipedia article on UTF-8.)
        //
        // That is, except for single-byte characters, the number of leading 1-bits determines the
        // character length (no leading 1-bits implying a single-byte character). Also, a byte that
        // begins with binary 10 is never the first byte of any character. So, we can look for such
        // bytes at the end of a string and back up until we find the first byte that doesn't begin
        // with binary 10.
        //
        // Note that some bit combinations and some sequences of bytes are invalid, if we detect
        // such things we will pretend there is no truncation which has the effect of letting the
        // caller's downstream logic deal with it.

        int32_t i = bufferLength-1;
        while ((i > 0) && ((buffer[i] & 0xC0) == 0x80))
        {
            i--;
        }

        // we have either backed up to the first byte of the last UTF-8 character, or
        // perhaps we have a character that lacks a first byte!

        if ((buffer[i] & 0xC0) == 0x80)
        {
            // The string starts in the middle of a character! To avoid our caller having some
            // infinite looping problem, just treat this as if no truncation occurred and let
            // downstream processing deal with it.
            rc = 0;
        }
        else if ((buffer[i] & 0x80) == 0)
        {
            // The last character is single-byte so there is no truncation
            rc = 0;
        }
        else 
        {
            // compute how many bytes actually present in the last character
            rc = bufferLength - i;  // and assume for the moment it is truncated

            // compute expected length of last character
            int32_t lastCharacterLength = 0;
            if ((buffer[i] & 0xE0) == 0xC0)
            {
                lastCharacterLength = 2;  // binary 110xxxxx, so a 2-byte character
            }
            else if ((buffer[i] & 0xF0) == 0xE0)
            {
                lastCharacterLength = 3;  // binary 1110xxxx, so a 3-byte character
            }
            else if ((buffer[i] & 0xF8) == 0xF0)
            {
                lastCharacterLength = 4;  // binary 11110xxx, so a 4-byte character
            }
            else if ((buffer[i] & 0xFC) == 0xF8)
            { 
                lastCharacterLength = 5;  // binary 111110xx, so a 5-byte character
            }
            else if ((buffer[i] & 0xFE) == 0xFC)
            {
                lastCharacterLength = 6;  // binary 1111110x, so a 6-byte character
            }
            // else it is an invalid UTF-8 character

            if (lastCharacterLength == 0)  // if invalid
            {
                rc = 0;  // just say no truncation; let downstream processing handle it
            }
            else if (lastCharacterLength <= rc)
            {
                // if lastCharacterLength == rc, there is no truncation; the buffer ends
                // exactly at the end of the last character; if lastCharacterLength < rc,
                // we have a bogus UTF-8 string, and we will pretend there is no truncation
                // and let downstream processing handle it
                rc = 0;
            }
            // else we have truncation, and rc already has the number of bytes present
        }
    }
    // else string is empty and there is no truncation

    return rc;
}

void Trinity::scrubUTF8String(char * buffer)
{
    // Check to see if the string is a valid UTF-8 string. If it is not,
    // replace any characters that are not valid ASCII with ?'s. (We do this
    // because it is likely that the string has SJIS or UCS2 characters in
    // it instead.)

    // see the previous method for a description of valid UTF-8 characters

    bool good = true;  // assume everything is good
    char * next = buffer;

    while ((*next) && (good))
    {
        int32_t expectedUTFCharLength = 0;

        if (*next <= 0x7f)
        {
            // single byte ASCII character, which is fine
            next++;
        }
        else if ((*next & 0xC0) == 0x80)
        {
            // We have a byte that can only occur in the middle or end of a UTF-8
            // character, but we are not in a UTF-8 character; this is invalid.
            // (What is probably true is we are seeing a UCS2 or SJIS character.)
            good = false;
        }
        else if ((*next & 0xE0) == 0xC0)
        {
            expectedUTFCharLength = 2;  // binary 110xxxxx, so a 2-byte character
        }
        else if ((*next & 0xF0) == 0xE0)
        {
            expectedUTFCharLength = 3;  // binary 1110xxxx, so a 3-byte character
        }
        else if ((*next & 0xF8) == 0xF0)
        {
            expectedUTFCharLength = 4;  // binary 11110xxx, so a 4-byte character
        }
        else if ((*next & 0xFC) == 0xF8)
        { 
            expectedUTFCharLength = 5;  // binary 111110xx, so a 5-byte character
        }
        else if ((*next & 0xFE) == 0xFC)
        {
            expectedUTFCharLength = 6;  // binary 1111110x, so a 6-byte character
        }
        else  // it is an invalid UTF-8 character
        {
            good = false;
        }

        // if we saw the first byte of what should be a multi-byte UTF-8 character,
        // check its middle and ending bytes (note in that case we have not yet
        // incremented next; next still points to first byte of the character)

        if (expectedUTFCharLength > 0)  // if we found first byte of multi-byte UTF-8 char
        {
            for (int32_t i = 1; good && (i < expectedUTFCharLength); i++)
            {
                if ((next[i] & 0xC0) != 0x80)
                {
                    good = false;  // expected a character of binary form 10xxxxxx
                }
            }

            if (good)  
            {
                next += expectedUTFCharLength;  // step to next UTF-8 character
            }
        }
    }

    if (!good)
    {
        // we found a bad character; assume the string is not
        // UTF-8 and replace all bytes that are x80 or above with ?'s
        next = buffer;  // start from beginning again
        while (*next)
        {
            if (*next >= 0x80)
            {
                *next = '?';
            }
            next++;
        }
    }
}
