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
#ifndef NDCS_UTILS_H_
#define NDCS_UTILS_H_

   // Platform independent utility functions used in ndcs

   #define strupr			_strupr
   char * _strupr(char * str);

   #define itoa			_itoa
   char * _itoa(int n, char *buff, int base);

   #define ltoa			_ltoa
   char * _ltoa(long n, char *buff, int base);

   __int64 _atoi64( const char *s );

   char* trim(char *string);


   #define markNewOperator\
	   markNOperator(__FILE__,__FUNCTION__, __LINE__)

   void listAllocatedMemory(char* description);
   void markNOperator(char* file,const char* function,long line);

#endif
