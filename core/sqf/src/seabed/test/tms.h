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

#ifndef __TMS_H_
#define __TMS_H_

typedef struct {
    union {
        short              s[6];
        struct {
            unsigned int   ctrl_size;
            unsigned int   data_size;
            unsigned int   errm;
        } t;
    } u;
} RT; // result type

enum {
    RT_MS_ERR      = 0x1,  // .<09> = err set by msgsys or net
    RT_DATA_RCVD   = 0x2,  // .<10> = reply data may hv bn rcvd
    RT_UPDATE_DEST = 0x4,  // .<11> = phandle has been updated
    RT_COUNT_IT    = 0x8,  // .<12> = should be counted
    RT_STARTED     = 0x10, // .<13> = may have been acted on
    RT_RETRYABLE   = 0x20, // .<14> = retryable path error
    RT_ERROR       = 0x40
};

#endif // !__TMS_H_
