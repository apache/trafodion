// **********************************************************************
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
// **********************************************************************
#ifndef SEQ_FILE_READER_H
#define SEQ_FILE_READER_H

#include "JavaObjectInterface.h"

// ===========================================================================
// ===== The SequenceFileReader class implements access to th Java 
// ===== SequenceFileReader class.
// ===========================================================================

typedef enum {
  SFR_OK     = JOI_OK
 ,SFR_NOMORE = JOI_LAST         // OK, last row read.
 ,SFR_ERROR_INITSERDE_PARAMS    // JNI NewStringUTF() in initSerDe()
 ,SFR_ERROR_INITSERDE_EXCEPTION // Java exception in initSerDe()
 ,SFR_ERROR_OPEN_PARAM          // JNI NewStringUTF() in open()
 ,SFR_ERROR_OPEN_EXCEPTION      // Java exception in open()
 ,SFR_ERROR_GETPOS_EXCEPTION    // Java exception in getPos()
 ,SFR_ERROR_SYNC_EXCEPTION      // Java exception in seeknSync(
 ,SFR_ERROR_ISEOF_EXCEPTION     // Java exception in isEOF()
 ,SFR_ERROR_FETCHROW_EXCEPTION  // Java exception in fetchNextRow()
 ,SFR_ERROR_CLOSE_EXCEPTION     // Java exception in close()
 ,SFR_LAST
} SFR_RetCode;

class SequenceFileReader : public JavaObjectInterface
{
public:
  // Default constructor - for creating a new JVM		
  SequenceFileReader(NAHeap *heap)
  :  JavaObjectInterface(heap) 
  {}

  // Constructor for reusing an existing JVM.
  SequenceFileReader(NAHeap *heap, JavaVM *jvm, JNIEnv *jenv)
    :  JavaObjectInterface(heap)
  {}

  // Destructor
  virtual ~SequenceFileReader();
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  SFR_RetCode    init();

  // Initialize SerDe configuration.
  // Should be called only if fetchArrayOfColumns() is needed.
//  SFR_RetCode    initSerDe(int         numColumns, 
//                           const char* fieldDelim,
//                           const char* columns, 
//                           const char* colTypes, 
//                           const char* nullFormat);

  // Open the HDFS SequenceFile 'path' for reading.
  SFR_RetCode    open(const char* path);
  
  // Get the current file position.
  SFR_RetCode    getPosition(Int64& pos);
  
  // Seek to offset 'pos' in the file, and then find 
  // the beginning of the next record.
  SFR_RetCode    seeknSync(Int64 pos);
  
  // Have we reached the end of the file yet?
  SFR_RetCode    isEOF(bool& isEOF);
  
  // Fetch the next row, as an array of columns.
  // This method uses the Hive SerDe code, and is much slower 
  // than the other calls.
//  char**         fetchArrayOfColumns();
  
  // Fetch the next row as a raw string into 'buffer'.
  SFR_RetCode    fetchNextRow(Int64 stopOffset, char* buffer);
  
  // Close the file.
  SFR_RetCode    close();

  SFR_RetCode    fetchRowsIntoBuffer(Int64 stopOffset, char* buffer, Int64 buffSize, Int64& bytesRead, char rowDelimiter);

  virtual char*  getErrorText(SFR_RetCode errEnum);

protected:
  jstring getLastError();

//  char** JStringArray2CharsArray(jobjectArray jarray);
  
private:
  enum JAVA_METHODS {
    JM_CTOR = 0, 
//    JM_INITSERDE,
    JM_GETERROR,
    JM_OPEN,
    JM_GETPOS,
    JM_SYNC,
    JM_ISEOF,
//    JM_FETCHCOLS,
    JM_FETCHROW,
    JM_FETCHROW2,
    JM_FETCHBUFF1,
    JM_FETCHBUFF2,
    JM_CLOSE,
    JM_LAST
  };
 
  static jclass javaClass_;
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
};

// ===========================================================================
// ===== The SequenceFileWriter class implements access to the Java 
// ===== SequenceFileWriter class.
// ===========================================================================

typedef enum {
  SFW_OK     = JOI_OK
 ,SFW_FIRST  = SFR_LAST
 ,SFW_ERROR_OPEN_PARAM = SFW_FIRST
 ,SFW_ERROR_OPEN_EXCEPTION
 ,SFW_ERROR_WRITE_PARAM
 ,SFW_ERROR_WRITE_EXCEPTION
 ,SFW_ERROR_CLOSE_EXCEPTION
 ,SFW_LAST
} SFW_RetCode;

typedef enum {
  SFW_COMP_NONE    = 0
 ,SFW_COMP_RECOPRD = 1
 ,SFW_COMP_BLOCK   = 2
} SFW_CompType;
  
class SequenceFileWriter : public JavaObjectInterface
{
public:
  // Default constructor - for creating a new JVM		
  SequenceFileWriter(NAHeap *heap)
  :  JavaObjectInterface(heap) 
  {}

  // Constructor for reusing an existing JVM.
  SequenceFileWriter(NAHeap *heap, JavaVM *jvm, JNIEnv *jenv)
    :  JavaObjectInterface(heap)
  {}

  // Destructor
  virtual ~SequenceFileWriter();
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  SFW_RetCode    init();
  
  // Open the HDFS SequenceFile 'path' for reading.
  SFW_RetCode    open(const char* path, SFW_CompType compression);

  // Write a single (null-terminated) row 'data' into the SequenceFile.
  SFW_RetCode    write(const char* data);

  // Write a buffer of rows, separated by 'rowDelimiter' characters, to the SequenceFile.
  // Note: rowDelimiter characters will be overwritten by nulls to become string terminators.
  SFW_RetCode    writeBuffer(char* data, Int64 buffSize, const char* rowDelimiter);
  
  // Close the file.
  SFW_RetCode    close();
  SFW_RetCode    release();

  virtual char*  getErrorText(SFW_RetCode errEnum);





private:  
  enum JAVA_METHODS {
    JM_CTOR = 0, 
    JM_OPEN,
    JM_WRITE,
    JM_CLOSE,
    JM_LAST
  };
  
  static jclass javaClass_;
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
};

#endif
