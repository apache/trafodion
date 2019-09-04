//------------------------------------------------------------------
//
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

//
// Exception module
//
#ifndef __SB_EXCEP_H_
#define __SB_EXCEP_H_

#include <exception>

//
// Use these for references.
//
#if __cplusplus < 201103L // Standards below C++2011 in which 
                          // dynamic throw is allowed

#define SB_THROW_FATAL(msg) throw SB_Fatal_Excep(msg)
#define SB_THROWS_EXCEP(exc) throw (exc)
#define SB_THROWS_FATAL SB_THROWS_EXCEP(SB_Fatal_Excep)
 
#else  // Starting C++2011,  use noexcept(bool)

#define SB_THROW_FATAL(msg) noexcept(false)
#define SB_THROWS_EXCEP(exc) noexcept(false)
#define SB_THROWS_FATAL noexcept(false)

#endif



//
// Base-class for seabed exceptions.
//
class SB_Excep : public std::exception {
public:
    SB_Excep(const char *msg);
    SB_Excep(const SB_Excep &excep); // copy const
    virtual ~SB_Excep() throw ();
    SB_Excep &operator =(const SB_Excep &excep);
    // called by terminate
    virtual const char *what() const throw ();
protected:
    char *ip_msg;
};

//
// Fatal seabed exception.
//
class SB_Fatal_Excep : public SB_Excep {
public:
    SB_Fatal_Excep(const char *msg);
    virtual ~SB_Fatal_Excep() throw ();
};

#endif // !__SB_EXCEP_H_
