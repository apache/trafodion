// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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

#include "wrapper/amqpwrapper.h"
#include "wrapper.test.pb.h"

// will explicitly create a connection, then destroy it for further tests
int test1()
{
    int error = 0;
    error = createAMQPConnection();
    if (error)
       return error;

    wrapper::test event;
    event.set_data(2);
    AMQPRoutingKey routingKey(SP_EVENT, "wrapper", SP_INSTANCE, SP_PUBLIC,"test");
    error = sendAMQPMessage( event.SerializeAsString(), SP_CONTENT_TYPE_APP, routingKey);
    if (error)
        return error;

    error = closeAMQPConnection();
    return error;
}

// no open connections
int test2()
{   
    int error = 0;
    wrapper::test event;
    event.set_data(1);
    AMQPRoutingKey routingKey(SP_EVENT, "wrapper", SP_INSTANCE, SP_PUBLIC,"test");
    error = sendAMQPMessage( event.SerializeAsString(), SP_CONTENT_TYPE_APP, routingKey);

   return error;
}
int main()
{
    int error = 0;
    int error_count ;

    sleep(10);
    error = test1();
    if (error != 0)
    { 
        printf("\n Test1 failed with last error of %d\n", error);
        error_count++;
    }

    error = test2();
    if (error != 0)
    {
        printf("\n Test2 failed with last error of %d\n", error);
        error_count++;
    }
    return 0;
}
