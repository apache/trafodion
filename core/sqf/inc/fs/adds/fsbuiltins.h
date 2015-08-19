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
#ifndef _FS_BUILTINS
#define _FS_BUILTINS
//
// This header defines some builtins for which we currently have no prototypes and
// which are required to compile

// Symbols used for "oversized" value arguments
//
#ifndef _INC_TDM_CEXTDECS_H
typedef int		fat_16;
typedef __int64 fat_32;
// Values used to indicate omitted arguments 
//
#ifdef OLD_OMIT_VALUES
short const				OMITSHORT			= -292;
unsigned short const	OMITUNSIGNEDSHORT	= 0xFEDC;
int const				OMITINT				= -19136512;
unsigned int const		OMITUNSIGNEDINT		= 0xFEDC0000;
long const				OMITLONG			= -19136512;
unsigned long const		OMITUNSIGNEDLONG	= 0xFEDC0000;
__int64 const			OMIT__INT64			= -82190693199511552; /*FEDC000000000000*/
#else
short const          OMITSHORT         = -291;
unsigned short const OMITUNSIGNEDSHORT = 0xFEDD;
int const            OMITINT           = -19070975;
fat_16 const         OMITFAT_16        = -19070975;
unsigned int const   OMITUNSIGNEDINT   = 0xFEDD0001;
long const           OMITLONG          = -19070975;
fat_32 const         OMITFAT_32        = 0xfedd000000000001LL;
unsigned long const  OMITUNSIGNEDLONG  = 0xFEDD0001;
__int64 const        OMIT__INT64       = 0xfedd000000000001LL;
#endif // OLD_OMIT_VALUES
//
// We don't want the Rosetta versions of _arg_present and _optional
//
#ifdef _arg_present
#undef _arg_present
#endif

#ifdef _optional
#undef _optional
#endif

#define _optional(a,b) (a ? (b) : OMIT )

// Type names for argument substitution classes. NOTE: Be sure to include
// a specialized instance of _arg_present for each of these.
//												
#define arg_16 OptionalArgument<short>
#define arg_32 OptionalArgument<int>
#define arg_64 OptionalArgument<__int64>

// Class ArgumentOmitted serves two pruposes:
//   1) When the argument is an instance of ArgumentOmitted template class, causes the
//      "omitted" constructor to be invoked during the call...and the != overload operator
//      to be invokes by the _arg_present function.
//   2) When the argument is a pre-define or user defined type, generates the specified value
//      to indicate that the argument is omitted during the call or when tested
//      by the _arg_present function.
// 

class ArgumentOmitted {
public:
	operator short() {return OMITSHORT;} 
	operator short*() {return NULL;} 
	operator int() {return OMITINT;} 
	operator int*() {return NULL;}
	operator long() {return OMITLONG;}
#ifndef NA_64BIT
        // dg64 - since __int64 is same as long, not necessary
	operator __int64() {return OMIT__INT64;}
#endif
	operator __int64*() {return NULL;} // gps11-11
#ifdef NA_64BIT
        //operator _int_64*() {return NULL;}
#endif 
#ifndef NA_64BIT
        // dg64 - since __int64 is same as long, not necessary
	operator long*() {return NULL;}
#endif
	operator char*() {return NULL;} 
	operator unsigned short() {return OMITUNSIGNEDSHORT;} 
	operator unsigned short*() {return NULL;} 
	operator unsigned int() {return OMITUNSIGNEDINT;} 
	operator unsigned int*() {return NULL;}
	operator unsigned long() {return OMITUNSIGNEDLONG;}
	operator unsigned long*() {return NULL;}
	operator unsigned char*() {return NULL;} 
	operator void*() {return NULL;}
	operator void**() {return NULL;}
};

#ifndef _USERDEFINEDARGOMITTED_		// gps11-12
static ArgumentOmitted OMIT;
#endif
#endif
//Single instantiation of ArgumentOmitted class

// Template class OptionalArgument generates the optional argument classes to use
// in the argument list in place of the corresponding pre-defined or user-defined "original"
// types when there is not a suitable argument value to indicate omission.
//
template <class T> class OptionalArgument {
	inline int _arg_present(OptionalArgument<T>);
public:
	OptionalArgument(T arg) : argument(arg), present(1) {}			// Arg present constructor
	OptionalArgument(ArgumentOmitted) : argument(0), present(0) {}	// Arg omitted constructor
	T operator=(T arg) {argument = arg; return argument;}			// Assignment from original type
	operator T() {return argument;}									// Conversion to original type
private:
	T       argument;
	int     present;
};
#ifdef ARG_PRESENT_OMIT

// Instances used for pre-defined and user-defined types
//    Note: template function was removed to eliminate the unnecessary instances of
//          of pointers to user-defined classes and structures. These can be resolved
//          by implicit casts.
// 
#define ORIGINAL_ARG_PRESENT(T,V) \
inline int _arg_present(T arg) { \
		return (arg != V); \
}
ORIGINAL_ARG_PRESENT(short, OMITSHORT)
ORIGINAL_ARG_PRESENT(short*, NULL)
ORIGINAL_ARG_PRESENT(int, OMITINT)
ORIGINAL_ARG_PRESENT(int*, NULL)
ORIGINAL_ARG_PRESENT(long, OMITLONG)
ORIGINAL_ARG_PRESENT(long*, NULL)
ORIGINAL_ARG_PRESENT(char*, NULL)
ORIGINAL_ARG_PRESENT(unsigned short, OMITUNSIGNEDSHORT)
ORIGINAL_ARG_PRESENT(unsigned short*, NULL)
ORIGINAL_ARG_PRESENT(unsigned int, OMITUNSIGNEDINT)
ORIGINAL_ARG_PRESENT(unsigned int*, NULL)
ORIGINAL_ARG_PRESENT(unsigned long, OMITUNSIGNEDLONG)
ORIGINAL_ARG_PRESENT(unsigned long*, NULL)
ORIGINAL_ARG_PRESENT(unsigned char*, NULL)
ORIGINAL_ARG_PRESENT(void*, NULL)
ORIGINAL_ARG_PRESENT(void**, NULL)

#endif // ARG_PRESENT_OMIT
#ifdef ARG_PRESENT_REPLACEMENT
//Instances used for OptionalArgument replacement classes
//
#define REPLACEMENT_ARG_PRESENT(T) \
inline int _arg_present(OptionalArgument<T> arg) { \
		return arg.present; \
}
REPLACEMENT_ARG_PRESENT(short)
REPLACEMENT_ARG_PRESENT(int)
REPLACEMENT_ARG_PRESENT(__int64)
#endif // ARG_PRESENT_REPLACEMENT


short _fixedtoascii ( fixed_0        qvalue,
                      void          *buffer,
                      unsigned short maxdigits);

short _asciitofixed ( void           *buffer,
                      unsigned short  maxdigits,
                      unsigned short *remainingdigits,
                      fixed_0         qvaluein,
                      fixed_0        *qvalueout,
                      short          *overflow);

#endif
