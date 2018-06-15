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
/* -*-C++-*-
****************************************************************************
*
* File:         ComCompressionInfo.h
* Description:  Description of the compression method used, for how
*               this is used for Hive tables, but it could be
*               expanded to other objects.
* Created:      4/20/16
* Language:     C++
*
****************************************************************************
*/

#ifndef COM_COMPRESSION_INFO_H
#define COM_COMPRESSION_INFO_H

#include "NAVersionedObject.h"

class ComCompressionInfo : public NAVersionedObject
{
public:
  // Update the COMPRESSION_TYPE[] at org/trafodion/sql/HDFSClient.java when new enum is added
  enum CompressionMethod
    { UNKNOWN_COMPRESSION = 0, // unable to determine compression method
      UNCOMPRESSED,            // file is not compressed
      LZO_DEFLATE,             // using LZO deflate compression
      DEFLATE,                 // using DEFLATE compression
      GZIP,                    // using GZIP compression
      LZOP,                   // using LZOP compression
      SUPPORTED_COMPRESSIONS }; // Add any compression type above this line 

  ComCompressionInfo(CompressionMethod cm = UNKNOWN_COMPRESSION) :
       NAVersionedObject(-1),
       compressionMethod_(cm)
  {}

  virtual ~ComCompressionInfo();

  bool operator==(const ComCompressionInfo &o) const
                         { return compressionMethod_ == o.compressionMethod_; }

  CompressionMethod getCompressionMethod() const { return compressionMethod_; }
  void setCompressionMethod(const char *fileName);

  NABoolean isCompressed() const
                       { return (compressionMethod_ != UNCOMPRESSED &&
                                 compressionMethod_ != UNKNOWN_COMPRESSION ); }

  NABoolean splitsAllowed() const                   { return !isCompressed(); }

  // try to determine the compression method just from a file name
  static CompressionMethod getCompressionMethodFromFileName(const char *f);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual char *findVTblPtr(short classID);
  virtual unsigned char getClassVersionID();
  virtual void populateImageVersionIDArray();
  virtual short getClassSize();
  virtual Long pack(void * space);
  virtual Lng32 unpack(void * base, void * reallocator);

private:

  CompressionMethod compressionMethod_;

};

#endif
