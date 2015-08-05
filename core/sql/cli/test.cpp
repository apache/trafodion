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
#include <limits.h>
#include <stdlib.h>
#include <iostream>

class A
{
  
public:
  void * operator new(size_t size, Int32 a);
  void * operator new(size_t size);
  void operator delete(void * ptr);
private:
  Int32 a;
};

class C
{
  friend class B;
  
  Int32 a;
public:
  C()
  {
    a = 10;
  };
  
};

  
class B : public A
{
public:
  B();
  ~B();
  
  Int32 b;

  C c;
};

void C()
{
  cout << "C\n";
  
}

void D()
{
  cout << "D \n";
  
}

void E()
{
  cout << "E \n";
}

void * A::operator new(size_t size, Int32 a)
{
  C();
  
  return ::operator new(size);
}

void * A::operator new(size_t size)
{
  D();
  
  return ::operator new(size);
}

void A::operator delete(void * ptr)
{
  E();
  
  ::operator delete(ptr);
}

B::B()
{
  b = 10;

  cout << c.a << endl;
}

B::~B()
{
  b = -1;
}

Int32 main()
{
  B * b = new(1) B();

  delete b;
  
  return 0;
}


