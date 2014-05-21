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
#include "../../../export/include/wrapper/externroutingkey.h"

using namespace std;

  // sets up a basic routing key
int test1()
{

  int error = 0;
  string testString = "dtm.event.private.mypublication.tenant.xml";

  SPExtRoutingKeyErr sperkerr = SPERKERR_NOERR;
  //********************
  // Set up routing key
  //********************

  AMQPRoutingKey localRoutingKey;

  sperkerr = ExtRoutingKey_ToInternal (testString, &localRoutingKey);

  if (sperkerr != SPERKERR_NOERR) {
    cout << "TEST1:  Error returned frome Internal to External! \n";
    error = 1;
  }

  //********************
  // Validate Category
  //********************
  if ( (localRoutingKey.GetCategory()) != SP_EVENT ) {

    cout << "TEST1:  Invalid Category Token \n";
    error = 1;
  }

  if ( (testString = localRoutingKey.GetCategoryString()) != "event") {
    cout << "TEST1:  Invalid Category string returned \n";
    error = 1;
  }


  //********************
  // Validate Package
  //********************
  if ( (localRoutingKey.GetPackage()) != SP_DTMPACKAGE ) {

    cout << "TEST1:  Invalid Package Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetPackageString()) != "dtm") {
    cout << "TEST1:  Invalid package string returned \n";
    error = 1;
  }

  //********************
  // Validate Instance
  //********************
  if ( (localRoutingKey.GetScope()) != SP_TENANT ) {

    cout << "TEST1:  Invalid Scope Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetScopeString()) != "tenant") {
    cout << "TEST1:  Invalid scope string returned \n";
    error = 1;
  }

  //********************
  // Validate Security
  //********************
  if ( (localRoutingKey.GetSecurity()) != SP_PRIVATE ) {

    cout << "TEST1:  Invalid Security Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetSecurityString()) != "private") {
    cout << "TEST1:  Invalid security string returned \n";
    error = 1;
  }


  //********************
  // Validate Protocol
  //********************
  if ( (localRoutingKey.GetProtocol()) != SP_XMLPROTOCOL ) {

    cout << "TEST1:  Invalid Protocol Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetProtocolString()) != "xml") {
    cout << "TEST1:  Invalid protocol string returned \n";
    error = 1;
  }


  //********************
  // Validate publicationname
  //********************

  if ( (testString = localRoutingKey.GetPublicationName()) != "mypublication") {
    cout << "TEST1:  Invalid publication string returned \n";
    error = 1;
  }

  return error;

}


// Routing key with some defaulted parts (scope/security)
int test2() {

  int error = 0;
  string testString = "dtm.event.mypublication.xml";

  SPExtRoutingKeyErr sperkerr = SPERKERR_NOERR;
  //********************
  // Set up routing key
  //********************

  AMQPRoutingKey localRoutingKey;

  sperkerr = ExtRoutingKey_ToInternal (testString, &localRoutingKey);

  if (sperkerr != SPERKERR_NOERR) {
    cout << "TEST2:  Error returned frome Internal to External! \n";
    error = 1;
  }

  //********************
  // Validate Category
  //********************
  if ( (localRoutingKey.GetCategory()) != SP_EVENT ) {

    cout << "TEST2:  Invalid Category Token \n";
    error = 1;
  }

  if ( (testString = localRoutingKey.GetCategoryString()) != "event") {
    cout << "TEST2:  Invalid Category string returned \n";
    error = 1;
  }


  //********************
  // Validate Package
  //********************
  if ( (localRoutingKey.GetPackage()) != SP_DTMPACKAGE ) {

    cout << "TEST2:  Invalid Package Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetPackageString()) != "dtm") {
    cout << "TEST2:  Invalid package string returned \n";
    error = 1;
  }

  //********************
  // Validate Instance
  //********************
  if ( (localRoutingKey.GetScope()) != SP_NULLSCOPE ) {

    cout << "TEST2:  Invalid Scope Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetScopeString()) != "nullscopetext") {
    cout << "TEST2:  Invalid scope string returned \n";
    error = 1;
  }

  //********************
  // Validate Security
  //********************
  if ( (localRoutingKey.GetSecurity()) != SP_NULLSECURITY ) {

    cout << "TEST2:  Invalid Security Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetSecurityString()) != "nullsecuritytext") {
    cout << "TEST2:  Invalid security string returned \n";
    error = 1;
  }


  //********************
  // Validate Protocol
  //********************
  if ( (localRoutingKey.GetProtocol()) != SP_XMLPROTOCOL ) {

    cout << "TEST2:  Invalid Protocol Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetProtocolString()) != "xml") {
    cout << "TEST2:  Invalid protocol string returned \n";
    error = 1;
  }


  //********************
  // Validate publicationname
  //********************

  if ( (testString = localRoutingKey.GetPublicationName()) != "mypublication") {
    cout << "TEST2:  Invalid publication string returned \n";
    error = 1;
  }

  //********************
  // Package and Pub name only test
  //********************

  testString = "se.mypublicationname";

  sperkerr = SPERKERR_NOERR;
  //********************
  // Set up routing key
  //********************

  sperkerr = ExtRoutingKey_ToInternal (testString, &localRoutingKey);

  if (sperkerr != SPERKERR_NOERR) {
    cout << "TEST2A:  Error returned frome Internal to External! \n";
    error = 1;
  }

  //********************
  // Validate Category
  //********************
  if ( (localRoutingKey.GetCategory()) != SP_NULLCATEGORY ) {

    cout << "TEST2A:  Invalid Category Token \n";
    error = 1;
  }

  if ( (testString = localRoutingKey.GetCategoryString()) != "nullcategorytext") {
    cout << "TEST2A:  Invalid Category string returned \n";
    error = 1;
  }


  //********************
  // Validate Package
  //********************
  if ( (localRoutingKey.GetPackage()) != SP_SEPACKAGE ) {

    cout << "TEST2A:  Invalid Package Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetPackageString()) != "se") {
    cout << "TEST2A:  Invalid package string returned \n";
    error = 1;
  }

  //********************
  // Validate Instance
  //********************
  if ( (localRoutingKey.GetScope()) != SP_NULLSCOPE ) {

    cout << "TEST2A:  Invalid Scope Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetScopeString()) != "nullscopetext") {
    cout << "TEST2A:  Invalid scope string returned \n";
    error = 1;
  }

  //********************
  // Validate Security
  //********************
  if ( (localRoutingKey.GetSecurity()) != SP_NULLSECURITY ) {

    cout << "TEST2A:  Invalid Security Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetSecurityString()) != "nullsecuritytext") {
    cout << "TEST2A:  Invalid security string returned \n";
    error = 1;
  }


  //********************
  // Validate Protocol
  //********************
  if ( (localRoutingKey.GetProtocol()) != SP_NULLPROTOCOL ) {

    cout << "TEST2A:  Invalid Protocol Token \n";
    error = 1;
  }


  if ( (testString = localRoutingKey.GetProtocolString()) != "nullprotocoltext") {
    cout << "TEST2A:  Invalid protocol string returned \n";
    error = 1;
  }


  //********************
  // Validate publicationname
  //********************

  if ( (testString = localRoutingKey.GetPublicationName()) != "mypublicationname") {
    cout << "TEST2A:  Invalid publication string returned \n";
    error = 1;
  }

  //*********************
  // Now let's incite a little mayhem:  get a few errors
  //*********************

  // First error is 2 non-standard strings in the sample routing key

  sperkerr = SPERKERR_NOERR;
  testString = "funky.dtm.chicken.instance.event.public";
  sperkerr = ExtRoutingKey_ToInternal (testString, &localRoutingKey);

  if (sperkerr != SPERKERR_TOOMANYFREESTRINGS) {
    cout << "TEST2B:  Returned unexpected error! \n";
    error = 1;
  }

  // Next comes a repeated component

  sperkerr = SPERKERR_NOERR;
  testString = "event.dtm.event.instance.event.public";
  sperkerr = ExtRoutingKey_ToInternal (testString, &localRoutingKey);

  if (sperkerr != SPERKERR_REPEATEDCOMPONENT) {
    cout << "TEST2B:  Returned unexpected error! \n";
    error = 1;
  }

  SPExtRoutingKeyErr sperkErr2 = SPERKERR_NOERR;
  sperkErr2 = ExtRoutingKey_ErrToText ( sperkerr, &testString);
  if (sperkErr2 != SPERKERR_NOERR) {
    cout << "TEST2B:  Err trans to text returned unexpected error! \n";
    error = 1;
  }				      

  if (testString != SPERKERR_REPEATEDCOMPONENT_TEXT) {
    cout << "TEST2B:  Err trans to text returned unexpected text! \n";
    error = 1;
  }				      

  return error;
}

/* test building subscription keys */
int test3() {

  string mySubscriptionKey = "";
  int error = 0;

  // Test #1 -- events.# like our topic brokers use

  SPExtRoutingKeyErr sperkerr = SPERKERR_NOERR;

  string testSubsString = "event";
  sperkerr = ExtRoutingKey_ToSubsKey (testSubsString, &mySubscriptionKey);
  if (sperkerr != SPERKERR_NOERR) {
    cout << "TEST3:  Category.# translation returned error \n";
    error = 1;
  }				      

  if (mySubscriptionKey != "event.#") {
    cout << "TEST3: Category.# test returned unexpected key\n";
    error = 1;
  }

  // Test #2 -- Packages!  We'll get all things ndcs


  sperkerr = SPERKERR_NOERR;
  testSubsString = "ndcs";
  sperkerr = ExtRoutingKey_ToSubsKey (testSubsString, &mySubscriptionKey);
  if (sperkerr != SPERKERR_NOERR) {
    cout << "TEST3:  *.ndcs.# translation returned error \n";
    error = 1;
  }				      
  if (mySubscriptionKey != "*.ndcs.#") {
    cout << "TEST3: *.package.# test returned mismatch \n";
    error = 1;
  }

  // Test #3 -- Skippadee Doo Dah!  We'll get Instance Scope and XML Protocol....

  sperkerr = SPERKERR_NOERR;
  testSubsString = "xml.instance";
  sperkerr = ExtRoutingKey_ToSubsKey (testSubsString, &mySubscriptionKey);
  if (sperkerr != SPERKERR_NOERR) {
    cout << "TEST3:  #.instance.*.xml.* translation returned error \n";
    error = 1;
  }				      

  if (mySubscriptionKey != "#.instance.*.xml.*") {
    cout << "TEST3: #.instance.*.xml.* test returned mismatch \n";
    error = 1;
  }

  // Test #4 -- Fenceposts anyone?  Let's get first and last

  sperkerr = SPERKERR_NOERR;
  testSubsString = "performance_stat.myspeshulpublikashun";
  sperkerr = ExtRoutingKey_ToSubsKey (testSubsString, &mySubscriptionKey);
  if (sperkerr != SPERKERR_NOERR) {
    cout << "TEST3: category.#.publication translation returned error \n";
    error = 1;
  }				      

  if (mySubscriptionKey != "performance_stat.#.myspeshulpublikashun") {
    cout << "TEST3: category.#.publication test returned mismatch \n";
    error = 1;
  }

  // Test # -- Gimme da ball! 

  sperkerr = SPERKERR_NOERR;
  testSubsString = "onespecificmessage";

  sperkerr = ExtRoutingKey_ToSubsKey (testSubsString, &mySubscriptionKey);
  if (sperkerr != SPERKERR_NOERR) {
    cout << "TEST3: #.publication translation returned error \n";
    error = 1;
  }				      

  if (mySubscriptionKey != "#.onespecificmessage")  {   
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
