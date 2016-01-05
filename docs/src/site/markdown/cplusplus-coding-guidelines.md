<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the 
  License.
-->

These are the C++ coding guidelines that are part of the acceptance criteria for code submitted to the Trafodion. Trafodion reviewers use these guidelines when reviewing changes.

The guidelines describe practices that are either required or preferred. In addition, areas where there is no preference between two or more practices are described.

Trafodion is composed of several distinct sub-projects, some of which have coding guidelines that differ from the Trafodion standard; for example, a requirement in most areas of the code may be only preferred in others.

There may also be existing code that violates one or more of the published requirements. The intent is to correct these over time, and corrections are encouraged when changing code to make a fix or implement new functionality. However, changes that are solely coding guideline changes are not recommended since this places undue burden on the reviewers.

# Header Files
Keep **<code>#include</code>** directives to a minimum in header files. 

You may forward declare classes and structs when the only use within the header file is a pointer or reference. While includes should be kept to a minimum, if something is used in a header file and cannot be forward declared, it must be explicitly included. 

All files, headers and implementation, should include everything they need to be self-sufficient. They should never assume something will be pre-included. In other words, the contents of a header file should compile cleanly by itself. To help ensure this, all implementation files should include their respective header file first.

Header files should not contain declarations for public items that are only used by the implementation file. Public items that are require in the implementation file but not the header file should be declared in the implementation file. The preference is NOT to create header files that consist only of includes of other header files.

Declare as little as possible in the header file and keep as much of the actual implementation private as is reasonable. For instance, don’t include in a header file declarations of types, enums, and functions that are only referenced by the implementation file.

## Including Standard Header Files
The preference is for C++ style includes over C style includes for standard header files.

    //Preferred
    #include <cstdio>
    //Accepted
    #include <stdio.h>

## Include Guards
All header files must use include guards. The name of the include guard **<code>#define</code>** should be the filename in all uppercase, with underscore used in place of periods.

For instance, if the header file is named **<code>ServerInterface_ODBC.h</code>**, the header file should begin and end as follows:

    #ifndef SERVERINTERFACE_ODBC_H
    #define SERVERINTERFACE_ODBC_H
    ...
    #endif /* SERVERINTERFACE_ODBC_H */

Comments following the **<code>#endif</code>** indicating the include guard are preferred.

# Variable Declaration and Naming Standards
Trafodion uses a combination of Pascal and Camel case. 

* Pascal case means that the first letter in each word in an identifier is capitalized. 
* Camel case is similar except the first letter is in lower case. 

For both, underscores are **not** used to separate words. The general rule is that identifiers with local scope start with a lower case letter and identifiers with global scope start with an upper case letter.

    //Pascal case
    class AuthenticationMessage; 
    //Camel case (aka lower Camel case or camelCase)
    int logonCount; 
    Class Names

Class names should be Pascal case and should describe the object contents (not what it does), with as little abbreviation as possible. When names include acronyms, the acronyms should be in all upper case.

**Acceptable Examples**

    //Preferred
     
    class SQLSessionContext; // an object that contains the context for a SQL session
    class PrivilegeList; // a list of privileges

**Poor Examples**

    class OutputInfo; // Doesn’t describe class contents, no context
    class ReadTableDef; // Describes what class does, not contents
    class Cmdline_Args; // Prefer Pascal case, no underscore, for class names

## Class Member Variable Names
Private member data variables should be suffixed with an underscore and should use Camel case. When names include acronyms, the acronyms should be in all upper or all lower case, dependent on which case the first letter should be.

**Example**

    class Employee
    {
    public:
       Employee ();
    private:
       std::string firstName_; 
       std::string lastName_; 
       uint16_t    departmentNumber_;
       std::string departmentName_;
       uint32_t    irsSSN_;
    }

## Function Names
Class member functions and static file functions should use Camel case. External non-class functions should use Pascal case. Except for constructors, destructors, and operators, the function name should include a verb that describes the action the function is performing.

**Good Examples**

    //Class member functions
    int32_t getSalary() const;
    int32_t setAuthID();
    int32_t closeAllCursors();

**Bad Examples**

    // Is it setting break enabled, returning it or ???
    int32_t SQLCLI_BreakEnabled();

## Enums
Enum types should use Pascal case and describe the class of enums. If the enum is declared outside of a class, the type name should include an indication of the scope of the enums.

Enums themselves should be declared as all upper case. The names may begin with a common prefix or be independent, depending on the usage.

When enums represent an arbitrary set of return values (that is, error codes, state codes, etc.), then avoid the values -1, 0, and 1 if using weakly typed enums, to reduce the chance of matches with Booleans or uninitialized variables.

The preference is to declare enums as strongly typed.

    enum class EnumName {...};

## Boolean Variables
Boolean variables names should include a verb, state, and optionally a noun (object whose state is in question) indicating the nature of the Boolean. Any combination is acceptable, however verbState is the most common.

**Good Examples**

    bool isValid;           // verbState
    bool isValidTable;      // verbStateNoun
    bool tableIsDroppable;  // nounVerbState
    bool hasData;           // verbState

**Bad Examples**

    bool valid;
    bool tableState;
    bool empty;

Functions that return a Boolean should also have names of the form verbState or verbStateNoun if the functions return state information. (This naming standard does not apply to functions returning Boolean as indication of success or failure.)

**Good Examples**

    bool isValidHbaseName();
    bool isHostNameExcluded();
    bool canUseCbServer();

**Bad Examples**

    // Don’t use get for Boolean accessors
    bool getUDRAccessModeViolation(); 
    
    // Don’t use integer return for Boolean functions
    short existsInHBase();
    
    // Function name implies it is sending settings to the compiler, but it is
    // actually only returning an indication that settings should be sent.  
    // A better name would be shouldSendSettingsToCompiler().
    bool sendSettingsToCompiler();

Parts of Trafodion code use one of two Boolean typedefs, **<code>NABoolean</code>** and **<code>ComBoolean</code>**, declared as follows:

    typedef int   Int32;
    typedef Int32 NABoolean;
    typedef NABoolean ComBoolean;
    const NABoolean  TRUE = (1 == 1);
    const NABoolean  FALSE = (0 == 1);

Exercise care when mixing usage of bool and **<code>NABoolean</code>**/**<code>ComBoolean</code>** types, as the latter are not guaranteed to only contain values of TRUE and FALSE. The use of non-standard Boolean types is gradually being phased out.

## Constants
All constant names should be all upper case, regardless of how the constant is declared. That is, enums, defines, and variables with the **<code>const</code>** modifier should be named in all upper case.

Defines, enums, and const are all permitted and used throughout Trafodion, although most code in Trafodion uses enum for numerical constants and defines for character constants. 

For new code, the use of const char or string is preferred for character constants instead of defines.

## Namespace Names
The preference is for namespaces to be all lower case, with preference to single words (note the exception to the rule that a name with global scope should start with an upper case). 

If a namespace must be dual-worded, use underscores. If mixed case names are used, Pascal case is preferred.

**Examples**

    //Preferred
    
    namespace compiler 
    //Accepted
    
    namespace Compiler

# Indentation and Formatting
## Indentation
TAB characters are not permitted in source files except for text files (for example, makefiles) that require them.

Trafodion code uses several indenting depths, including 2, 3, 4, and 8 spaces. Most common is 2 and 3. 

Use the style found in existing code, and when writing new code, use either 2, 3, or 4, and remain consistent.

A variety of control block indentation styles are used throughout Trafodion, most commonly Allman, Whitesmith, Stroustrup, and GNU. Follow the predominant style when making small to medium changes to existing code. For new code, the Allman style is preferred.

    //Allman
       if (x > 5)
       {
          error = doThis(x);
          if (error != 0)
          {
             return false;
          }
       }
       else
       {
          doThat(x);
       }
    
    //Whitesmith
       if (x > 5)
          {
          error = doThis(x);
          if (error != 0)
             {
             return false;
             }
          }
       else
          {
          doThat(x);
          }
    
    //Stroustrup 
       if (x > 5) {
          error = doThis(x);
          if (error != 0) {
              return false;
          }
       }
       else {
          doThat(x);
       }

Note that the Stroustrup and the similar K&R formats were popularized by usage in books where conservation of line count was a goal.

    //GNU
      if (x > 5)
        {
          error = doThis(x);
          if (error != 0)
            {
              return false;
            }
        }
      else
        {
          doThat(x);
        }

# Comments
## Comment Style
C++ style comments are preferred, but C comments are acceptable as well.

Some code uses Doxygen style comments, but this is not required.

## When/Where Comments Should Be Used
Every file should have a comment at the beginning describing the purpose of the file.

In header files where classes are declared, there should be a comment describing the class, including purpose and usage. Also describe anything out of the ordinary, such as the use of multiple inheritance.

Within implementation files, in addition to the comment at the beginning of the file, add comments for any global or static variables defined in the file, and how the variable is handled in a multi-threaded environment (if applicable).

Also, for each function defined, describe the purpose and intent of the function. For each parameter, list whether it is input or output (or both), how it is used, and any range restrictions imposed. For functions not returning void, describe the possible return values.

Within the body of the function, there is no need to write comments that document the obvious. But if there is any complexity to the logic, at a minimum document the intent, and consider documenting the details (assumptions, limits, unexpected side effects from function calls, etc.)

If a feature is only partially implemented, add a comment indicating at a high level what work remains. Prefix the comment with //TODO.

    //TODO Code is currently not thread safe.  Need to protect allocation of ...

# Error Handling
## Asserts
Trafodion uses asserts in the "debug" build. Use asserts freely, but ensure they do not contain any side effects as the code is not present in the "release" build.

## Error Return/Retrieval
Avoid the use of integer return codes for success and error codes. Instead, use bool for simple succeeded/failed and enum types for returns with multiple conditions.

## Exception Handling
Trafodion code mixes usage of exceptions and error returns. When using try/catch blocks, keep the scope as small as possible. Ensure all exceptions thrown are handled, potentially in main() if nowhere else.

# General Guidelines
## Casting
Avoid using C-style casts. Use C++-style casts instead, as they are often safer and easier to search for.

    //Preferred
    
    int x = static_cast<int>(shortVariable);
    
    MyType *myVar = reinterpret_cast<MyType *>(voidPtr);

Don’t blindly cast to remove a compiler error or warning. Ensure the cast is safe.

Use const casting sparingly. Often the use of mutable or changing a function to be const correct is a better solution.

## Types
Use standard types defined in **<code>\<cstdint\></code>** or **<code>\<stdint.h\></code>**. Note that Trafodion defines and uses many non-standard types (for example, **<code>Int32</code>**, **<code>Lng32</code>**), but this usage is being phased out.

Use types with explicit sizes (for example, **<code>int32_t</code>**, **<code>int64_t</code>**) when size is a factor in the code, such as an external API, or column in a table. 

Where size is not a factor (counters, indexes) a non-sized type such as **<code>int</code>** or **<code>long</code>** may be used. However, in general, **<code>size_t</code>** and **<code>ssize_t</code>** are preferred for variables where fixed size is not required.