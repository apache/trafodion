// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef _MAL_TRINITY_EXCEPTION_
#define _MAL_TRINITY_EXCEPTION_

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include "MalTrinityErrorStack.h"

//! the class MalTrinityException holds a *MalTrinityErrorStack
namespace Trinity {

using namespace std;

void malAssertFailure(const char* exprText, const char * file, uint32_t line);

#define THROWMALTRINITYEXCEPTION(e) do {e.setFile(__FILE__); e.setLine(__LINE__); throw e;} while(0)
#define CREATEMALTRINITYEXCEPTION(e) MalTrinityException e(__FILE__ , __LINE__)

#ifdef NDEBUG
#define MALASSERT(expr) {}
#else
#define MALASSERT(expr) do { if(!(expr)) malAssertFailure(#expr, __FILE__, __LINE__); } while(0)
#endif
#define MALPERMASSERT(expr) do { if(!(expr)) malAssertFailure(#expr, __FILE__, __LINE__); } while(0)

//! forward declaration
class MalTrinityErrorStack;
//! The class MalTrinityException holds a MalTrinityErrorStack object
class MalTrinityException
{
public:
    //! constructor for MalTrinityException
    MalTrinityException();
    //! constructor for MalTrinityException initializes *MalTrinityErrorStack *t_err
    //! \param [in] t_err is a pointer to a MalTrinityErrorStack
    MalTrinityException(MalTrinityErrorStack *t_err);
    //! constructor for MalTrinityException creates a new *MalTrinityErrorStack
    MalTrinityException(string filename,uint32_t line);
    //! constructor for MalTrinityException creates a new *MalTrinityErrorStack and initializes filename and line
    MalTrinityException(MalTrinityErrorStack *t_err,string filename,uint32_t line);
    //! copy constructor for MalTrinityException
    MalTrinityException(const MalTrinityException &mte);
    //! destructor for MalTrinityException releases all objects
    ~MalTrinityException(void);
    //! returns a *MalTrinityErrorStack or NULL if none initialized
    //! \return a pointer to a MalTrinityErrorStack
    MalTrinityErrorStack *getMalTrinityErrorStack();
    //! getMalTrinityErrorStackRef returns a reference to a MalTrinityErrorStack
    const MalTrinityErrorStack &getMalTrinityErrorStackRef() const;
    //! sets a MalTrinityErrorStack
    //! \param[in] t_err is a pointer to a MalTrinityErrorStack
    void setMalTrinityErrorStack(MalTrinityErrorStack *t_err);
    //! sets the line number
    //! param[in] line uint32_t holding line number
    void setLine(uint32_t line);
    //! sets the file name
    //! param[in] file string holding file name
    void setFile(string file);
    //! returns line number
    uint32_t getLine() const;
    //! returns file name
    string getFile() const;


private:
    //! stores a MalTrinityErrorStack
    MalTrinityErrorStack *triError;
    //! holds a line number
    uint32_t lineNumber;
    //! holds the sourcefile path
    string sourceFile;
    //! determine if MalTrinityErrorStack has been passed in
    bool deleteStack;
    //! function to handle call to TFDS_TRIGGER_
    void tfds(const char* file,uint32_t line);
};
}

#endif // _MAL_TRINITY_EXCEPTION_
