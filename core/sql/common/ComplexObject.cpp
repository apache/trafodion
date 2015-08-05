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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComplexObject.cpp
 * Description:  This file defines a Message Pattern
 *               to facilitate the future extension and maintenance 
 * Created:      12/5/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "NAMemory.h"
#include "NAAssert.h"
#include "IpcMessageObj.h"
#include "str.h"
#include "NAVersionedObject.h"
#include "ComplexObject.h"

#include <iostream>
// begin abstract class definitions

// MessageOperator methods
MessageOperator::MessageOperator() {}
MessageOperator::~MessageOperator() {}

// ObjectContainer methods
ObjectContainer::ObjectContainer() {}
ObjectContainer::~ObjectContainer() {}

// InputContainer methods
InputContainer::InputContainer() {}
InputContainer::~InputContainer() {}

// OutputContainer methods
OutputContainer::OutputContainer() {}
OutputContainer::~OutputContainer() {}

// ComplexObject methods
ComplexObject::ComplexObject(NAMemory *heap, ComplexObjectType type) :
  heap_(heap), type_(type) {}

ComplexObject::~ComplexObject(){}
// Subclass of ComplexObject can override to change the Pack, UnPacker,
// and Lengther behavior, for instance, starts to use a different heap
// by passng a new UnPacker object.
void ComplexObject::pack(Packer *packer, PackerInput *input,
                         PackerOutput *output){
  sharedOperationSequence(packer, input, output);
}

void ComplexObject::unPack(UnPacker *unPacker, UnPackerInput *input,
			   UnPackerOutput *output){
  sharedOperationSequence(unPacker, input, output);
}

void ComplexObject::length(Lengther *lengther, LengtherInput *input,
			   LengtherOutput *output){
  sharedOperationSequence(lengther, input, output);
}

void ComplexObject::baseOperation(MessageOperator *msgOp, InputContainer *input,
				  OutputContainer *output){
  EnumObjectContainer enumObjWrap(&type_);
  msgOp->execute(&enumObjWrap, input, output);
}

NAMemory * ComplexObject::getHeap() { return heap_; }

void ComplexObject::operator delete(void *p){
  if(p){
    NAMemory *heap = ((ComplexObject *)p)->getHeap();
    if(heap){
      heap->deallocateMemory(p);
    }
    else{
      ::delete((ComplexObject *)p);
    }
  }
}

ComplexObjectType ComplexObject::getType() { return type_; }

// factory method for all Complex Objects, based on the type id in buffer
// return NULL for now.
ComplexObject * ComplexObject::manufacture(NAMemory *heap, char *bufferPtr,
							 ComplexObjectFactory *factory){
  ComplexObjectType type = InvalidComplexObjectType;
  EnumObjectContainer enumObjWrap(&type);
  UnPacker unPacker(heap, factory);
  UnPackerInput unPackerInput(bufferPtr);
  UnPackerOutput unPackerOutput;
  enumObjWrap.unPack(&unPacker, &unPackerInput, &unPackerOutput); // get the type description
  switch(type){
  case EmptyComplexObjectType:
    return new (heap) EmptyComplexObject(heap);
  default:
    return factory->manufacture(heap, type); // forward to extension
  }
}
// end of abstract class definitions

// begin stack-allocated objects
// In principle, Packer, UnPacker and Lengther classes should ues singleton
// Pattern, However, NSK security does not allow global variables.

// Packer related class definitions
// Packer definitions
Packer::Packer() {}

Packer::~Packer() {}

void Packer::execute(ObjectContainer *objWrap, InputContainer *input,
		     OutputContainer *output)
{
  objWrap->pack(this, (PackerInput *)input, (PackerOutput *)output);
}

void Packer::setInputOutputForNextOperation(InputContainer *input,
					    OutputContainer *output){
  char * bufferPtr = ((PackerOutput *)output)->getBufferPtr();
  ((PackerInput *)input)->setBufferPtr(bufferPtr);
  IpcMessageObjSize currMsgSize = ((PackerOutput *)output)->getCurrMsgSize();
  ((PackerInput *)input)->setAccMsgSize(currMsgSize);
}


// PackerInput definitions
PackerInput::PackerInput(char *bufferPtr, IpcMessageObjSize accMsgSize) : 
  bufferPtr_(bufferPtr), accMsgSize_(accMsgSize) {}

PackerInput::~PackerInput() {}

char * PackerInput::getBufferPtr() { return bufferPtr_; }

IpcMessageObjSize PackerInput::getAccMsgSize() { return accMsgSize_; }

void PackerInput::setBufferPtr(char * bufferPtr) { bufferPtr_ = bufferPtr; }

void PackerInput::setAccMsgSize(IpcMessageObjSize accMsgSize) { 
  accMsgSize_ = accMsgSize;
}

// PackerOutput 
PackerOutput::PackerOutput() :
  bufferPtr_(NULL), currMsgSize_(0) {}

PackerOutput::~PackerOutput() {}

char * PackerOutput::getBufferPtr() { return bufferPtr_; }

IpcMessageObjSize PackerOutput::getCurrMsgSize() { return currMsgSize_; }

void PackerOutput::setBufferPtr(char *bufferPtr) { bufferPtr_ = bufferPtr; }

void PackerOutput::setCurrMsgSize(IpcMessageObjSize currMsgSize){
  currMsgSize_ = currMsgSize;
}

// UnPacker related class definitions
// UnPacker defintions
UnPacker::UnPacker(NAMemory *heap, ComplexObjectFactory *factory) : 
heap_(heap), factory_(factory) {}
UnPacker::~UnPacker() {}

void UnPacker::execute(ObjectContainer *objWrap, InputContainer *input,
		       OutputContainer *output){
  objWrap->unPack(this, (UnPackerInput *)input, (UnPackerOutput *)output);
}

void UnPacker::setInputOutputForNextOperation(InputContainer *input,
					      OutputContainer *output){
  char * bufferPtr = ((UnPackerOutput *)output)->getBufferPtr();
  ((UnPackerInput *)input)->setBufferPtr(bufferPtr);
}

NAMemory * UnPacker::getHeap() { return heap_; }
ComplexObjectFactory * UnPacker::getFactory() { return factory_; }
// UnPackerInput definitions
UnPackerInput::UnPackerInput(char *bufferPtr) : bufferPtr_(bufferPtr) {}

UnPackerInput::~UnPackerInput() {}

char * UnPackerInput::getBufferPtr() { return bufferPtr_; }

void UnPackerInput::setBufferPtr(char *bufferPtr) { bufferPtr_ = bufferPtr; }

// UnPackerOutput definitions
UnPackerOutput::UnPackerOutput() : bufferPtr_(NULL) {}

UnPackerOutput::~UnPackerOutput() {}

char * UnPackerOutput::getBufferPtr() { return bufferPtr_; }

void UnPackerOutput::setBufferPtr(char *bufferPtr) { bufferPtr_ = bufferPtr; }

// Lengther related class definitions
// Lengther definitions
Lengther::Lengther() {}

Lengther::~Lengther() {}

void Lengther::execute(ObjectContainer *objWrap, InputContainer *input,
		       OutputContainer *output){
  objWrap->length(this, (LengtherInput *)input, (LengtherOutput *)output);
}

void Lengther::setInputOutputForNextOperation(InputContainer *input,
				    OutputContainer *output){
  IpcMessageObjSize currMsgSize = ((LengtherOutput *)output)->getCurrMsgSize();
  ((LengtherInput *)input)->setAccMsgSize(currMsgSize);
}

// LengtherInput definitions
LengtherInput::LengtherInput(IpcMessageObjSize accMsgSize) :
  accMsgSize_(accMsgSize) {}

LengtherInput::~LengtherInput() {}

IpcMessageObjSize LengtherInput::getAccMsgSize() { return accMsgSize_; }

void LengtherInput::setAccMsgSize(IpcMessageObjSize accMsgSize){
  accMsgSize_ = accMsgSize; 
}

// LengtherOutput definitions
LengtherOutput::LengtherOutput() : currMsgSize_(0) {}

LengtherOutput::~LengtherOutput() {}

IpcMessageObjSize LengtherOutput::getCurrMsgSize() { return currMsgSize_; }

void LengtherOutput::setCurrMsgSize(IpcMessageObjSize currMsgSize){
  currMsgSize_ = currMsgSize;
}

// ComplexObjectPtrContainer definitions
ComplexObjectPtrContainer::ComplexObjectPtrContainer(ComplexObjectPtr *objectPtr) :
  objectPtr_(objectPtr) 
{ BriefAssertion(objectPtr, "a pointer to ComplexObjectPtr should not be NULL"); }
  
ComplexObjectPtrContainer::~ComplexObjectPtrContainer() {}

ComplexObjectPtr * ComplexObjectPtrContainer::getComplexObjectPtr() 
{ return objectPtr_; }

void ComplexObjectPtrContainer::setComplexObjectPtr(ComplexObjectPtr *objectPtr) {
  objectPtr_ = objectPtr;
}

void ComplexObjectPtrContainer::pack(Packer *packer, PackerInput *input,
				  PackerOutput *output){
  ComplexObject *object = *objectPtr_;
  if(object){
    object->pack(packer, input, output);
  }
  else{
    EmptyComplexObject eObj;
    eObj.pack(packer, input, output);
  }
}

void ComplexObjectPtrContainer::unPack(UnPacker *unPacker, UnPackerInput *input,
				    UnPackerOutput *output){
  ComplexObject *object = *objectPtr_;
  if(object){
    object->unPack(unPacker, input, output);
  }
  else{
    ComplexObject *plexObj = ComplexObject::manufacture(unPacker->getHeap(),
      input->getBufferPtr(), unPacker->getFactory());
    BriefAssertion(plexObj, "unable to retrieve ComplexObject from buffer");
    plexObj->unPack(unPacker, input, output);
    if(plexObj->getType() != EmptyComplexObjectType) {
      *objectPtr_ = plexObj;
    }
    else{
      *objectPtr_ = NULL; // *objectPtr_ should already be NULL, this is a nop.
      delete (EmptyComplexObject *)plexObj; // overloaded delete method called.
    }
  }
}

void ComplexObjectPtrContainer::length(Lengther *lengther, LengtherInput *input,
				       LengtherOutput *output){
  ComplexObject *object = *objectPtr_;
  if(object){
    object->length(lengther, input, output);
  }
  else{
    EmptyComplexObject eObj;
    eObj.length(lengther, input, output);
  }
}

// CharPtrObjectContainer definitions

CharPtrObjectContainer::CharPtrObjectContainer(CharPtr *charPtr) :
  charPtr_(charPtr) 
{ BriefAssertion(charPtr, "a pointer to CharPtr should not be NULL"); }


CharPtrObjectContainer::~CharPtrObjectContainer() {}

CharPtr * CharPtrObjectContainer::getCharPtr() {return charPtr_; }

void CharPtrObjectContainer::setCharPtr(CharPtr *charPtr) { charPtr_ = charPtr; }

void CharPtrObjectContainer::pack(Packer *packer, PackerInput *input,
			     PackerOutput *output){
  char * bufferPtr = input->getBufferPtr();
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += ::packCharStarIntoBuffer(bufferPtr, *charPtr_);
  output->setBufferPtr(bufferPtr);
  output->setCurrMsgSize(accMsgSize);
}

void CharPtrObjectContainer::unPack(UnPacker *unPacker, UnPackerInput *input,
	    UnPackerOutput *output){
  const char * bufferPtr = input->getBufferPtr();
  ::unpackBuffer(bufferPtr, *charPtr_, unPacker->getHeap());
  output->setBufferPtr((char *)bufferPtr);
}

void CharPtrObjectContainer::length(Lengther *lengther, LengtherInput *input,
				    LengtherOutput *output){
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += packedStringLength();
  output->setCurrMsgSize(accMsgSize);
}

IpcMessageObjSize CharPtrObjectContainer::packedStringLength()
{
  //
  //  The string will be preceded by a 4-byte length field and will
  // include the null-terminator
  //
  IpcMessageObjSize result;
  result = sizeof(Lng32);
  char *s = *charPtr_;
  if (s)
  {
    result += str_len(s) + 1;
  }
  return result;
}

// Int16ObjectContainer definitions

Int16ObjectContainer::Int16ObjectContainer(Int16 *value) : value_(value) 
{ BriefAssertion(value, "a pointer to Int16 should not be NULL"); }


Int16ObjectContainer::~Int16ObjectContainer() {}

Int16* Int16ObjectContainer::getValue() { return value_; }

void Int16ObjectContainer::setValue(Int16* value) { value_ = value; }

void Int16ObjectContainer::pack(Packer *packer, PackerInput *input,
				PackerOutput *output){
  Int16 data = *value_;
  char * bufferPtr = input->getBufferPtr();
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += packIntoBuffer(bufferPtr, data);
  output->setBufferPtr(bufferPtr);
  output->setCurrMsgSize(accMsgSize);
}

void Int16ObjectContainer::unPack(UnPacker *unPacker, UnPackerInput *input,
				  UnPackerOutput *output){
  Int16 data;
  const char * bufferPtr = input->getBufferPtr();
  ::unpackBuffer(bufferPtr, data);
  output->setBufferPtr((char *)bufferPtr);
  *value_ = data;
}

void Int16ObjectContainer::length(Lengther *lengther, LengtherInput *input,
				  LengtherOutput *output){
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += sizeof(*value_);
  output->setCurrMsgSize(accMsgSize);
}

// IntegerObjectContainer definitions
IntegerObjectContainer::IntegerObjectContainer(Int32 *value) : value_(value) 
{ BriefAssertion(value, "a pointer to int should not be NULL"); }


IntegerObjectContainer::~IntegerObjectContainer() {}

Int32* IntegerObjectContainer::getValue() { return value_; }

void IntegerObjectContainer::setValue(Int32* value) { value_ = value; }

void IntegerObjectContainer::pack(Packer *packer, PackerInput *input,
				PackerOutput *output){
  Int32 data = *value_;
  char * bufferPtr = input->getBufferPtr();
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += packIntoBuffer(bufferPtr, data);
  output->setBufferPtr(bufferPtr);
  output->setCurrMsgSize(accMsgSize);
}

void IntegerObjectContainer::unPack(UnPacker *unPacker, UnPackerInput *input,
				  UnPackerOutput *output){
  Int32 data;
  const char * bufferPtr = input->getBufferPtr();
  ::unpackBuffer(bufferPtr, data);
  output->setBufferPtr((char *)bufferPtr);
  *value_ = data;
}

void IntegerObjectContainer::length(Lengther *lengther, LengtherInput *input,
				  LengtherOutput *output){
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += sizeof(*value_);
  output->setCurrMsgSize(accMsgSize);
}

// LongObjectContainer definitions
LongObjectContainer::LongObjectContainer(Lng32 *value) : value_(value) 
{ BriefAssertion(value, "a pointer to int should not be NULL"); }


LongObjectContainer::~LongObjectContainer() {}

Lng32* LongObjectContainer::getValue() { return value_; }

void LongObjectContainer::setValue(Lng32* value) { value_ = value; }

void LongObjectContainer::pack(Packer *packer, PackerInput *input,
					 PackerOutput *output){
  Lng32 data = *value_;
  char * bufferPtr = input->getBufferPtr();
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += packIntoBuffer(bufferPtr, data);
  output->setBufferPtr(bufferPtr);
  output->setCurrMsgSize(accMsgSize);
}

void LongObjectContainer::unPack(UnPacker *unPacker, UnPackerInput *input,
					   UnPackerOutput *output){
  Lng32 data;
  const char * bufferPtr = input->getBufferPtr();
  ::unpackBuffer(bufferPtr, data);
  output->setBufferPtr((char *)bufferPtr);
  *value_ = data;
}

void LongObjectContainer::length(Lengther *lengther, LengtherInput *input,
					   LengtherOutput *output){
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += sizeof(*value_);
  output->setCurrMsgSize(accMsgSize);
}

// EnumObjectContainer definitions
EnumObjectContainer::EnumObjectContainer(ComplexObjectType *value) :
value_(value) {}

EnumObjectContainer::~EnumObjectContainer() {}

ComplexObjectType * EnumObjectContainer::getValue() { return value_; }

void EnumObjectContainer::setValue(ComplexObjectType * value) { value_ = value; }

void EnumObjectContainer::pack(Packer *packer, PackerInput *input,
			       PackerOutput *output){
  ComplexObjectType data = *value_;
  char * bufferPtr = input->getBufferPtr();
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += packIntoBuffer(bufferPtr, data);
  output->setBufferPtr(bufferPtr);
  output->setCurrMsgSize(accMsgSize);
}

void EnumObjectContainer::unPack(UnPacker *unPacker, UnPackerInput *input,
				 UnPackerOutput *output){
  ComplexObjectType data;
  const char * bufferPtr = input->getBufferPtr();
  ::unpackBuffer(bufferPtr, data);
  output->setBufferPtr((char *)bufferPtr);
  *value_ = data;
}

void EnumObjectContainer::length(Lengther *lengther, LengtherInput *input,
	    LengtherOutput *output){
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  accMsgSize += sizeof(*value_);
  output->setCurrMsgSize(accMsgSize);
}

// NopObjectContainer definitions
NopObjectContainer::NopObjectContainer() {}
NopObjectContainer::~NopObjectContainer() {}

void NopObjectContainer::pack(Packer *packer, PackerInput *input,
			       PackerOutput *output){
  char * bufferPtr = input->getBufferPtr();
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  output->setBufferPtr(bufferPtr);
  output->setCurrMsgSize(accMsgSize);
}

void NopObjectContainer::unPack(UnPacker *unPacker, UnPackerInput *input,
				 UnPackerOutput *output){
  char * bufferPtr = input->getBufferPtr();
  output->setBufferPtr(bufferPtr);
}

void NopObjectContainer::length(Lengther *lengther, LengtherInput *input,
	    LengtherOutput *output){
  IpcMessageObjSize accMsgSize = input->getAccMsgSize();
  output->setCurrMsgSize(accMsgSize);
}

// complex object class definitions
// EmptyComplexObject defintions
EmptyComplexObject::EmptyComplexObject(NAMemory *heap) :
  ComplexObject(heap, EmptyComplexObjectType) {}

EmptyComplexObject::EmptyComplexObject() :
  ComplexObject(NULL, EmptyComplexObjectType) {}
 
EmptyComplexObject::~EmptyComplexObject() {}

void EmptyComplexObject::freeSubObjects() {}

void EmptyComplexObject::sharedOperationSequence(MessageOperator *msgOp,
				       InputContainer *input,
				       OutputContainer *output) {
  baseOperation(msgOp, input, output);
}











