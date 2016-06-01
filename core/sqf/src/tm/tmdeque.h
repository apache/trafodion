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

#ifndef TMDEQUE_H_
#define TMDEQUE_H_

#include <deque>
#include "tmmutex.h"
#include "dtm/tm_util.h"
//
// General TM deque class to hold free lists
//

class TM_DEQUE
{
    public:
        TM_DEQUE(){}; 
        ~TM_DEQUE(){};

        void clear();
        bool empty();
        void push(void *pp_data);
        void push_back(void *pp_data);
        void * pop();
        void * pop_end();
        int64 size();

        // methods to walk the queue FIFO
        void * get_firstFIFO();
        void * get_nextFIFO();
        void remove(void * pp_data);
        void erase();

        void lock();
        void unlock();
    
    private:
        std::deque<void*> iv_deque;
        std::deque<void*>::iterator iv_iterator;
        TM_Mutex          iv_mutex;
};

#endif // TMDEQUE_H_
