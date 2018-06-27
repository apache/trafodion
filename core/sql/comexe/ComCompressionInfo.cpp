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

#include "ComCompressionInfo.h"

ComCompressionInfo::~ComCompressionInfo()
{}

void ComCompressionInfo::setCompressionMethod(const char *fileName)
{
  compressionMethod_ = getCompressionMethodFromFileName(fileName);
}

ComCompressionInfo::CompressionMethod ComCompressionInfo::getCompressionMethodFromFileName(
     const char *f)
{
  const char * ret = strcasestr(f, ".lzo_deflate");
  if (ret)
    return LZO_DEFLATE;
  ret = strcasestr(f, ".lzo");
  if (ret)
    return LZOP;
  ret = strcasestr(f, ".deflate");
  if (ret)
    return DEFLATE;
  ret = strcasestr(f, ".gz");
  if (ret)
    return GZIP;

  return UNCOMPRESSED;
}

// virtual methods overridden from NAVersionedObject base class

char *ComCompressionInfo::findVTblPtr(short classID)
{
  char *vtblPtr;
  GetVTblPtr(vtblPtr, ComCompressionInfo);

  return vtblPtr;
}

unsigned char ComCompressionInfo::getClassVersionID()
{
  return 1;
}

void ComCompressionInfo::populateImageVersionIDArray()
{
  setImageVersionID(0,getClassVersionID());
}

short ComCompressionInfo::getClassSize()
{
  return (short) sizeof(ComCompressionInfo);
}

Long ComCompressionInfo::pack(void * space)
{
  return NAVersionedObject::pack(space);
}

Lng32 ComCompressionInfo::unpack(void * base, void * reallocator)
{
  return NAVersionedObject::unpack(base, reallocator);
}
