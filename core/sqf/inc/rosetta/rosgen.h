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

//*********************************************************************
//*********************************************************************

//-****************************************************************************
//  rosgen.h
//
//  This file contains defines, typedefs, and inline function definitions
//  which will be used to make Rosetta-translated code less cluttered and
//  confusing.
//
//  The utilities in the "rosgen.h" include file are all the most
//  frequently used utilities.
//-****************************************************************************

#ifndef _rosgenh_
#define _rosgenh_


// Set pTAL defaults for pragmas FIELDALIGN, OVERFLOW_TRAPS, and GP_OK
#ifndef set_fieldalign
#endif /* set_fieldalign */
#ifndef set_overflow
#endif /* set_overflow */
#ifndef set_map
#endif /* set_map */
#ifndef set_extern_data
#endif /* set_extern_data */



#include <math.h>
#include <stdlib.h>
#include <stddef.h>
#include <setjmp.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

// Defines and typedefs to get non-Tandem C++ to accept the source:
#define _far
#define _near
#define _baddr
#define _waddr
#define _sg
#define _sgx
#define _procaddr
#define _cspace
#define _lowmem
#define _lowmem64
#define _lowmem256
#define _tal
#define _optional(a,b) b
// #define _arg_present(a) 1
#define _arg_present_always(a) 1
#define _arg_present_ifnot0(a) (a!=0)
#define _arg_present_ifge0(a) (a>=0)
#define _arg_present_ifnot4321(a) (a!=4321)

// Likewise, 'fixed' is because C++ doesn't support long long:
class long_long {public: long high; long low;};

// Defines to allow compilation of procs etc with Tandem's proc attributes:
#define _alias(name)
#define _interrupt
#define _priv
#define _resident
#define _extensible
#define _extensible_n(n)
#define _callable
#define _c
#define _cobol
#define _fortran
#define _pascal
#define _unspecified
#define _variable

// Translation of the $TYPE builtin is one of the following defines:
#define _type_undef     0
#define _type_char      1
#define _type_int_16    2
#define _type_int_32    3
#define _type_fixed     4
#define _type_real_32   5
#define _type_real_64   6
#define _type_substruct 7
#define _type_struct    8
#define _type_bitfield  9
#define _type_int_ptr   10

#ifndef ROSETTA_LITTLE_ENDIAN
#define ROSETTA_BIG_ENDIAN 1
#else
#define ROSETTA_BIG_ENDIAN 0
#endif

// Does the compiler understand the LL suffix on long long constants?
#ifdef _MSC_VER
#define USE_LL 0
#define USE_L  1
#elif defined(__linux__)
#define USE_LL 1
#define USE_L  1
#else
#define USE_LL 0
#define USE_L  0
#endif
//-------------------------------------------------------------------------
// Punctuation in Macro Bodies and in Macro Actual Parameters
//
// The C++ language contains a macro substitution mechanism.  A list of
// actual parameters supplied in an invocation of a parameterized
// macro is enclosed in parentheses.  Each parameter in that list is
// separated by a comma.
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
#if 0

Embedded Comma and Parentheses

It is possible for the pTAL to C++ Translator to generate macro actual
parameters that contain unbalanced parentheses or commas that are embedded
in a macro actual parameter and do not function as parameter delimiters.
Invocations of the macros _lparen, _rparen, and _comma take the place of
these punctuation marks.  These macros are expanded after the macro to
which they are passed as parameter text, so the enclosing macro invocation
is not confused by the punctuation.

In the following example, parentheses are necessary in the generated C++
code to preserve the precedence of the shift operator over the
multiplication operator, thus preserving the meaning of the original pTAL
expression.  The generated right parenthesis sits in the macro actual
parameter.

  pTAL Code                   Generated C++ Code

  DEFINE fred(a,b) = a + b#;  #define fred(a,b) a + b
  ...                         ...
  j := 6 >> fred(7 * 8, 9);   j = (6 >> fred(7 _rparen * 8, 9);

The C++ macro substitution mechanism expands the macro fred first, yielding
the following expanded statement.

  j = (6 >> 7 _rparen * 8 + 9;

The C++ macro substitution mechanism then expands the macro invoked within
the actual parameter: _rparen.

  j = (6 >> 7 ) * 8 + 9;

The following example illustrates a use of the _comma macro in a macro
actual parameter.

  pTAL Code                   Generated C++ Code

  PROC p(x,y) EXTENSIBLE;     extern "C" _extensible
    INT(16) x;                     void P(int_16 x,
    INT(16) y;                            int_16 y);
  EXTERNAL;

  DEFINE callp(a) = p(a)#;    #define callp(a) P(a)

  PROC q;                     extern "C" void Q()
  BEGIN                       {
    callp(2);                   callp(2);
    callp(4 ',' 5);             callp(4 _comma 5);

The C++ macro substitution mechanism expands the macro callp first,
yielding the following expanded statement.

  P(4 _comma 5)

The C++ macro substitution mechanism then expands the macro invoked within
the actual parameter: _comma.

  P(4 , 5)


#endif
//-------------------------------------------------------------------------

#define _comma ,
#define _lparen (
#define _rparen )

//-------------------------------------------------------------------------
#if 0

Embedded Braces

The C++ language requires a semicolon terminator on statements, except on
compound statements.  The following examples are all correctly terminated
C++ statements.

  a = b + c;
  while (a < 10) do
    FOO(a);
  while (a < 10) do {
    FOO(a);
  }

A semicolon immediately following a compound statement terminates a null
statement.  The following example depicts a null statement following a while
statement which contains a compound statement.

  while (a < 10) do {
    FOO(a);
  };

The null statement is harmless when it appears in a simple statement
sequence.  In some contexts it can be harmful because it can mislead
the programmer.  For example, the C++ language if..else statement
contains two embedded statements.

  if ( condition ) true_action else false_action

condition
  is the controlling expression of the if statement.

true_action
  is the statement that is executed if the value of condition is nonzero.

false_action
  is the statement that is executed if the value of condition is zero.

If true_action were a simple statement, it would be terminated by a
semicolon.  If it were a compound statement, it would be terminated
by the closing brace ( } ).  For example:

  if (a < 10)                 if (a < 10) {
    a = 10;                     a = 10;
  else                          FOO(a);
    a = 20;                   }
                              else
                                a = 20;

The true_action embedded in this if..else statement can be encapsulated by
a macro, which is invoked as if it were a statement.  A straightforward
encapsulation of the examples above, however, require slightly different
syntax at the point of invocation.  The invocation of the macro containing
the compound statement cannot be followed by a semicolon.

  #define mac1 a = 10         #define mac2 {             \
                                             a = 10;     \
                                             FOO(a);     \
                                           }
  ...                         ...
  if (a < 10)                 if (a < 10) {
    mac1;                       mac2
  else                        else
    a = 20;                     a = 20;

This code would be more maintainable if the programmer did not have to
know whether or not the macro contained a compound statement in order to
use it correctly.

The pTAL to C++ Translator translates a pTAL macro that contains a statement
in such a way that the generated C++ macro is always used as if it were a
simple statement; it is always followed by semicolon.  The pTAL to C++
Translator generates uses of the macros _begin and _end in place of the
opening brace ( { ) and closing brace ( } ) respectively.  The latter
example would appear as follows.

  pTAL Code                   Generated C++ Code

  DEFINE mac2 = BEGIN         #define mac2 _begin        \
                 a := 10;                    a = 10;     \
                 CALL FOO(a);                FOO(a);     \
                END#;                      _end
    ...                         ...
    IF a < 10 THEN              if (a < 10)
      mac2;                       mac2;
    ELSE                        else
      a := 20;                    a = 20;

_begin

The _begin macro is expanded to do { and is used to begin a compound
statement.  That compound statement must end with the _end macro.

_end

The _end macro is expanded to } while (0) and is used to end a compound
statement.  That compound statement must begin with the _begin macro.  That
compound statement also requires a semicolon terminator.

#endif
//-------------------------------------------------------------------------
// Defines '_begin' and '_end' may be used whereever the reserved words
// 'begin' and 'end' were used in pTAL.  Curly braces ("{" and "}")
// in general cannot.

#define _begin do {
#define _end  } while (0)

//-------------------------------------------------------------------------
#if 0
Global Variables (_global, _value(initial_value), and _export_globals)

The pTAL and C++ languages use slightly different mechanisms to define and
initialize global variables that are visible in more than one separately
compiled module.  Code generated by the pTAL to C++ Translator emulates
the pTAL mechanism.  This section describes the two mechanisms and the
code generated by the pTAL to C++ Translator to emulate the pTAL mechanism
in C++.

The pTAL to C++ Translator produces a straightforward translation of
static global variables, which are only visible in the module in which
they are defined.


Definitions of Terms

The following terms are used in this discussion:

A variable definition states the variable s type and causes storage to
be allocated.  A variable may be initialized when it is defined.  A variable
is defined in exactly one module.

A variable declaration makes a variable known to a module of code.  It does
not cause storage to be allocated, and variable initialization does not
take place when it is declared.

A global variable that is visible in more than one separately compiled
module is exported from the module that defines it.  The variable is
exported from exactly one module.

A global variable that is visible in more than one separately compiled
module is imported to the modules that declare it.


C++ Global Variables

In C++, a programmer usually defines a variable in one module''s
implementation file and declares it in a header file.

In the following example, the compilation unit pebbles contains a
definition of the global variable fred which causes the variable to
be allocated and initialized.  The variable is visible in the
compilation unit bambam.  The compilation unit pebbles exports the
variable, and the compilation unit bambam imports it.

  --------------
  C++ file inclh
  --------------

  extern int_16 fred;

  ----------------
  C++ file pebbles
  ----------------

  #include "incl.h"
  int_16 fred = 13;
  ...
    if (fred == 13) ...

  ---------------
  C++ file bambam
  ---------------

  #include "incl.h"
  ...

    x = fred;


pTAL Global Variables

In pTAL, a global variable is declared and defined with exactly the same
syntax.  If the pTAL EXPORT_GLOBALS directive is set for a particular
compilation unit, then the variable is declared.  If not, then the variable
is defined.  A programmer usually supplies that syntax in a header file
that defines the variable in one compilation unit, in which the
EXPORT_GLOBALS directive is set, and declares it in all of the others.

In the following example, the compilation unit pebbles contains a definition
of the global variable fred which causes the variable to be allocated and
initialized.  The variable is visible in the compilation unit bambam.  The
compilation unit pebbles exports the variable, and the compilation unit
bambam imports it.

  ---------------
  pTAL file inclh
  ---------------

  BLOCK fred;
    INT(16) f := 13;
  END BLOCK;

  -----------------
  pTAL file pebbles
  -----------------

  ?EXPORT_GLOBALS
  ?SOURCE inclh
  ...
    IF f = 13 THEN ...

  ----------------
  pTAL file bambam
  ----------------

  ?SOURCE inclh
  ...
    x := f;


Generated C++ Global Variables

The pTAL to C++ Translator emulates pTAL-style global variable definition
and declaration.  It generates the same text for defining exported variables
as for declaring imported variables.

In the following example, the compilation unit pebbles contains a definition
of the global variable fred which causes the variable to be allocated and
initialized.  The variable is visible in the compilation unit bambam.  The
compilation unit pebbles exports the variable, and the compilation unit
bambam imports it.

  --------------
  C++ file inclh
  --------------

  _global int_16 fred _value(13);

  ----------------
  C++ file pebbles
  ----------------

  #define _export_globals
  #include "inclh"
  ...
    if (fred == 13) ...

  ---------------
  C++ file bambam
  ---------------

  #include "inclh"
  ...
    x = fred;


_global

The _global macro is expanded to nothing if the preprocessor symbol
_export_globals is defined.  It is expanded to the keyword extern if
_export_globals is not defined.  The extern keyword identifies a
declaration of an imported variable.

  _global


_value

The _value macro initializes the global variable if the preprocessor symbol
_export_globals is defined.  It is expanded to nothing if _export_globals
is not defined.

  _value(initial_value)

initial_value
  is an expression that evaluates to the initial value for the global
  variable.

Usage Considerations

A comma-separated list of constants is often used to initialize arrays.
This style of initialization cannot appear as the argument to the _value
macro without addressing the problem of commas acting as constant separators
in the list rather than as macro actual parameter separators.  In this case,
the pTAL to C++ Translator emits the variable initialization protected by a
conditional compilation toggle.

For example:

--  pTAL Code                   Generated C++ Code
--
--  INT(16) ROGER [0:5]         _global int_16 ROGER[6]
--    := [0,1,2,3,4,5];         #ifdef _export_globals
--                              ={0,1,2,3,4,5}
--                              #endif
--                              ;
--

#endif
//-------------------------------------------------------------------------

#ifdef _export_globals
#define _global
#define _value(x) = x
#else
#define _global extern
#define _value(x)
#endif /* _export_globals */

//-------------------------------------------------------------------------
#if 0

Relational expressions, conditional expressions, and group comparison
expressions produce boolean results in both pTAL and C++.  The pTAL
language produces the value -1 to represents the boolean value TRUE,
and the C++ language produces the value 1 to represent the boolean value
TRUE.  Both languages produce the value zero to represent the boolean
value FALSE.


Equivalent Boolean Value

Both pTAL and C++ accept a nonzero value as indicating the boolean value
true in a conditional expression or in an IF statement.  For example:

  INT(16) PROC maggie(a);
    INT(16) a;
  BEGIN
    RETURN a < 4;
  END;

  INT(16) PROC joel(a,b);
    INT(16) a,b;
  BEGIN
    RETURN a AND b;
  END;
    ...
    result0 := data '=' [4,5,6];
    IF result0 THEN ...
    result1 := (a > 0) AND (a < 10);
    IF result1 AND result0 THEN ...
    result2 := (a AND 4) + (b > 2);
    result3 := (a < 4) LAND 7;
    result4 := (b = 6) << 2;
    ...

A boolean value is most often used as:

- an IF statement conditional expression
- an operand to a conditional operator

Both consider any nonzero value to be TRUE, so it does not matter whether
the value is represented as a -1 or a 1.

  result0 := data '=' [4,5,6];
  IF result0 THEN ...
  result1 := (a > 0) AND (a < 10);
  IF result1 AND result0 THEN ...



Differing Boolean Value

The pTAL language produces the value -1 to represents the boolean value
TRUE, and the C++ language produces the value 1 to represent the boolean
value TRUE.  This can be significant if the boolean value is used as an
operand to:

- a bitwise logical operator
- an arithmetic operator
- a bit extraction
- a shift operator

These expressions might evaluate to a different result if the value of
TRUE were represented as 1 than they would if the value of TRUE were
represented as -1.  It is usually appropriate to direct the pTAL to C++
Translator to generate the pTAL representation of TRUE when this boolean
value is used in a bitwise logical operator, an arithmetic operator, or
a shift operator.

  result2 := (a AND 4) + (b > 2);
  result3 := (a < 4) LAND 7;
  result4 := (b = 6) << 2;

To ensure a faithful translation of the original pTAL code, the code
generated by the pTAL to C++ Translator converts a C++ boolean value
to a pTAL boolean value, via the _tbv macro.  The following example
illustrates the C++ translation of the previous examples.

  extern "C" int_16 MAGGIE(int_16 a) {
    return _tbv(a < 4);
  };

  int_16 JOEL(int_16 a, int_16 b) {
    return _tbv(a AND b);
  };
    ...
    const int_16 _temp1[] = {4,5,6};
    result0 = _tbv(_mov_const_list(word,data,_temp1) == 0);
    if (result0) ...
    result1 = _tbv((a > 0) && (a < 10));
    if (result1 && result0) ...
    result2 = (_tbv(a && 4)) + (b > 2);
    result3 = (_tbv(a < 4)) & 7;
    result4 = (_tbv(b = 6)) << 2;

If the correctness of your program does not depend on the pTAL
representation, you can create more attractive and efficient code
by removing the invocation of _tbv.


_tbv

The _tbv macro, supplied in the rosgen.h include file, converts a boolean
expression value from a C++ representation to a pTAL representation.

  int_16 _tbv(int_16 expr)

expr
  is a C++ boolean expression.

Usage Considerations

The _tbv macro converts the representation of the boolean value expr
by negating the value of the expression given as an argument.

In a context which accepts any nonzero value as indicating the boolean
value true, an IF statement conditional expression or an operand to a
conditional operator,  the invocation of the _tbv macro can be removed
without disturbing the behavior of the code.


#endif
//-------------------------------------------------------------------------
// This define is used to map boolean expression values from the C++
// canonical value to the pTAL canonical value.
#define _tbv(expr) ((expr) ? -1 : 0)

//---------------------
// Simple type defines:
//---------------------
#define unsigned_char unsigned char
#define int_16        short
// This typedef and the those below for unsigned short and unsigned long are
// here so token pasting in the _item2, _redef2, _array2, and _redefarray2
// defines works correctly.  They insure that type-denoting defines always have
// at most one token.

// dg64 - vvv - Add xint_32/xunsigned_32
// xint_32 is a REAL 32-bit item
typedef int           _xint_32;
#define xint_32       _xint_32
typedef unsigned int  _xunsigned_32;
#define xunsigned_32  _xunsigned_32
// dg64 - ^^^
#ifdef NA_64BIT
  typedef struct { long l1; long l2; } _int_128;
  #define int_128                      _int_128
#endif

typedef long          Long;
typedef int           _int_32;
#define int_32        _int_32
typedef long          _int_ptr;
#define int_ptr       _int_ptr
#define long_ptraddr_val Long
  #ifdef _MSC_VER
    #ifdef NA_64BIT
      // dg64 - the right type
      typedef unsigned int  _unsigned_32;
      typedef unsigned long _unsigned_64;
    #else
      typedef unsigned long _unsigned_32;
    #endif
    //typedef __int64       _int_64;
  #else
    #ifdef NA_64BIT
      // dg64 - the right type
      typedef unsigned int  _unsigned_32;
      typedef unsigned long _unsigned_64; // sss use the define in sqtypes.h
      typedef long          _int_64;
    #else
      typedef unsigned long          _unsigned_32;
      typedef unsigned long long int _unsigned_64;
      typedef long long              _int_64;
    #endif
  #endif
  #define unsigned_64   _unsigned_64
#define unsigned_32   _unsigned_32
#define int_64        _int_64

typedef float         _real_32;
#define real_32       _real_32
typedef double        _real_64;
#define real_64       _real_64
typedef unsigned short _unsigned_16;
#define unsigned_16   _unsigned_16
#define fixed_n19     int_64
#define fixed_n18     int_64
#define fixed_n17     int_64
#define fixed_n16     int_64
#define fixed_n15     int_64
#define fixed_n14     int_64
#define fixed_n13     int_64
#define fixed_n12     int_64
#define fixed_n11     int_64
#define fixed_n10     int_64
#define fixed_n9      int_64
#define fixed_n8      int_64
#define fixed_n7      int_64
#define fixed_n6      int_64
#define fixed_n5      int_64
#define fixed_n4      int_64
#define fixed_n3      int_64
#define fixed_n2      int_64
#define fixed_n1      int_64
#define fixed_0       int_64
#define fixed_1       int_64
#define fixed_2       int_64
#define fixed_3       int_64
#define fixed_4       int_64
#define fixed_5       int_64
#define fixed_6       int_64
#define fixed_7       int_64
#define fixed_8       int_64
#define fixed_9       int_64
#define fixed_10      int_64
#define fixed_11      int_64
#define fixed_12      int_64
#define fixed_13      int_64
#define fixed_14      int_64
#define fixed_15      int_64
#define fixed_16      int_64
#define fixed_17      int_64
#define fixed_18      int_64
#define fixed_19      int_64
#define fixed_star    int_64

//------------------
// Address typedefs:
//------------------
typedef void _near _baddr *                        baddr;
typedef void _near _waddr *                        waddr;
typedef void _far *                                extaddr;
typedef void _procaddr *                           procaddr;
typedef unsigned_char _cspace const _baddr _near * cbaddr;
typedef int_16 _cspace const _waddr _near *        cwaddr;

//-------------------------------------------------------------------------
#if 0

Bit Field Data Type

The pTAL language supports the declaration of a bit field within a structure
or as a variable in its own right.  The C++ language only supports the
declaration of a bit field within a structure.

Bit Field Declared as a Structure Item

The pTAL to C++ Translator translates a pTAL bit field within a structure
to a C++ bit field within a structure.  It declares the bit field with
either the conventional C++ bit field declaration syntax or the _bitfield
macro.  Either means of declaring the data item directs the compiler to
allocate a bit field with the same size and position within the structure
as its pTAL counterpart.

The following example illustrates one pTAL bit field structure item that
is translated to a conventional C++ bit field declaration and one that is
translated to the _bitfield macro which declares the data and an access
function.

  pTAL Code                   Generated C++ Code

  ?ENCAPSULATE_STRUCT ttype

  STRUCT stype(*);            struct stype {
  BEGIN                         unsigned  a:4;
    UNSIGNED(4) a;            };
  END;

  STRUCT ttype (*);           class ttype {
  BEGIN                       public:
    UNSIGNED(4) a;              _bitfield(ttype,a,4);
  END;                        };

  STRUCT  s(stype);           stype  s;
  STRUCT  t(ttype);           ttype  t;
  INT(16) x;                  int_16 x;

  s.a := 7;                   s.a = 7;
  x := s.a;                   x = s.a;

  t.a := 7;                   t.a() = 7;
  x := t.a;                   x = t.a();

Bit Field Declared as a Variable

A bit field variable generated by the pTAL to C++ Translator occupies a
16-bit or 32-bit container.  Values that are stored into a bit field are
trimmed to occupy the appropriate number of bits.  The classes _unsigned_1,
_unsigned_2, through _unsigned_15 and _unsigned_17 through _unsigned_31
implement bit fields.

_unsigned_n

The classes _unsigned_1, _unsigned_2, _unsigned_3, _unsigned_4,
_unsigned_5 _unsigned_6, _unsigned_7, _unsigned_8, _unsigned_9
_unsigned_10, _unsigned_11, _unsigned_12, _unsigned_13, _unsigned_14,
_unsigned_15, _unsigned_17, _unsigned_18, _unsigned_19, _unsigned_20,
_unsigned_21, _unsigned_22, _unsigned_23, _unsigned_24, _unsigned_25,
_unsigned_26, _unsigned_27, _unsigned_28, _unsigned_29, _unsigned_30,
and _unsigned_31 implement bit field variables.

These classes each provide a default constructor and a type conversion
constructor that trims an integer to fit into the bit field.  They each
also provide an assignment operator.

  _unsigned_n()
  _unsigned_n(int_type value)

n
  is an integer 1 through 15 or 17 through 31.

int_type
  is a 16-bit integer if n is 1 through 15 and a 32-bit integer if n is 17
  through 31.

value
  is a 16-bit integer value being converted to an _unsigned_1 through
  _unsigned_15 or a 32-bit integer value being converted to an _unsigned_17
  through _unsigned_31.


  unsigned_type& operator=(const int_type value)
  unsigned_type& operator=(_unsigned_n unsigned_class)

n
  is an integer 1 through 15 or 17 through 31.

int_type
  is a 16-bit integer if n is 1 through 15 and a 32-bit integer if n is 17
  through 31.

unsigned_type
  is a 16-bit unsigned integer if n is 1 through 15 and a 32-bit unsigned
  integer if n is 17 through 31.

value
  is a 16-bit integer value being converted to an _unsigned_1 through
  _unsigned_15 or a 32-bit integer value being converted to an _unsigned_17
  through _unsigned_31.

unsigned_class
  is an _unsigned_1 through _unsigned_15 if this is an _unsigned_1 through
  _unsigned_15 and is an _unsigned_17 through _unsigned_31 if this is an
  _unsigned_17 through _unsigned_31.

Example

The following example illustrates the declaration of a 6-bit variable named
a and an array named b made up of four 4-bit cells.  The _unsigned_n classes
define an assignment operator and an access function which must be employed
to use the value stored in the bit field variable as an lvalue.  In this
example, the value 15 can fit into the six bits that make up the storage
available to the variable a.  The value 18 cannot fit into the four bits
that make up the storage available to the second cell of the variable b,
so the most significant bits of that value are trimmed.  The value 3 is
stored in b[1] after the trimming.

  pTAL Code                   Generated C++ Code

  unsigned(6) a;              _unsigned_6  a;
  unsigned(4) b[0:3];         _unsigned_4  b[4];
  int_16      x;              int_16       x;

  a := 15;                    a = 15;
  x := a;                     x = a();

  b[1] := 18;                 b[1] = 18;
  x := b[1];                  x = b[1]();


#endif
//-------------------------------------------------------------------------
class _bitfield16_outside_struct {
  public:
    _bitfield16_outside_struct(){};
    _bitfield16_outside_struct(int_16 d){d = d;};
    unsigned_16& operator ()(){return data;};
  protected:
    unsigned_16 data;
};

class _bitfield32_outside_struct {
  public:
    _bitfield32_outside_struct(){};
    _bitfield32_outside_struct(int_16 d){d = d;};
    unsigned_32& operator ()(){return data;};
  protected:
    unsigned_32 data;
};

#define _decl_bitfield_outside_struct(bitcount)                               \
      class _unsigned_##bitcount : public _bitfield16_outside_struct {        \
        public:                                                               \
          _unsigned_##bitcount () { data = 0;}                                \
          _unsigned_##bitcount (int_16 d) { data = d & ((1u << bitcount)-1);} \
          unsigned_16& operator=(const int d) {                               \
                                       data = d & ((1u << bitcount)-1);       \
                                       return data;}                          \
          unsigned_16& operator=(_bitfield16_outside_struct d) {              \
                                       data = d() & ((1u << bitcount)-1);     \
                                       return data;}                          \
          unsigned_16& operator=(_bitfield32_outside_struct d) {              \
                                       data = d() & ((1u << bitcount)-1);     \
                                       return data;}                          \
  operator int() {return data;}  /*pdv*/ \
      }
_decl_bitfield_outside_struct(1);
_decl_bitfield_outside_struct(2);
_decl_bitfield_outside_struct(3);
_decl_bitfield_outside_struct(4);
_decl_bitfield_outside_struct(5);
_decl_bitfield_outside_struct(6);
_decl_bitfield_outside_struct(7);
_decl_bitfield_outside_struct(8);
_decl_bitfield_outside_struct(9);
_decl_bitfield_outside_struct(10);
_decl_bitfield_outside_struct(11);
_decl_bitfield_outside_struct(12);
_decl_bitfield_outside_struct(13);
_decl_bitfield_outside_struct(14);
_decl_bitfield_outside_struct(15);
#undef _decl_bitfield_outside_struct

#define _decl_bitfield_outside_struct(bitcount)                               \
      class _unsigned_##bitcount : public _bitfield32_outside_struct {        \
        public:                                                               \
          _unsigned_##bitcount () { data = 0;}                                \
          _unsigned_##bitcount (int_32 d) { data = d & ((1u << bitcount)-1);} \
          unsigned_32& operator=(const int d) {                               \
                                       data = d & ((1u << bitcount)-1);       \
                                       return data;}                          \
          unsigned_32& operator=(_bitfield16_outside_struct d) {              \
                                       data = d() & ((1u << bitcount)-1);     \
                                       return data;}                          \
          unsigned_32& operator=(_bitfield32_outside_struct d) {              \
                                       data = d() & ((1u << bitcount)-1);     \
                                       return data;}                          \
      }
_decl_bitfield_outside_struct(17);
_decl_bitfield_outside_struct(18);
_decl_bitfield_outside_struct(19);
_decl_bitfield_outside_struct(20);
_decl_bitfield_outside_struct(21);
_decl_bitfield_outside_struct(22);
_decl_bitfield_outside_struct(23);
_decl_bitfield_outside_struct(24);
_decl_bitfield_outside_struct(25);
_decl_bitfield_outside_struct(26);
_decl_bitfield_outside_struct(27);
_decl_bitfield_outside_struct(28);
_decl_bitfield_outside_struct(29);
_decl_bitfield_outside_struct(30);
_decl_bitfield_outside_struct(31);
#undef _decl_bitfield_outside_struct


//-------------------------------------------------------------------------
#if 0

The pTAL language allows you to explicitly check whether or not an
arithmetic operation produced an arithmetic overflow by disabling
OVERFLOW_TRAPS and checking the value returned by the $OVERFLOW
standard function.

This section describes the code generated by the pTAL to C++ Translator
to emulate pTAL overflow checking.

If your code uses the routines described in this section, you must bind
the overflow trapping library, roselib.o, into your program before
executing it.  This library implements all of the routines described
in this section.


Emulating pTAL Overflow Checking

The pTAL to C++ Translator emulates pTAL overflow checking by generating
calls to functions that perform the arithmetic operation and return an
indication of whether or not the operation resulted in an arithmetic overflow.

For example:

  pTAL Code                   Generated C++ Code

                              #pragma push overflow_traps
                              #pragma nooverflow_traps
  PROC jed NOOVERFLOW_TRAPS;  extern "C" void JED()
  BEGIN                       {
    ...                         ...
    x := a + b;                 x = _int16_ov_add(a,b,
    IF $OVERFLOW THEN ...                         &_ov_temp);
                                if (_ov_temp) ...
    x := $FIXI(f);              x = _int64_to_int16_ov(f,
                                                  &_ov_temp);
    if $OVERFLOW THEN ...       if (_ov_temp) ...



_int16_ov_add,
_int32_ov_add,
_real32_ov_add,
_real64_ov_add,  and
_fixed_ov_add

The _int16_ov_add, _int32_ov_add, _real32_ov_add, _real64_ov_add, and
_fixed_ov_add functions add two values and explicitly check for overflow.
Explicit overflow checking is only appropriate when the nooverflow_traps
pragma is in effect.

  int_16 _int16_ov_add(int_16 first_operand,
                       int_16 second_operand,
                       int_16 *overflow_result)

  int_32 _int32_ov_add(int_32 first_operand,
                       int_32 second_operand,
                       int_16 *overflow_result)

  real_32 _real32_ov_add(real_32 first_operand,
                         real_32 second_operand,
                         int_16 *overflow_result)

  real_64 _real64_ov_add(real_64 first_operand,
                         real_64 second_operand,
                         int_16 *overflow_result)

  int_64 _fixed_ov_add(int_64 first_operand,
                       int_64 second_operand,
                       int_16 *overflow_result)

first_operand
  is an expression that is the first operand of the addition operator.

second_operand
  is an expression that is the second operand of the addition operator.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the addition does not result in an overflow and the value 1
  is placed if the addition does result in an overflow.

Usage Considerations

first_operand is added to second_operand to calculate the returned result.

The following example illustrates a use of the _int16_ov_add function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16  _ov_temp;
  INT(16) a := 13;            int_16  a = 13;
  INT(16) b := 14;            int_16  b = 14;
  INT(16) x;                  int_16  x;

  x := a + b;                 x = _int16_ov_add(a,b,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_int16_ov_sub,
_int32_ov_sub,
_real32_ov_sub,
_real64_ov_sub, and
_fixed_ov_sub

The _int16_ov_sub, _int32_ov_sub, _real32_ov_sub, _real64_ov_sub, and
_fixed_ov_sub functions subtract one value from another and explicitly
check for overflow.  Explicit overflow checking is only appropriate
when the nooverflow_traps pragma is in effect.

  int_16 _int16_ov_sub(int_16 first_operand,
                       int_16 second_operand,
                       int_16 *overflow_result)

  int_32 _int32_ov_sub(int_32 first_operand,
                       int_32 second_operand,
                       int_16 *overflow_result)

  real_32 _real32_ov_sub(real_32 first_operand,
                         real_32 second_operand,
                         int_16 *overflow_result)

  real_64 _real64_ov_sub(real_64 first_operand,
                         real_64 second_operand,
                         int_16 *overflow_result)

  int_64 _fixed_ov_sub(int_64 first_operand,
                       int_64 second_operand,
                       int_16 *overflow_result)

first_operand
  is an expression that is the first operand of the subtraction operator.

second_operand
  is an expression that is the second operand of the subtraction operator.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the subtraction does not result in an overflow and the value
  1 is placed if the subtraction does result in an overflow.

Usage Considerations

second_operand is subtracted from first_operand to calculate the result.

The following example illustrates a use of the _int16_ov_sub function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16  _ov_temp;
  INT(16) a := 13;            int_16  a = 13;
  INT(16) b := 14;            int_16  b = 14;
  INT(16) x;                  int_16  x;

  x := a - b;                 x = _int16_ov_sub(a,b,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_int16_ov_mul,
_int32_ov_mul,
_real32_ov_mul,
_real64_ov_mul, and
_fixed_ov_mul

The _int16_ov_mul, _int32_ov_mul, _real32_ov_mul, _real64_ov_mul, and
_fixed_ov_mul functions multiply one value by another and explicitly
check for overflow.  Explicit overflow checking is only appropriate
when the nooverflow_traps pragma is in effect.

  int_16 _int16_ov_mul(int_16 first_operand,
                       int_16 second_operand,
                       int_16 *overflow_result)

  int_32 _int32_ov_mul(int_32 first_operand,
                       int_32 second_operand,
                       int_16 *overflow_result)

  real_32 _real32_ov_mul(real_32 first_operand,
                         real_32 second_operand,
                         int_16 *overflow_result)

  real_64 _real64_ov_mul(real_64 first_operand,
                         real_64 second_operand,
                         int_16 *overflow_result)

  int_64 _fixed_ov_mul(int_64 first_operand,
                       int_64 second_operand,
                       int_16 *overflow_result)

first_operand
  is an expression that is the first operand of the multiplication operator.

second_operand
  is an expression that is the second operand of the multiplication operator.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the multiplication does not result in an overflow and the
  value 1 is placed if the multiplication does result in an overflow.

Usage Considerations

first_operand is multiplied by second_operand to calculate the returned result.

The following example illustrates a use of the _int16_ov_mul function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16  _ov_temp;
  INT(16) a := 13;            int_16  a = 13;
  INT(16) b := 14;            int_16  b = 14;
  INT(16) x;                  int_16  x;

  x := a * b;                 x = _int16_ov_mul(a,b,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_int16_ov_div,
_int32_ov_div,
_real32_ov_div,
_real64_ov_div, and
_fixed_ov_div

The _int16_ov_div, _int32_ov_div, _real32_ov_div, _real64_ov_div, and
_fixed_ov_div functions divide one value by another and explicitly check
for overflow.  Explicit overflow checking is only appropriate when the
nooverflow_traps pragma is in effect.

  int_16 _int16_ov_div(int_16 first_operand,
                       int_16 second_operand,
                       int_16 *overflow_result)

  int_32 _int32_ov_div(int_32 first_operand,
                       int_32 second_operand,
                       int_16 *overflow_result)

  real_32 _real32_ov_div(real_32 first_operand,
                         real_32 second_operand,
                         int_16 *overflow_result)

  real_64 _real64_ov_div(real_64 first_operand,
                         real_64 second_operand,
                         int_16 *overflow_result)

  int_64 _fixed_ov_div(int_64 first_operand,
                       int_64 second_operand,
                       int_16 *overflow_result)

first_operand
  is an expression that is the first operand of the division operator.

second_operand
  is an expression that is the second operand of the division operator.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is placed
  if the division does not result in an overflow and the value 1 is placed
  if the division does result in an overflow.

Usage Considerations

first_operand is divided by second_operand to calculate the returned result.

The following example illustrates a use of the _int16_ov_div function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16  _ov_temp;
  INT(16) a := 13;            int_16  a = 13;
  INT(16) b := 14;            int_16  b = 14;
  INT(16) x;                  int_16  x;

  x := a / b;                 x = _int16_ov_div(a,b,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_int16_ov_negate
_int32_ov_negate
_fixed_ov_negate

The _int16_ov_negate, _int32_ov_negate, and _fixed_ov_negate functions
negate a value and explicitly check for overflow.  Explicit overflow
checking is only appropriate when the nooverflow_traps pragma is in effect.

  int_16 _int16_ov_negate(int_16 operand)
  int_32 _int32_ov_negate(int_32 operand)
  int_64 _fixed_ov_negate(int_64 operand)

operand
  is an expression to be negated.

Usage Considerations

operand is negated to obtain the returned result.

The following example illustrates a use of the _int16_ov_negate function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16  _ov_temp;
  INT(16) a := 13;            int_16  a = 13;
  INT(16) x;                  int_16  x;

  x := -a;                    x = _int16_ov_negate(a,
                                                   &_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_int64_to_int16_ov

The _int64_to_int16_ov function converts an int_64 value to an int_16 value
and explicitly checks for overflow.  Explicit overflow checking is only
appropriate when the nooverflow_traps pragma is in effect.

  int_16 _int64_to_int16_ov(int_64 argument
                            int_16 *overflow_result)

argument
  is the value to be converted to the return type.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the conversion does not result in an overflow and the value
  1 is placed if the conversion does result in an overflow.

Usage Considerations

The _int_64_to_int16_ov function is equivalent to the pTAL $FIXI standard
function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16   _ov_temp;
  INT(16)  i16;               int_16   i16;
  FIXED(0) f0 := 25F;         fixed_0  f0 = 25LL;

  i16 := $FIXI(f0);           i16 = (int_16) _int64_to_int16_ov
                                              (f0,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_int64_to_uint16_ov

The _int64_to_uint16_ov function converts an int_64 value to an unsigned_16
value and explicitly checks for overflow.  Explicit overflow checking is only
appropriate when the nooverflow_traps pragma is in effect.

  unsigned_16 _int64_to_uint16_ov(int_64 argument
                                  int_16 *overflow_result)

argument
  is the value to be converted to the return type.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the conversion does not result in an overflow and the value
  1 is placed if the conversion does result in an overflow.

Usage Considerations

The _int64_to_uint16_ov function is equivalent to the pTAL $FIXL standard
function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16   _ov_temp;
  INT(16)  i16;               int_16   i16;
  FIXED(0) f0 := 25F;         fixed_0  f0 = 25LL;

  i16 := $FIXL(f0);           i16 = (int_16)_int64_to_uint16_ov
                                              (f0,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_int64_to_int32_ov

The _int64_to_int32_ov function converts an int_64 value to an int_32
value and explicitly checks for overflow.  Explicit overflow checking
is only appropriate when the nooverflow_traps pragma is in effect.

  int_32 _int64_to_int32_ov(int_64 argument
                            int_16 *overflow_result)

argument
  is the value to be converted to the return type.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the conversion does not result in an overflow and the value 1
  is placed if the conversion does result in an overflow.

Usage Considerations

The _int64_to_int32_ov function is equivalent to the pTAL $FIXD standard
function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16   _ov_temp;
  INT(32)  i32;               int_32   i32;
  FIXED(0) f0 := 35F;         fixed_0  f0 = 35LL;

  i16 := $FIXD(f0);           i16 = (int_32) _int64_to_int32_ov
                                              (f0,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_real32_to_int64_ov

The _real32_to_int64_ov function converts a real_32 value to an int_64
value and explicitly checks for overflow.  Explicit overflow checking
is only appropriate when the nooverflow_traps pragma is in effect.

  int_64 _real32_to_int64_ov(real_32 argument
                             int_16 *overflow_result)

argument
  is the value to be converted to the return type.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the conversion does not result in an overflow and the value 1
  is placed if the conversion does result in an overflow.

Usage Considerations

The _real32_to_int64_ov function is equivalent to the pTAL $FIX standard
function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16   _ov_temp;
  REAL(32) r32 := 2.2E00;     real_32  r32 = 2.2E+00F;
  FIXED(0) f0;                fixed_0  f0;

  f0 := $FIX(r32);            f0 = _real32_to_int64_ov
                                           (r32,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_real64_to_int64_ov

The _real64_to_int64_ov function converts a real_64 value to an int_64
value and explicitly checks for overflow.  Explicit overflow checking
is only appropriate when the nooverflow_traps pragma is in effect.

  int_64 _real64_to_int64_ov(real_64 argument
                             int_16 *overflow_result)

argument
  is the value to be converted to the return type.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the conversion does not result in an overflow and the value 1
  is placed if the conversion does result in an overflow.

Usage Considerations

The _real64_to_int64_ov function is equivalent to the pTAL $FIX standard
function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16   _ov_temp;
  REAL(64) r64 := 2.2L00;     real_64  r64 = 2.2E+00;
  FIXED(0) f0;                fixed_0  f0;

  f0 := $FIX(r64);            f0 = _real64_to_int64_ov
                                           (r64,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_real32_to_int64_rnd_ov

The _real32_to_int64_rnd_ov function converts a real_32 value to an int_64
value, rounds the result, and explicitly checks for overflow.  Explicit
overflow checking is only appropriate when the nooverflow_traps pragma is
in effect.

  int_64 _real32_to_int64_rnd_ov(real_32 argument
                                 int_16 *overflow_result)

argument
  is the value to be converted to the return type.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the conversion does not result in an overflow and the value 1
  is placed if the conversion does result in an overflow.

Usage Considerations

The _real32_to_int64_rnd_ov function is equivalent to the pTAL $FIXR standard
function.

  pTAL Code                   Generated C++ Code

  ?ROUND
  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16   _ov_temp;
  REAL(32) r32 := 2.2E00;     real_32  r32 = 2.2E+00F;
  FIXED(0) f0;                fixed_0  f0;

  f0 := $FIXR(r32);           f0 = _real32_to_int64_rnd_ov
                                           (r32,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



_real64_to_int64_rnd_ov

The _real64_to_int64_rnd_ov function converts a real_64 value to an int_64
value, rounds the result and explicitly checks for overflow.  Explicit
overflow checking is only appropriate when the nooverflow_traps pragma is
in effect.

  int_64 _real64_to_int64_rnd_ov(real_64 argument
                                 int_16 *overflow_result)

argument
  is the value to be converted to the return type.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the conversion does not result in an overflow and the value 1
  is placed if the conversion does result in an overflow.

Usage Considerations

The _real64_to_int64_rnd_ov function is equivalent to the pTAL $FIXR standard
function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
                              int_16   _ov_temp;
  REAL(64) r64 := 2.2L00;     real_64  r64 = 2.2E+00;
  FIXED(0) f0;                fixed_0  f0;

  f0 := $FIXR(r64);           f0 = _real64_to_int64_rnd_ov
                                           (r64,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)



#endif
//-------------------------------------------------------------------------

// Interfaces for overflow check routines (implementation in roslib.tal)
#define _binary_ov_proc(outType, inType, cppName, ptalName)                \
          extern "C" _alias (ptalName) outType cppName (inType x,          \
                                                        inType y,          \
                                                        int_16 _near *ov)

#define _unary_ov_proc(outType, inType, cppName, ptalName)                 \
          extern "C" _alias (ptalName) outType cppName (inType x,          \
                                                        int_16 _near *ov)

#define _unary_proc(outType, inType, cppName, ptalName)             \
           extern "C" _alias (ptalName) outType cppName (inType x)

_binary_ov_proc (int_16,  int_16,  _int16_ov_add,  "_INT16_OV_ADD");
_binary_ov_proc (int_32,  int_32,  _int32_ov_add,  "_INT32_OV_ADD");
_binary_ov_proc (real_32, real_32, _real32_ov_add, "_REAL32_OV_ADD");
_binary_ov_proc (real_64, real_64, _real64_ov_add, "_REAL64_OV_ADD");
_binary_ov_proc (int_64,  int_64,  _fixed_ov_add,  "_FIXED_OV_ADD");
_binary_ov_proc (int_16,  int_16,  _int16_ov_sub,  "_INT16_OV_SUB");
_binary_ov_proc (int_32,  int_32,  _int32_ov_sub,  "_INT32_OV_SUB");
_binary_ov_proc (real_32, real_32, _real32_ov_sub, "_REAL32_OV_SUB");
_binary_ov_proc (real_64, real_64, _real64_ov_sub, "_REAL64_OV_SUB");
_binary_ov_proc (int_64,  int_64,  _fixed_ov_sub,  "_FIXED_OV_SUB");
_binary_ov_proc (int_16,  int_16,  _int16_ov_mul,  "_INT16_OV_MUL");
_binary_ov_proc (int_32,  int_32,  _int32_ov_mul,  "_INT32_OV_MUL");
_binary_ov_proc (real_32, real_32, _real32_ov_mul, "_REAL32_OV_MUL");
_binary_ov_proc (real_64, real_64, _real64_ov_mul, "_REAL64_OV_MUL");
_binary_ov_proc (int_64,  int_64,  _fixed_ov_mul,  "_FIXED_OV_MUL");
_binary_ov_proc (int_16,  int_16,  _int16_ov_div,  "_INT16_OV_DIV");
_binary_ov_proc (int_32,  int_32,  _int32_ov_div,  "_INT32_OV_DIV");
_binary_ov_proc (real_32, real_32, _real32_ov_div, "_REAL32_OV_DIV");
_binary_ov_proc (real_64, real_64, _real64_ov_div, "_REAL64_OV_DIV");
_binary_ov_proc (int_64,  int_64,  _fixed_ov_div,  "_FIXED_OV_DIV");

_unary_proc (int_64,  real_32, _real32_to_int64,      "_REAL32_TO_INT64");
_unary_proc (int_64,  real_64, _real64_to_int64,      "_REAL64_TO_INT64");
_unary_proc (int_64,  real_32, _real32_to_int64_rnd,  "_REAL32_TO_INT64_RND");
_unary_proc (int_64,  real_64, _real64_to_int64_rnd,  "_REAL64_TO_INT64_RND");
_unary_proc (real_32, int_32,  _int32_to_real32_rnd,  "_INT32_TO_REAL32_RND");
_unary_proc (real_64, int_32,  _int32_to_real64_rnd,  "_INT32_TO_REAL64_RND");
_unary_proc (real_32, int_64,  _int64_to_real32_rnd,  "_INT64_TO_REAL32_RND");
_unary_proc (real_64, int_64,  _int64_to_real64_rnd,  "_INT64_TO_REAL64_RND");
_unary_proc (real_32, real_64, _real64_to_real32_rnd, "_REAL64_TO_REAL32_RND");

_unary_ov_proc (int_64,  real_32, _real32_to_int64_ov, "_REAL32_TO_INT64_OV");
_unary_ov_proc (int_64,  real_64, _real64_to_int64_ov, "_REAL64_TO_INT64_OV");
_unary_ov_proc
   (int_64,  real_32, _real32_to_int64_rnd_ov, "_REAL32_TO_INT64_RND_OV");
_unary_ov_proc
   (int_64,  real_64, _real64_to_int64_rnd_ov, "_REAL64_TO_INT64_RND_OV");
_unary_ov_proc
   (real_32,  real_64, _real64_to_real32_rnd_ov, "_REAL64_TO_REAL32_RND_OV");
_unary_ov_proc (int_16,  int_64,  _int64_to_int16_ov,  "_INT64_TO_INT16_OV");
_unary_ov_proc
   (unsigned_16, int_64, _int64_to_uint16_ov, "_INT64_TO_UINT16_OV");
_unary_ov_proc (int_32,  int_64,  _int64_to_int32_ov,  "_INT64_TO_INT32_OV");
_unary_ov_proc (int_16,  int_16,  _int16_ov_negate,    "_INT16_OV_NEGATE");
_unary_ov_proc (int_32,  int_32,  _int32_ov_negate,    "_INT32_OV_NEGATE");
_unary_ov_proc (int_64,  int_64,  _fixed_ov_negate,    "_FIXED_OV_NEGATE");

#undef _binary_ov_proc
#undef _unary_ov_proc
#undef _unary_proc


//-------------------------------------------------------------------------
#if 0

_pow_of_10

The _pow_of_10 function returns a constant that is the appropriate power
of 10.

  int_64 _pow_of_10(int_16 expr)

expr
  is an expression.

Usage Considerations

The _pow_of_10 function returns 10 raised to the expr power.

This function is useful for converting a fixed-point value to a real value.
The following example converts a fixed-point value stored in the variable
f2 to a real_32 type value, and stores the resultant value in the variable
result.

  pTAL Code                   Generated C++ Code

  REAL(32) r32;               real_32 r32;
  REAL(64) r64;               real_64 r64;
  FIXED(2) f2 := 2F;          fixed_2 f2 = _scale_up(2LL,2);

  r32 := $FLT(f2);            r32 = (real_32)f2/
                                       (real_32)_pow_of_10(2);
  r64 := $EFLT(f2);           r64 = (real_64)f2/_pow_of_10(2);


#endif
//-------------------------------------------------------------------------
_resident inline int_64 _pow_of_10 (int_16 exp) {
   switch (exp) {
      case  -9: return (int_64)1000000000;
      case  -8: return (int_64)100000000;
      case  -7: return (int_64)10000000;
      case  -6: return (int_64)1000000;
      case  -5: return (int_64)100000;
      case  -4: return (int_64)10000;
      case  -3: return (int_64)1000;
      case  -2: return (int_64)100;
      case  -1: return (int_64)10;
      case   0: return (int_64)1;
      case   1: return (int_64)10;
      case   2: return (int_64)100;
      case   3: return (int_64)1000;
      case   4: return (int_64)10000;
      case   5: return (int_64)100000;
      case   6: return (int_64)1000000;
      case   7: return (int_64)10000000;
      case   8: return (int_64)100000000;
      case   9: return (int_64)1000000000;
   }

#if USE_LL
   return 0x7FFFFFFFFFFFFFFFLL;
#else
#if USE_L
#if (defined(NA_LINUX) || defined(SQ_LINUX)) && !defined(NA_64BIT)
   return -1;
#else
   return 0x7FFFFFFFFFFFFFFFL;
#endif
#else
   return 0x7FFFFFFFFFFFFFFF;
#endif
#endif
} // _pow_of_10

//-------------------------------------------------------------------------
#if 0

FIXED Data Type

The Tandem C++ language has no types equivalent to pTAL fixed point types
with a nonzero fixed point setting.  The pTAL to C++ Translator emits code
to explicitly scale 64-bit integer values in order to emulate fixed point
type operations.  For example:

  pTAL Code                   Generated C++ Code

  FIXED(0) f0 := 2.0F;        fixed_0  f0 = _scale_down(20,1);
  FIXED(1) f1 := 2.0F;        fixed_1  f1 = 20;
  FIXED(2) f2 := 2.0F;        fixed_2  f2 = _scale_up(20,1);

  f0 := f0 + 1F;              f0 = f0 + 1;
  f1 := f1 + 1F;              f1 = f1 + _scale_up(1,1);
  f2 := f2 + 1F;              f2 = f2 + _scale_up(1,2);

  f1 := f0 + 2F;              f1 = _scale_up(f0 + 2,1);
  f1 := f2 + 3F;              f1 = _scale_down(f2 +
                                            _scale_up(3,2),1);

The macros fixed_0, fixed_1, fixed_2, through fixed_1 and fixed_n1 through
fixed_n19 are aliases for int_64 which is itself an alias for long long
which is a 64-bit integer type on Tandem machines.

Scaling up multiplies the argument by a power of ten, and scaling down
divides the argument by a power of ten.


#endif
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#if 0

_point

The _point macro returns the scale factor associated with a fixed point
expression.

  int_16 _point(fixed_n expr_to_scale,
                int_16 scale_factor)

expr
  is a fixed-point type expression.

scale_factor
  is the number of decimal points by which to scale expr_to_scale.

Usage Considerations

The _point macro is intended to document a use of the pTAL $POINT standard
function.  In the following example, the variable x is assigned the value 3,
to which the macro _point evaluates.

  pTAL Code                   Generated C++ Code

  INT(16)  x;                 int_16   x;
  FIXED(3) f3;                fixed_3  f3;

  x := $POINT(f3);            x = _point(f3,3);


#endif
//-------------------------------------------------------------------------
#define _point(expr, scale) (scale)

//-------------------------------------------------------------------------
#if 0

_scale_up

The _scale_up macro scales an expression up by the given number of decimal
points.

  int_64 _scale_up(fixed_n expr_to_scale,
                   int_16 scale_factor)

expr_to_scale
  is an expression whose value is to be scaled.

scale_factor
  is an integer value that is the number of decimal points by which to scale
  expr_to_scale.

Usage Considerations

A value is scaled up by moving its decimal point to the right.

In the following example, the 64-bit integer variable f2 gets the value 4500,
the result of moving the decimal point two places to the right.

  pTAL Code                   Generated C++ Code

  FIXED(2)  f2 := 45F;        fixed_2  f2 := _scale_up(45,2)


#endif
//-------------------------------------------------------------------------
#define _scale_up(x, factor)            \
           ((x) * _pow_of_10 (factor))

//-------------------------------------------------------------------------
#if 0

_scale_up_ov

The _scale_up_ov macro scales an expression up by the given number of decimal
points and explicitly checks for overflow.

  int_64 _scale_up_ov(fixed_n expr_to_scale,
                      int_16 scale_factor
                      int_16 *overflow_result)

expr_to_scale
  is an expression whose value is to be scaled.

scale_factor
  is an integer value that is the number of decimal points by which to scale
  expr_to_scale.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is placed
  if the scaling does not result in an overflow and the value 1 is placed if
  the scaling does result in an overflow.

Usage Considerations

A value is scaled up by moving its decimal point to the right.

Explicit overflow checking is only appropriate when the nooverflow_traps
pragma is in effect.

In the following example, the 64-bit integer variable f2 gets the value 4500,
the result of moving the decimal point two places to the right.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps

                              int_16   _ov_temp;
  FIXED(2)  f2 := 45F;        fixed_2  f2 := _scale_up_ov(45,2,
                                                    &_ov_temp);
  if $OVERFLOW then ...       if (_ov_temp) ...


#endif
//-------------------------------------------------------------------------
_resident inline fixed_star _scale_up_ov (fixed_star  x,
                                          int_16      factor,
                                          int_16*     ov) {
  if (factor > 18) {
    *ov = 1;
    return 0;
  }

  return _fixed_ov_mul (x, _pow_of_10 (factor), ov);
} // _scale_up_ov

//-------------------------------------------------------------------------
#if 0

_scale_down

The _scale_down macro scales an expression down by the given number of
decimal points.

  int_64 _scale_down(fixed_n expr_to_scale,
                     int_16 scale_factor)

expr_to_scale
  is an expression whose value is to be scaled.

scale_factor
  is the number of decimal points by which to scale expr_to_scale.

Usage Considerations

A value is scaled down by moving its decimal point to the left.

In the following example, the 64-bit integer variable f0 gets the value .45,
the result of moving the decimal point two places to the left in the
expression 45.

  pTAL Code                   Generated C++ Code

  FIXED(0)  f0 := 0.45F;      fixed_0  f0 = _scale_down(45,2);


#endif
//-------------------------------------------------------------------------
_resident inline fixed_star _scale_down (fixed_star x, int_16 factor) {
  if (factor > 18)
    return 0;

  return x / _pow_of_10 (factor);
} // _scale_down

//-------------------------------------------------------------------------
#if 0

_scale_down_ov

The _scale_down_ov macro scales an expression down by the given number of
decimal points and explicitly checks for overflow.

  int_64 _scale_down_ov(fixed_n expr_to_scale,
                        int_16 scale_factor,
                        int_16 *overflow_result)

expr_to_scale
  is an expression whose value is to be scaled.

scale_factor
  is the number of decimal points by which to scale expr_to_scale.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is placed
  if the scaling does not result in an overflow and the value 1 is placed if
  the scaling does result in an overflow.

Usage Considerations

A value is scaled down by moving its decimal point to the left.

Explicit overflow checking is only appropriate when the nooverflow_traps
pragma is in effect.

In the following example, the 64-bit integer variable f2 gets the value
stored in the variable f7, scaling the value and checking for overflow.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps

                              int_16   _ov_temp;
  FIXED(2) f2 := 2.0F;        fixed_2  f2 = _scale_up(20,1);
  FIXED(7) f7;                fixed_7  f7;

  f2 := f7;                   f2 = _scale_down_ov(f7,5,
                                                  &_ov_temp);
  if $OVERFLOW then ...       if (_ov_temp) ...


#endif
//-------------------------------------------------------------------------
_resident inline fixed_star _scale_down_ov (fixed_star  x,
                                            int_16      factor,
                                            int_16*     ov) {
  if (factor > 18) {
    *ov = 0;
    return 0;
  }

  return _fixed_ov_div (x, _pow_of_10 (factor), ov);
} // _scale_down_ov

//-------------------------------------------------------------------------
#if 0

_scale_down_rnd

The _scale_down_rnd macro scales an expression down by the given number of
decimal points, applying rounding to the result.

  int_64 _scale_down_rnd(fixed_n expr_to_scale,
                         int_16 scale_factor)

expr_to_scale
  is an expression whose value is to be scaled.

scale_factor
  is the number of decimal points by which to scale expr_to_scale.

Usage Considerations

A value is scaled down by moving its decimal point to the left.

In the following example, the 64-bit integer variable f2 gets the value
stored in the variable f7, scaling the value and rounding the result.

  pTAL Code                   Generated C++ Code

  ?ROUND

  FIXED(2) f2 := 2.0F;        fixed_2  f2 = _scale_up(20,1);
  FIXED(7) f7;                fixed_7  f7;

  f2 := f7;                   f2 = _scale_down_rnd(f7,4);


#endif
//-------------------------------------------------------------------------
_resident inline fixed_star _scale_down_rnd (fixed_star x, int_16 factor) {
  if (factor < 0)
     factor = -factor;
  if ((factor - 1) > 0)
    x = x / _pow_of_10 (factor - 1);
  x = x + (x < 0 ? -5 : 5);
  return factor > 0 ? x / 10 : x;
} // _scale_down_rnd

//-------------------------------------------------------------------------
#if 0

_scale_down_rnd_ov

The _scale_down_rnd_ov macro scales an expression down by the given number
of decimal points, rounds the result and explicitly checks for overflow.

  int_64 _scale_down_rnd_ov(fixed_n expr_to_scale,
                            int_16 scale_factor,
                            int_16 *overflow_result)

expr_to_scale
  is an expression whose value is to be scaled.

scale_factor
  is the number of decimal points by which to scale expr_to_scale.

overflow_result
  is a pointer to a 16-bit integer variable into which the value 0 is
  placed if the scaling does not result in an overflow and the value 1
  is placed if the scaling does result in an overflow.

Usage Considerations

A value is scaled down by moving its decimal point to the left.

Explicit overflow checking is only appropriate when the nooverflow_traps
pragma is in effect.

In the following example, the 64-bit integer variable f2 gets the value
stored in the variable f7, scaling the value, rounding the result, and
checking for overflow.

  pTAL Code                   Generated C++ Code

  ?ROUND
  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps

                              int_16   _ov_temp;
  FIXED(2) f2 := 2.0F;        fixed_2  f2 = _scale_up(20,1);
  FIXED(7) f7;                fixed_7  f7;

  f2 := f7;                   f2 = _scale_down_rnd_ov(f7,5,
                                                   &_ov_temp);
  if $OVERFLOW then ...       if (_ov_temp) ...


#endif
//-------------------------------------------------------------------------
inline fixed_star _scale_down_rnd_ov (fixed_star x, int_16 factor, int_16 *ov) {
  if (factor < 0)
     factor = -factor;
  if ((factor - 1) > 0)
    x = x / _pow_of_10 (factor - 1);
  x = _fixed_ov_add (x, x < 0 ? -5 : 5, ov);
  return factor > 0 ? x / 10 : x;
} // _scale_down_rnd_ov


//-------------------------------------------------------------------------
#if 0

Structure Data Types

By default, the pTAL to C++ Translator translates structure elements to
straightforward C++ structure elements.  For example:

  pTAL Code                   Generated C++ Code

  STRUCT fred(*);             struct fred {
  BEGIN                         int_16      a;
    INT(16) a;                  struct {
    STRUCT ralph;                 int_32    b;
    BEGIN                         union {
      INT(32) b;                    int_16  c;
      INT(16) c;                    int_16  ccc;
      INT(16) ccc = c;            };
    END;                        } ralph;
    INT(16) d[0:3];             int_16      d[4];
  END;                        };

The pTAL to C++ Translator uses special macros to declare structure fields
whose types cannot be declared in a straightforward way in C++.  This
section describes the pTAL to C++ Translator''s translation of pTAL structure
fields that do not map directly to C++.

Inline Substructure Declarations

The TNS/R native C++ compiler uses different layout rules for inline
substructures than for substructures declared with separately declared
structure template types, when field alignment is shared2 or shared8.

The pTAL to C++ Translator generates a typedef in the structure to create
a name for the substructure type without disturbing the inline substructure
layout rules.  For example:

  pTAL Code                   Generated C++ Code

  STRUCT samantha(*);         struct samantha {
  BEGIN                         unsigned_char    i;
    STRING i;                   typedef struct {
    STRUCT tabitha;               unsigned_char  j;
    BEGIN                       } __tabitha;
      STRING j;                 __tabitha        tabitha;
    END;                      };
  END;

You can use this name whenever you need a type name for the substructure.
For example:

  sizeof(samantha::__tabitha)

Macros That Declare Structure Fields

Some pTAL constructs cannot be translated to straightforward C++ structure
elements.  These include:

- a bit array

- a larger data item overlaying a smaller data item, by equating the larger
  to the smaller, or by declaring the larger as a zero-length array

- a data item with larger alignment requirements overlaying a data item
  with smaller alignment requirements whose size is not a multiple of the
  larger alignment requirement

- an array with a nonzero lower bound

To translate these pTAL constructs to C++, the pTAL to C++ Translator emits
an invocation of a macro, defined in the rosgen.h include file, to declare
the data and access functions for each of these pTAL structure items.  For
example:

  pTAL Code                   Generated C++ Code

  STRUCT paul(*);             class paul {
  BEGIN                       public:
    UNSIGNED(4) a[0:3];         _bitarray(paul,a,4,0,3)
    INT(16)     b[0:-1];        _redef(int_16,b,x);
    STRING      x;              unsigned_char  x;
    STRING      y;              unsigned_char  y;
    INT(32)     c[-2:2];        _array(int_32,c,-2,2);
  END;                        };

The pTAL to C++ Translator s ENCAPSULATE_STRUCT option results in a
structure with macro invocations declaring data and access functions for
every field in the structure, whether or not a simple mapping to C++ exists.


#endif
//-------------------------------------------------------------------------

// This section handles mis-aligned struct items that require 4- or 8-byte
// alignment, but are only 2-byte aligned, and are accessed by member
// functions.
typedef _unsigned_32 *_p2__unsigned_32;
typedef _unsigned_32 &_r2__unsigned_32;
typedef _int_64 *_p2__int_64;
#ifdef _MSC_VER
typedef _int_64 *_p2___int64;
#endif
typedef _int_64 &_r2__int_64;
#ifdef _MSC_VER
typedef _int_64 &_r2___int64;
#endif
typedef int_32 *_p2__int_32;
typedef int_32 &_r2__int_32;
typedef int_ptr *_p2__int_ptr;
typedef int_ptr &_r2__int_ptr;
typedef real_32 *_p2__real_32;
typedef real_32 &_r2__real_32;
typedef real_64 *_p2__real_64;
typedef real_64 &_r2__real_64;
#ifdef NA_64BIT
  typedef xint_32 &_r2__xint_32;
#endif
typedef long_ptraddr_val *_p2_Long;

//------------------
// Address typedefs:
//------------------
typedef baddr *_p2_baddr;
typedef baddr *_r2_baddr;
typedef waddr *_p2_waddr;
typedef waddr *_r2_waddr;
typedef extaddr *_p2_extaddr;
typedef extaddr *_r2_extaddr;
typedef procaddr *_p2_procaddr;
typedef procaddr *_r2_procaddr;
typedef cbaddr *_p2_cbaddr;
typedef cbaddr *_r2_cbaddr;
typedef cwaddr *_p2_cwaddr;
typedef cwaddr *_r2_cwaddr;

#define _prepend_r2_(p) _r2_##p
#define _prepend_p2_(p) _p2_##p


//-------------------------------------------------------------------------
// For structure data items:
//
// The define _item declares the actual data by prepending "__" to the
// given name, and an access function of the given name.

#define _cpp_dataname(name) ___##name
#define _cpp_lowdataname(name) ___##name##_low

//-------------------------------------------------------------------------
#if 0

_item and _item2

The _item macro and the _item2 macro declare a data field in a structure
and defines an access function for that data field.

  _item(field_type,
        field_access_name)
  _item2(field_type,
         field_access_name)

field_type
  is the type of the data field.

field_access_name
  is the name of the access function for the data field.

Usage Considerations

The _item macro and the _item2 macro declare a data field in a structure and
defines an access function for that data field.

The _item2 macro declares a structure item that requires 4- or 8-byte
alignment but is only aligned on a 2-byte boundary.  This item is declared
in a structure with the FIELDALIGN(SHARED2) attribute.

The pTAL to C++ Translator generates the _item macro or the _item2 macro to
declare a data field if the ENCAPSULATE_STRUCT option is enabled during the
translation session.

The access function returns a reference to the data field, so it can be used
on both the left and right side of an assignment statement.  For example:

  pTAL Code                   Generated C++ Code

  STRUCT structtype(*);       class structtype {
  BEGIN                       public:
    INT(16) i;                  _item(int_16,i);
  END;                        };

  STRUCT fred(*)              #pragma fieldalign shared2 fred
        FIELDALIGN(SHARED2);  class fred {
  BEGIN                       public:
    INT(16) i;                  _item(int_16,i);
    INT(32) j;                  _item2(int_32,j);
  END;                        };

  STRUCT s(structtype);       structtype s;
  STRUCT f(fred);             fred f;
  ...                         ...
  s.i := 2;                   s.i() = 2;
  x := s.i;                   x = s.i();

  f.j := 10D;                 f.j() = 10;


#endif
//-------------------------------------------------------------------------
#define _item(type,name)                                 \
type _cpp_dataname(name);                                \
_resident inline type & name (void) {return _cpp_dataname(name);}

#define _item2(type,name)                                            \
type _cpp_dataname(name);                                            \
typedef _prepend_r2_(type) _r2_##type;                               \
_resident inline _r2_##type name (void) {return _cpp_dataname(name);}

//-------------------------------------------------------------------------
#if 0

_array and _array2

The _array macro and the _array2 macro declare an array data field in a
structure and defines an access function for that data field.  The array
need not have a lower bound of zero.

  _array(element_type,
         field_access_name,
         lower_bound,
         upper_bound)
  _array2(element_type,
          field_access_name,
          lower_bound,
          upper_bound)

element_type
  is the type of the array elements.

field_access_name
  is the name of the access function for the data field.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

Usage Considerations

The _array macro and the _array2 macro declare an array data field in a
structure and defines an access function for that data field.  The array
need not have a lower bound of zero.

The _array2 macro declares a structure item that requires 4- or 8-byte
alignment but is only aligned on a 2-byte boundary.  This item is declared
in a structure with the FIELDALIGN(SHARED2) attribute.

The _array macro and the _array2 macro define the constant
_lb_field_access_name, in the class implementing the enclosing structure,
which represents the lower bound of the array.  It represents the value
given as lower_bound.

The _array macro and the _array2 macro define the constant
_ub_field_access_name, in the class implementing the enclosing structure,
which represents the upper bound of the array.  It represents the value
given as upper_bound.

The pTAL to C++ Translator generates the _array macro or the _array2 macro to
declare a data field if the ENCAPSULATE_STRUCT option is enabled during the
translation session.

The access function returns a reference to the data field, so it can be
used on both the left and right side of an assignment statement.  The data
field contains cells numbered from the lower bound to the upper bound,
inclusive.  The data field cells are indexed from the lower bound to the
upper bound, inclusive.  For example:

  pTAL Code                   Generated C++ Code

  STRUCT s(*);                class s {
  BEGIN                       public:
    INT(16) alvin [5:7];        _array(int_16,alvin,5,7);
  END;                        };
  STRUCT chip(s);             s  chip;
  ...                         ...
  chip.alvin[5] := 2;         chip.alvin()[5] = 2;
  x := chip.alvin[5];         x = chip.alvin()[7];

                              for (x = chip._lb_alvin;
                                   x <= chip._ub_alvin; x++)
                                ...

#endif
//-------------------------------------------------------------------------
// The define _array declares normal (non-zero length, non-redefined) arrays,
// and defines an accessing member function.
// It subtracts the lower bound inside the accessing member function, so that
// it does not need to be subtracted at the indexing site.

#define _array(type,name,lb,ub)                   \
type _cpp_dataname(name)[((ub)-(lb))+1];          \
_resident inline type _far * name(void)           \
  {return &(_cpp_dataname(name)[(-(lb))]);};      \
enum {_num_elem_##name = (ub)-(lb)+1};            \
enum {_lb_##name = lb};                           \
enum {_ub_##name = ub}

#define _array2(type,name,lb,ub)                  \
type _cpp_dataname(name)[((ub)-(lb))+1];          \
typedef _prepend_p2_(type) _p2_##type;            \
_resident inline _p2_##type name(void)            \
  {return &(_cpp_dataname(name)[-(lb)]);};        \
enum {_num_elem_##name = (ub)-(lb)+1};            \
enum {_lb_##name = lb};                           \
enum {_ub_##name = ub}

//-------------------------------------------------------------------------
#if 0

_bitfield

The _bitfield macro declares a bit field in a structure and defines an
access function for that data field.

  _bitfield(enclosing_struct_type,
            field_access_name,
            bit_field_width)

enclosing_struct_type
  is the type name of the structure in which this bit field is declared.

field_access_name
  is the name of the access function for the data field.

bit_field_width
  is an integer constant that indicates the number of bits in the bit field.

Usage Considerations

The _bitfield macro declares a bit field in a structure and defines an
access function for that data field.

The pTAL to C++ Translator generates the _bitfield macro to declare a
data field if the ENCAPSULATE_STRUCT option is enabled during the
translation session.

The access function returns a reference to the data field, so it can be
used on both the left and right side of an assignment statement.  For
example:

  pTAL Code                   Generated C++ Code

  STRUCT s(*);                class s {
  BEGIN                       public:
    UNSIGNED(4) i;              _bitfield(s,i,4);
  END;                        };
  STRUCT sam(s);              s  sam;
  ...                         ...
  sam.i := 2;                 sam.i() = 2;
  x := sam.i;                 x = sam.i();


#endif
//-------------------------------------------------------------------------
// Unsigned pTAL items are declared using the _bitfield define, which
// declares a nested class specifically designed to allow easy access
// to a single item of the containing struct.  This class represents a
// type that these values assume during assignment; the assignment
// operator in this class is what allows the member function representing
// the bitfield to be on the left hand side of the assignment.  The
// define delcares the C++ bitfield itself, and an accessing function
// that can be used on the left or right side of an assignment.

#define _bitfield(type,name,size)                                   \
unsigned int _cpp_dataname(name):size;                              \
struct __##type##_bitfield##name;                                   \
friend struct __##type##_bitfield##name;                            \
struct __##type##_bitfield##name {                                  \
  __##type##_bitfield##name(int) {};                                \
  _resident inline operator int ()                                  \
        {return ((type _far *)this)->_cpp_dataname(name);};         \
  _resident inline __##type##_bitfield##name _far & operator=(const int i)    \
        {((type _far *)this)->_cpp_dataname(name) = i;              \
         return *this;};                                            \
};                                                                  \
_resident inline __##type##_bitfield##name &name()                  \
       {return *(__##type##_bitfield##name _far *)this;}

//-------------------------------------------------------------------------
#if 0

_bitarray _bitarray_neglb and _bigbitarray

The _bitarray macro, the _bitarray_neglb macro, and the _bigbitarray
macro each declare an array of bit fields in a structure and define access
functions for that array.  The declaration and its accompanying access
functions emulate the pTAL bit array construct, which is not available
in the C++ programming language.

  _bitarray(enclosing_struct_type,
            field_access_name,
            element_width,
            lower_bound,
            upper_bound)

  _bitarray_neglb(enclosing_struct_type,
                  field_access_name,
                  element_width,
                  lower_bound,
                  upper_bound)

  _bigbitarray(enclosing_struct_type,
               field_access_name,
               element_width,
               lower_bound,
               upper_bound)

enclosing_struct_type
  is the type name of the structure in which this bit array is declared.

field_access_name
  is the name of the access function for the data field.

element_width
  is an integer constant that specifies the number of bits in each array
  element.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

Usage Considerations

The _bitarray macro, the _bitarray_neglb macro, and the _bigbitarray
macro each declare an array of bit fields in a structure and define
access functions for that array.  This declaration and its accompanying
access functions emulate the pTAL bit array construct, which is not
available in the C++ programming language.

This C++ simulation of a pTAL bit array does not perform as well at
execution time as the original pTAL bit array construct.  You should
consider avoiding this construct in performance-critical software.

The _bitarray macro declares an array of bit fields with a lower bound
greater than or equal to zero.  The bit array may not occupy more than
32 bits.

The _bitarray_neglb macro declares an array of bit fields with a lower
bound less than zero.  The bit array may not occupy more than 32 bits.
The two nearly identical declarations are necessary to emulate pTAL bit
array structure field data layout rules.

The _bigbitarray macro declares an array of bit fields that occupies at
least 32 bits.  The bit array must occupy a multiple of 16 bits.

The data field contains cells numbered from the lower bound to the upper
bound, inclusive.  The data field cells are indexed from the lower bound
to the upper bound, inclusive.

Both the _bitarray macro, the _bitarray_neglb macro, and the _bigbitarray
macro define the access functions _deposit and _extract in the class that
implements the bit array.  You must use the _deposit and _extract functions
to store and retrieve values to and from cells in the bit field.

For example, the following code declares a structure field i that is an
array of four cells, indexed from 2 to 5 inclusively, each of which is
four bits wide.

  pTAL Code                   Generated C++ Code

  STRUCT structtype(*);       class structtype {
  BEGIN                       public:
    UNSIGNED(4) i [2:5];        _bitarray(structtype,i,4,2,5);
  END;                        };
  STRUCT s(structtype);       structtype  s;
  ...                         ...
  s.i[2] := 2;                s.i()._deposit(2,2);
  x := s.i[5];                x = s.i()._extract(5);

The following code declares a structure field i that is an array of six
cells, indexed from -2 to 3 inclusively, each of which is four bits wide.

  pTAL Code                   Generated C++ Code

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    UNSIGNED(4) i [-2:3];       _bitarray_neglb(
  END;                                   stype,i,4,-2,3);
                              };
  STRUCT s(stype);            stype  s;
  ...                         ...
  s.i[-2] := 2;               s.i()._deposit(-2,2);
  x := s.i[3];                x = s.i()._extract(3);

The following code declares a structure field fred that is an array of
16 cells, indexed from 0 to 15 inclusively, each of which is four bits wide.

  pTAL Code                   Generated C++ Code

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    UNSIGNED(4) fred [0:15];    _bigbitarray(
  END;                                   stype,fred,4,0,15);
                              };
  STRUCT s(stype);            stype  s;
  ...                         ...
  s.fred[3] := 2;             s.fred()._deposit(3,2);
  x := s.fred[10];            x = s.fred()._extract(10);

Constants

The _bitarray macro, the _bitarray_neglb macro, and the _bigbitarray
macro define the following constants:

- _width_field_access_name, defined in the class implementing the enclosing
  structure, represents the number of bits in each cell of the bit array.
  It represents the value given as element_width.

- _lb_field_access_name, defined in the class implementing the enclosing
  structure, represents the lower bound of the bit array.  It represents
  the value given as lower_bound.

- _ub_field_access_name, defined in the class implementing the enclosing
  structure, represents the upper bound of the bit array.  It represents
  the value given as upper_bound.

In the following example, the _bitarray macro creates a constant called
structtype::_lb_i representing the lower bound of the bit array field i in
the class structtype.  It also creates a constant called structtype::_ub_i
representing the upper bound of the bit array field i in the class structtype,
and a constant called structtype::_width_i representing the number of bits
in each cell of the bit array field i in the class structtype.

  class structtype {
  public:
    _bitarray(structtype,i,4,2,5);
  } s;
  for (x = s._lb_i; x <= s._lb_i; x++)



_deposit

The _deposit function, which is a member function defined by invoking the
_bitarray macro, the _bitarray_neglb macro, or the _bigbitarray macro,
deposits a value at a particular index in the bit array field.

  void _deposit(int index,
                unsigned int value)

index
  is an index into the bit array.

value
  is the value to be deposited at the given index in the bit array.

In the following example, the field opie is a bit array indexed from -1
to 3, in which each cell is four bits wide.  The value 5 is deposited
into cell 1 of the bit array field opie.

  pTAL Code                         Generated C++ Code

  STRUCT sheriff(*);                class sheriff {
  BEGIN public:
    UNSIGNED(4) opie[-1,3];            _bitarray(sheriff,opie,4,-1,3);
  END;                              };
  STRUCT andy(sheriff);             sheriff  andy;
  ...                               ...
  andy.opie[1] := 5;                andy.opie()._deposit(1,5);



_extract

The _extract function, which is a member function defined by invoking the
_bitarray macro, the _bitarray_neglb macro, or the _bigbitarray macro,
extracts a value from a particular index in the bit array field.

  unsigned int _extract(int index)

index
  is an index into the bit array.

The _extract function returns the value stored in the bit array at index index.
In the following example, the field barney is a bit array indexed from 0
to 3, in which each cell is four bits wide.  The value extracted from
cell 0 of the bit array field barney is stored in the variable x.

pTAL Code                       Generated C++ Code

LITERAL BULLET = 1;             enum { BULLET = 1};
STRUCT deputy(*);               class deputy {
BEGIN                           public:
  UNSIGNED(4) pocket[0:3];        _bitarray(deputy,pocket,4,0,3);
END;                            };
STRUCT barney(deputy);          deputy  barney;
...                             ...
x := barney.pocket[BULLET];     x = barney.pocket()._extract(BULLET);


#endif
//-------------------------------------------------------------------------
// declare array of bitfields

#define _bitarray(type,name,width,lb,ub)                                   \
enum {_num_elem_##name = (ub)-(lb)+1};                                     \
enum {_lb_##name = lb};                                                    \
enum {_ub_##name = ub};                                                    \
enum {_width_##name = width};                                              \
unsigned int _cpp_dataname(name):(((ub)-(lb))+1)*(width);                  \
struct __##type##_bitarray##name;                                          \
friend struct __##type##_bitarray##name;                                   \
struct __##type##_bitarray##name {                                         \
  __##type##_bitarray##name(int) {};                                       \
  _resident inline operator int ()                                         \
        {return ((type _far *)this)->_cpp_dataname(name);};                \
  _resident inline unsigned int _extract(int i) {                          \
      return (_extract_bits(((type _far *)this)->_cpp_dataname(name),      \
                    ((((_ub_##name) - (_lb_##name)) + 1) * _width_##name), \
                               ((i - _lb_##name) * _width_##name),         \
                               (((i - _lb_##name) + 1) * _width_##name - 1)));\
      };                                                                   \
  _resident inline void _deposit(int i,unsigned int v) {                   \
      _deposit_bits(((type _far *)this)->_cpp_dataname(name),              \
                    ((((_ub_##name) - (_lb_##name)) + 1) * _width_##name), \
                       (unsigned int_16)v,                                 \
                       ((i - _lb_##name) * _width_##name),                 \
                       (((i - _lb_##name) + 1) * _width_##name - 1));      \
      };                                                                   \
  _resident inline __##type##_bitarray##name _far & operator=(const int i) \
        {((type _far *)this)->_cpp_dataname(name) = i;                     \
         return *this;};                                                   \
};                                                                         \
_resident inline __##type##_bitarray##name &name()                         \
       {return *(__##type##_bitarray##name _far *)this;}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#define _bitarray_neglb(type,name,width,lb,ub)                             \
enum {_lb_##name = lb};                                                    \
enum {_ub_##name = ub};                                                    \
enum {_width_##name = width};                                              \
unsigned int _cpp_lowdataname(name):(-(lb))*width;                         \
unsigned int _cpp_dataname(name):((ub)+1)*(width);                         \
struct __##type##_bitarray##name;                                          \
friend struct __##type##_bitarray##name;                                   \
struct __##type##_bitarray##name {                                         \
  __##type##_bitarray##name(int) {};                                       \
  _resident inline operator int ()                                         \
        {return ((type _far *)this)->_cpp_dataname(name);};                \
  _resident inline unsigned_16 _extract(int i) {                           \
    if (i < 0) {                                                           \
      return (_extract_bits(                                               \
                    ((type _far *)this)->_cpp_lowdataname(name),           \
                     ((-(_lb_##name)) * _width_##name),                    \
                     ((i - _lb_##name) * _width_##name),                   \
                     (((i - _lb_##name) + 1) * _width_##name - 1)));       \
    } else {                                                               \
      return (_extract_bits(                                               \
                    ((type _far *)this)->_cpp_dataname(name),              \
                     (((_ub_##name) + 1) * _width_##name),                 \
                     (i * _width_##name),                                  \
                     (((i + 1) * _width_##name) - 1)));                    \
    }                                                                      \
  };                                                                       \
  _resident inline void _deposit(int i,unsigned_16 v) {                    \
    if (i < 0) {                                                           \
      _deposit_bits(((type _far *)this)->_cpp_lowdataname(name),           \
                    ((-(_lb_##name)) * _width_##name),                     \
                    (unsigned_16)v,                                        \
                    ((i - _lb_##name) * _width_##name),                    \
                    (((i - _lb_##name) + 1) * _width_##name - 1));         \
    } else {                                                               \
      _deposit_bits(((type _far *)this)->_cpp_dataname(name),              \
                    (((_ub_##name) + 1) * _width_##name),                  \
                    (unsigned_16)v,                                        \
                    (i * _width_##name),                                   \
                    (((i + 1) * _width_##name) - 1));                      \
    }                                                                      \
  };                                                                       \
  _resident inline __##type##_bitarray##name _far & operator=(const int i) \
        {((type _far *)this)->_cpp_dataname(name) = i;                     \
         return *this;};                                                   \
};                                                                         \
_resident inline __##type##_bitarray##name &name()                         \
       {return *(__##type##_bitarray##name _far *)this;}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#define _bigbitarray(type,name,width,lb,ub)                                \
enum {_lb_##name = lb};                                                    \
enum {_ub_##name = ub};                                                    \
enum {_width_##name = width};                                              \
unsigned short _cpp_dataname(name)[((((ub)-(lb))+1)*(width)+15)/16];       \
struct __##type##_bitarray##name;                                          \
friend struct __##type##_bitarray##name;                                   \
struct __##type##_bitarray##name {                                         \
  _resident inline unsigned int _extract(int i) {                          \
      return _extract_bits((((type _far *)this)->                          \
                   _cpp_dataname(name)[((i-(lb))*(width))/16]),            \
                   16,                                                     \
                   (((i-(lb))*(width)) % 16),                              \
                   ((((i-(lb))*(width)) % 16)+(width)-1));                 \
      };                                                                   \
  _resident inline void _deposit(int i,unsigned int v) {                   \
      _deposit_bits((((type _far *)this)->                                 \
                   _cpp_dataname(name)[((i-(lb))*(width))/16]),            \
                   16,                                                     \
                   v,                                                      \
                   (((i-(lb))*(width)) % 16),                              \
                   ((((i-(lb))*(width)) % 16)+(width)-1));                 \
      };                                                                   \
};                                                                         \
_resident inline __##type##_bitarray##name &name()                         \
       {return *(__##type##_bitarray##name _far *)this;}
/////////////////

//-------------------------------------------------------------------------
#if 0

_coerce

The _coerce macro creates a variable into which the result of a conversion
with the _xadr macro can be assigned.  It makes type conversion within a
macro body more attractive than straightforward type casts would be.

  _coerce(type,var)

type
  is the type of variable that would make var assignment compatible with
  the left-hand-side of the assignment.

var
  is a variable into which the assignment is to take place.

Usage Considerations

The _coerce macro operates on the variable which resides on the
left-hand-side of an assignment statement.  It appears in macro bodies
when the left-hand-side of the assignment statement is contained in a
macro actual parameter and the right-hand-side is a call to _xadr.
It exists to improve the presevation of the macro body during translation.

  pTAL Code                   Generated C++ Code

  define gets(x,y)            #define gets(x,y)            \
        = @x := y#;                 _coerce(void *,x) = y

  ...                         ...
  gets(a,$extaddr(fred));     gets(a,_xadr(fred));


#endif
//-------------------------------------------------------------------------
#if 0

_redef and _redef2

The _redef macro and the _redef2 macro declare a data field that overlays
another data field in a structure.  It defines an access function for that
data field.  The declaration allocates no data.

  _redef(field_type,
         field_access_name,
         space_allocating_data_name)
  _redef2(field_type,
          field_access_name,
          space_allocating_data_name)

field_type
  is the type of the data field.

field_access_name
  is the name of the access function for the data field.

space_allocating_data_name
  is the name of a space-allocating data field which this field will overlay.

Usage Considerations

The _redef macro and the _redef2 macro declare a data field that overlays
another data field in a structure.  It defines an access function for that
data field.  The declaration allocates no data.

The _redef2 macro declares a structure item that requires 4- or 8-byte
alignment but is only aligned on a 2-byte boundary.  This item is declared
in a structure with the FIELDALIGN(SHARED2) attribute.

A C++ union is the preferred way to declare structure data fields that
overlay one another, but not all pTAL redefinition and zero-length array
fields can be declared using a union.  The pTAL to C++ Translator generates
an invocation of the _redef macro when it cannot generate a correct C++ union.

The access function returns a reference to the data field, so it can be used
on both the left and right side of an assignment statement.

In the following example, the fields a and b each allocate 16 bits.  The
field jed begins at the same place as field a and extends for 32 bits.

  pTAL Code                   Generated C++ Code

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    INT(16) a;                  int_16 a;
    INT(16) b;                  int_16 b;
    INT(32) jed = a;            _redef(int_32,jed,a);
  END;                        };
  STRUCT s(stype);            stype  s;
  ...                         ...
  s.jed := 13;                s.jed() = 13;
  x := s.jed;                 x = s.jed();

In the following example, the fields huey, dewey, and louie each allocate
16 bits.  The field x begins at the same place as field dewey and extends
for 32 bits.  Field x is a 4-byte field that is not aligned on a 4-byte
boundary.

  pTAL Code                   Generated C++ Code

                              #define set_fieldalign
  ?FIELDALIGN(SHARED2)        #pragma fieldalign shared2

  STRUCT dtype(*);            class dtype {
  BEGIN                       public:
    INT(16) huey;               int_16 huey;
    INT(32) x[0:-1];            _redef2(int_32,x,dewey);
    INT(16) dewey;              int_16 dewey;
    INT(16) louie;              int_16 louie;
  END;                        };
  STRUCT d(dtype);            dtype  d;
  ...                         ...
  d.x := 13D;                 d.x() = 13;
  x := d.x;                   x = d.x();


#endif
//-------------------------------------------------------------------------
// Redefinition is handled by creating a member function that takes
// the address of the defining item, and uses it to form a pointer
// or reference to the appropriate type.  Redefinitions of non-arrays
// use the _redef define.  Redefinitions of arrays use the _redefarray
// define.

#define _coerce(type,var) (*((type _far *)(&var)))

#define _redef(type,name1,name2)                             \
   _resident inline type _far & name1 (void) {return _coerce(type,name2);}

#define _redef2(type,name1,name2)                                        \
typedef _prepend_r2_(type) _r2_##type;                                   \
   _resident inline _r2_##type name1 (void) {return _coerce(type,name2);}

//-------------------------------------------------------------------------
#if 0

_to

The _to macro converts a field access name of a macro-defined structure
field into the name of the actual data item, which is normally known only
to the access function.

When defining a field via _redef, _redef2, _redefarray, _redefarray2,
_redefafter, _redefafter2, _redefarrayafter or _redefarrayafter2, you
must supply the name of the space allocating item that the new field will
overlay.  If that space-allocating field were defined using a special
macro _item, or _array, you would use the _to macro to create the name of
that space-allocating item.

  _to(field_access_name)

field_access_name
  is the name of the access function for the space-allocating data field.

In the following example, the fields a, b, and c each allocate 16 bits.
The array field sam begins at the same place as field a and extends for
three 16-bit cells.

  pTAL Code                   Generated C++ Code

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    INT(16) a;                  _item(int_16,a);
    INT(16) b;                  _item(int_16,b);
    INT(16) c;                  _item(int_16,c);
    INT(16) sam[0:2] = a;       _redefarray(int_16,sam,0,2,
  END;                                      _to(a));
                              };
  STRUCT s(stype);            stype  s;
  ...                         ...
  s.sam[0] := 13;             s.sam()[0] = 13;
  x := s.sam[2];              x = s.sam()[2];


#endif
//-------------------------------------------------------------------------
#define _to(fl) _cpp_dataname(fl)

//-------------------------------------------------------------------------
#if 0

_redefarray and _redefarray2

The _redefarray macro and the _redefarray2 macro declare an array data
field that overlays another data field in a structure.  It defines an
access function for that array.  The declaration allocates no data.

  _redefarray(field_type,
              field_access_name,
              lower_bound,
              upper_bound,
              space_allocating_data_name)
  _redefarray2(field_type,
               field_access_name,
               lower_bound,
               upper_bound,
               space_allocating_data_name)

field_type
  is the type of the data field.

field_access_name
  is the name of the access function for the data field.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

space_allocating_data_name
  is the name of a space-allocating data field which this field will overlay.

Usage Considerations

The _redefarray macro and the _redefarray2 macro declare an array data
field that overlays another data field in a structure.  It defines an
access function for that array.  The declaration allocates no data.

The _redefarray2 macro declares a structure item that requires 4- or 8-byte
alignment but is only aligned on a 2-byte boundary.  This item is declared
in a structure with the FIELDALIGN(SHARED2) attribute.

A C++ union is the preferred way to declare structure data fields that
overlay one another, but not all pTAL redefinition and zero-length array
fields can be declared using a union.  The pTAL to C++ Translator
generates an invocation of the _redefarray macro when it cannot generate
a correct C++ union.

The access function returns a reference to the array, so it can be used
on both the left and right side of an assignment statement.

The _redefarray macro and the _redefarray2 macro define the constant
_lb_field_access_name, in the class implementing the enclosing structure,
which represents the lower bound of the array.  It represents the value
given as lower_bound.

The _redefarray macro and the _redefarray2 macro define the constant
_ub_field_access_name, in the class implementing the enclosing structure,
which represents the upper bound of the array.  It represents the value
given as upper_bound.

In the following example, the fields a and b each allocate 16 bits.
The array field sam begins at the same place as field a and extends
for two 16-bit cells.

  pTAL Code                   Generated C++ Code

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    INT(16) a;                  int_16 a;
    INT(16) b;                  int_16 b;
    INT(16) sam[0:1] = a;       _redefarray(int_16,sam,0,1,a);
  END;                        };
  STRUCT s(stype);            stype  s;
  s.sam[0] := 13;             s.sam()[0] = 13;
  x := s.sam[1];              x = s.sam()[1];

In the following example, the fields a, b, c, d, and e each allocate 16 bits.
The array joe begins at the same place as field b and extends for two 32-bit
cells.  Array joe has 4-byte cells that are not aligned on a 4-byte boundary.

  pTAL Code                   Generated C++ Code

                              #define set_fieldalign
  ?FIELDALIGN(SHARED2)        #pragma fieldalign shared2

  STRUCT jtype(*);            class jtype {
  BEGIN                       public:
    INT(16) a;                  int_16 a;
    INT(16) b;                  int_16 b;
    INT(16) c;                  int_16 c;
    INT(16) d;                  int_16 d;
    INT(16) e;                  int_16 e;
    INT(32) joe[0:1] = b;       _redefarray2(int_32,joe,0,1,b);
  END;                        };
  STRUCT j(jtype);            jtype  j;
  j.joe[0] := 17D;            j.joe()[0] = 13;
  x := j.joe[1];              x = j.joe()[1];

The _redefarray macro or the _redefarray2 macro can be used to declare an
array with a nonzero lower bound.  That array s lower bound is aligned with
the zero cell (whether it is allocated or not) of the space allocating data
item that it overlays.  In the following example, the array field b aligns
its lower bound with the zero cell of field a.  The cell b[3] is aligned
with the cell a[0].

  pTAL Code                   Generated C++ Code

  STRUCT ttype(*);            class ttype {
  BEGIN                       public:
    INT(16) a[0:4];             int_16 a[5];
    INT(16) b[3:6] = a;         _redefarray(int_16,b,3,6,a);
  END;                        };

  STRUCT t(ttype);            ttype  t;
  s.a[0] := 13;               s.a[0] = 13;
  x := s.b[2];                x = s.b()[2];


#endif
//-------------------------------------------------------------------------
#define _redefarray(type,name1,lb,ub,name2)                  \
_resident inline type _far * name1 (void)                    \
   {return (type _far *)                                     \
           &(((type _far *)&name2)[(-(lb))]);};                \
enum {_num_elem_##name1 = (ub)-(lb)+1};                      \
enum {_lb_##name1 = lb};                                     \
enum {_ub_##name1 = ub}

#define _redefarray2(type,name1,lb,ub,name2)                 \
typedef _prepend_p2_(type) _p2_##type;                       \
_resident inline _p2_##type name1 (void)                     \
   {return (type _far *)                                     \
           &(((type _far *)&name2)[-(lb)]);};                \
enum {_num_elem_##name1 = (ub)-(lb)+1};                      \
enum {_lb_##name1 = lb};                                     \
enum {_ub_##name1 = ub}

//-------------------------------------------------------------------------
#if 0

_tobitfield

The _tobitfield macro specifies a space-allocating bit field or bit array item.

When defining a field via _redef, _redef2, _redefarray or _redefarray2, you
must supply the name of the space allocating item that the new field will
overlay.  If that space-allocating item were a bit field or a bit array
then you would use the _tobitfield macro to specify the bit field
space-allocating item that the new field is to overlay.

  _tobitfield(enclosing_struct_type,
              space_allocating_data_name)

enclosing_struct_type
  is the type name of the structure in which this bit array is declared.

space_allocating_data_name
  is the name of a space-allocating data field which this field will overlay.

In the following example, the field a overlays the data fields beginning
with i.  The field b overlays the data field z.

  pTAL Code                   Generated C++ Code

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    INT(16) a[0:-1];            _redef(int_16,a,
    UNSIGNED(2) i;                  _tobitfield(stype,i));
    UNSIGNED(2) j;              unsigned_16 i:2;
    UNSIGNED(2) k;              unsigned_16 j:2;
    BIT_FILLER 10;              unsigned_16 k:2;
    INT(16) b[0:-1];            unsigned_16 filler:10
    UNSIGNED(2) z[0:8];         _redef(int_16,b,
  END;                              _tobitfield(stype,_to(z)));
                                _bitarray(stype,z,2,0,8);
                              };
  STRUCT s(stype);            stype  s;
  ...                         ...
  s.a[0] := 13;               s.a() = 13;

If the space-allocating item were defined with a macro, then you would
use the _to macro to obtain the name of the actual data item, given the
name of the access function.

  pTAL Code                   Generated C++ Code

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    INT(16) a[0:-1];            _redef(int_16,a,
    UNSIGNED(2) i;                  _tobitfield(stype,_to(i)));
    UNSIGNED(2) j;              _bitfield(int_16,i);
    UNSIGNED(2) k;              _bitfield(int_16,j);
    INT(16) b[0:-1];            _bitfield(int_16,k);
    UNSIGNED(2) z[0:8];         _redef(int_16,b,
  END;                              _tobitfield(stype,_to(z)));
                                _bitarray(stype,z,2,0,8);
                              };
  STRUCT s(stype);            stype  s;
  ...                         ...
  s.a[0] := 13;               s.a() = 13;


#endif
//-------------------------------------------------------------------------
// Beware: Tandem C++ does not tolerate parens around _bitoffset arguments
#ifdef _MSC_VER
#define _bitoffset(x,y)  0
#elif defined(__linux__)
#define _bitoffset(x,y)  0 // TODO __linux__
#endif
#define _tobitfield(st,fl) (((unsigned_char *)this)[_bitoffset(st,fl)/8])

//-------------------------------------------------------------------------
#if 0

_redefafter and _redefafter2

The _redefafter macro and the _redefafter2 macro declare a data field that
overlays the data field following the field specified in this declaration
macro.  It defines an access function for that data field.  The declaration
allocates no data.

  _redefafter(field_type,
              field_access_name,
              space_allocating_data_name)
  _redefafter2(field_type,
               field_access_name,
               space_allocating_data_name)

field_type
  is the type of the data field.

field_access_name
  is the name of the access function for the data field.

space_allocating_data_name
  is the name of a space-allocating data field after which resides the
  field which this field will overlay.

Usage Considerations

The _redefafter macro and the _redefafter2 macro declare a data field that
overlays the data field following the field specified in this declaration
macro.  It defines an access function for that data field.  The declaration
allocates no data.

The _redefafter2 macro declares a structure item that requires 4- or 8-byte
alignment but is only aligned on a 2-byte boundary.  This item is declared
in a structure with the FIELDALIGN(SHARED2) attribute.

A C++ union is the preferred way to declare structure data fields that
overlay one another, but not all pTAL redefinition and zero-length array
fields can be declared using a union.  The pTAL to C++ Translator generates
an invocation of the _redefafter macro when it cannot generate a correct
C++ union, and when an invocation of _redef might lead to a textual mismatch
in the translation of a define or file section.

The access function returns a reference to the data field, so it can be
used on both the left and right side of an assignment statement.

In the following example, the fields a, c and d each allocate 16 bits.
The field b begins at the same place as field c in structure stype and
begins at the same place as field d in structure ttype.  Field b extends
for 16 bits.

  pTAL Code                   Generated C++ Code

  define fields =             #define fields             \
    INT(16) a;                  int_16 a;                \
    INT(16) b[0:-1]#;           _redefafter(int_16,b,a)

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    fields;                     fields;
    INT(16) c;                  int_16 c;
  END;                        };
  STRUCT ttype(*);            class ttype {
  BEGIN                       public:
    fields;                     fields;
    INT(16) d;                  int_16 d;
  END;                        };

  STRUCT s(stype);            stype  s;
  ...                         ...
  s.b := 13;                  s.b() = 13;
  x := s.b;                   x = s.b();

In the following example, the field x allocates 16 bits and field z
allocates 32 bits.  The field k begins at the same place as field y
in structure ytype and begins at the same place as field z in structure
ztype.  Field k extends for 32 bits.  Field k is a 4-byte field that is
not aligned on a 4-byte boundary.

  pTAL Code                   Generated C++ Code

                              #define set_fieldalign
  ?FIELDALIGN(SHARED2)        #pragma fieldalign shared2

  define fields =             #define fields             \
    INT(16) x;                  int_16 x;                \
    INT(32) k[0:-1]#;           _redefafter2(int_32,k,x)

  STRUCT ytype(*);            class ytype {
  BEGIN                       public:
    fields;                     fields;
    INT(32) y;                  int_32 y;
  END;                        };
  STRUCT ztype(*);            class ztype {
  BEGIN                       public:
    fields;                     fields;
    INT(32) z;                  int_32 z;
  END;                        };

  STRUCT y(ytype);            ytype  y;
  ...                         ...
  y.k := 13D;                 y.k() = 13;
  x := y.k;                   x = y.k();


#endif
//-------------------------------------------------------------------------
#define _redefafter(type,name1,name2)              \
_resident inline type _far & name1 (void)          \
   {return _coerce(type,(*(((unsigned_char *)&name2)+sizeof(name2))));}

#define _redefafter2(type,name1,name2)                    \
typedef _prepend_r2_(type) _r2_##type;                    \
_resident inline _r2_##type name1 (void)                  \
   {return _coerce(type,(*(((unsigned_char *)&name2)+sizeof(name2))));}

//-------------------------------------------------------------------------
#if 0

_redefarrayafter and _redefarrayafter2

The _redefarrayafter macro and the _redefarrayafter2 macro declare an
array data field that overlays the data field following the field specified
in this declaration macro.  It defines an access function for that array.
The declaration allocates no data.

  _redefarrayafter(field_type,
                   field_access_name,
                   lower_bound,
                   upper_bound,
                   space_allocating_data_name)
  _redefarrayafter2(field_type,
                    field_access_name,
                    lower_bound,
                    upper_bound,
                    space_allocating_data_name)

field_type
  is the type of the data field.

field_access_name
  is the name of the access function for the data field.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

space_allocating_data_name
  is the name of a space-allocating data field after which resides the
  field which this field will overlay.

Usage Considerations

The _redefarrayafter macro and the _redefarrayafter2 macro declare an
array data field that overlays the data field following the field specified
in this declaration macro.  It defines an access function for that array.
The declaration allocates no data.

The _redefarrayafter2 macro declares a structure item that requires
4- or 8-byte alignment but is only aligned on a 2-byte boundary.  This
item is declared in a structure with the FIELDALIGN(SHARED2) attribute.

A C++ union is the preferred way to declare structure data fields that
overlay one another, but not all pTAL redefinition and zero-length array
fields can be declared using a union.  The pTAL to C++ Translator generates
an invocation of the _redefarrayafter macro when it cannot generate a
correct C++ union, and when an invocation of _redefarray might lead to a
textual mismatch in the translation of a define or file section.

The access function returns a reference to the array, so it can be used
on both the left and right side of an assignment statement.

The _redefarrayafter macro and the _redefarrayafter2 macro define the
constant _lb_field_access_name, in the class implementing the enclosing
structure, which represents the lower bound of the array.  It represents
the value given as lower_bound.

The _redefarrayafter macro and the _redefarrayafter2 macro define the
constant _ub_field_access_name, in the class implementing the enclosing
structure, which represents the upper bound of the array.  It represents
the value given as upper_bound.

In the following example, the fields a and c each allocate four 16-bit cells.
The array field b begins at the same place as field c.

  pTAL Code                   Generated C++ Code

  define fields =             #define fields             \
    INT(16) a[0:3];             int_16 a[4];             \
    INT(16) b[0:-1]#;           _redefarrayafter(int_16,b,a)

  STRUCT stype(*);            class stype {
  BEGIN                       public:
    fields;                     fields;
    INT(16) c[0:3];             INT(16) c[0:3];
  END;                        };
  STRUCT s(stype);            stype  s;

  s.b[0] := 13;               s.b()[0] = 13;
  x := s.b[1];                x = s.b()[1];

In the following example, the array a allocates four 16-bit cells and the
array c allocates four 32-bit cells.  The array b begins at the same place
as field c.  Array b has 4-byte cells that are not aligned on a 4-byte
boundary.

  pTAL Code                   Generated C++ Code

                              #define set_fieldalign
  ?FIELDALIGN(SHARED2)        #pragma fieldalign shared2

  define fields =             #define fields             \
    INT(16) a[0:3];             int_16 a[4];             \
    INT(32) b[0:-1]#;           _redefarrayafter2(int_32,b,a)


  STRUCT ttype(*);            class ttype {
  BEGIN                       public:
    fields;                     fields;
    INT(32) c[0:3];             int_32 c[4];
  END;                        };
  STRUCT t(ttype);            ttype  t;

  t.b[0] := 13D;              t.b()[0] = 13;
  x := t.b[1];                x = t.b()[1];


#endif
//-------------------------------------------------------------------------
#define _redefarrayafter(type,name1,lb,ub,name2)                       \
_resident inline type _far * name1 (void)                              \
   {return (type _far *)                                               \
       (((unsigned_char *)&(((type _far *)&name2)[(-(lb))]))+sizeof(name2));};\
enum {_num_elem_##name1 = (ub)-(lb)+1};                                \
enum {_lb_##name1 = lb};                                               \
enum {_ub_##name1 = ub}

#define _redefarrayafter2(type,name1,lb,ub,name2)                      \
typedef _prepend_p2_(type) _p2_##type;                                 \
_resident inline _p2_##type name1 (void)                               \
   {return (type _far *)                                               \
      (((unsigned_char *)&(((type _far *)(&name2))[(-(lb))]))+sizeof(name2));};\
enum {_num_elem_##name1 = (ub)-(lb)+1};                                \
enum {_lb_##name1 = lb};                                               \
enum {_ub_##name1 = ub}


//-------------------------------------------------------------------------
#if 0

Array Data Types

The pTAL to C++ Translator translates array variables with a zero lower
bound in a straightforward way.  For example:

  pTAL Code                   Generated C++ Code

  STRUCT norton[0:9];         struct __norton {
  BEGIN                         int_16   a;
    INT(16) a;                  int_16   b[10];
    INT(16) b [0:9];          } norton [10];
  END;

  STRING ralph [0:9]          char ralph [10]
    := [0,1,2,3,4,5,6,7,8,9];    = {0,1,2,3,4,5,6,7,8,9};

The pTAL to C++ Translator uses special macros to declare array variables
with nonzero lower bounds and read-only arrays.  This subsection describes
these macros.

Array With a Nonzero Lower Bound

The pTAL language allows you to declare an array with a lower bound that
is any constant value.  The C++ language requires that arrays have a lower
bound of zero.  The pTAL to C++ Translator generates macros that expand to
array types in variable declarations to emulate arrays with nonzero lower
bounds.

Read-Only Array

The pTAL to C++ Translator emits macro invocations to declare read-only
arrays with a nonzero lower bound.


#endif
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
#if 0

_array_type

The _array_type macro defines an array type with nonzero lower bounds.

  _array_type(element_type,
              lower_bound,
              upper_bound)

element_type
  is the type of the array elements.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

Usage Considerations

The _array_type macro defines an array type with nonzero lower bounds.  An
invocation of this macro is used as the type in a variable definition.  The
_array_type macro expands to a class containing a data item and access
functions for the array container.

Use the _struct_array_type macro to define an array with a nonzero lower
bound whose elements are structures.

Access to the array variable requires no special syntax.

The following example defines an array type variable, fred, which has
six int_16 type cells, indexed from -2 to 3 inclusively.

  pTAL Code                   Generated C++ Code

  INT(16) PROC TOP_HAT;       extern "C" int_16 TOP_HAT()
  BEGIN                       {
    INT(16) x;                  int_16 x;
    INT(16) fred[-2:3];         _array_type(int_16,-2,3) fred;
    ...                         ...
    fred[-2] := 13;             fred[-2] = 13;
    x := fred[0];               x = fred[0];
  END;                        }


#endif
//-------------------------------------------------------------------------
template <class type, const int lb, const int ub>  class _at_template {
    public:
      _resident inline type& operator [] (int i) { return __data [i - (lb)]; };
      _resident inline type* operator & () { return &__data [-(lb)]; };
      _resident inline operator type * () { return &__data [-(lb)]; };
      /* _resident inline operator void * () { return &__data [-(lb)]; }; */
    private:
      type __data [(ub) - (lb) + 1];
  };

#define _array_type(type, lb, ub) _at_template<type,lb,ub>

//-------------------------------------------------------------------------
#if 0

_struct_array_type

The _struct_array_type macro defines an array type with nonzero lower bounds
whose elements are structures.

  _struct_array_type(element_type,
                     lower_bound,
                     upper_bound)

element_type
  is the type of the array elements.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

Usage Considerations

An invocation of this macro is used as the type in a variable definition.

The _struct_array_type macro expands to a class containing a data item and
access functions for the array container.  This class differs from that
defined by the _array_type macro in that the pointer dereference and field
access operator -> is defined for structure elements.

The following example defines an array type variable, ginger, which has four
structure g type cells, indexed from 2 to 5 inclusively.  It is necessary to
define the structure type before declaring the array of structures.  Access
to the array variable requires no special syntax.

  pTAL Code                   Generated C++ Code

  INT(16) PROC STAGE_DOOR;    extern "C" int_16 STAGE_DOOR()
  BEGIN                       {
    INT(16) x;                  int_16 x;
    STRUCT k(*);                struct k{
    BEGIN                         int_16 i;
      INT(16) i;                  int_16 j;
      INT(16) j;                };
    END;                        _struct_array_type(k,2,5)
    STRUCT hepburn(k)[2:5];                        hepburn;
    ...                         ...
    hepburn[2].i := 13;         hepburn[2].i = 13;
    x := hepburn[5].j;          x = hepburn[5].j;
    x := $LEN(hepburn.i);       x = _len(hepburn->i);

    return 0;                   return 0;
  END;                        }


#endif
//-------------------------------------------------------------------------
#define _struct_array_type(type, lb, ub)                                     \
  class {                                                                    \
    public:                                                                  \
      _resident inline type& operator [] (int i) { return __data [i-(lb)];}; \
      _resident inline type* operator -> () { return &__data [0 - (lb)]; };  \
      _resident inline type* operator & () { return &__data [-(lb)]; };      \
      _resident inline operator type * () { return &__data [-(lb)]; };       \
      /* _resident inline operator void * () { return &__data [-(lb)]; }; */ \
    private:                                                                 \
      type __data [(ub) - (lb) + 1];                                         \
  }

//-----------------------------------------------
// Initialized Arrays With Non-Zero Lower Bounds:
//-----------------------------------------------
//-------------------------------------------------------------------------
#if 0

_initialized_array

The _initialized_array macro expands to the definition of an initialized
array variable with a nonzero lower bound.

  _initialized_array(element_type,
                     variable_name,
                     lower_bound,
                     upper_bound)

element_type
  is the type of the array elements.

variable_name
  is the name of the variable that this macro invocation defines.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

Usage Considerations

The _initialized_array macro expands to the definition of an initialized
array variable with a nonzero lower bound.

The _initialized_array macro defines an enumeration,
_num_elems_variable-name, which evaluates to the number of elements in
the array variable.

Use the _initialized_char_array macro to define an initialized array with a
nonzero lower bound and with character type elements.

You must immediately precede a use of this macro with a use of the
_init_array_value macro.  For an example, see the description of
_init_array_value.


#endif
//-------------------------------------------------------------------------
#define _initialized_array(type, name, lb, ub)                   \
  type * const name _value( ((type * const) __value_for_##name) - (lb) ); \
  enum {_num_elems_##name = (ub) - (lb) + 1}

//-------------------------------------------------------------------------
#if 0

_init_array_value

The _init_array_value macro expands to the definition of an initialized
array variable.

  _init_array_value(element_type,
                    variable_name)

element_type
  is the type of the array elements.

variable_name
  is the name of the variable that this macro invocation defines.

Usage Considerations

The _init_array_value macro expands to the definition of an initialized
array variable.  The array, declared with no bounds, holds the list of
initial values.

You must immediately follow a use of this macro with a use of the
_initialized_array macro.

Access to the array variable requires no special syntax.

The following example defines an initialized array type variable, scarlett,
which has four int_32 type cells, indexed from 2 to 5 inclusively.

  pTAL Code                   Generated C++ Code

  INT(16) PROC WIND;          extern "C" int_16 WIND()
  BEGIN                       {
    INT(32) x;                  int_32 x;
    INT(32) scarlett[2:5]       _init_array_value(
            := [0D,1D];               int_32,scarlett)={0,1};
                                _initialized_array(
                                      int_32,scarlett,2,5);
    ...                         ...
    scarlett[2] := 13;          scarlett[2] = 13;
    x := scarlett[5];           x = scarlett[5];
  END;                        }


#endif
//-------------------------------------------------------------------------
#define _init_array_value(type, name)                            \
  type __value_for_##name []

//-------------------------------------------------------------------------
#if 0

_initialized_char_array

The _initialized_char_array macro expands to the definition of an initialized
character type array variable with a nonzero lower bound.

  _initialized_char_array(element_type,
                          variable_name,
                          lower_bound,
                          upper_bound,
                          initial_value)

element_type
  is the type of the array elements.

variable_name
  is the name of the variable that this macro invocation defines.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

initial_value
  is a string constant that specifies the initial value of this array of
  characters.

Usage Considerations

The _initialized_char_array macro expand to the definition of an initialized
character array variable with a nonzero lower bound.

The _initialized_char_array macro defines an enumeration,
_num_elems_variable-name, which evaluates to the number of elements in
the array variable.

Use the _initialized_array macro to define an initialized array with a nonzero
lower bound whose elements are not characters.

Access to the array variable requires no special syntax.

The following example defines an initialized array of characters, ilsa,
which has five character type cells, indexed from -2 to 2 inclusively.

  pTAL Code                   Generated C++ Code

  INT(16) PROC RICKS;         extern "C" int_16 RICKS()
  BEGIN                       {
    STRING x;                   char x;
    STRING ilsa[-2:2]           _initialized_char_array(
           := "paris";                 char,ilsa,-2,2,"paris");
    ilsa[-2] := "m";            ilsa[-2] = 'm';
  END;                        }


#endif
//-------------------------------------------------------------------------
#define _initialized_char_array(type, name, lb, ub, value)       \
  _init_array_value(unsigned_char, name) = value;                \
  _initialized_array (type, name, lb, ub)

//---------------------------------------------
// Read-only Arrays With Non-Zero Lower Bounds:
//---------------------------------------------
//-------------------------------------------------------------------------
#if 0

_const_array

The _const_array macro expands to the definition of a read-only array
variable with a nonzero lower bound.

You must immediately precede a use of one of these macros with a use of the
_const_array_value macro.

Use the _const_char_array macro to define a read-only array with a nonzero
lower bound whose elements are characters.

  _const_array(element_type,
               variable_name,
               lower_bound,
               upper_bound)

element_type
  is the type of the array elements.

variable_name
  is the name of the variable that this macro invocation defines.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

Usage Considerations

The _const_array macro expands to the definition of a read-only array
variable with a nonzero lower bound.

The _const_array macro defines an enumeration,
_num_elems_variable-name, which evaluates to the number of elements in
the array variable.

Use the _const_char_array macro to define a read-only array with a nonzero
lower bound and with character type elements.

You must immediately precede each use of this macro with a use of the
_const_array_value macro.  For an example, see the description of
_const_array_value.


#endif
//-------------------------------------------------------------------------
#define _const_array(type, name, lb, ub)                            \
  type _cspace const _near* const name =                            \
      ((type _cspace const _near* const) __value_for_##name) - (lb); \
  enum {_num_elems_##name = (ub) - (lb) + 1}

//-------------------------------------------------------------------------
#if 0

_const_array_value

The _const_array_value macro expands to the definition of a read-only array
variable.

  _const_array_value(element_type,
                     variable_name)

element_type
  is the type of the array elements.

variable_name
  is the name of the variable that this macro invocation defines.

Usage Considerations

The _const_array_value macro expands to the definition of a read-only array
variable.  The array, declared with no bounds, holds the list of initial
values.

You must immediately follow each use of this macro with a use of the
_const_array macro.

Access to the array variable requires no special syntax.

The following example defines an initialized array type variable, rhett,
which has four int_32 type cells, indexed from 2 to 5 inclusively.

  pTAL Code                   Generated C++ Code

  INT(32) rhett               _global _const_array_value(
          := [0D,1D,2D,3D];               int_32,rhett)
//                            #ifdef _export_globals
//                            = {0,1,2,3}
//                            #endif
                              ;
                              _global _const_array(
                                          int_32,rhett,2,5);
  ...                         ...
    INT(32) .darn;              int_32 *darn;
    darn := rhett;              *darn = rhett;


#endif
//-------------------------------------------------------------------------
#define _const_array_value(type, name)                           \
  const _cspace type __value_for_##name []

//-------------------------------------------------------------------------
#if 0

_const_char_array

The _const_char_array macro expands to the definition of a read-only character
type array variable with a nonzero lower bound.

  _const_char_array(element_type,
                    variable_name,
                    lower_bound,
                    upper_bound,
                    initial_value)

element_type
  is the type of the array elements.

variable_name
  is the name of the variable that this macro invocation defines.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

initial_value
  is a string constant that specifies the initial value of this array of
  characters.

Usage Considerations

The _const_char_array macro expands to the definition of a read-only character
array variable with a nonzero lower bound.

The _const_char_array macro defines an enumeration,
_num_elems_variable-name, which evaluates to the number of elements in
the array variable.

Use the _const_array macro to define an initialized array with a nonzero lower
bound whose elements are not characters.

Access to the array variable requires no special syntax.

The following example defines a read-only array of characters, lazlo, which
has five character type cells, indexed from -2 to 2 inclusively.

  pTAL Code                   Generated C++ Code

  INT(16) PROC RICKS;         extern "C" int_16 RICKS()
  BEGIN                       {
    STRING x;                   char x;
    STRING lazlo[-2,2]          _const_char_array(
        := "lisbon";                char,lazlo,-2,2,"lisbon");
    x := lazlo[-2];             x = lazlo[-2];
  END;                        }


#endif
//-------------------------------------------------------------------------
#define _const_char_array(var, name, lb, ub, value)             \
  _const_array_value(unsigned_char, name) = value;              \
  _const_array (var, name, lb, ub)

//---------------
// Equivalencing:
//---------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#define _cast(type, var) *((type *)&(var))

//-------------------------------------------------------------------------
#if 0

_equiv

The _equiv macro defines a local variable that overlays a previously
defined variable.  The previously defined variable can be an array cell,
a structure, or a scalar variable.

  _equiv(variable_type,
         variable_name,
         previous_identifier)

variable_type
  is the type of the equivalenced variable that this macro defines.

variable_name
  is the name of the equivalenced variable.

previous_identifier
  is the name of a previously defined variable which this variable is to
  overlay.

Usage Considerations

An equivalenced variable causes no space to be allocated.

Use the _equiv_global macro to define a global variable that overlays a
previously defined variable.

Use the _equiv_array macro or _equiv_array_global macro to define an array
type variable that overlays a previously defined variable.

Access to an equivalenced variable requires no special syntax.

The following example defines a variable graham which is equivalenced to the
variable patti.  It also defines a variable jordan which is equivalenced to
the array angela beginning at its cell zero.

  pTAL Code                   Generated C++ Code

  PROC SO_CALLED;             extern "C" void SO_CALLED()
  BEGIN                       {
    INT(16) patti;              int_16 patti;
    INT(16) graham = patti;     _equiv(int_16,graham,patti);

    INT(16) angela[0:1];        int_16 angela[2];
    INT(32) jordan = angela;    _equiv(int_32,jordan,
                                       angela[0]);
    patti := 13;                patti = 13;
    graham := 14;               graham = 14;
    angela[1] := 4;             angela[1] = 4;
    jordan := 5D;               jordan = 5;
  END;                        }

The following example defines a variable rayanne which is equivalenced to
the variable rickie.  It also defines a variable b which is equivalenced
to the array a beginning at its cell zero.

  pTAL Code                   Generated C++ Code

  PROC LIFE;                  extern  C  void LIFE()
  BEGIN                       {
    STRUCT r(*);                struct r {
    BEGIN                         int_16 i;
      INT(16) i;                  int_16 j;
      INT(16) j;                };
    END;
    STRUCT rickie(r);           r  rickie;
    INT(32) rayanne = rickie;   _equiv(int_32,rayanne,rickie);

    INT(16) a[-1:3];            _array_type(int_16,-1,3) a;
    INT(16) b = a;              _equiv(int_16,b,a[0]);

    rayanne := 3D;              rayanne = 3;
  END;                        }


#endif
//-------------------------------------------------------------------------
#define _equiv(type, var, prev) type & var = _cast (type, prev)

//-------------------------------------------------------------------------
#if 0

_equiv_global

The _equiv_global macro defines a global variable that overlays a previously
defined variable.  The previously defined variable can be an array cell, a
structure, or a scalar variable.

  _equiv_global(variable_type,
                variable_name,
                previous_identifier)

variable_type
  is the type of the equivalenced variable that this macro defines.

variable_name
  is the name of the equivalenced variable.

previous_identifier
  is the name of a previously defined variable which this variable is to
  overlay.

Usage Considerations

An equivalenced variable causes no space to be allocated.

Use the _equiv macro to define a local variable that overlays a previously
defined variable.  Use the _equiv_array macro or the _equiv_array_global
macro to define an array type variable that overlays a previously defined
variable.

Access to an equivalenced variable requires no special syntax.

The following example defines a variable chandler that is equivalenced to
the variable joey.  It also defines a variable manana that is equivalenced
to the zero cell of the array mon.  It also defines the variable ross that
is equivalenced to the zero cell of the array rachel.

  pTAL Code                   Generated C++ Code

  INT(16) joey;               _global int_16 JOEY;
  INT(16) chandler = joey;    _equiv_global(int_16,CHANDLER, JOEY);

  INT(16) phoebe[0:1];        _global int_16 PHOEBE[3];
  INT(32) monica = phoebe;    _equiv_global(int_16,MONICA,PHOEBE[0]);

  INT(16) rachel[-1:1];       _global _array_type(int_16,-1,1) RACHEL;
  INT(16) ross = rachel;      _equiv_global(int_16,ROSS, RACHEL[0]);


#endif
//-------------------------------------------------------------------------
#define _equiv_global(type, var, prev) _resident inline type & var(void) \
                                       {return _cast (type, prev);}

//-------------------------------------------------------------------------
#if 0

_equiv_array

The _equiv_array macro defines a local array variable that overlays a
previously defined variable.  The previously defined variable can be
an array cell, a structure, or a scalar variable.

  _equiv_array(element_type,
               variable_name,
               lower_bound,
               upper_bound,
               previous_identifier)

element_type
  is the type of the array elements.

variable_name
  is the name of the equivalenced variable.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

previous_identifier
  is the name of a previously defined variable which this variable is
  to overlay.

Usage Considerations

Cell zero of the equivalenced array variable is aligned with the beginning
of a previously defined scalar variable.  Cell zero of the equivalenced
array variable is aligned with cell zero of the previously defined array
variable, even if either array is defined with a nonzero lower bound.

The _equiv_array macro defines an enumeration,
_num_elems_variable-name, which evaluates to the number of elements in
the array variable.

Use the _equiv or _equiv_global macro to define a variable that is not
an array and that overlays a previously defined variable.

An equivalenced variable causes no space to be allocated.

Access to an equivalenced variable requires no special syntax.

The following example defines an array variable bing which overlays the
variable cherries.  It also defines an array variable granny_smith which
overlays the array apples beginning at its cell zero; the variable
granny_smith does not overlay apples[-2] or apples[-1].  The array variable
pippin also overlays the array apples with pippin[0] aligned with apples[0].
The array variable concord overlays the structure grapes.

  pTAL Code                   Generated C++ Code

  PROC SALAD;                 extern "C" void SALAD()
  BEGIN                       {
    INT(32) cherries;           int_32 cherries;
    INT(16) bing[0:1]           _equiv_array(int_16,bing,0,1,
            = cherries;                      cherries);

    INT(16) apples[-2:7];       _array_type(int_16,-2,7)
                                                   apples;
    INT(16) granny_smith[0:7]   _equiv_array(int_16,
            = apples;                        granny_smith,0,7,
                                             apples[0]);
    INT(16) pippin[-1:5]        _equiv_array(int_16,pippin,
            = apples;                        -1,5,apples[0]);

    STRUCT g(*);                struct g {
    BEGIN                         int_16 i;
      INT(16) i;                  int_16 j;
      INT(16) j;                };
    END;                        g  grapes;
    STRUCT grapes(g);           _equiv_array(int_16,concord,
    INT(16) concord[0:1]                     0,1,grapes);
            = grapes;

    cherries := 3D;             cherries = 3;
    bing[0] := 4;               bing[0] = 4;
    apples[-2] := 5;            apples[-2] = 5;
    granny_smith[0] := 6;       granny_smith[0] = 6;
    pippin[-1] := 7;            pippin[-1] = 7;
    grapes.i := 8;              grapes.i = 8;
    concord[0] := 9;            concord[0] = 9;
  END;                        }


#endif
//-------------------------------------------------------------------------
#define _equiv_array(type, array, lb, ub, prev)          \
            type * const array = ((type * const) &(prev)); /*- (lb)*/ \
            enum {_num_elems_##array = (ub) - (lb) + 1}

//-------------------------------------------------------------------------
#if 0

_equiv_array_global

The _equiv_array_global macro defines a global array variable that
overlays a previously defined variable.  The previously defined variable
can be an array cell, a structure, or a scalar variable.

  _equiv_array_global(element_type,
                      variable_name,
                      lower_bound,
                      upper_bound,
                      previous_identifier)

element_type
  is the type of the array elements.

variable_name
  is the name of the equivalenced variable.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

previous_identifier
  is the name of a previously defined variable which this variable is
  to overlay.

Usage Considerations

Cell zero of the equivalenced array variable is aligned with the beginning
of a previously defined scalar variable.  Cell zero of the equivalenced
array variable is aligned with cell zero of the previously defined array
variable, even if either array is defined with a nonzero lower bound.

The _equiv_array_global macro defines an enumeration,
_num_elems_variable-name, which evaluates to the number of elements in
the array variable.

Use the _equiv_array macro to define a local array variable that overlays
a previously defined variable.  Use the _equiv or _equiv_global macro to
define a variable that is not an array and that overlays a previously
defined variable.

An equivalenced variable causes no space to be allocated.

Access to an equivalenced variable requires no special syntax.


#endif
//-------------------------------------------------------------------------
#define _equiv_array_global(type, array, lb, ub, prev)   \
            _resident inline type * const array(void)    \
            {return ((type * const) &(prev)) /*- (lb)*/;}; \
            enum {_num_elems_##array = (ub) - (lb) + 1}

//-------------------------------------------------------------------------
#if 0

_equiv_offset

The _equiv_offset macro defines a local variable that overlays a previously
defined variable, offset by the given number of 16-bit words.  The previously
defined variable can be an array cell, a structure, or a scalar variable.

  _equiv_offset(variable_type,
                variable_name,
                previous_identifier,
                16_bit_word_offset)

variable_type
  is the type of the equivalenced variable that this macro defines.

variable_name
  is the name of the equivalenced variable.

previous_identifier
  is the name of a previously defined variable.

16_bit_word_offset
  is an integer constant that specifies the number of 16-bit words to
  offset from the storage address of the previous_identifier which is
  the address which this variable is to overlay.

Usage Considerations

The equivalenced variable is aligned with the beginning of the previously
defined scalar variable plus the indicated offset.  The equivalenced
variable is aligned with cell zero of the previously defined array
variable plust the indicated offset.

Use the _equiv_offset_global macro to define a global variable that overlays
a previously defined variable.  Use the _equiv_array_offset macro or the
_equiv_array_offset_global macro to define an array type variable that
overlays a previously defined variable offset by a constant number of
16-bit words.

An equivalenced variable causes no space to be allocated.

Access to an equivalenced variable requires no special syntax.

The following example defines the variable brandon which is equivalenced
to the variable brenda but begins 16 bits past the beginning of the
variable brenda.  It defines the variable donna which is equivalenced
to the variable david but begins 32 bits past the zero cell of the
variable david.  It also defines the variable jesse which is equivalenced
to the variable andrea but begins 16 bits past the zero cell of the
variable andrea.

  pTAL Code                   Generated C++ Code

  PROC BH;                    extern "C" void BH()
  BEGIN                       {
    INT(32) brenda;             int_32 brenda;
    INT(16) brandon             _equiv_offset(int_16,brandon,
            = brenda + 1;                     brenda,1);

    STRING david[0:9];          char david[10];
    STRING donna = david + 2;   _equiv_offset(char,donna,
                                              *david,2);

    INT(16) andrea[-2:2];       _array_type(int_16,-2,2)
                                            andrea;
    INT(16) jesse               _equiv_offset(int_16,jesse,
            = andrea + 1;                     *andrea,1);

    brandon := 3;               brandon = 3;
    donna := "a";               donna = 'a';
    jesse := 16;                jesse = 16;
  END;                        }


#endif
//-------------------------------------------------------------------------
#define _equiv_offset(type, var, prev, _16_bit_offset)                      \
            type &var = _cast (type, *(((int_16*)&prev) + (_16_bit_offset)))

//-------------------------------------------------------------------------
#if 0

_equiv_offset_global

The _equiv_offset_global macro defines a global variable that overlays a
previously defined variable, offset by the given number of 16-bit words.
The previously defined variable can be an array cell, a structure, or a
scalar variable.

  _equiv_offset_global(variable_type,
                       variable_name,
                       previous_identifier,
                       16_bit_word_offset)

variable_type
  is the type of the equivalenced variable that this macro defines.

variable_name
  is the name of the equivalenced variable.

previous_identifier
  is the name of a previously defined variable.

16_bit_word_offset
  is an integer constant that specifies the number of 16-bit words to
  offset from the storage address of the previous_identifier which is
  the address which this variable is to overlay.

Usage Considerations

The equivalenced variable is aligned with the beginning of the previously
defined scalar variable plus the indicated offset.  The equivalenced
variable is aligned with cell zero of the previously defined array
variable plust the indicated offset.

Use the _equiv_offset macro to define a local variable that overlays
a previously defined variable.  Use the _equiv_array_offset macro or the
_equiv_array_offset_global macro to define an array type variable that
overlays a previously defined variable offset by a constant number of
16-bit words.

An equivalenced variable causes no space to be allocated.

Access to an equivalenced variable requires no special syntax.

The following example defines the variable vera which is equivalenced to
the variable norm but begins 16 bits past the beginning of the variable
norm.  It defines the variable coach which is equivalenced to the array
variable woody but begins 32 bits past the zero cell of the array woody.
It also defines the variable carla which is equivalenced to the array
variable diane but begins 16 bits past the zero cell of the variable diane.

  pTAL Code                   Generated C++ Code

  INT(32) norm;               _global int_32 NORM;
  INT(16) vera = norm + 1;    _global _equiv_offset(int_16,VERA,NORM,1);

  INT(16) woody[-1:2];        _global _array_type(int_16,-1,2) WOODY;
  INT(16) coach = woody + 2;  _equiv_offset_global(int_16,COACH,*WOODY,2);

  INT(16) diane[0:3];         _global int_16 DIANE[4];
  INT(16) carla = diane + 1;  _equiv_offset_global(int_16,CARLA,*DIANE,1);


#endif
//-------------------------------------------------------------------------
#define _equiv_offset_global(type, var, prev, _16_bit_offset)             \
            _resident inline type &var(void)                              \
            {return _cast (type, *(((int_16*)&prev) + (_16_bit_offset)));}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//---------------------------------------------------------------------------
// _equiv_array_offset:
// --------------------
// Note that, as in Ptal, the array bounds on an equivalenced array are
// for documentation only; it is the 0th element which gets equivalenced
// to the specified 16-bit offset above the previous variable:
//---------------------------------------------------------------------------

//-------------------------------------------------------------------------
#if 0

_equiv_array_offset

The _equiv_array_offset macro defines a local array variable that overlays
a previously defined variable, offset by the given number of 16-bit words.
The previously defined variable can be an array cell, a structure, or a
scalar variable.

  _equiv_array_offset(element_type,
                      variable_name,
                      lower_bound,
                      upper_bound,
                      previous_identifier,
                      16_bit_word_offset)

element_type
  is the type of the array elements.

variable_name
  is the name of the equivalenced variable.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

previous_identifier
  is the name of a previously defined variable which this variable is
  to overlay.

16_bit_word_offset
  is an integer constant that specifies the number of 16-bit words to
  offset from the storage address of the previous_identifier which is
  the address which this variable is to overlay.

Usage Considerations

Cell zero of the equivalenced array variable is aligned with the beginning
of the previously defined scalar variable plus the indicated offset.  Cell
zero of the equivalenced array variable is aligned with cell zero of the
previously defined array variable plus the indicated offset, even if that
array is defined with a nonzero lower bound.

The _equiv_array_offset macro defines an enumeration,
_num_elems_variable-name, which evaluates to the number of elements in
the array variable.

Use the _equiv_array_offset_global macro to define a global array variable
that overlays a previously defined variable offset by a constant number of
16-bit words.  Use the _equiv_offset macro or the _equiv_offset_global macro
to define variable that is not an array and that overlays a previously
defined variable offset by a constant number of 16-bit words.

An equivalenced variable causes no space to be allocated.

Access to an equivalenced variable requires no special syntax.

The following example defines the array variable skipper which is
equivalenced to the variable gilligan but whose zero cell begins 16 bits
past the beginning of the variable gilligan.  It defines the array
variable maryann which is equivalenced to the array variable ginger but
whose zero cell begins 32 bits past the zero cell of the array ginger.
It defines the array variable lovey which is equivalenced to the array
variable howell but whose zero cell begins 16 bits past the zero cell
of the variable howell.  It also defines the array variable minnow which
is equivalenced to the array variable professor but whose virtual zero
cell begins 16 bits past the zero cell of the variable professor.

  pTAL Code                   Generated C++ Code

  PROC ISLAND;                extern "C" void ISLAND()
  BEGIN                       {
    INT(32) gilligan;           int_32 gilligan;
    STRING skipper[0:1]         _equiv_array_offset(char,
          = gilligan + 1;             skipper,0,1,gilligan,1);

    INT(16) ginger[0:5];        int_16 ginger[6];
    INT(16) maryann[0:3]        _equiv_array_offset(int_16,
          = ginger + 2;               maryann,0,7,*ginger,2);

    INT(16) howell[-2:2];       _array_type(int_16,-2,2) howell;
    INT(16) lovey[0:1]          _equiv_array_offset(int_16,
          = howell + 1;               lovey,0,1,*howell,1);

    INT(16) prof[0:4];          int_16 prof[5];
    INT(16) minnow[1:3]         _equiv_array_offset(int_16,
          = prof + 1;                 minnow,1,3,*professor,1);

    skipper[0] := "a";          skipper[0] = 'a';
    maryann[0] := 20;           maryann[0] = 20;
    lovey[0] := 21;             lovey[0] = 21;
    minnow[1] := 22;            minnow[1] = 22;
  END;                        }


#endif
//-------------------------------------------------------------------------
#define _equiv_array_offset(type, array, lb, ub, prev, _16_bit_offset)        \
            type * const array =                                              \
            (type * const) (((int_16*)&(prev))+(_16_bit_offset));             \
            enum {_num_elems_##array = (ub) - (lb) + 1}

//-------------------------------------------------------------------------
#if 0

_equiv_array_offset_global

The _equiv_array_offset_global macro defines a global array variable that
overlays a previously defined variable, offset by the given number of
16-bit words.  The previously defined variable can be an array cell, a
structure, or a scalar variable.

  _equiv_array_offset_global(element_type,
                             variable_name,
                             lower_bound,
                             upper_bound,
                             previous_identifier,
                             16_bit_word_offset)

element_type
  is the type of the array elements.

variable_name
  is the name of the equivalenced variable.

lower_bound
  is an integer constant that specifies the lower bound of the array.

upper_bound
  is an integer constant that specifies the upper bound of the array.

previous_identifier
  is the name of a previously defined variable which this variable is
  to overlay.

16_bit_word_offset
  is an integer constant that specifies the number of 16-bit words to
  offset from the storage address of the previous_identifier which is
  the address which this variable is to overlay.

Usage Considerations

Cell zero of the equivalenced array variable is aligned with the beginning
of the previously defined scalar variable plus the indicated offset.  Cell
zero of the equivalenced array variable is aligned with cell zero of the
previously defined array variable plus the indicated offset, even if that
array is defined with a nonzero lower bound.

The _equiv_array_offset macro defines an enumeration,
_num_elems_variable-name, which evaluates to the number of elements in
the array variable.

Use the _equiv_array_offset macro to define a local array variable that
overlays a previously defined variable offset by a constant number of
16-bit words.  Use the _equiv_offset macro or the _equiv_offset_global
macro to define variable that is not an array and that overlays a
previously defined variable offset by a constant number of 16-bit words.

An equivalenced variable causes no space to be allocated.

Access to an equivalenced variable requires no special syntax.

The following example defines the array variable frasier which is
equivalenced to the variable roz but whose cell zero is located 16
bits past the beginning of the variable roz.  The array variable x
is also equivalenced to roz, and has a zero cell located 16 bits past
the beginning of the variable roz.  It defines the array variable niles
which is equivalenced to the array variable daphne but has a zero cell
located 32 bits past the zero cell of the variable daphne.

  pTAL Code                   Generated C++ Code

  INT(32) roz;                _global int_32 ROZ;
  STRING frasier[0:1]         _equiv_array_offset_global(unsigned_char,
        = roz + 1;                                       FRASIER,0,1,ROZ,1);
  STRING x[-2:0]              _equiv_array_offset_global(unsigned_char,
        = roz + 1;                                       X,-2,0,ROZ,1);

  INT(16) daphne[-1:4];       _global _array_type(int_16,-1,4) DAPHNE;
  INT(16) niles[-1:1]         _equiv_array_offset_global(int_16,NILES,
        = daphne + 2;                                    -1,1,*DAPHNE,2);


#endif
//-------------------------------------------------------------------------
#define _equiv_array_offset_global(type, array, lb, ub, prev, _16_bit_offset) \
            _resident inline type * const array(void)                         \
            {return (type * const) (((int_16*)&(prev))+(_16_bit_offset));};   \
            enum {_num_elems_##array = (ub) - (lb) + 1}

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
#if 0

The pTAL language allows you to extract a value stored in a bit or contiguous
sequence of bits in any 16-bit l-value.  It also allows you to deposit a value into a bit or
contiguous sequence of bits in any 16-bit expression.

The pTAL to C++ Translator emulates pTAL bit deposit and bit extract operations
with calls to the functions _bit_deposit and _bit_extract.  For example:

  pTAL Code                   Generated C++ Code

  INT(16) fields, x;          int_16  fields, x;

  fields.<0:3> := 9;          _bit_deposit(fields,0,3,9);
  x := fields.<0:3>;          x = _bit_extract(fields,0,3);

  fields.<4> := 1;            _bit_deposit(fields,4,4,1);
  x := fields.<4>;            x = _bit_extract(fields,4,4);



Bit Extraction

The _bit_extract function, supplied in the rosgen.h include file,
extracts a value stored in a bit or contiguous sequence of bits in
any 16-bit expression.

  int_16 _bit_extract(int_16 expr,
                      int_16 lower_bound,
                      int_16 upper_bound)

expr
  is the expression from which this function will extract a value occupying
  a bit field.

lower_bound
  indicates the most significant bit in the field to be extracted, where
  the most significant bit in a 16-bit word is numbered zero.  lower_bound
  is an integer constant in the range 0 through 15.

upper_bound
  indicates the least significant bit in the field to be extracted, where
  the least significant bit in a 16-bit word is numbered 15.  upper_bound
  is an integer constant in the range 0 through 15.

Usage Considerations

The value of upper_bound must be at least as large as lower_bound.

In the following example, the variable y gets the value that occupies the
field composed of bits 0, 1,  and 2 (the most significant three bits) of
the variable x.  The variable z gets the value that occupies bit 3 of the
variable x.

  int_16 x,y,z;

  y = _bit_extract(x,0,2);
  z = _bit_extract(x,3,3);



Bit Deposit

The _bit_deposit function, supplied in the rosgen.h include file, deposits
a value into a bit or contiguous sequence of bits in any 16-bit expression.

  int_16 _bit_deposit(int_16 expr,
                      int_16 lower_bound,
                      int_16 upper_bound,
                      int_16 value)

expr
  is the expression from which this function will extract bits.

lower_bound
  indicates the most significant bit in the field in which the value is
  to be deposited, where the most significant bit in a 16-bit word is
  numbered zero.  lower_bound is an integer constant in the range 0
  through 15.

upper_bound
  indicates the least significant bit in the field in which the value is
  to be deposited, where the least significant bit in a 16-bit word is
  numbered 15.  upper_bound is an integer constant in the range 0 through 15.

value
  is the expression whose value is to be stored in the bit field delimited
  by the bits numbered lower_bound and upper_bound.

Usage Considerations

The value of upper_bound must be at least as large as lower_bound.

In the following example, the value 7 is stored in the field composed
of bits 0, 1, and 2 (the most significant three bits) of the variable x.
The value 1 is stored in bit 3 of the variable x.

  int_16 x;

  _bit_deposit(x,0,2,7);
  _bit_deposit(x,3,3,1);

#endif
//-------------------------------------------------------------------------
#define __mask(lb, rb)                   \
        (((rb) - (lb) + 1) == 32 ?       \
         0xFFFFFFFF :                    \
         (1u << ((rb) - (lb) + 1)) - 1)

#define __shifted_mask(wordlen, lb, rb)                \
        (__mask (lb, rb) << (((wordlen) - 1) - (rb)))

#define _deposit_bits(varble, wordlen, expr, lb, rb)                        \
        (varble = ((varble & ~__shifted_mask (wordlen, lb, rb)) |           \
                  ((expr & __mask (lb, rb)) << (((wordlen) - 1) - (rb)))))

// Two versions of _bit_deposit to allow deposit in 8-bit or 16-bit target:

_resident inline short _bit_deposit (unsigned_32  &varble,
                                     unsigned_16   lb,
                                     unsigned_16   rb,
                                     int_16        expr) {
  _deposit_bits (varble, 32, expr, lb, rb);
  return expr;
}

_resident inline short _bit_deposit (unsigned_16  &varble,
                                     unsigned_16   lb,
                                     unsigned_16   rb,
                                     int_16        expr) {
  _deposit_bits (varble, 16, expr, lb, rb);
  return expr;
}

_resident inline short _bit_deposit (int_16      &varble,
                                     unsigned_16  lb,
                                     unsigned_16  rb,
                                     int_16       expr) {
  _deposit_bits (varble, 16, expr, lb, rb);
  return expr;
}

_resident inline short _bit_deposit (unsigned_char &varble,
                                     unsigned_16    lb,
                                     unsigned_16    rb,
                                     int_16         expr) {
  _deposit_bits (varble, 16, expr, lb, rb);
  return expr;
}

#define _bit_deposit_item(item, lb, rb, expr)          \
        _deposit_bits (item, 16, expr, lb, rb), expr;

#define _decl_bit_deposit_uns(bitcount)                                       \
      _resident inline short _bit_deposit_uns (_unsigned_##bitcount &varble,  \
                                               unsigned_16 lb,                \
                                               unsigned_16 rb,                \
                                               int_16      expr) {            \
        _deposit_bits (varble (), 16, expr, lb, rb);                          \
        return expr;                                                          \
      }
_decl_bit_deposit_uns (1)
_decl_bit_deposit_uns (2)
_decl_bit_deposit_uns (3)
_decl_bit_deposit_uns (4)
_decl_bit_deposit_uns (5)
_decl_bit_deposit_uns (6)
_decl_bit_deposit_uns (7)
_decl_bit_deposit_uns (8)
_decl_bit_deposit_uns (9)
_decl_bit_deposit_uns (10)
_decl_bit_deposit_uns (11)
_decl_bit_deposit_uns (12)
_decl_bit_deposit_uns (13)
_decl_bit_deposit_uns (14)
_decl_bit_deposit_uns (15)
#undef _decl_bit_deposit_uns
#define _decl_bit_deposit_uns(bitcount)                                       \
      _resident inline short _bit_deposit_uns (_unsigned_##bitcount &varble,  \
                                               unsigned_16 lb,                \
                                               unsigned_16 rb,                \
                                               int_32 expr) {                 \
        _deposit_bits (varble (), 32, expr, lb, rb);                          \
        return expr;                                                          \
      }
_decl_bit_deposit_uns (17)
_decl_bit_deposit_uns (18)
_decl_bit_deposit_uns (19)
_decl_bit_deposit_uns (20)
_decl_bit_deposit_uns (21)
_decl_bit_deposit_uns (22)
_decl_bit_deposit_uns (23)
_decl_bit_deposit_uns (24)
_decl_bit_deposit_uns (25)
_decl_bit_deposit_uns (26)
_decl_bit_deposit_uns (27)
_decl_bit_deposit_uns (28)
_decl_bit_deposit_uns (29)
_decl_bit_deposit_uns (30)
_decl_bit_deposit_uns (31)
#undef _decl_bit_deposit_uns

_resident inline int_32 _bit_deposit_long (int_32      &varble,
                                           unsigned_16 lb,
                                           unsigned_16 rb,
                                           int_32      expr) {
  _deposit_bits (varble, 32, expr, lb, rb);
  return expr;
}

#define _bit_deposit_long_item(item, lb, rb, expr)     \
        _deposit_bits (item, 32, expr, lb, rb), expr;

#define _extract_bits(expr, wordlen, lb, rb)        \
        (((wordlen) == ((rb) - (lb) + 1)) ?         \
         (expr) :                                   \
         (((expr) >> (((wordlen) - 1) - (rb))) &    \
          ((1u << ((rb) - (lb) + 1)) - 1)))

// Define versions of _bit_extract, for use in contexts where inline
// functions can't be called:

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#define _bit_extract_n(expr, lb, rb) _extract_bits(expr, 16, lb, rb)
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#define _bit_extract_long_n(expr, lb, rb) _extract_bits(expr, 32, lb, rb)
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#define _bit_extract_long_to_short_n(expr, lb, rb)    \
          ((int_16)_extract_bits(expr, 32, lb, rb))

// Inline function versions of _bit_extract, for use in contexts where inline
// functions can be called:

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
_resident inline int_16 _bit_extract (int_16 expr, short lb, short rb) {
  return (int_16)_extract_bits (expr, 16, lb, rb);
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
_resident inline int_32 _bit_extract_long (int_32 expr, short lb, short rb) {
  return _extract_bits (expr, 32, lb, rb);
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
_resident inline int_16 _bit_extract_long_to_short
                          (int_32 expr, short lb, short rb) {
  return (int_16)_extract_bits(expr, 32, lb, rb);
}



//-------------------------------------------------------------------------
#if 0

Emulating an Alternate Entry Point

A pTAL procedure can have multiple entry points; a C++ function cannot.
Local variables are initialized when control enters a pTAL procedure via
any entry point.  Control entering a pTAL procedure from its primary
entry point begins executing statements at the beginning of the procedure;
control entering a pTAL procedure from an alternate entry point begins
executing statements at the label representing the alternate entry point.

The pTAL to C++ Translator translates a pTAL procedure that has alternate
entry points to a C++ function representing the body of the routine and a
set of interface routines, each of which represents an entry point.

Consider the following pTAL example:

  INT(16) PROC jekyll(a);
    INT(16) a;
  BEGIN
    ENTRY hyde;
    INT(16) i := 13;
    INT(16) j := 17;

    i := i + 1;
  hyde:
    j := j - 1;
    RETURN i + j + a;
  END;
    ...
    x := jekyll(2);
    y := hyde(3);

This pTAL procedure contains two entry points.  The pTAL to C++ Translator
emits the following C++ code that is equivalent to the pTAL code.

  inline int_16 _inline_JEKYLL(int_16 a,
                               int    entry)
  {
    int_16 i = 13;
    int_16 j = 17;

    Entry(HYDE,1);
    ++i;
    HYDE: --j;
    return i + j + a;
  }

  //--- Translator-generated shell for entry-point JEKYLL: ---
  extern "C" int_16 JEKYLL(int_16 a)
  {
    return _inline_JEKYLL(a,0);
  }

  //--- Translator-generated shell for entry-point HYDE: ---
  extern "C" int_16 HYDE(int_16 a)
  {
    return _inline_JEKYLL(a,1);
  }

    ...
    x = JEKYLL(2);
    x = HYDE(3);

The common procedure body, _inline_JEKYLL, is an inline function.  If
compiled with a C++ compiler that performs inlining, there is no
performance penalty associated with the call, but there is some expansion
of code file space which could adversely effect performance.

Entry

The Entry macro tests to see if the parameter entry passed into the
surrounding function is equal to the second parameter to the macro.  If so,
it transfers control to the label whose name is given as the first parameter
to the macro.

  void Entry(label_name, entry_num)

label_name
  is the name of a label to which control transfers if the parameter
  entry matches entry-num.

entry_num
  is an integer constant associated with the entry point which is being invoked.


Optional Parameters

The translation of a pTAL procedure with multiple entry points that has
the EXTENSIBLE or VARIABLE attribute requires the C++-coded entry point
shells to pass parameters that might be optional to the routine
representing the main body of the procedure.  The _pass_along macro
provides a shorthand way to pass an optional parameter to another routine
for which the parameter is optional.

Consider the following pTAL example:

  INT(16) PROC CLARK_KENT(a) EXTENSIBLE;
    INT(16) a;
  BEGIN
    INT(16) x;
    ENTRY SUPERMAN;
    ...
  SUPERMAN:
    ...
  END;
    ...
    z := CLARK_KENT(5);
    z := SUPERMAN();

This pTAL procedure contains two entry points.  The pTAL to C++ Translator
emits the following C++ code that is equivalent to the pTAL code.

  inline int_16 _inline_CLARK_KENT(int_16 a
                                   int    entry)
  {
    int_16 x;
    Entry(SUPERMAN,1);
    ...
    SUPERMAN:
    ...
  }

  // Translator-generated shell for entry-point CLARK_KENT:
  extern "C" _extensible int_16 CLARK_KENT(int_16 a)
  {
    return _inline_CLARK_KENT(_pass_along(a),0);
  }

  // Translator-generated shell for entry-point SUPERMAN:
  extern "C" _extensible int_16 SUPERMAN(int_16 a)
  {
    return _inline_CLARK_KENT(_pass_along(a),1);
  }

  ...
  z = CLARK_KENT(5);
  z = SUPERMAN();


Modifying an Entry Point

Suppose you were to add a new entry point to this function.  You would add
a shell for the new entry point, add an invocation of the Entry macro which
transfers control to a new label, and add the label to the appropriate place
in the code.  The following example illustrates the addition of the entry
point STEVENSON to the procedure.

  inline int_16 _inline_JEKYLL(int_16 a,
                               int    entry)
  {
    int_16 i = 13;
    int_16 j = 17;

    Entry(HYDE,1);
    Entry(STEVENSON,2);
    ++i;
    HYDE: --j;
    STEVENSON:
    return i + j + a;
  }

  //--- Translator-generated shell for entry-point JEKYLL: ---
  extern "C" int_16 JEKYLL(int_16 a)
  {
    return _inline_JEKYLL(a,0);
  }

  //--- Translator-generated shell for entry-point HYDE: ---
  extern "C" int_16 HYDE(int_16 a)
  {
    return _inline_JEKYLL(a,1);
  }

  //--- Additional entry-point STEVENSON: ---
  extern "C" int_16 STEVENSON(int_16 a)
  {
    return _inline_JEKYLL(a,2);
  }

    ...
    x = JEKYLL(2);
    x = HYDE(3);
    x = STEVENSON(25);


#endif
//-------------------------------------------------------------------------
// Entry lets entry declarations translate almost 1:1; only parens are added:
#define Entry(name,entry_num) if (entry == entry_num) goto name

//-------------------------------------------------------------------------
#if 0

_pass_along

The _pass_along macro, supplied in the rosgen.h include file, provides a
shorthand way to pass an optional parameter in a function with the _variable
or _extensible attribute to another _variable or _extensible function for
which the parameter is optional.

  _pass_along(formal_param_name)

formal_param_name
  is the name of a formal parameter for a function with the _variable or
  _extensible attribute.

The _pass_along macro expands to the following:

  _optional(_arg_present(formal_param_name), formal_param_name)


#endif
//-------------------------------------------------------------------------
// _pass_along is used when invoking an extensible or variable proc from
// within a variable or extensible proc:
#define _pass_along(param) _optional (_arg_present (param), param)


//-------------------------------------------------------------------------
#if 0

The pTAL to C++ Translator emulates the pTAL ASSERT statement with the
assert macro, the _ASSERTION_LEVEL_ macro, and the _ASSERTION_PROC_ macro.
The assert macro invokes the procedure whose name is encapsulated by the
_ASSERTION_PROC_ if the level parameter passed to assert is at least as
large as _ASSERTION_LEVEL_ and the condition parameter passed to assert
evaluates to a nonzero value.


assert

The assert macro invokes a particular procedure.

  assert(int level,
         int condition)

level
  is an integer expression.

condition
  is an integer expression.

Usage Considerations

The macro _ASSERTION_LEVEL_ must be defined before the first invocation of
assert.

The macro _ASSERTION_PROC_ must be defined before the first invocation of
assert.

The macro _ASSERTION_LEVEL_ must expand to an integer expression.

The macro _ASSERTION_PROC_ must expand to the name of a procedure with the
following profile:

  extern "C" void proc_name()

The procedure named in the body of the _ASSERTION_PROC_ macro must be
declared before the first invocation of assert.

If level is greater than or equal to the value of the expression in the body
of _ASSERTION_LEVEL_ and condition evaluates to a nonzero value, then the
procedure named in _ASSERTION_PROC_ is invoked.

In the following example, the procedure APROC is invoked by the assert
statement, which is given a level of at least 10 and a condition that
evaluates to a nonzero value.

  pTAL Code                   Generated C++ Code

  ?ASSERTION = 10, APROC      #undef _ASSERTION_LEVEL_
                              #define _ASSERTION_LEVEL_ 10
                              #undef _ASSERTION_PROC_
                              #define _ASSERTION_PROC_ APROC
  ...                         ...
  PROC APROC;                 extern "C" void APROC()
  BEGIN                       {
    ...                         ...
  END;                        }

  ...                         ...
  j := 2;                     j = 2
  ASSERT 20:( j = 2 );        assert(20, (j == 2));


#endif
//-------------------------------------------------------------------------
// For pTAL assert statement
#define assert_at_level(level,expr)          \
  if (level >= _ASSERTION_LEVEL_ && (expr))  \
    _ASSERTION_PROC_();                      \
  else

//-------------------------------------------------------------------------
#if 0

The pTAL language specifies that if a labeled CASE statement, an unlabeled
CASE statement, or a CASE expression contains no default CASE specification,
and the selector expression evaluates to a value that does not match a case
alternative, then a run-time error occurs.

Emulating pTAL Case Statements and Expressions

The pTAL to C++ Translator generates code to force an arithmetic trap in
the default case of a switch statement when the original pTAL CASE statement
or expression contained no OTHERWISE clause.  The pTAL to C++ Translator
always generates this check for labeled CASE statements and for CASE
expressions, and it generates this check for unlabeled CASE statements when
the CHECK directive is in effect during translation.

The following example illustrates the translation of pTAL CASE statements
and expressions that contain no default cases.

  pTAL Code                   Generated C++ Code

  CASE x OF BEGIN             switch (x) {
    0 -> y := 10;               case 0 : y = 10;
    1 -> y := 11;                        break;
    2 -> y := 12;               case 1 : y = 11;
  END;                                   break;
                                case 2 : y = 12;
                                         break;
                                default: {
                                           _force_arith_trap();
                                         }
                              }
  ?CHECK
  CASE x OF BEGIN             switch (x) {
    y := 10;                    case 0 : y = 10;
    y := 11;                             break;
    y := 12;                    case 1 : y = 11;
  END;                                   break;
                                case 2 : y = 12;
                                         break;
                                default: _force_arith_trap();
                              }
  y := CASE x OF BEGIN        x = (x == 0 ? 10 :
         10;                       x == 1 ? 11 :
         11;                       x == 2 ? 12 :
         12;                       _force_arith_trap());
       END;


#endif
//-------------------------------------------------------------------------
_resident inline short _zero (void) { return 0; }
_resident inline short _force_arith_trap (void) { return _zero () /_zero (); }

//-------------------------------------------------------------------------
#if 0

Emulating pTAL Optimized For Loops

A pTAL optimized FOR loop uses a reserved index register, declared in a
USE statement, as a control variable.  The pTAL to C++ Translator generates
an invocation of the _for macro to emulate the intricate semantics of the
pTAL optimized FOR loop.

The _for macro emulates pTAL optimized FOR loop semantics.

  _for(control_variable,
       initial_value,
       limit)
    statement

control_variable
  is a 16-bit integer variable.

initial_value
  is a 16-bit integer expression whose value is assigned to the
  control_variable before the first iteration through the loop.

limit
  is a 16-bit integer expression that terminates the FOR loop s execution.

statement
  is any statement.

Usage Considerations

A pTAL optimized FOR loop uses a reserved index register, declared in a USE
statement, as a control variable.

The _for define preserves the behavior of pTAL optimized FOR loops.

- The limit is calculated once, at the beginning of the first iteration
  of the loop.
- If the initial_value is -32768, then the loop will execute zero times.
- If the limit is 32767, then overflow will not occur, as it does for
  non-optimized FOR loops.
- The value of the control_variable after normal termination of the loop
  is the limit, not the limit + 1 as it is for non-optimized FOR loops.

The following example illustrates the translation of a normal pTAL FOR
loop and an optimized pTAL FOR loop.

  pTAL Code                   Generated C++ Code

  INT(16) i;                  int_16 i;
  FOR i := 1 TO 10 DO         for (i = 1; i <= 10; ++i)
  USE j;                      int_16 j;
  FOR j := 1 TO 10 DO         _for (j, 1, 10)


#endif
//-------------------------------------------------------------------------
// This define implements pTAL "optimized" for loop semantics.  It
// preserves three funny behaviors of pTAL optimized for loops.  1) If
// lower bound is -32768, the loop will execute 0 times.  2)  If the
// upper bound is 32767, overflow will not occur, as it does for
// non-optimized for loops.  3)  The value of the control variable after
// normal termination of the loop is the limit, not the limit+1 as it is
// for non-optimized for loops.
#define _for(i,lb,ub) for (i = (unsigned)(lb)-1u;((int)(i) < \
                                                  (int)(ub)) ? i++,1 : 0;)

//-------------------------------------------------------------------------
#if 0

Emulating pTAL Condition Code Indicators

The pTAL to C++ Translator emulates pTAL condition code indicators by
generating Tandem C++ code that explicitly checks the outcome of the
procedure or assignment expression rather than checking the hardware
condition code settings.

Status of Condition Code

The following macros test a condition code value.  These macros mask
architectural differences in the definition of condition code settings
between the TNS and TNS/R native C++ compilers.

  int_16 _status_gt(int cc_value)
  int_16 _status_lt(int cc_value)
  int_16 _status_eq(int cc_value)
  int_16 _status_ge(int cc_value)
  int_16 _status_le(int cc_value)
  int_16 _status_ne(int cc_value)

cc_value
  is a condition code value.

Usage Considerations

These macros return the value 1 (TRUE) or the value 0 (FALSE).

Note
The _status_gt, _status_lt, and _status_eq defines are available in the
Tandem C++ runtime library.  The _status_ge, _status_le, and _status_ne
defines are available in the rosgen.h include file.

Example

The following example checks the outcome of assignment expressions.

  pTAL Code                   Generated C++ Code

  PROC P;                     extern "C" void P()
  BEGIN                       {
                                int_32       _cc_temp;
    INT(16) i16 := 8;           int_16       i16 = 8;
    INT(32) i32 := 7;           int_32       i32 = 7;
    FIXED(0) f0 := 5F;          fixed_0      f0 = 5LL;
    REAL(32) r32;               real_32      r32;
    REAL(64) r64;               real_64      r64;

    i16 := i16 + 8;             _cc_temp = i16 = i16 + 8;
    IF < THEN;                  if (_status_lt(_cc_temp))
                                  ;
    i32 := i32 + 2D;            _cc_temp = i32 = i32 + 2;
    IF > THEN                   if (_status_gt(_cc_temp))
      BEGIN END                   {}
    ELSE IF < THEN              else if (_status_lt(_cc_temp))
      BEGIN END;                       {};

    f0 := f0 + 2F;              _cc_temp = _determine_cc(
                                f0 = f0 + 2LL,0LL);
    IF > THEN;                  if (_status_gt(_cc_temp)) ;
    IF (f0 > 2F) THEN           if (_status_gt(_cc_temp =
                                     _determine_cc(
                                       f0,(fixed_0)2LL)))
      BEGIN END                   {}
    ELSE IF < THEN              else if (_status_lt(_cc_temp))
      BEGIN END;                       {};

    IF (r32 < 2.20E0) THEN      if (_status_lt(_cc_temp =
                                     _determine_cc(
                                       r32,(real_32)2.20E+0F)))
      BEGIN END                   {}
    ELSE IF < THEN;             else if (_status_lt(_cc_temp))
                                    ;
    IF (r64 > 2.20L0) THEN      if (_status_gt(_cc_temp =
                                     _determine_cc(
                                       r64,(real_64)2.20E+0)))
      BEGIN END                   {}
    ELSE IF < THEN;             else if (_status_lt(_cc_temp))
                                    ;
  END;                        }


The following example checks the outcome of a procedure.

  pTAL Code                   Generated C++ Code

  PROC FRED RETURNSCC;        extern "C" _cc_status RETCC()
  BEGIN                       {
    RETURN ,1;                  return 1;
  END;                        }

  PROC Q;                     extern "C" void Q()
  BEGIN                       {
                                int_32  _cc_temp;
    FRED;                       _cc_temp = FRED();
    IF > THEN;                  if (_status_lt(_cc_temp))
                                  ;
  END;                        }


Note

This model of checking condition codes differs from the normal way of
checking condition codes in C++.  See the C/C++ Programmer''s Guide for
more information about how to check condition codes in C++.



Returning Both a Value and a Condition Code

The pTAL language allows a function to return a value and also to set
condition codes.  The pTAL to C++ Translator generates code to package
a return value and a condition code for function return, then separate
the two values after they are returned.

The code generated by the pTAL to C++ Translator does interoperate with
pTAL language code.  A pTAL-coded function that returns a value and has
the RETURNSCC attribute can be called from a routine that has been
generated by the pTAL to C++ Translator, and vice versa, because the
format of the return result is identical.

The following example illustrates the code that the pTAL to C++ Translator
generates.

  pTAL Code                   Generated C++ Code

  PROC fred (x), RETURNSCC;   extern "C" _cc_status FRED(
     INT(16) x;                                     int_16 x)
  BEGIN                       {
     RETURN ,0;                  return 0;
  END;                        }

  INT(16) PROC barney (x),    extern "C" _int_16_CC BARNEY(
               RETURNSCC;                           int_16 x)
     INT(16) x;               {
  BEGIN                          _return_2(x*2,1);
     RETURN x*2,1;            }
  END;

   ...                         ...
   CALL fred(2);               _cc_temp = FRED(2);
   IF < THEN                   if (_status_lt(_cc_temp))
      ...                         ...
   temp := barney(3);          temp = _split_int_16_CC(
                                      BARNEY(3),_cc_temp);

   IF < THEN                   if (_status_lt(_cc_temp))
      ...                         ...


#endif
//-------------------------------------------------------------------------
// condition codes

#define _cc_status int
#define _status_lt(x) ((x) < 0)
#define _status_gt(x) ((x) > 0)
#define _status_eq(x) ((x) == 0)
#define _status_le(x) ((x) <= 0)
#define _status_ge(x) ((x) >= 0)
#define _status_ne(x) ((x) != 0)

//-------------------------------------------------------------------------
#if 0

Determining a Condition Code Value

The outcome of an assignment expression whose operand type is a 16-bit
integer or a 32-bit integer can be checked by using the value of the
expression as a condition code value.  The outcome of a conditional
operation of any operand type or of an assignment expression with
another type requires that an integer condition code value be obtained
from the assignment expression or from the operands of the conditional
operation.

_determine_cc

The _determine_cc functions determine a condition code value from a pair of
values.

  int _determine_cc(type first_operand,
                    type second_operand)

type
  is a scalar type, such as: unsigned_char, int_16, int_32, int_64,
  unsigned_16, unsigned_32, real_32, real_64.

first_operand
  is the first operand.

second_operand
  is the second operand.

Usage Considerations

The first_operand is the first operand of a conditional operation whose
result is to be checked via a condition code, or is an assignment
expression whose value''s relationship to zero is to be checked via a
condition code.

The second_operand is the second operand of a conditional operation
whose result is to be checked via a condition code, or is the value
zero which is to be checked against the value of an assignment
expression via a condition code.

#endif
//-------------------------------------------------------------------------
template<class T1, class T2> _resident inline int_32 _determine_cc(T1 x,T2 y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
}

//----------------------------------------------------------------------------
//  Implement Ptal RETURNSCC:
//----------------------------------------------------------------------------
#ifdef NA_64BIT
// dg64 - define structure to return value and cc
struct _val_cc_combo {
  int_ptr _value;
  int_32  _cc;
};
typedef _val_cc_combo _unsigned_char_CC;
typedef _val_cc_combo _int_16_CC;
typedef _val_cc_combo _int_32_CC;
typedef _val_cc_combo _real_32_CC;
typedef _val_cc_combo _unsigned_char_CC;
typedef _val_cc_combo _baddr_CC;
typedef _val_cc_combo _waddr_CC;
typedef _val_cc_combo _extaddr_CC;
typedef _val_cc_combo _procaddr_CC;
typedef _val_cc_combo _cbaddr_CC;
typedef _val_cc_combo _cwaddr_CC;
typedef _val_cc_combo _int_64_CC;
#else
typedef int_64 _unsigned_char_CC;
typedef int_64 _int_16_CC;
typedef int_64 _int_32_CC;
typedef int_64 _real_32_CC;
typedef int_64 _unsigned_char_CC;
typedef int_64 _baddr_CC;
typedef int_64 _waddr_CC;
typedef int_64 _extaddr_CC;
typedef int_64 _procaddr_CC;
typedef int_64 _cbaddr_CC;
typedef int_64 _cwaddr_CC;

struct _val_cc_combo {
  union {
    _int_32_CC _combo_item;
    struct {
      int_32  _value;
      int_32  _cc;
    } _parts;
  };
};
#endif

//-------------------------------------------------------------------------
#if 0

_return_2

The _return_2 macro packages up a value and a condition code and returns
it from a function whose pTAL-coded counterpart returned both a value and
a condition code.  The type of the value returned is one of those described
below.

  _return_2(val, cc_value)

val
  is a value to be returned by the function.

cc_value
  is the condition code value to be returned by the function.

Usage Considerations

The type of val is an unsigned_char, an int_16, an int_32, a real_32,
a baddr, a waddr, an extaddr, a procaddr, a cbaddr, or a cwaddr.

cc_value can be decoded using the _status_gt, _status_lt, _status_eq,
_status_ge, _status_le, _status_ne macros.


#endif
//-------------------------------------------------------------------------
#ifdef NA_64BIT
// dg64 - fill in combo and return it
#define _return_2(val,cc)                     \
  _begin                                      \
  _val_cc_combo _set_cc_combo;                \
  _set_cc_combo._cc = cc;                     \
  _set_cc_combo._value = (int_ptr) val;       \
  return _set_cc_combo;                       \
  _end
#else
#define _return_2(val,cc)                     \
  _begin                                      \
  _val_cc_combo _set_cc_combo;                \
  _set_cc_combo._parts._cc = cc;              \
  _set_cc_combo._parts._value = (int_32) val; \
  return _set_cc_combo._combo_item;           \
  _end
#endif

#ifdef NA_64BIT
// dg64 - set cc and return value
#define _create_split_val_cc(type, name)               \
  inline type name (_##type##_CC _vcc, int_32 &_cc) {  \
    _cc = _vcc._cc;                                    \
    return (type) _vcc._value;                         \
}
#else
#define _create_split_val_cc(type, name)               \
  inline type name (_##type##_CC _vcc, int_32 &_cc) {  \
    _val_cc_combo  _combine_em;                        \
    _combine_em._combo_item = _vcc;                    \
    _cc = (int_16) _combine_em._parts._cc;             \
    return (type) _combine_em._parts._value;           \
}
#endif

//-------------------------------------------------------------------------
#if 0

_split_int_16_CC, et. al.

The _split_int_16_CC function separates a return value from a condition
code.  A combined return value and condition code is returned by a C++
function generated by the pTAL to C++ Translator whose pTAL counterpart
returned a value and implicitly set a condition code.  Similar functions
exist for all types that can be returned from a pTAL function that sets
condition codes.  The functions _split_sgbaddr_CC, _split_sgwaddr_CC,
_split_sgxbaddr_CC, and _split_sgxwaddr_CC are defined in the rossg.h
include file; all others are defined in the rosgen.h include file.

  unsigned_char _split_unsigned_char_CC(_unsigned_char_CC value_and_cc,
                                        int_32 &cc_value)
  int_16 _split_int_16_CC(_int_16_CC value_and_cc,
                          int_32 &cc_value)
  int_32 _split_int_32_CC(_int_32_CC value_and_cc,
                          int_32 &cc_value)
  real_32 _split_real_32_CC(_real_32_CC value_and_cc,
                            int_32 &cc_value)
  baddr _split_baddr_CC(_baddr_CC value_and_cc,
                        int_32 &cc_value)
  waddr _split_waddr_CC(_waddr_CC value_and_cc,
                        int_32 &cc_value)
  extaddr _split_extaddr_CC(_extaddr_CC value_and_cc,
                            int_32 &cc_value)
  procaddr _split_procaddr_CC(_procaddr_CC value_and_cc,
                              int_32 &cc_value)
  cbaddr _split_cbaddr_CC(_cbaddr_CC value_and_cc,
                          int_32 &cc_value)
  cwaddr _split_cwaddr_CC(_cwaddr_CC value_and_cc,
                          int_32 &cc_value)
  sgbaddr _split_sgbaddr_CC(_sgbaddr_CC value_and_cc,
                            int_32 &cc_value)
  sgwaddr _split_sgwaddr_CC(_sgwaddr_CC value_and_cc,
                            int_32 &cc_value)
  sgxbaddr _split_sgxbaddr_CC(_sgxbaddr_CC value_and_cc,
                              int_32 &cc_value)
  sgxwaddr _split_sgxwaddr_CC(_sgxwaddr_CC value_and_cc,
                              int_32 &cc_value)

value_and_cc
  is a combined value and condition code.

cc_value
  is the condition code value separated from the value.

Usage Considerations

The functions return the value separated from the condition code.


#endif
//-------------------------------------------------------------------------
_create_split_val_cc(unsigned_char, _split_unsigned_char_CC)
_create_split_val_cc(int_16,        _split_int_16_CC       )
_create_split_val_cc(int_32,        _split_int_32_CC       )
_create_split_val_cc(real_32,       _split_real_32_CC      )
_create_split_val_cc(baddr,         _split_baddr_CC        )
_create_split_val_cc(waddr,         _split_waddr_CC        )
_create_split_val_cc(extaddr,       _split_extaddr_CC      )
_create_split_val_cc(procaddr,      _split_procaddr_CC     )
_create_split_val_cc(cbaddr,        _split_cbaddr_CC       )
_create_split_val_cc(cwaddr,        _split_cwaddr_CC       )
#ifdef NA_64BIT
// dg64 - return 64-bit
_create_split_val_cc(int_64,        _split_int_64_CC       )
#endif

#if 0 // Replace overloaded _determine_cc's with the template above.
//-------------------------------------------------------------------------
// These routines compare the input parameters and return
// the following results:
//
//          if                     | then return
//     ----------------------------+------------
//      x > y (signed)             |     1
//      x < y (signed)             |    -1
//      x > y (unsigned)           |     1
//      x < y (unsigned)           |    -1
//      x = y (signed or unsigned) |     0

inline int_32 _determine_cc (unsigned_char x, unsigned_char y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
} // _determine_cc

inline int_32 _determine_cc (int_16 x, int_16 y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
} // _determine_cc

inline int_32 _determine_cc (unsigned_char x, int_16 y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
} // _determine_cc

inline int_32 _determine_cc (int_16 x, unsigned_char y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
} // _determine_cc

inline int_32 _determine_cc (unsigned_16 x, unsigned_16 y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
} // _determine_cc

inline int_32 _determine_cc (unsigned_32 x, unsigned_32 y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
} // _determine_cc

inline int_32 _determine_cc (int_32 x, int_32 y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
} // _determine_cc


inline int_32 _determine_cc (real_32 x, real_32 y) {
  return ((int_32)((x) > (y)) - (int_32)((x) < (y)));
} // _determine_cc

#endif

//-------------------------------------------------------------------------
#if 0

_abs

The _abs macro returns the absolute value of its argument.

  int_16  _abs(int_16  expr)
  int_32  _abs(int_32  expr)
  int_64  _abs(int_64  expr)
  real_32 _abs(real_32 expr)
  real_64 _abs(real_64 expr)

expr
  is an integer or real type expression.

Usage Considerations

The _abs macro returns the numerical value of expr without regard to its sign.

The absolute value of the most negative integer cannot be represented.

The result of the _abs macro has the same type as expr.

The _abs function is equivalent to the pTAL $ABS standard function

Unlike the C++ library function abs, the _abs macro can take a real number as an
argument.

The following example illustrates a use of the _abs function with an integer
expression and a use with a real expression.

  pTAL Code                   Generated C++ Code

  INT(32) x := -2;            int_32  x = -2;
  REAL(32) r := 2.20E0;       real_32 r =  2.20E+0F;

  x := $ABS(x);               x = _abs((int_32)x);
  r := $ABS(r);               r = _abs((real_32)r);


#endif
//-------------------------------------------------------------------------
#undef _abs
#define _abs(x) ((x) < 0 ? -(x) : (x))

//-------------------------------------------------------------------------
#if 0

_alpha

The _alpha function returns the value -1 (TRUE) if the low-order 8 bits of
its argument contain the ASCII representation of an alphabetic character,
and returns the value 0 (FALSE) if not.

  int_16 _alpha(int_16 expr)

expr
  is an expression whose low-order byte  might represent the ASCII encoding of an
  alphabetic character.

Usage Considerations

The _alpha function is equivalent to the pTAL $ALPHA standard function,
except that it cannot be used in a static context, such as an array bounds
specification, an enumeration constant, a bit field width specifier, or a
case label constant.

Unlike the C++ standard function isalpha, the _alpha function ignores the
high-order 8 bits of expr.

The following example illustrates a use of the _alpha function with a 16-bit
integer argument, and a use with a char type argument that is automatically
promoted by the C++ compiler to a 16-bit integer.

  pTAL Code                   Generated C++ Code

  INT(16) result;             int_16 result;
  INT(16) int_val;            int_16 int_val;
  STRING  c_val;              char   c_val;

  int_val := "a";             int_val = 'a';
  c_val := "Q";               c_val = 'Q';
  IF $ALPHA(int_val) THEN ... if (_alpha(int_val)) ...
  IF $ALPHA(c_val) THEN ...   if (_alpha(c_val)) ...


#endif
//-------------------------------------------------------------------------
_resident inline int_16 _alpha (int_16 i) {
#if ROSETTA_BIG_ENDIAN
  return -((int_16)((((unsigned)(i & 0xFF) - (unsigned)'A') <= 25u) |
                    (((unsigned)(i & 0xFF) - (unsigned)'a') <= 25u)));
#else
  return -((int_16)((((unsigned)(i % 0x100) - (unsigned)'A') <= 25u) |
                    (((unsigned)(i % 0x100) - (unsigned)'a') <= 25u)));
#endif
} // _alpha

//-------------------------------------------------------------------------
#if 0

_exchange

The _exchange function exchanges the contents of the two variables given as
parameters.

  void _exchange(type *var1,
                 type *var2)

type
  is scalar type, such as: unsigned_char, int_16, int_32, int_64, unsigned_16,
  unsigned_32, real_32, real_64, void *.

var1
  is a pointer to a variable.

var2
  is a pointer to a variable of the same type as var1.

Usage Considerations

var1 gets the value of var2 and var2 gets the value of var1.

Indexing and field selection are allowed in var1 and var2, but the
expression must be a nonstruct lvalue.

The _exchange function is equivalent to the pTAL $EXCHANGE standard
function, except that it cannot be used in a static context, such as
an array bounds specification, an enumeration constant, a bit field
width specifier, or a case label constant.

The following example illustrates the use of the _exchange function on
variables of type int_16 and on variables of type real_32.

  pTAL Code                   Generated C++ Code

  INT(16) a, b;               int_16 a,b;
  REAL(32) c, d;              real_32 c,d;

  $EXCHANGE(a,b);             _exchange(&a,&b);
  $EXCHANGE(c,d);             _exchange(&c,&d);


#endif
//-------------------------------------------------------------------------

template<class T1, class T2> _resident inline void _exchange (T1 *p1, T2 *p2) {
  T1 temp;
  temp = (T1)*p1;
  *p1 = (T1)*p2;
  *p2 = (T2)temp;
}


#if 0 // the following overloaded _exchange's are replaced by the
      // template above.
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//pTAL no longer allows STRING arguments, so Rosetta should not generate char
//type arguments to _exchange (ditto for all types except int_16 and int_32)
inline void _exchange(unsigned_char *s1,
                      unsigned_char *s2) {
  unsigned_char temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}
inline void _exchange(int_16 *s1,
                      int_16 *s2) {
  int_16 temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}
inline void _exchange(int_32 *s1,
                      int_32 *s2) {
  int_32 temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}
inline void _exchange(unsigned_16 *s1,
                      unsigned_16 *s2) {
  unsigned_16 temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}
inline void _exchange(unsigned_32 *s1,
                      unsigned_32 *s2) {
  unsigned_32 temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}
inline void _exchange(real_32 *s1,
                      real_32 *s2) {
  real_32 temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}
inline void _exchange(real_64 *s1,
                      real_64 *s2) {
  real_64 temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}
inline void _exchange(void **s1,
                      void **s2) {
  void *temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}
#endif

//-------------------------------------------------------------------------
#if 0

_dbl

The _dbl macro creates an int_32 type value by placing the first int_16 type
argument in the high-order 16 bits and the second int_16 type argument in
the low-order 16 bits.

  int_32 _dbl(int_16 expr1,
              int_16 expr2)

expr1
  is an expression.

expr2
  is an expression.

Usage Considerations

The _dbl macro creates a value of type int_32 by placing expr1 in the
high-order 16 bits and expr2 in the low-order 16 bits.

The _dbl macro is equivalent to the pTAL $DBLL standard function.

The following example places the value 1 in the high-order 16 bits of the
variable result and places the value 125 in the low-order 16 bits of the
variable result.

  pTAL Code                   Generated C++ Code

  INT(32) result;             int_32 result;
  INT(16) a := 1;             int_16 a = 1;
  INT(16) b := 125;           int_16 b = 125;

  result := $DBLL(a,b);       result = _dbl(a,b);


#endif
//-------------------------------------------------------------------------
// implements pTAL builtin $DBLL and $UDBL
#if ROSETTA_BIG_ENDIAN
#define _dbl(a,b) (((a) << 16) + (((int_32)(b)) & 0xffff))
#else
#define _dbl(a,b) (((b) << 16) + (((int_32)(a)) & 0xffff))
#endif

#ifdef NA_64BIT
// dg64 - make 64-bit dbl from 2 32-bit numbers
#define _xdbl(a,b) (((unsigned_64)(a) << 32) + (((unsigned_64)(b)) & 0xffffffff))
#else
#define _xdbl(a,b) (((unsigned_32)(a) << 16) + (((unsigned_32)(b)) & 0xffff))
#endif

//-------------------------------------------------------------------------
#if 0

_high

The _high macro returns the 16-bit value that occupies the high-order
16 bits of the 32-bit argument.

  int_16 _high(int_32 expr)

expr
  is an expression.

Usage Considerations

The _high macro returns the 16-bit value that occupies the high-order
16 bits of expr.

The _high macro ignores the low-order 16 bits of expr.

The _high macro is equivalent to the pTAL $HIGH standard function.

In the following example, the variable result gets the value 1, which is
the value that occupies the high-order 16 bits of the variable a.

  pTAL Code                   Generated C++ Code

  INT(16) result;             int_16 result;
  INT(32) a := %H1FFFF%D;     int_32 a = 0x1FFFF;

  result := $HIGH(a);         result = _high(a);


#endif
//-------------------------------------------------------------------------
#if ROSETTA_BIG_ENDIAN
#define _high(a) ((int_16)((int_32)(a) >> 16))
#else
#define _high(a) ((int_16)((int_32)(a) / (unsigned_32)0x10000))
#endif

#ifdef NA_64BIT
#define _xhigh(a) ((int_32)((int_64)(a) >> 32))
#else
#define _xhigh(a) ((int_16)((int_32)(a) >> 16))
#endif
//-------------------------------------------------------------------------
#if 0

_int_ov

The _int_ov function returns the 16-bit value that occupies the low-order
16 bits of the 32-bit argument, setting the overflow indicator as necessary.

  int_16 _int_ov(int_32 expr,
                 int_16 *ov_temp)

expr
  is an expression.

ov_temp
  is a reference paramter returning the overflow status.

Usage Considerations

The _int_ov function returns the 16-bit value that occupies the low-order
16 bits of expr.

The _int_ov function ignores the high-order 16 bits of expr.

The ov_temp reference parameter returns a nonzero value if the expr is
greater than 32767 or less than -32768.

The _int_ov function is equivalent to the pTAL $INT_OV standard function.

In the following example, the variable result gets the value 0xFFF0 which
is the value that occupies the low-order 16 bits of the variable a.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
                              int_16 _ov_temp;
  INT(16) result;             int_16 result;
  INT(32) a := %H1FFF0%D;     int_32 a = 0x1FFF0;

  result := $INT_OV(a);       result = _int_ov(a,&_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)


#endif
//-------------------------------------------------------------------------
#define _int_ov(i,ov) ((*(ov) = (_low (i) != (i))), _low (i))

//-------------------------------------------------------------------------
#if 0

_len

The _len macro returns the storage size of the argument, in bytes.

  int_16 _len(expr)
  int_16 _len(type_name)

expr
  is an expression.

type_name
  is the name of a type.

Usage Considerations

The size is determined from the declarations of the variables in the
expression expr or, if applied to a type_name, the result is the size
in bytes of a variable of the indicated type.

The storage size of an aggregate type is the total number of bytes in
the aggregate, including any filler bytes required for alignment purposes.

The _len macro is equivalent to the pTAL $LEN standard function except:

- The _len macro counts implicit filler bytes.

- The _len macro always returns a nonzero value, even for structures that
  contain no space-allocating items.  The pTAL to C++ Translator translates
  an invocation of the $LEN standard funtion operating on a structure that
  contains no space-allocating items to a reference to a constant representing
  the correct size.

- For array variables, the _len macro returns the size of the entire array,
  not the size of one element which $LEN returns.  The pTAL to C++ Translator
  translates an array argument found in a $LEN standard function call to a
  single element argument in a _len macro invocation.

This macro differs from the operator sizeof in that it returns a signed
16-bit integer type value, while sizeof returns a size_t type value.

The following example illustrates the translation of the pTAL $LEN standard
function.

  pTAL Code                   Generated C++ Code

  STRUCT s(*);                struct s {
  BEGIN                         int_16 i;
    INT(16) i;                };
  END;
  INT(16) val;                int_16 val;
  STRUCT t(*);                struct t {
  BEGIN                         //*** If you add any data to
  END;                          //*** this struct, change this
                                //*** literal to reflect the
                                //*** new size of the struct.
                                enum {_size = 0;};
                              };
  INT(16) a[0:9];             int_16 a[10];
  INT(16) result;             int_16 result;

  result := $LEN(s);          result = _len(s);
  result := $LEN(val);        result = _len(val);
  result := $LEN(t);          result = t::_size;
  result := $LEN(a);          result = _len(*a);


#endif
//-------------------------------------------------------------------------
// sizeof returns an unsigned value; _len casts it to signed
#define _len(x) ((int)sizeof(x))

//-------------------------------------------------------------------------
#if 0

_lmax

The _lmax macro returns the greater of its arguments, comparing two unsigned
values.

  int_16 _lmax(expr1,
               expr2)

expr1
  is a 16-bit integer expression.

expr2
  is a 16-bit integer expression.

Usage Considerations

The _lmax macro evaluates to the greater of its arguments after the types
of those arguments have been cast to an unsigned_16 type.  Its resultant
type is int_16.

The _lmax macro evaluates the greater argument more than once, so you should
avoid providing arguments with side-effects.

The _lmax macro is equivalent to the pTAL $LMAX standard function.

In the following example, the value of the second argument to _lmax is
greater than the value of the first argument, so the value of the second
argument, -2, is stored in the variable result.

  pTAL Code                   Generated C++ Code

  INT(16) k := 65534;         int_16 k = -2;
  INT(16) result;             int_16 result;

  result := $LMAX(13,k);      result = _lmax(13,k);


#endif
//-------------------------------------------------------------------------
#define _lmax(a,b) (int_16)_max((unsigned_16)(a),(unsigned_16)(b))

//-------------------------------------------------------------------------
#if 0

_lmin

The _lmin macro returns the lesser of its arguments, comparing two unsigned
values.

  int_16 _lmin(expr1,
               expr2)

expr1
  is a 16-bit integer expression.

expr2
  is a 16-bit integer expression.

Usage Considerations

The _lmin macro evaluates to the lesser of its arguments after the types
of those arguments have been cast to an unsigned_16 type.  Its resultant
type is int_16.

The _lmin macro evaluates the lesser argument more than once, so you should
avoid providing arguments with side-effects.

The _lmin macro is equivalent to the pTAL $LMIN standard function.

In the following example, the value of the second argument to _lmin is
lesser than the value of the first argument, so the value of the second
argument, 21, is stored in the variable result.

  pTAL Code                   Generated C++ Code

  INT(16) k := 21;            int_16 k = 21;
  INT(16) result;             int_16 result;

  result := $LMIN(65534,k);   result = _lmin(-2,k);


#endif
//-------------------------------------------------------------------------
#define _lmin(a,b) (int_16)_min((unsigned_16)(a),(unsigned_16)(b))

//-------------------------------------------------------------------------
#if 0

_low

The _low function returns the 16-bit value that occupies the low-order
16 bits of the 32-bit argument.

  int_16 _low(int_32 expr)

expr
  is an expression.

Usage Considerations

The _low function returns the 16-bit value that occupies the low-order
16 bits of expr.

The _low function ignores the high-order 16 bits of expr.

The _low function is equivalent to the pTAL $INT standard function.

In the following example, the variable result gets the value 0xFFF0 which
is the value that occupies the low-order 16 bits of the variable a.

  pTAL Code                   Generated C++ Code

  INT(16) result;             int_16 result;
  INT(32) a := %H1FFF0%D;     int_32 a = 0x1FFF0;

  result := $INTR(a);         result = _low(a);


#endif
//-------------------------------------------------------------------------
#define _low(x) ((int_16)((int_32)(x)))

#ifdef NA_64BIT
#define _xlow(x) ((int_32)((int_64)(x)))
#else
#define _xlow(x) ((int_16)((int_32)(x)))
#endif
//-------------------------------------------------------------------------
#if 0

_lowbyte

The _lowbyte macro returns the 8 bit value that occupies the low-order
byte of the argument.

  int_16 _lowbyte(int expr)

expr
  is a 16-bit or 32-bit integer expression.

Usage Considerations

The _lowbyte macro returns the value that occupies the low-order byte of expr.

The _lowbyte macro ignores the higher-order bits of expr.

In the following example, the variable result gets the value 0xF0 which
is the value that occupies the low-order byte of the variable a.

  pTAL Code                   Generated C++ Code

  STRING a[0:3]               unsigned_char a[4]
        := ["a","b",34,12D];        = {'a','b',_lowbyte(34),
                                       _lowbyte(12)};


#endif
//-------------------------------------------------------------------------
#if ROSETTA_BIG_ENDIAN
#define _lowbyte(p) ((unsigned_char)(((int)p) & 0xff))
#else
#define _lowbyte(p) ((unsigned_char)(((int)p) % 0x100))
#endif

//-------------------------------------------------------------------------
#if 0

_max

The _max macro returns the greater of its arguments.

  _max(expr1,
       expr2)

expr1
  is an expression of a type for which the greater than operator ( > ) is defined.

expr2
  is an expression of a type compatible with expr1.

Usage Considerations

The _max macro evaluates to the lesser of its arguments.  Its resultant
type is the type of that argument.

The _max macro is equivalent to the pTAL $MAX standard function, except that
it evaluates the lesser argument more than once, so you should avoid providing
arguments with side-effects.

In the following example, the value of the second argument to _max is
greater than the value of the first argument, so the value of the second
argument, 21, is stored in the variable result.

  pTAL Code                   Generated C++ Code

  INT(16) k := 21;            int_16 k = 21;
  INT(16) result;             int_16 result;

  result := $MAX(13,k);       result = _max(13,k);



_min

The _min macro returns the lesser of its arguments.

  _min(expr1,
       expr2)

expr1
  is an expression of a type for which the less than operator ( < ) is defined.

expr2
  is an expression of a type compatible with expr1.

Usage Considerations

The _min macro evaluates to the lesser of its arguments.  Its resultant
type is the type of that argument.

The _min macro is equivalent to the pTAL $MIN standard function, except that
it evaluates the lesser argument more than once, so you should avoid providing
arguments with side-effects.

In the following example, the value of the first argument to _min is less
than the value of the second argument, so the value of the first argument,
13, is stored in the variable result.

  pTAL Code                   Generated C++ Code

  INT(16) k := 21;            int_16 k = 21;
  INT(16) result;             int_16 result;

  result := $MIN(13,k);       result = _min(13,k);


#endif
//-------------------------------------------------------------------------
#ifdef _min
#undef _min
#endif
#ifdef _max
#undef _max
#endif
#define _min(a,b) (((a) < (b)) ? (a) : (b))
#define _max(a,b) (((a) > (b)) ? (a) : (b))

//-------------------------------------------------------------------------
#if 0

_numeric

The _numeric function returns the value -1 (TRUE) if the low-order 8 bits
of its argument contain the ASCII representation of a numeric character,
and returns the value 0 (FALSE) if not.

  int_16 _numeric(int_16 expr)

expr
  is an expression whose low-order byte  might represent the ASCII encoding of a
numeric character.

Usage Considerations

The _numeric function is equivalent to the pTAL $NUMERIC standard function,
except that it cannot be used in a static context, such as an array bounds
specification, an enumeration constant, a bit field width specifier, or a
case label constant.

Unlike the C++ standard function isdigit, the _numeric function ignores the
high-order 8 bits of expr.

The following example illustrates a use of the _numeric macro with a 16-bit
integer argument, and a use with a char type argument that is automatically
promoted by the C++ compiler to a 16-bit integer.

  pTAL Code                   Generated C++ Code

  INT(16) result;             int_16 result;
  INT(16) int_val;            int_16 int_val;
  STRING  c_val;              char   c_val;

  int_val := "2";             int_val = '2';
  c_val := "5";               c_val = '5';
  IF $NUMERIC(int_val) THEN   if (_numeric(int_val)) ...
  IF $NUMERIC(c_val) THEN     if (_numeric(c_val)) ...


#endif
//-------------------------------------------------------------------------
_resident inline int_16 _numeric (int_16 i) {
#if ROSETTA_BIG_ENDIAN
  return -((int_16)(((unsigned)(i & 0xFF) - (unsigned)'0') <= 9u));
#else
  return -((int_16)(((unsigned)(i % 0x100) - (unsigned)'0') <= 9u));
#endif
} // _numeric

//-------------------------------------------------------------------------
#if 0

_num_elems_varname

The _num_elems_variable_name enumeration evaluates to the number of
elements in the array variable.

  _num_elems_variable_name

variable_name
  is the name of an array variable defined by the macros listed below.

Usage Considerations

The _num_elems_variable_name enumeration is defined by the following macros,
all supplied in the rosgen.h include file, which expand to the definition of
an array.
  - _equiv_array
  - _equiv_array_global
  - _equiv_array_offset
  - _equiv_array_offset_global
  - _const_array
  - _const_char_array
  - _initialized_array
  - _initialized_char_array

The pTAL to C++ Translator generates the _num_elems_variable_name
enumeration as the translation of the pTAL $OCCURS standard function when
that function operates on an array variable defined with one of the macros
listed above.

  pTAL Code                   Generated C++ Code

  STRING hello[1:10]          _initialized_char_array(
     := ["hello"];               unsigned_char,hello,1,10,
                                 "hello");
  STRING bye[0:9] := ["bye"]; unsigned_char bye[10] = {"bye"};
  INT(16) x;                  int_16        x;

  x := $OCCURS(hello);        x = _num_elems_hello;
  x := $OCCURS(bye);          x = _occurs(bye);

For More Information.  See the discussion of the _occurs macro for more
information about the translation of the pTAL $OCCURS standard function.



_occurs

The _occurs macro returns the number of elements in the variable provided as
a parameter.

  size_t _occurs(array_variable)

array_variable
  is the name of an array variable or structure field.

Usage Considerations

The _occurs macro returns unpredictable results when the array_variable
supplied is not actually an array variable or array structure field.

The _occurs macro is equivalent to the pTAL $OCCURS standard function, except
that it only operates on arrays.

In the following example, the first expansion of the _occurs macro
evaluates to the integer value 4.  The second expansion of the _occurs
macro evaluates to the integer value 5.

  pTAL Code                   Generated C++ Code

  INT(16) x;                  int_16 x;
  INT(16) a[0:3];             int_16 a[4];
  STRUCT spade(*);            struct spade {
  BEGIN                         int_16 b[5];
    INT(16) b[0:4];           };
  END;
  STRUCT sam(spade);          spade sam;

  x := $OCCURS(a);            x = _occurs(a);
  x := $OCCURS(sam.b);        x = _occurs(sam.b);
  x := $OCCURS(x);            x = 1;

For More Information.  See the discussion of the _num_elems_variable_name
macro for more information about the translation of the pTAL $OCCURS
standard function when the argument is an array variable declared with
one of the following macros:
  _equiv_array, _equiv_array_global, _equiv_array_offset,
  _equiv_array_offset_global, _const_array, _const_char_array,
  _initialized_array, and _initialized_char_array.


#endif
//-------------------------------------------------------------------------
#define _occurs(a) ((sizeof (a)) / (sizeof ((a)[0])))

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Note: _pow_of_10 is described near fixed point scaling

//-------------------------------------------------------------------------
#if 0

_special

The _special function evaluates to the value -1 (TRUE) if the low-order
8 bits of its argument contain the ASCII representation of a character
that is neither alphabetic nor numeric, and evaluates to the value 0
(FALSE) if the low-order 8 bits of its argument contains the ASCII
representation of an alphabetic or numeric character.

  int_16 _special(int_16 expr)

expr
  is an expression whose low-order byte might represent the ASCII encoding
  of a character.

Usage Considerations

The _special function is equivalent to the pTAL $SPECIAL standard function,
except that it cannot be used in a static context, such as an array bounds
specification, an enumeration constant, a bit field width specifier, or a
case label constant.

The _special function ignores the high-order 8 bits of expr.

The following example illustrates a use of the _special function with a
16-bit integer argument, and a use with a char type argument that is
automatically promoted by the C++ compiler to a 16-bit integer.

  pTAL Code                   Generated C++ Code

  INT(16) result;             int_16 result;
  INT(16) int_val;            int_16 int_val;
  STRING  c_val;              char   c_val;

  int_val := "!";             int_val = '!';
  c_val := "@";               c_val = '@';
  IF $SPECIAL(int_val) THEN   if (_special(int_val)) ...
  IF $SPECIAL(c_val) THEN     if (_special(c_val)) ...


#endif
//-------------------------------------------------------------------------
_resident inline int_16 _special (int_16 i) {
#if ROSETTA_BIG_ENDIAN
  return -((int_16)((((unsigned)(i & 0xFF) - (unsigned)'A') > 25u) &
                    (((unsigned)(i & 0xFF) - (unsigned)'a') > 25u) &
                    (((unsigned)(i & 0xFF) - (unsigned)'0') >  9u)));
#else
  int_16 temp = i % 0x100;
  return -((int_16)((((unsigned)(temp) - (unsigned)'A') > 25u) &
                    (((unsigned)(temp) - (unsigned)'a') > 25u) &
                    (((unsigned)(temp) - (unsigned)'0') >  9u)));
#endif
} // _special

//-------------------------------------------------------------------------
#if 0

_udivrem

The _udivrem function calculates a quotient and remainder for an integer
divisor and dividend, checking for overflow.

  int_16 _udivrem(int_32 dividend,
                  int_16 divisor,
                  int_16 *quotient,
                  int_16 *remainder)

  int_16 _udivrem(int_32 dividend,
                  int_16 divisor,
                  int_32 *quotient,
                  int_16 *remainder)

dividend
  is the value to be divided.

divisor
  is the value by which dividend is divided.

quotient
  is a container for the result obtained by dividing dividend by divisor.

remainder
  is a container for the difference between the divisor times the quotient
  and the dividend.

Usage Considerations

The _udivrem function returns the value 1 if the divisor is zero.  The
version of this function with a 16-bit quotient returns the value 1 if
the quotient cannot fit into a 16-bit container.  Otherwise, this function
returns the value 0.

The quotient and remainder are undefined when an overflow is reported.

These functions are equivalent to the pTAL standard functions $UDIVREM16 and
$UDIVREM32.

The following example illustrates a use of each version of the _udivrem
function; one calculating a 16-bit quotient and the other calculating a
32-bit quotient.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
  ...                         ...
  INT(32) quotient1;          int_32 quotient1;
  INT(16) quotient2;          int_16 quotient2;
  INT(16) remainder;          int_16 remainder;
                              int_16 _ov_temp;

  $UDIVREM(100D,10,           _udivrem(100,10,&quotient1,
     quotient1,remainder);           &remainder);
  $UDIVREM(100D,10,           _ov_temp = _udivrem(100,10,
     quotient2,remainder);           &quotient2,&remainder);
  IF $OVERFLOW THEN ...       if (_ov_temp) ...


#endif
//-------------------------------------------------------------------------
//#ifndef __TANDEM  It appears _udivrem32 doesn't work in native C++
// This version of _udivrem32x is being used until it does.  It is
// only called by the two overloaded _udivrem's that immediately follow.
_resident inline int_16 _udivrem32x(int_32 dividend,
                                    int_16 divisor,
                                    int_32 *quotient,
                                    int_16 *remainder) {
  if (divisor == 0)
    return 1;
  union {
    int_32 signed_value;
    unsigned_32 unsigned_value;
  } quo;
  union {
    int_16 signed_value;
    unsigned_16 unsigned_value;
  } rem;
  quo.unsigned_value = (unsigned_32)dividend / (unsigned_16)divisor;
  *quotient = quo.signed_value;
  rem.unsigned_value = (unsigned_32)dividend -
                       quo.unsigned_value * (unsigned_16)divisor;
  *remainder = rem.signed_value;
  return 0;
}
//#endif _udivrem32

_resident inline int_16 _udivrem(int_32 dividend,
                                 int_16 divisor,
                                 int_16 *quotient,
                                 int_16 *remainder) {
  union {
    int_32 int32val;
    int_16 int16val[2];
  } quo;
  int_16 result = _udivrem32x(dividend, divisor, &quo.int32val, remainder);
  *quotient = quo.int16val[1];
  return result || (quo.int16val[0] != 0);
}
_resident inline int_16 _udivrem(int_32 dividend,
                                 int_16 divisor,
                                 int_32 *quotient,
                                 int_16 *remainder) {
  return _udivrem32x(dividend, divisor, quotient, remainder);
}

//-------------------------------------------------------------------------
#if 0

_varify

The _varify macro returns what appears to be a field reference within a
structure variable rather than a field reference within a structure field.

  _varify(struct_type_name,
          field_name)

struct_type_name
  is the name of a structure type.

field_name
  is the name of a field within the structure struct_type_name.

Usage Considerations

The _varify macro, after expansion, appears to be a field reference within
a structure variable of type struct_type_name.  This macro can be used as
an argument to the sizeof operator.  It is not necessary to allocate a
variable of that type to inquire about the size of a field.  The _varify
macro causes no space to be allocated.

The pTAL language allows a programmer to use the $LEN standard function to
find the length of a structure field, referring to a structure template
rather than a structure variable.  The C++ language requires a field
reference in the sizeof operator to work with a structure variable.

The following example illustrates pTAL code that makes use of the length
of a structure template.

  pTAL Code                   Generated C++ Code

  STRUCT s(*);                struct s {
  BEGIN                         int_16 i;
    INT(16) i;                };
  END;
  STRING stuff                unsigned_char STUFF
         [0:$LEN(s.i)-1];            [_len(_varify(s,i))];

  PROC P;                     extern  C  void P() {
  BEGIN
    ...                         ...
    STRUCT sal(s);              s  sal;
    x := $LEN(sal.i);           x = _len(sal.i);
    ...                         ...


#endif
//-------------------------------------------------------------------------
#define _varify(a,b) (((a *)0)->b)


//-------------------------------------------------------------------------
#if 0

_dblr

The _dblr macro returns an int_32 value converted from a real_32 or
real_64 value, applying rounding to the result.

  int_32 _dblr(real_32 expr)
  int_32 _dblr(real_64 expr)

expr
  is a real type expression to be converted to an int_32 type value.

Usage Considerations

The _dblr macro is equivalent to the pTAL $DBLR standard function.

In the following example, the first assignment statement rounds up the
real value and stores the value 3 in the variable i32.  The second
assignment statement rounds down the real value and stores the value 2
in the variable i32.

  pTAL Code                   Generated C++ Code

  INT(32)  i32;               int_32  i32;
  REAL(32) r32 := 2.60E00;    real_32 r32 = 2.60E+00F;
  REAL(64) r64 := 4.20L00;    real_64 r64 = 4.20E+00;

  i32 := $DBLR(r32);          i32 = _dblr(r32);
  i32 := $DBLR(r64);          i32 = _dblr(r64);


#endif
//-------------------------------------------------------------------------
#define _dblr(x) ((int_32)((x) + ((x) < 0.0 ? -0.5 : 0.5)))

//-------------------------------------------------------------------------
#if 0

_dfix

The _dfix macro returns a fixed-point type expression converted from an
int_32 type expression.

  int_64 _dfix(int_32 expr,
               int_16 scale_factor)

expr
  is an expression to be converted to a fixed-point value.

scale_factor
  is the number of decimal points by which to scale the value of the
  expression.

Usage Considerations

The _dfix macro is equivalent to the pTAL $DFIX standard function.

In the following example, the value stored in the variable i32 is converted
to a fixed point value with the appropriate scale factor, and is then stored
in a fixed-point variable with the appropriate scale factor.

  pTAL Code                   Generated C++ Code

  FIXED(2) f2;                fixed_2 f2;
  FIXED(3) f3;                fixed_3 f3;
  INT(32)  i32 := 35D;        int_32  i32 = 35;

  f2 := $DFIX(i32,2);         f2 = _dfix(i32,2);
  f3 := $DFIX(i32,3);         f3 = _dfix(i32,3);


#endif
//-------------------------------------------------------------------------
#define _dfix(expr, scale) ((int_64)(expr))

//-------------------------------------------------------------------------
#if 0

_ifix

The _ifix macro returns a fixed-point type expression converted from an
int_16 type expression.

  int_64 _ifix(int_16 expr,
               int_16 scale_factor)

expr
  is an expression to be converted to a fixed-point value.

scale_factor
  is the number of decimal points by which to scale the value of the
  expression.

Usage Considerations

The _ifix macro is equivalent to the pTAL $IFIX standard function.

In the following example, the value stored in the variable i16 is converted
to a fixed point value with the appropriate scale factor, and is then stored
in a fixed-point variable with the appropriate scale factor.

  pTAL Code                   Generated C++ Code

  FIXED(2) f2;                fixed_2 f2;
  FIXED(3) f3;                fixed_3 f3;
  INT(16)  i16 := 25;         int_16  i16 = 25;

  f2 := $IFIX(i16,2);         f2 = _ifix(i16,2);
  f3 := $IFIX(i16,3);         f3 = _ifix(i16,3);


#endif
//-------------------------------------------------------------------------
#define _ifix(expr, scale) ((int_64)(expr))

//-------------------------------------------------------------------------
#if 0

_intr

The _intr macro returns an int_16 value converted from a real_32 or real_64
value, applying rounding to the result.

  int_16 _intr(real_32 expr)
  int_16 _intr(real_64 expr)

expr
  is an expression to be converted to a 16-bit integer.

Usage Considerations

The _intr macro is equivalent to the pTAL $INTR standard function.

In the following example, the first assignment statement rounds up the
real value and stores the value 3 in the variable i16.  The second assignment
statement rounds down the real value and stores the value 2 in the variable
i16.

  pTAL Code                   Generated C++ Code

  REAL(32) r32 := 2.60E00;    real_32 r32 = 2.60E+00F;
  REAL(64) r64 := 4.20L00;    real_64 r64 = 4.20E+00;
  INT(16)  i16;               int_16  i16;

  i16 := $INTR(r32);          i16 = _intr(r32);
  i16 := $INTR(r64);          i16 = _intr(r64);


#endif
//-------------------------------------------------------------------------
#define _intr(x) ((int_16)((x) + ((x) < 0.0 ? -0.5 : 0.5)))

//-------------------------------------------------------------------------
#if 0
#endif
//-------------------------------------------------------------------------
// _int64_to_int16_ov is defined with the overflow trapping routines
// _int64_to_uint16_ov is defined with the overflow trapping routines
// _int64_to_int32_ov is defined with the overflow trapping routines

//-------------------------------------------------------------------------
#if 0

_lfix

The _lfix macro returns a fixed-point type expression converted from an
unsigned_16 type expression.

  int_64 _lfix(unsigned_16 expr,
               int_16 scale_factor)

expr
  is an expression to be converted to a fixed-point value.

scale_factor
  is the number of decimal points by which to scale the value of the
  expression.

Usage Considerations

The result of the _lfix macro is a fixed-point type expression.

The _lfix macro is equivalent to the pTAL $LFIX standard function.

In the following example, the value stored in the variable i16 is converted
to a fixed point value with the appropriate scale factor, and is then stored
in a fixed-point variable with the appropriate scale factor.

  pTAL Code                   Generated C++ Code

  FIXED(2) f2;                fixed_2 f2;
  FIXED(3) f3;                fixed_3 f3;
  INT(16)  i16 := %h0ff0;     int_16  i16 = 0xFF0;

  f2 := $LFIX(i16,2)          f2 = _lfix(i16,2);
  f3 := $LFIX(i16,3);         f3 = _lfix(i16,3);


#endif
//-------------------------------------------------------------------------
#if ROSETTA_BIG_ENDIAN
#define _lfix(expr, scale) (((int_64)(expr)) & 0xFFFF)
#else
#define _lfix(expr, scale) ((int_64)((expr) % (unsigned_32)0x10000))
#endif

//-------------------------------------------------------------------------
#if 0

_make_int_16 and _make_int_16_d

The _make_int_16 function combines two character values into one 16-bit
integer value by extracting one or two characters from a string and placing
them in an integer container.

The _make_int_16_d macro combines two character values into one 16-bit
integer container.

  unsigned_16 _make_int_16(char *char_string)
  int_16      _make_int_16_d(char char_expr1,
                             char char_expr2)

char_string
  is a character string.

char_expr1
  is a character expression.

char_expr2
  is a character expression.

Usage Considerations

The _make_int_16 function and _make_int_16_d macro place the ASCII
representation of the first character in the high-order byte and the
second in the low-order byte.  If the parameter contains only one character,
its ASCII representation is placed in the low-order byte.

The _make_int_16 function ignores all characters past the second in
char_string and ignores the null terminator in char_string.

If char_string contains one character, then this character s ASCII
representation fills the low-order byte of the resultant 16-bit value.
The high-order byte contains the value zero.  To obtain zero padding in
the low order byte (which mimicks pTAL-style initialization), use the
function _make_int_16_d which takes individual characters as parameters
rather than a character string.

Because _make_int_16_d is a macro rather than a function, it is appropriate
for use in the definition of an enumeration constant.  To create a 16-bit
variable from a character string in other contexts, use the function
_make_int_16.

In the following example, the array b is initialized with four 16-bit
values, two of which are created from character values.

  pTAL Code                   Generated C++ Code

  int(16) b[0:3] :=           int_16 b[4] =
        [12,"a","bc",24];        {12, _make_int_16_d('a','\0'),
                                  _make_int_16_d('b','c') ,24};


#endif
//-------------------------------------------------------------------------
// does not pad with nulls: for padding during integer initialization,
// pass individual characters
_resident inline unsigned_16 _make_int_16 (void *str_) {
        unsigned_char *str = (unsigned_char *)str_;
        unsigned_16 ret_val = 0;

#ifndef _LITTLE_ENDIAN_
        int num_chars = 0;
        while (*str != '\0' && num_chars < 2) {
          ret_val = (ret_val << 8) + (int_16)*str;
          str++;
          num_chars++;
        }
#else
        if (*str != '\0') {
          unsigned_char a = *str;
          str++;
          unsigned_char b = *str;
          ret_val = (b << 8) + (int_16)a;
        }
#endif
        return (ret_val);
}
// literal definition requires macro; function call not allowed
#ifndef _LITTLE_ENDIAN_
#define _make_int_16_d(a, b) (((unsigned_16)a << 8) + (unsigned_16)b)
#else
#define _make_int_16_d(a, b) (((unsigned_16)b << 8) + (unsigned_16)a)
#endif

//-------------------------------------------------------------------------
#if 0

_make_int_32 and _make_real_32

The _make_int_32 function combines four character values, passed as
separate parameters, into one 32-bit integer value by placing the ASCII
representation of the first character in the high-order byte, the second
in the next byte, and so on.  The version that takes a character string
as a parameter right-justifies the characters  ASCII representations within
the 32-bit integer.

The _make_real_32 function behaves exactly the same as the _make_int_32
function, but it casts the type of the resultant value to real_32.

  unsigned_32 _make_int_32(char *char_string)
  unsigned_32 _make_int_32(char char_expr1,
                           char char_expr2,
                           char char_expr3,
                           char char_expr4)
  unsigned_32 _make_int_32(int_16 int_expr1,
                           int_16 int_expr2)

  real_32 _make_real_32(char *char_string)
  real_32 _make_real_32(char char_expr1,
                        char char_expr2,
                        char char_expr3,
                        char char_expr4)

char_string
  is a character string.

char_exprn
  is a character expression.

int_exprn
  is a 16-bit integer expression.

Usage Considerations

The _make_int_32 function and the _make_real_32 function ignore all
characters past the fourth in char_string and ignore the null terminator
in char_string.

If char_string contains less than four characters, then the character''s
ASCII representations fill the low-order bytes.  The remaining high-order
bytes contain zero values.  To obtain zero padding in the low order bytes
(which mimicks pTAL-style initialization), use the function that takes
individual characters as parameters rather than a character string.

In the following example, the arrays are initialized with values from a
constant list; those values can be character strings.

  pTAL Code                   Generated C++ Code

  int(32) c[0:3] :=           int_32  c[4] =
        ["abcd",35D,36D,2,4];   {_make_int_32('a','b','c','d'),
                                 35, 36, _make_int_32(2,4)};

  real(32) d[0:3] :=          real_32 d[4] =
        [1.2E00,"abcd"];      {1.2E+00F,
                               _make_real_32('a','b','c','d')};


#endif
//-------------------------------------------------------------------------
// does not pad with nulls: for padding during integer initialization,
// pass individual characters
_resident inline unsigned_32 _make_int_32 (void *str_) {
  unsigned_char *str = (unsigned_char *)str_;
  unsigned_32 ret_val = 0;

#ifndef _LITTLE_ENDIAN_
  int num_chars = 0;
  while (*str != '\0' && num_chars < 4) {
    ret_val = (ret_val << 8) + (int_16)*str;
    str++;
    num_chars++;
  }
#else
  int num_chars = 0;
  unsigned_char a = *str;
  unsigned_char b = '\0';
  unsigned_char c = '\0';
  unsigned_char d = '\0';
  struct {
    union {
      int_32 i;
      struct {
        unsigned_char w;
        unsigned_char x;
        unsigned_char y;
        unsigned_char z;
      } parts;
    };
  } s;
  s.i = 0;
  if (*str != '\0') {
    a = *str;
    num_chars++;
    str++;
  }
  if (*str != '\0') {
    b = *str;
    num_chars++;
    str++;
  }
  if (*str != '\0') {
    c = *str;
    num_chars++;
    str++;
  }
  if (*str != '\0') {
    d = *str;
    num_chars++;
    str++;
  }
  switch (num_chars) {
    case 0: break;
    case 1: s.parts.z = a;
            break;
    case 2: s.parts.y = a;
            s.parts.z = b;
            break;
    case 3: s.parts.x = a;
            s.parts.y = b;
            s.parts.z = c;
            break;
    case 4: s.parts.w = a;
            s.parts.x = b;
            s.parts.y = c;
            s.parts.z = d;
            break;
  }
  ret_val = s.i;
#endif
  return (ret_val);
}

// literal definition requires macro; function call not allowed
#ifndef _LITTLE_ENDIAN_
#define _make_int_32_d(a, b, c, d)    \
           (((unsigned_32)a << 24) +  \
            ((unsigned_32)b << 16) +  \
            ((unsigned_32)c <<  8) +  \
            ((unsigned_32)d <<  0))
#else
#define _make_int_32_d(a, b, c, d)    \
           (((unsigned_32)d << 24) +  \
            ((unsigned_32)c << 16) +  \
            ((unsigned_32)b <<  8) +  \
            ((unsigned_32)a <<  0))
#endif

_resident inline real_32 _make_real_32 (void *str_) {
  unsigned_char *str = (unsigned_char *)str_;
  union {
    int_32 i;
    real_32 r;
  } ret_val;
  ret_val.i = 0;

#ifndef _LITTLE_ENDIAN_
  int num_chars = 0;
  while (*str != '\0' && num_chars < 4) {
    ret_val.i = (ret_val.i << 8) + (int_16)*str;
    str++;
    num_chars++;
  }
#else
  if (*str != '\0') {
    unsigned_char a = *str;
    str++;
    unsigned_char b = *str;
    if (*str != '\0')
      str++;
    ret_val.i = (b << 8) + (int_16)a;
    if (*str != '\0') {
      unsigned_char c = *str;
      str++;
      unsigned_char d = *str;
      ret_val.i = (((d << 8) + (int_16)c) << 16) + ret_val.i;
    }
  }
#endif
  return (ret_val.r);
}

_resident inline unsigned_32 _make_int_32 (unsigned_char a, unsigned_char b,
                                           unsigned_char c, unsigned_char d) {
#ifndef _LITTLE_ENDIAN_
  return ((((((unsigned_16)a << 8) + (unsigned_16)b) << 8) +
           (unsigned_16)c) << 8) + (unsigned_16)d;
#else
  return ((((((unsigned_16)d << 8) + (unsigned_16)c) << 8) +
           (unsigned_16)b) << 8) + (unsigned_16)a;
#endif
}

_resident inline int_32 _make_int_32 (int_16 a, int_16 b) {
  return (int_32)(((unsigned_16)a << 16) + (unsigned_16)b);
}

_resident inline real_32 _make_real_32 (unsigned_char a, unsigned_char b,
                                        unsigned_char c, unsigned_char d) {
  union {
    int_32 i;
    real_32 r;
  } ret_val;
#ifndef _LITTLE_ENDIAN_
  ret_val.i = ((((((unsigned_16)a << 8) + (unsigned_16)b) << 8) +
              (unsigned_16)c) << 8) + (unsigned_16)d;
#else
  ret_val.i = ((((((unsigned_16)d << 8) + (unsigned_16)c) << 8) +
              (unsigned_16)b) << 8) + (unsigned_16)a;
#endif
  return ret_val.r;
}

//-------------------------------------------------------------------------
#if 0

_make_int_64 and _make_real_64

The _make_int_64 function combines eight character values into one 64-bit
integer value by placing the ASCII representation of the first character
in the high-order byte, the second in the next byte, and so on.  The
version that takes a character string as a parameter right-justifies the
characters  ASCII representations within the 64-bit integer.

The _make_real_64 function behaves exactly the same as the _make_int_64
function, but it casts the type of the resultant value to real_64.

  int_64 _make_int_64(char *char_string)
  int_64 _make_int_64(char char_expr,
                      char char_expr,
                      char char_expr,
                      char char_expr,
                      char char_expr,
                      char char_expr,
                      char char_expr,
                      char char_expr)
  int_64 _make_int_64(int_32 int_expr1,
                      int_32 int_expr2)

  real_64 _make_real_64(char *char_string)
  real_64 _make_real_64(char char_expr,
                        char char_expr,
                        char char_expr,
                        char char_expr,
                        char char_expr,
                        char char_expr,
                        char char_expr,
                        char char_expr)

char_string
  is a character string.

char_expr
  is a character constant or variable.

Usage Considerations

The _make_int_64 function and the _make_real_64 function ignore all characters
past the eighth in char_string and ignore the null terminator in char_string.

If char_string contains less than eight characters, then the character''s
ASCII representations fill the low-order bytes.  The remaining high-order
bytes contain zero values.  To obtain zero padding in the low order bytes
(which mimicks pTAL-style initialization), use the function that takes
individual characters as parameters rather than a character string.

In the following example, the arrays are initialized with values from a
constant list; those values can be character strings.

  pTAL Code                   Generated C++ Code

  int(64) e[0:3] :=           fixed_0 e[4] =
     [34D,35D,10F,11F,         {_make_int_64(34,35),10LL,11LL,
      "abcdefgh"];              _make_int_64('a','b','c','d',
                                           'e','f','g','h')};

  real(64) f[0:3] :=          real_64 f[4] =
     ["abcdefgh"];           {_make_real_64('a','b','c','d',
                                            'e','f','g','h')};


#endif
//-------------------------------------------------------------------------
// does not pad with nulls: for padding during integer initialization,
// pass individual characters
_resident inline int_64 _make_int_64 (void *str_) {
  int_64 ret_val = 0;

#ifndef _LITTLE_ENDIAN_
  unsigned_char *str = (unsigned_char *)str_;
  int num_chars = 0;
  while (*str != '\0' && num_chars < 8) {
    ret_val = (ret_val << 8) + (int_16)*str;
    str++;
    num_chars++;
  }
#else
#endif
  return (ret_val);
}
_resident inline int_64 _make_int_64 (unsigned_char a, unsigned_char b,
                                      unsigned_char c, unsigned_char d,
                                      unsigned_char e, unsigned_char f,
                                      unsigned_char g, unsigned_char h) {
#ifndef _LITTLE_ENDIAN_
  return (((int_64)_make_int_32 (a,b,c,d) << 32) +
           (int_64)_make_int_32 (e,f,g,h));
#else
  return ((int_64)_make_int_32 (a,b,c,d) +
          ((int_64)_make_int_32 (e,f,g,h) << 32));
#endif
}

_resident inline int_64 _make_int_64 (int_32 a, int_32 b) {
#ifndef _LITTLE_ENDIAN_
  return ((((int_64)(unsigned_32)a) << 32) + ((int_64)(unsigned_32)b));
#else
    int_32 parts[2];
    int_64 *i64 = (int_64 *)&parts[0];

    parts[0] = b;
    parts[1] = a;

    return *i64;

#endif
}

_resident inline real_64 _make_real_64 (unsigned_char *str) {
  union {
    int_64 i;
    real_64 r;
  } ret_val;
  ret_val.i = 0;
  int num_chars = 0;

  while (*str != '\0' && num_chars < 8) {
#ifndef _LITTLE_ENDIAN_
    ret_val.i = (ret_val.i << 8) + (int_16)*str;
#else
    ret_val.i = (ret_val.i * 0x100) + (int_16)*str;
#endif
    str++;
    num_chars++;
  }
  return (ret_val.r);
}

_resident inline real_64 _make_real_64 (unsigned_char a, unsigned_char b,
                                        unsigned_char c, unsigned_char d,
                                        unsigned_char e, unsigned_char f,
                                        unsigned_char g, unsigned_char h) {
  union {
    int_64 i;
    real_64 r;
  } ret_val;
#ifndef _LITTLE_ENDIAN_
  ret_val.i = (((int_64)_make_int_32 (a,b,c,d) << 32) +
               (int_64)_make_int_32 (e,f,g,h));
#else
  ret_val.i = ((int_64)_make_int_32 (a,b,c,d) +
               ((int_64)_make_int_32 (e,f,g,h) << 32));
#endif
  return (ret_val.r);
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// _real32_to_int64_ov is defined with the overflow trapping routines
// _real32_to_int64_rnd_ov is defined with the overflow trapping routines
// _real64_to_int64_ov is defined with the overflow trapping routines
// _real64_to_int64_rnd_ov is defined with the overflow trapping routines

//-------------------------------------------------------------------------
#if 0

_split32_in_2 and _split64_in_4

The _split32_in_2 macro converts one 32-bit constant value to two
comma-separated 16-bit constant values.  The pTAL to C++ Translator
generates it to emulate pTAL constant lists that contained 32-bit
constants intended to fill two cells in its associated array of 16-bit
values.

The _split64_in_4 macro converts one 64-bit constant value to four
comma-separated int_16 constant values.  The pTAL to C++ Translator
generates it to emulate pTAL constant lists that contained 64-bit constants
intended to fill four cells in its associated array of int_16 values.

  _split32_in_2(32_bit_constant)
  _split64_in_4(64_bit_constant)
  _split64_in_2(64_bit_constant)

32_bit_constant
  is a 32-bit constant.

64_bit_constant
  is a 64-bit constant.

Usage Considerations

In the following example, the constant 35D is intended to fill two cells
in the array g.  Likewise, the constant 35F is intended to fill four cells
in the array i, and the constant 35F is intended to fill two cells in the
array h.

  pTAL Code                   Generated C++ Code

  int(16) g[0:3] :=           int_16 g[4] =
     [35D,10,11];                {_split32_in_2(35),10,11};

  int(16) i[0:3] :=           int_16 i[4] =
     [35F];                      {_split64_in_4(35LL)};

  int(32) h[0:3] :=           int_32 h[4] =
     [35F,20D,21D];              {_split64_in_2(35LL),20,21};


#endif
//-------------------------------------------------------------------------
// convert 32-bit and 64-bit values to 8-bit and 16-bit values in
// constant lists
#if ROSETTA_BIG_ENDIAN
#define _split32_in_2(x)                                \
    (int_16)(((x) & 0xffff0000) >> 16) _comma           \
    (int_16)((x) & 0x0000ffff)
#else
#define _split32_in_2(x)                                \
    (int_16)((x) / (unsigned_32)0x00010000) _comma      \
    (int_16)((x) % (unsigned_32)0x00010000)
#endif

#if USE_LL
#define _split64_in_2(x)                                \
   (int_32)(((x) & 0xffffffff00000000LL) >> 32) _comma  \
   (int_32)((x) & 0xffffffffLL)
#else
#if USE_L
#define _split64_in_2(x)                                \
   (int_32)(((x) & 0xffffffff00000000L) >> 32) _comma   \
   (int_32)((x) & 0xffffffffL)
#else
#define _split64_in_2(x)                                \
   (int_32)(((x) & 0xffffffff00000000) >> 32) _comma    \
   (int_32)((x) & 0xffffffff)
#endif
#endif /* USE_LL */

#if USE_LL
#define _split64_in_4(x)                                \
   (int_16)(((x) & 0xffff000000000000LL) >> 48) _comma  \
   (int_16)(((x) & 0x0000ffff00000000LL) >> 32) _comma  \
   (int_16)(((x) & 0x00000000ffff0000LL) >> 16) _comma  \
    (int_16)((x) & 0x000000000000ffffLL)
#else
#if USE_L
#define _split64_in_4(x)                                \
   (int_16)(((x) & 0xffff000000000000L) >> 48) _comma  \
   (int_16)(((x) & 0x0000ffff00000000L) >> 32) _comma  \
   (int_16)(((x) & 0x00000000ffff0000L) >> 16) _comma  \
    (int_16)((x) & 0x000000000000ffffL)
#else
#define _split64_in_4(x)                                \
   (int_16)(((x) & 0xffff000000000000) >> 48) _comma    \
   (int_16)(((x) & 0x0000ffff00000000) >> 32) _comma    \
   (int_16)(((x) & 0x00000000ffff0000) >> 16) _comma    \
    (int_16)((x) & 0x000000000000ffff)
#endif
#endif /* USE_LL */

//-------------------------------------------------------------------------
#if 0

_unsigned_32

The _unsigned_32 macro casts an 8- or 16-bit signed value to a 32-bit
unsigned value without sign extension.

  unsigned_32 _unsigned_32(int_16 value)

value
  is an 8- or 16-bit signed value.


#endif
//-------------------------------------------------------------------------
// Define for casting to unsigned 32 without sign extension for 8 and
// 16 bit signed quantities.
#define _unsigned_32(p) ((unsigned_32)((unsigned_16)(int_32)(p)))

//----------------------------------------------------------------------------

_resident inline int _most_positive (void) {
  return 0x7fffffff;
} // _most_positive

_resident inline int _do_overflow (void) {
   return _most_positive () + _most_positive ();
} // _do_overflow

_resident inline void _force_ov_trap (void) {
  (void)_do_overflow ();
} // _force_ov_trap

//-------------------------------------------------------------------------
#if 0

_uns_16_mul

The _uns_16_mul macro takes two unsigned 16-bit values and returns the
32-bit product of these values.

  int_32 _uns_16_mul(int_16 first_operand,
                     int_16 second_operand)

first_operand
  is a 16-bit unsigned value.

second_operand
  is a 16-bit unsigned value.

Usage Considerations

The _uns_16_mul macro returns a 32-bit value that is the product of
first_operand and second_operand.

Overflow is not detected.

The following example illustrates a use of this macro.

  pTAL Code                   Generated C++ Code

  INT(16)  i16a, i16b;        int_16  i16a, i16b;
  INT(32)  r32;               int_32  r32;

  r32 := i16a '*' i16b;       r32 = _uns_16_mul(i16a,i16b);

#endif
//-------------------------------------------------------------------------

#define _uns_16_mul(x, y)                          \
  ((int_32)(_unsigned_32 (x) * _unsigned_32 (y)))

//-------------------------------------------------------------------------
#if 0

_uns_32_mul

The _uns_32_mul macro takes two unsigned 32-bit values and returns the
63-bit product of these values.

  int_64 _uns_32_mul(int_32 first_operand,
                     int_32 second_operand)

first_operand
  is a 32-bit unsigned value.

second_operand
  is a 32-bit unsigned value.

Usage Considerations

The _uns_32_mul macro returns a 63-bit value that is the product of
first_operand and second_operand.

Overflow is not detected.

The following example illustrates a use of this macro.

  pTAL Code                   Generated C++ Code

  INT(32)  i32a, i32b;        int_32  i32a, i32b;
  INT(64)  r64;               int_64  r64;

  r64 := i32a '*' i32b;       r64 = _uns_32_mul(i32a,i32b);

#endif
//-------------------------------------------------------------------------

#define _uns_32_mul(x, y)                                \
  ((int_64)(unsigned_32)(x) * (int_64)(unsigned_32)(y))

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Inline function to determine if a 32-bit dividend divided by a 16-bit
// divisor will cause a 16-bit overflow.
//
// For the divide and modulo operation with a 32-bit unsigned dividend and
// a 16-bit unsigned divisor, overflow occurs when the most significant
// 16 bits of the dividend are greater than or equal to the divisor.  This
// automatically includes the case of a zero divisor since the operation
// is unsigned.

_resident inline int_16 _check_for_uns_16_div_ov (int_32 dvnd, int_16 dvsr) {
  return _tbv (((unsigned_16)((dvnd >> 16) & 0xFFFF)) >= (unsigned_16)dvsr);
} // _check_for_uns_16_div_ov

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Inline function to determine if a 63-bit dividend divided by a 32-bit
// divisor will cause overflow.
//
// For the divide and modulo operation with a 64-bit unsigned dividend and
// a 32-bit unsigned divisor, overflow occurs when the most significant
// 32 bits of the dividend are greater than or equal to the divisor.  This
// automatically includes the case of a zero divisor since the operation
// is unsigned.

_resident inline int_16 _check_for_uns_32_div_ov (int_64 dvnd, int_32 dvsr) {
  return _tbv (((unsigned_32)((dvnd >> 32) & 0xFFFFFFFF)) >= (unsigned_32)dvsr);
} // _check_for_uns_32_div_ov

//-------------------------------------------------------------------------
#if 0

_uns_16_div

The _uns_16_div macro takes an unsigned 32-bit dividend and an unsigned
16-bit divisor and returns the 16-bit quotient of these values.

  int_16 _uns_16_div(int_32 dividend,
                     int_16 divisor)

dividend
  is a 32-bit unsigned dividend.

divisor
  is a 16-bit unsigned divisor.

Usage Considerations

The _uns_16_div macro returns a 16-bit quotient of the dividend and divisor.

Overflow trapping takes place if the quotient is greater than the largest
unsigned 16- bit value.  The following example illustrates a use of this
macro.

  pTAL Code                   Generated C++ Code

  INT(16)  r16, i16;          int_16  r16, i16;
  INT(32)  i32;               int_32  i32;

  r16 := i32 '/' i16;         r16 = _uns_16_div(i32,i16);

#endif
//-------------------------------------------------------------------------

#define _uns_16_div(dvnd, dvsr)                                       \
  ((int_16)((unsigned_16)((unsigned_32)dvnd / _unsigned_32 (dvsr))))

//-------------------------------------------------------------------------
#if 0

_uns_32_div

The _uns_32_div macro takes an unsigned 63-bit dividend and an unsigned
32-bit divisor and returns the 32-bit quotient of these values.

  int_32 _uns_32_div(int_64 dividend,
                     int_32 divisor)

dividend
  is a 63-bit unsigned dividend.

divisor
  is a 32-bit unsigned divisor.

Usage Considerations

The _uns_32_div macro returns a 32-bit quotient of the dividend and divisor.

Overflow trapping takes place if the quotient is greater than the largest
unsigned 32- bit value.

The following example illustrates a use of this macro.

  pTAL Code                   Generated C++ Code

  INT(32)  r32, i32;          int_32  r32, i32;
  INT(64)  i64;               int_64  i64;

  r32 := i64 '/' i32;         r32 = _uns_32_div(i64,i32);

#endif
//-------------------------------------------------------------------------

#define _uns_32_div(dvnd, dvsr)                                    \
  ((int_32)((unsigned_32)(dvnd / (int_64)((unsigned_32)(dvsr)))))

//-------------------------------------------------------------------------
#if 0

_uns_16_mod

The _uns_16_mod macro takes an unsigned 32-bit dividend and an unsigned
16-bit divisor and returns the 16-bit modulo of these values.

  int_16 _uns_16_mod(int_32 dividend,
                     int_16 divisor)

dividend
  is a 32-bit unsigned dividend.

divisor
  is a 16-bit unsigned divisor.

Usage Considerations

The _uns_16_mod macro returns a 16-bit modulo of the dividend and divisor.

Overflow trapping takes place if the quotient is greater than the largest
unsigned 16-bit value.

The following example illustrates a use of this macro.

  pTAL Code                   Generated C++ Code

  INT(16)  r16, i16;          int_16  r16, i16;
  INT(32)  i32;               int_32  i32;

  r16 := i32 ''\'' i16;       r16 = _uns_16_mod(i32,i16);

#endif
//-------------------------------------------------------------------------
// Define to modulo a 32-bit unsigned dividend by a 16-bit unsigned divisor.

#define _uns_16_mod(dvnd, dvsr)                                              \
   ((int_16)                                                                 \
      ((unsigned_16)                                                         \
        ((unsigned_32)(dvnd) -                                               \
         (_unsigned_32 (_uns_16_div (dvnd, dvsr)) * _unsigned_32 (dvsr)))))

//-------------------------------------------------------------------------
#if 0

_uns_32_mod

The _uns_32_mod macro takes an unsigned 63-bit dividend and an unsigned
32-bit divisor and returns the 32-bit modulo of these values.

  int_32 _uns_32_mod(int_64 dividend,
                     int_32 divisor)

dividend
  is a 63-bit unsigned dividend.

divisor
  is a 32-bit unsigned divisor.

Usage Considerations

The _uns_32_mod macro returns a 32-bit modulo of the dividend and divisor.

Overflow trapping takes place if the quotient is greater than the largest
unsigned 32- bit value.

The following example illustrates a use of this macro.

  pTAL Code                   Generated C++ Code

  INT(32)  r32, i32;          int_32  r32, i32;
  INT(64)  i64;               int_64  i64;

  r32 := i64 ''\'' i32;       r32 = _uns_32_mod(i64,i32);

#endif
//-------------------------------------------------------------------------
// Define to modulo a 63-bit unsigned dividend by a 32-bit unsigned divisor.

#define _uns_32_mod(dvnd, dvsr)                               \
   ((int_32)                                                  \
      ((unsigned_32)                                          \
        (dvnd -                                               \
         ((int_64)((unsigned_32)_uns_32_div (dvnd, dvsr)) *   \
          (int_64)((unsigned_32)dvsr)))))

//-------------------------------------------------------------------------
#if 0

_uns_16_div_ov

The _uns_16_div_ov function takes an unsigned 32-bit dividend and an
unsigned 16-bit divisor and returns the 16-bit quotient of these values,
setting an overflow indicator.

  int_16 _uns_16_div_ov(int_32 dividend,
                        int_16 divisor
                        int_16 *ov)

dividend
  is a 32-bit unsigned dividend.

divisor
  is a 16-bit unsigned divisor.

ov
  is a reference parameter that indicates whether or not the quotient is
  greater than the largest unsigned 16-bit value.

Usage Considerations

The _uns_16_div_ov function returns a 16-bit quotient of the dividend and
divisor.

The ov reference parameter returns -1 if the quotient (the function result)
is greater than the largest unsigned 16-bit value, and 0 otherwise.

The function result is undefined if overflow is detected.

The following example illustrates a use of this function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
                            int_16  _ov_temp;
  INT(16)  r16, i16;          int_16  r16, i16;
  INT(32)  i32;               int_32  i32;

  r16 := i32 '/' i16;         r16 = _uns_16_div_ov(i32,i16
                                                   &_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)

#endif
//-------------------------------------------------------------------------
// Inline function to divide a 32-bit unsigned dividend by a 16-bit
// unsigned divisor.  Overflow trapping does not occur; the formal
// reference parameter ov is set to -1 if overflow occured and 0
// otherwise.  The result of the function is undefined if overflow is
// detected.

_resident inline int_16 _uns_16_div_ov (int_32 dvnd, int_16 dvsr, int_16* ov) {
  *ov = _check_for_uns_16_div_ov (dvnd, dvsr);

  return (int_16)((unsigned_16)((unsigned_32)dvnd / _unsigned_32 (dvsr)));
} // _uns_16_div_ov

//-------------------------------------------------------------------------
#if 0

_uns_32_div_ov

The _uns_32_div_ov function takes an unsigned 63-bit dividend and an
unsigned 32-bit divisor and returns the 32-bit quotient of these values,
setting an overflow indicator.

  int_32 _uns_32_div_ov(int_64 dividend,
                        int_32 divisor
                        int_16 *ov)

dividend
  is a 63-bit unsigned dividend.

divisor
  is a 32-bit unsigned divisor.

ov
  is a reference parameter that indicates whether or not the quotient is
  greater than the largest unsigned 16-bit value.

Usage Considerations

The _uns_32_div_ov function returns a 16-bit quotient of the dividend and
divisor.

The ov reference parameter returns -1 if the quotient (the function result)
is greater than the largest unsigned 16-bit value, and 0 otherwise.

The function result is undefined if overflow is detected.

The following example illustrates a use of this function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
                              int_16  _ov_temp;
  INT(32)  r32, i32;          int_32  r32, i32;
  INT(64)  i64;               int_64  i64;

  r32 := i64 '/' i32;         r32 = _uns_32_div_ov(i64,i32
                                                   &_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)

#endif
//-------------------------------------------------------------------------
// Inline function to divide a 63-bit unsigned dividend by a 32-bit
// unsigned divisor.  Overflow trapping does not occur; the formal
// reference parameter ov is set to -1 if overflow occured and 0
// otherwise.  The result of the function is undefined if overflow is
// detected.

_resident inline int_32 _uns_32_div_ov (int_64 dvnd, int_32 dvsr, int_16* ov) {
#define i32_to_i64(x) ((int_64)((unsigned_32)(x)))
  *ov = _check_for_uns_32_div_ov (dvnd, dvsr);

  return (int_32)((unsigned_32)(dvnd / i32_to_i64 (dvsr)));
#undef i32_to_i64
} // _uns_32_div_ov

//-------------------------------------------------------------------------
#if 0

_uns_16_mod_ov

The _uns_16_mod_ov macro takes an unsigned 32-bit dividend and an unsigned
16-bit divisor and returns the 16-bit modulo of these values, setting an
overflow indicator.

  int_16 _uns_16_mod_ov(int_32 dividend,
                        int_16 divisor
                        int_16 *ov)

dividend
  is a 32-bit unsigned dividend.

divisor
  is a 16-bit unsigned divisor.

ov
  is a reference parameter that indicates whether or not the quotient is
  greater than the largest unsigned 16-bit value.

Usage Considerations

The _uns_16_mod_ov macro returns a 16-bit modulo of the dividend and
divisor.

The ov reference parameter returns -1 if the quotient of the dividend
and divisor is greater than the largest unsigned 16-bit value, and 0
otherwise.

The function result is undefined if overflow is detected.

The following example illustrates a use of this function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
                              int_16  _ov_temp;
  INT(16)  r16, i16;          int_16  r16, i16;
  INT(32)  i32;               int_32  i32;

  r16 := i32 ''\'' i16;       r16 = _uns_16_mod_ov(i32,i16
                                                   &_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)

#endif
//-------------------------------------------------------------------------
// Inline function to modulo a 32-bit unsigned dividend by a 16-bit
// unsigned divisor.  Overflow trapping does not occur; the formal
// reference parameter ov is set to -1 if overflow occured and 0
// otherwise.  The result of the function is undefined if overflow is
// detected.

_resident inline int_16 _uns_16_mod_ov (int_32 dvnd, int_16 dvsr, int_16* ov) {
  *ov = _check_for_uns_16_div_ov (dvnd, dvsr);

  return
    (int_16)
      ((unsigned_16)
        ((unsigned_32)(dvnd) -
         (((unsigned_32)dvnd / _unsigned_32 (dvsr)) * _unsigned_32 (dvsr))));
} // _uns_16_mod_ov

//-------------------------------------------------------------------------
#if 0

_uns_32_mod_ov

The _uns_32_mod_ov macro takes an unsigned 63-bit dividend and an
unsigned 32-bit divisor and returns the 32-bit modulo of these values,
setting an overflow indicator.

  int_32 _uns_32_mod_ov(int_64 dividend,
                        int_32 divisor
                        int_16 *ov)

dividend
  is a 63-bit unsigned dividend.

divisor
  is a 32-bit unsigned divisor.

ov
  is a reference parameter that indicates whether or not the quotient is
  greater than the largest unsigned 32-bit value.

Usage Considerations

The _uns_32_mod_ov macro returns a 32-bit modulo of the dividend and
divisor.

The ov reference parameter returns -1 if the quotient of the dividend and
divisor is greater than the largest unsigned 32-bit value, and 0 otherwise.

The function result is undefined if overflow is detected.

The following example illustrates a use of this function.

  pTAL Code                   Generated C++ Code

  ?NOOVERFLOW_TRAPS           #pragma nooverflow_traps
                              int_16  _ov_temp;
  INT(32)  r32, i32;          int_32  r32, i32;
  INT(64)  i64;               int_64  i64;

  r32 := i64 ''\'' i32;       r32 = _uns_32_mod_ov(i64,i32
                                                   &_ov_temp);
  IF $OVERFLOW THEN           if (_ov_temp)

#endif
//-------------------------------------------------------------------------
// Inline function to modulo a 63-bit unsigned dividend by a 32-bit
// unsigned divisor.  Overflow trapping does not occur; the formal
// reference parameter ov is set to -1 if overflow occured and 0
// otherwise.  The result of the function is undefined if overflow is
// detected.

_resident inline int_32 _uns_32_mod_ov (int_64 dvnd, int_32 dvsr, int_16* ov) {
#define i32_to_i64(x) ((int_64)((unsigned_32)(x)))
  *ov = _check_for_uns_32_div_ov (dvnd, dvsr);

  return
    (int_32)
      ((unsigned_32)
        (dvnd - ((dvnd / i32_to_i64 (dvsr)) * i32_to_i64 (dvsr))));
#undef i32_to_i64
} // _uns_32_mod_ov

//-------------------------------------------------------------------------
#if 0

_xadr

The _xadr function converts the pointer argument to type unsigned_char*.

  unsigned_char _xadr(const void *address)

address
  is an address to be converted.

Usage Considerations

The _xadr function is equivalent to the pTAL $XADR standard function.

  pTAL Code                   Generated C++ Code

  STRING .EXT xp;             unsigned_char *xp;
  INT(32) xa;                 int_32  xa;
  @xp := $XADR(xp);           xp = _xadr(xp);
  @xp := $XADR(xa);           xp = _xadr(&xa);


#endif
//-------------------------------------------------------------------------
// Implements Ptal builtin $XADR:

#define _xadr(addr) _xadr_Function((const void*)(addr))
_resident inline unsigned_char _far * _xadr_Function (const void *address) {
  return (unsigned_char _far*) address;
} // _xadr_Function

#define _bit_length(a,b) _bitlength((a).(b))
//--------------------------------------------------------


#endif
//#endif
