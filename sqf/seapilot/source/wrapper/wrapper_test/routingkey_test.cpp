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

#include <iostream>
#include <string>


#include "../../../export/include/wrapper/routingkey.h"

using namespace std;

  // sets up a basic routing key
int test1()
{

  int error = 0;
  string testString = "";

  //********************
  // Set up routing key
  //********************
  AMQPRoutingKey routingKey (SP_EVENT, SP_UNCPACKAGE, SP_INSTANCE, 
			     SP_PUBLIC, SP_GPBPROTOCOL, "test");


  //********************
  // Validate Category
  //********************
  if ( (routingKey.GetCategory()) != SP_EVENT ) {

    cout << "TEST1:  Invalid Category Token \n";
    error = 1;
  }

  if ( (testString = routingKey.GetCategoryString()) != "event") {
    cout << "TEST1:  Invalid Category string returned \n";
    error = 1;
  }


  //********************
  // Validate Package
  //********************
  if ( (routingKey.GetPackage()) != SP_UNCPACKAGE ) {

    cout << "TEST1:  Invalid Package Token \n";
    error = 1;
  }


  if ( (testString = routingKey.GetPackageString()) != "unc") {
    cout << "TEST1:  Invalid package string returned \n";
    error = 1;
  }

  //********************
  // Validate Instance
  //********************
  if ( (routingKey.GetScope()) != SP_INSTANCE ) {

    cout << "TEST1:  Invalid Scope Token \n";
    error = 1;
  }


  if ( (testString = routingKey.GetScopeString()) != "instance") {
    cout << "TEST1:  Invalid scope string returned \n";
    error = 1;
  }

  //********************
  // Validate Security
  //********************
  if ( (routingKey.GetSecurity()) != SP_PUBLIC ) {

    cout << "TEST1:  Invalid Security Token \n";
    error = 1;
  }


  if ( (testString = routingKey.GetSecurityString()) != "public") {
    cout << "TEST1:  Invalid security string returned \n";
    error = 1;
  }


  //********************
  // Validate Protocol
  //********************
  if ( (routingKey.GetProtocol()) != SP_GPBPROTOCOL ) {

    cout << "TEST1:  Invalid Protocol Token \n";
    error = 1;
  }


  if ( (testString = routingKey.GetProtocolString()) != "gpb") {
    cout << "TEST1:  Invalid protocol string returned \n";
    error = 1;
  }


  //********************
  // Validate publicationname
  //********************

  if ( (testString = routingKey.GetPublicationName()) != "test") {
    cout << "TEST1:  Invalid publication string returned \n";
    error = 1;
  }

  return error;

}

int test2() {

  string myRoutingKey = "event.unc.instance.public.xml.events";
  string myProtofileName = "unc.events.proto";
  string myMessageName = "unc.events";

  int error = 0;

  AMQPRoutingKey wrapperRoutingKey (myRoutingKey);

  string protofilename = wrapperRoutingKey.GetAsProtofileName();

  if (protofilename != myProtofileName) {
    cout << "TEST2:  protofileName invalid \n";
    error = 1;
  }

  string messageName = wrapperRoutingKey.GetAsMessageName();
  if (messageName != myMessageName) {
    cout << "TEST2:  messageName invalid \n";
    error = 1;
  }

  return error;
}

/* test building subscription keys */
int test3() {

  string mySubscriptionKey = "";
  int error = 0;

  // Test #1 -- events.# like our topic brokers use

  mySubscriptionKey = AMQPRoutingKey_BuildSubscriptionRoutingKey
                                       (SP_EVENT,
					SP_NULLPACKAGE,
					SP_NULLSCOPE,
					SP_NULLSECURITY,
					SP_NULLPROTOCOL,
					"");

  if (mySubscriptionKey != "event.#") {
    cout << "TEST3: Category.# test returned mismatch \n";
    error = 1;
  }

  // Test #2 -- Packages!  We'll get all things ndcs

  mySubscriptionKey = AMQPRoutingKey_BuildSubscriptionRoutingKey
                                       (SP_NULLCATEGORY,
					SP_NDCSPACKAGE,
					SP_NULLSCOPE,
					SP_NULLSECURITY,
					SP_NULLPROTOCOL,
					"");

  if (mySubscriptionKey != "*.ndcs.#") {
    cout << "TEST3: *.package.# test returned mismatch \n";
    error = 1;
  }

  // Test #3 -- Skippadee Doo Dah!  We'll get Instance Scope and XML Protocol....

  mySubscriptionKey = AMQPRoutingKey_BuildSubscriptionRoutingKey
                                       (SP_NULLCATEGORY,
					SP_NULLPACKAGE,
					SP_INSTANCE,
					SP_NULLSECURITY,
					SP_XMLPROTOCOL,
					"");

  if (mySubscriptionKey != "#.instance.*.xml.*") {
    cout << "TEST3: #.instance.*.xml.* test returned mismatch \n";
    error = 1;
  }

  // Test #4 -- Fenceposts anyone?  Let's get first and last

  mySubscriptionKey = AMQPRoutingKey_BuildSubscriptionRoutingKey
                                       (SP_PERF_STAT,
					SP_NULLPACKAGE,
					SP_NULLSCOPE,
					SP_NULLSECURITY,
					SP_NULLPROTOCOL,
					"testmessage");

  if (mySubscriptionKey != "performance_stat.#.testmessage") {
    cout << "TEST3: category.#.publication test returned mismatch \n";
    error = 1;
  }

  // Test # -- Gimme da ball! 

  mySubscriptionKey = AMQPRoutingKey_BuildSubscriptionRoutingKey
                                       (SP_NULLCATEGORY,
					SP_NULLPACKAGE,
					SP_NULLSCOPE,
					SP_NULLSECURITY,
					SP_NULLPROTOCOL,
					"onespecificmessage");

  if (mySubscriptionKey != "#.onespecificmessage") {
    cout << "TEST3: #.publication-only test returned mismatch \n";
    error = 1;
  }


  return error;

}

int main()
{
    int error = 0;
    int error_count ;

    error = test1();
    if (error != 0)
    { 
        printf("\n Test1 failed with last error of %d\n", error);
        error_count++;
    }
    else
    { 
        printf("\n Test1 Passed!\n", error);
    }

    error = test2();
    if (error != 0)
    { 
        printf("\n Test2 failed with last error of %d\n", error);
        error_count++;
    }
    else
    { 
        printf("\n Test2 Passed!\n", error);
    }

    error = test3();
    if (error != 0)
    { 
        printf("\n Test3 failed with last error of %d\n", error);
        error_count++;
    }
    else
    { 
        printf("\n Test3 Passed!\n", error);
    }


    return 0;
}
