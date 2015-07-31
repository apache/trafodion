/**********************************************************************
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
**********************************************************************/
#ifndef _COMPLEX_OBJECT_H_
#define _COMPLEX_OBJECT_H_

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComplexObject.h
 * Description:  This file defines a Message Pattern
 *               to facilitate the future extension and maintenance
 * Created:      12/5/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

// forwarded class declartions:
class MessageOperator;
class ObjectContainer;
class InputContainer;
class OutputContainer;
class ComplexObject;
class ComplexObjectFactory;
class Packer;
class PackerInput;
class PackerOutput;
class UnPacker;
class UnPackerInput;
class UnPackerOutput;
class Lengther;
class LengtherInput;
class LengtherOutput;
class ComplexObjectPtrContainer;
class Int16ObjectContainer;
class LongObjectContainer;
class IntegerObjectContainer;
class CharPtrObjectContainer;
class EnumObjectContainer;
class EmptyComplexObject;
// begin abstract class declarations:
// abstract class of Packer, UnPacker and Lengther
class MessageOperator{
protected:
  MessageOperator();
  virtual ~MessageOperator();
public:
  virtual void execute(ObjectContainer *objWrap,
		  InputContainer *input,
		  OutputContainer *output) = 0;
  virtual void setInputOutputForNextOperation(InputContainer *input,
					 OutputContainer *output) = 0;
};

// abstract class of Inputs/Outputs Objects to MessageOperator::execute
// may add new pack, unPack, and length methods to take extra input and output
// for instance,
// virtual void pack(Packer *, PackerInput *, PackerOutput *,
//                   InputContainer *, OutputContainer *)
// depends on what you needs.
class ObjectContainer{
protected:
  ObjectContainer();
  virtual ~ObjectContainer();
public:
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output) = 0;
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output) = 0;
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output) = 0;
};

class InputContainer{
protected:
  InputContainer();
  virtual ~InputContainer();
};

class OutputContainer{
protected:
  OutputContainer();
  virtual ~OutputContainer();
};

// abstract Class of complex objects to be packed/unpacked
// define memory operations
// may add new pack, unPack, and length methods to take extra input and output
// for instance,
// virtual void pack(Packer *, PackerInput *, PackerOutput *,
//                   InputContainer *, OutputContainer *)
// depends on what you need

enum ComplexObjectType {
	InvalidComplexObjectType, EmptyComplexObjectType /* mark null pointer */,
	CtrlStmtComplexObjectType, TransAttrComplexObjectType
};

class ComplexObject {
protected:
  ComplexObject(NAMemory *heap, ComplexObjectType type);
  virtual ~ComplexObject();
  void baseOperation(MessageOperator *msgOp, InputContainer *input,
		     OutputContainer *output);
public:
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output);
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output);
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output);
  virtual void sharedOperationSequence(MessageOperator *msgOp,
				       InputContainer *input,
				       OutputContainer *output) = 0;
  virtual NAMemory * getHeap();
  ComplexObjectType getType();
  void operator delete(void *p);

  virtual void freeSubObjects() = 0;
  static ComplexObject *manufacture(NAMemory *heap, char * bufferPtr,
    ComplexObjectFactory *factory);
private:
  NAMemory *heap_;
  ComplexObjectType type_;
};

class ComplexObjectFactory{
public:
  virtual ComplexObject * manufacture(NAMemory *heap, ComplexObjectType objType) = 0;
};

// end of abstract class declarations

// begin stack-allocated objects
// In principle, Packer, UnPacker and Lengther classes should ues singleton
// Pattern, However, NSK security does not allow global variables.

// Packer related class declarations
class Packer : public MessageOperator{
public:
  Packer();
  virtual ~Packer();
  virtual void execute(ObjectContainer *objWrap, InputContainer *input,
		  OutputContainer *output);
  virtual void setInputOutputForNextOperation(InputContainer *input,
					 OutputContainer *output);
};

class PackerInput : public InputContainer{
public:
  PackerInput(char * bufferPtr, IpcMessageObjSize accMsgSize);
  virtual ~PackerInput();
  char * getBufferPtr();
  IpcMessageObjSize getAccMsgSize();
  void setBufferPtr(char * bufferPtr);
  void setAccMsgSize(IpcMessageObjSize accMsgSize);
private:
  char *bufferPtr_;
  IpcMessageObjSize accMsgSize_;
};

class PackerOutput : public OutputContainer{
public:
  PackerOutput();
  virtual ~PackerOutput();
  char * getBufferPtr();
  IpcMessageObjSize getCurrMsgSize();
  void setBufferPtr(char * bufferPtr);
  void setCurrMsgSize(IpcMessageObjSize currMsgSize);
private:
  char *bufferPtr_;
  IpcMessageObjSize currMsgSize_;
};

// UnPacker related class declartions
class UnPacker : public MessageOperator{
private:
  NAMemory *heap_;
  ComplexObjectFactory * factory_;
public:
  UnPacker(NAMemory *heap, ComplexObjectFactory *factory);
  virtual ~UnPacker();
  virtual void execute(ObjectContainer *objWrap, InputContainer *input,
		  OutputContainer *output);
  virtual void setInputOutputForNextOperation(InputContainer *input,
					 OutputContainer *output);
  NAMemory * getHeap();
  ComplexObjectFactory * getFactory();
};

class UnPackerInput : public InputContainer{
public:
  UnPackerInput(char * bufferPtr);
  virtual ~UnPackerInput();
  char * getBufferPtr();
  void setBufferPtr(char * bufferPtr);
private:
  char *bufferPtr_;
};

class UnPackerOutput : public OutputContainer{
public:
  UnPackerOutput();
  virtual ~UnPackerOutput();
  char * getBufferPtr();
  void setBufferPtr(char * bufferPtr);
private:
  char *bufferPtr_;
};

// Lengther related class declarations
class Lengther : public MessageOperator{
public:
  Lengther();
  virtual ~Lengther();
  virtual void execute(ObjectContainer *objWrap, InputContainer *input,
		   OutputContainer *output);
  virtual void setInputOutputForNextOperation(InputContainer *input,
					 OutputContainer *output);
};

class LengtherInput : public InputContainer{
public:
  LengtherInput(IpcMessageObjSize accMsgSize);
  virtual ~LengtherInput();
  IpcMessageObjSize getAccMsgSize();
  void setAccMsgSize(IpcMessageObjSize accMsgSize);
private:
  IpcMessageObjSize accMsgSize_;
};

class LengtherOutput : public OutputContainer{
public:
  LengtherOutput();
  virtual ~LengtherOutput();
  IpcMessageObjSize getCurrMsgSize();
  void setCurrMsgSize(IpcMessageObjSize currMsgSize);
private:
  IpcMessageObjSize currMsgSize_;
};

// object container class declarations
// stack-allocated class declarations
typedef ComplexObject * ComplexObjectPtr;

// contains the pointer to a ComplexObject
class ComplexObjectPtrContainer : public ObjectContainer{
public:
  ComplexObjectPtrContainer(ComplexObjectPtr * objectPtr);
  virtual ~ComplexObjectPtrContainer();
  ComplexObjectPtr *getComplexObjectPtr();
  void setComplexObjectPtr(ComplexObjectPtr *objectPtr);
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output);
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output);
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output);
private:
  ComplexObjectPtr * objectPtr_;
};

class Int16ObjectContainer : public ObjectContainer{
public:
  Int16ObjectContainer(Int16 *value);
  virtual ~Int16ObjectContainer();
  Int16* getValue();
  void setValue(Int16* value);
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output);
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output);
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output);
private:
  Int16 *value_;
};

class IntegerObjectContainer : public ObjectContainer{
public:
  IntegerObjectContainer(Int32 *value);
  virtual ~IntegerObjectContainer();
  Int32* getValue();
  void setValue(Int32 *value);
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output);
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output);
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output);
private:
  Int32 *value_;
};

class LongObjectContainer : public ObjectContainer{
public:
  LongObjectContainer(Lng32 *value);
  virtual ~LongObjectContainer();
  Lng32* getValue();
  void setValue(Lng32 *value);
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output);
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output);
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output);
private:
  Lng32 *value_;
};

typedef char * CharPtr;

class CharPtrObjectContainer : public ObjectContainer{
public:
  CharPtrObjectContainer(CharPtr *charPtr);
  virtual ~CharPtrObjectContainer();
  CharPtr * getCharPtr();
  void setCharPtr(CharPtr *charPtr);
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output);
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output);
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output);
private:
  IpcMessageObjSize packedStringLength();
  CharPtr *charPtr_;
};

class EnumObjectContainer : public ObjectContainer{
public:
  EnumObjectContainer(ComplexObjectType *value);
  virtual ~EnumObjectContainer();
  ComplexObjectType * getValue();
  void setValue(ComplexObjectType * value);
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output);
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output);
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output);
private:
  ComplexObjectType *value_; // use ComplexObjectType as our enum proxy, assuming all enums
                             // occupy same number of bytes.
};


class NopObjectContainer : public ObjectContainer{
public:
  NopObjectContainer();
  virtual ~NopObjectContainer();
  virtual void pack(Packer *packer, PackerInput *input,
		    PackerOutput *output);
  virtual void unPack(UnPacker *unPacker, UnPackerInput *input,
		      UnPackerOutput *output);
  virtual void length(Lengther *lengther, LengtherInput *input,
		      LengtherOutput *output);
};

// complex object class declarations
// could have used singleton pattern, but NSK security does not allow globals
class EmptyComplexObject : public ComplexObject {
public:
  EmptyComplexObject(NAMemory *heap);
  EmptyComplexObject();
  virtual ~EmptyComplexObject();
  virtual void freeSubObjects();
  virtual void sharedOperationSequence(MessageOperator *msgOp,
				       InputContainer *input,
				       OutputContainer *output);
};

#endif /* _COMPLEX_OBECT_H_ */






