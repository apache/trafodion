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
 * File:         CmpStoredProc.h
 * Description:  The definition of internal stored procedure related classes
 *               used in arkcmp.
 *
 *
 * Created:      03/14/97
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef CMPSTOREDPROC__H
#define CMPSTOREDPROC__H

#include "NABasicObject.h"
#include "NAString.h"
#include "CmpISPStd.h"

// for the table descriptions
#include "CmpCommon.h"
#include "Collections.h"  // for NAList
#include "Int64.h"

// execution of the ISP
#include "StoredProcInterface.h"

// contents of this file
class CmpSPInputFormat; // format description for stored procedure input parameters.
class CmpSPOutputFormat; // format description for stored procedure output table.
class CmpSPExecDataItem; // the data item(data/expr) passed from executor for execution of SP.
class CmpSPDataObject; // the data object (in/out/key) passed from executor for exection of SP.
class CmpStoredProc; // interface to stored procedure.
class CmpInternalSP; // interface to built-in stored procedure.
class CmpISPFuncs; // class to hold the built-in SP function pointers.
class CmpExecDataItem; // the basic data item sent from executor for ISP execution
class CmpExecDataItemInput; // the input data sent from executor
class CmpExecDataItemReply; // the reply sent back to executor for ISP execution result
class CmpSPDataObject; // the data object for stored procedure execution
class CmpISPDataObject; // the data object for internal stored procedure execution

// forward declaration
class ElemDDLColDef;
class ItemExpr;
class CmpMessageISPRequest;
class TrafDesc;
class TrafColumnsDesc;

// -----------------------------------------------------------------------
// CmpSPInputFormat class contains the definition of input data for the
// stored procedure. 
// This class provides methods for arkcmp to retrieve the input data format,
// an ITEM_LIST of NATypeToItem at sql compile/bind time.
// It also provides methods ( currently 04/04/97 only one ) to
// convert the different type of description of input parameters into
// CmpSPInputFormat.
// -----------------------------------------------------------------------

class CmpSPInputFormat : public NABasicObject
{
public:
  CmpSPInputFormat(CmpContext* context);

  // Set the definition from SP_FIELDDESC_STRUCT[].
  // return TRUE if success, FALSE if failed.
  NABoolean SetFormat(Lng32 nCols, // number of columns
		      SP_FIELDDESC_STRUCT* fields);

  ItemExpr* itemExpr() {  return inputType_;  }

  virtual ~CmpSPInputFormat();

private:
  // members

  // input format is described as an ITEM_LIST of NATypeToItem ItemExprs. 
  ItemExpr* inputType_;
  CmpContext* context_;

  CmpSPInputFormat(const CmpSPInputFormat&);
  const CmpSPInputFormat& operator=(const CmpSPInputFormat&);
}; // end of CmpSPInputFormat

// -----------------------------------------------------------------------
// CmpSPOutputFormat class describes the format of the stored procedure
// virtual tables. This class provides the interface routines to be used
// in binder. It also includes methods to convert description of output
// data format into CmpSPOutputFormat.
// -----------------------------------------------------------------------

class CmpSPOutputFormat : public NABasicObject
{
public:
  CmpSPOutputFormat(CmpContext* context);

  virtual ~CmpSPOutputFormat();

  // Set the definition from SP_FIELDDESC_STRUCT[].
  // return TRUE if success, FALSE if failed.
  NABoolean SetFormat(Lng32 nCols, // number of columns
    const char* tableName,   // table name
    SP_FIELDDESC_STRUCT* fields, // description
    Lng32 nKeys, // number of keys
    SP_KEYDESC_STRUCT *keys // index to the key columns
    );
  
  // generate the table_desc for binder, this method will create the
  // TrafDesc for the virtual table. The memory is allocated from 
  // sqlcat/read.C allocate routine, i.e. HEAP defined in common/CmpCommon.h
  // The memory allocated belongs to the caller, 
  // the caller should delete it accordingly.
  TrafDesc* tableDesc() { return tableDesc_; }
  
  NABoolean ValidTableFormat() { return !tableDesc_ ; }

  // static function to convert an ElemDDLColDef into TrafColumnsDesc
  // return TRUE if success, FALSE if not.
  static NABoolean ElemDDLColDef2ColumnDescStruct(ElemDDLColDef* elemDDL,
    const char* tableName,
    TrafColumnsDesc* colDesc);

private:

  // output format is described as TrafTableDesc
  TrafDesc* tableDesc_;
  Lng32 nCols_;
  CmpContext* context_;

  // methods to pupulate column_desc from SP_FIELDDESC_STRUCT
  // return TRUE : success, FALSE : fail
  NABoolean getColumnDesc(SP_FIELDDESC_STRUCT* fDesc,
    TrafColumnsDesc* colsDesc);
  
  NABoolean getKeysDesc(Lng32 nkeys, SP_KEYDESC_STRUCT* keys, 
    TrafDesc* &keysDesc);

  CmpSPOutputFormat(const CmpSPOutputFormat&);
  const CmpSPOutputFormat& operator=(const CmpSPOutputFormat&);
}; // CmpSPOutputFormat

// -----------------------------------------------------------------------
// CmpSPExecDataItem class contains the actual data/expression for execution
// of the stored procedure. The expression and actual data are prepared by
// executor and passed to arkcmp. This class also contains the methods to 
// access/format the data from/to executor.
// ----------------------------------------------------------------------- 

class CmpSPExecDataItem : public NABasicObject
{
public:
  // Construct the CmpSPExecDataItem from expr and data
  CmpSPExecDataItem(ULng32 exprSize, void* expr, 		
    ULng32 dataSize, void* data,
    CmpContext* context);
  virtual ~CmpSPExecDataItem();
  friend ostream& operator<<(ostream&, const CmpSPExecDataItem&);

  ULng32 exprSize() { return exprSize_; }
  void* & expr() { return expr_; }
  ULng32 dataSize() { return dataSize_; }
  void* data() { return data_; }

  ComDiagsArea* SPFuncsDiags() { return SPFuncsDiags_; }

  CollHeap* wHeap();

protected:
  ULng32 exprSize_;
  void* expr_;
  ULng32 dataSize_;
  void* data_;
  // The ComDiagsArea used in the expr->data conversion routines, 
  // It is used locally in CmpSPExtractFunc_ and CmpSPFormatFunc_.
  // The reason to put it here is for performance reason. Since the
  // CmpSPExtractFunc_ and CmpSPFormatFunc_ are called for each fields
  // of each row, it is very time consuming to call the constructor/dtor
  // of ComDiagsArea each time. So a copy of ComDiagsArea is kept here, 
  // initialized to 0 entries, cleanup only when error occurs, it will
  // save some time to construct/destruct ComDiagsArea in most cases, 
  // i.e. no error, no cleanup needed at all.
  ComDiagsArea* SPFuncsDiags_; 

  CmpContext* context_;

  CmpSPExecDataItem(const CmpSPExecDataItem&);
  const CmpSPExecDataItem& operator=(const CmpSPExecDataItem&);

}; // CmpSPExecDataItem

extern ostream& operator<<(ostream&, const CmpSPExecDataItem&);

// class that contains input expression/data sent from executor
class CmpSPExecDataItemInput : public CmpSPExecDataItem
{
public:
  // In the constructor, the input data will be
  // . prepared : i.e. unpacked, since it was packed by executor before sent.
  // . positioned to the first row.
  CmpSPExecDataItemInput(ULng32 exprSize, 
    void* expr,
    ULng32 dataSize,
    void* data,
    CmpContext* context);
  virtual ~CmpSPExecDataItemInput();

  // advance the current buffer data() for one tuple, 
  // returns 0 if success, 
  // 1 if no more row ( the current pointer will not point to a valid row.
  // -1 if fail.  
  short next();

  // extract certain field of data from current row
  // returns 0 if success, non 0 if failed.
  short extract(ULng32 num, char* data, ULng32 datalen, NABoolean casting,
    ComDiagsArea* diags)
  { return ExSPExtractInputValue(expr(), num, currentRow(), data, datalen, casting, diags); }
    
  // methods to retrieve private members
  void* control() { return control_; }
  char* currentRow() { return currentRow_; }
  ULng32 rowLength() { return rowLength_; }

private:
  // private members

  char* currentRow_;
  ULng32 rowLength_;
  void* control_;

  CmpSPExecDataItemInput(const CmpSPExecDataItemInput&);
  const CmpSPExecDataItemInput& operator=(const CmpSPExecDataItemInput&);

}; // CmpSPExecDataItemInput

// class that contains the reply tuples to be sent back to executor
class CmpSPExecDataItemReply : public CmpSPExecDataItem
{
public:
  // In the constructor, the output data will be
  // . allocated with size specified from outHeap. this data field will be taken
  //   away ( i.e. pointer owned by other routine ) to avoid copying. so it needs
  //   to come from the heap as specified.
  // . initialized to the executor expected format
  CmpSPExecDataItemReply(ULng32 exprSize,
    void* expr,
    ULng32 dataRowSize,
    ULng32 dataTotalSize,
    CmpSPExecDataItemInput* inputData,
    CollHeap* outHeap,
    CmpContext* context);
  virtual ~CmpSPExecDataItemReply();

  // prepare the reply buffer before sent back to executor.
  // returns TRUE if success, FALSE otherwise.
  NABoolean prepare();

  // put certain field of data into output data row
  // returns 0 if success, non 0 if failed.
  short moveOutput(ULng32 num,char* data,ULng32 datalen, NABoolean casting,
    ComDiagsArea* diags)
                       { return ExSPMoveOutputValue(expr(),
						    num,
						    (char*)rowBuffer_,
						    data,
						    datalen,
						    casting,
						    diags,
						    wHeap());
		       }

  // move the diags info from SP_ERROR_STRUCT into diags_
  NABoolean MoveDiags(const SP_ERROR_STRUCT*, const SP_STATUS spStatus);
  // merge the diags info from *pDiags into diags_
  NABoolean MergeDiags(ComDiagsArea *pDiags);
  ComDiagsArea& diags() { return diags_; }

  // methods to get the private members
  ULng32 rowLength() { return rowLength_; }
  void* rowBuffer() { return rowBuffer_; }
	
  // The following routines move the interter rowBuffer_ or diags_ into data_ buffer to be
  // sent back to executor. 
  // Return 1 : if rowBuffer_ is full, the buffer is not moved in, need to ask executor for
  //            more space.
  //        0 : if success, the buffer is moved in.
  //       -1 : fail with error.
  // Add a row(rowBuffer_) into the data_ buffer
  short AddARow();
  // Add an EOR (End of Row) into the data_ buffer, this is required by executor.
  short AddEOR();

  NABoolean rowExist() const { return rowExist_; }
  NABoolean EORExist() const { return EORExist_; }
  NABoolean moreData() const { return rowExist_ || EORExist_; }

  // This method takes the data_ out, it is the caller's responsibility to 
  // delete the memory allocated for data_ field. So data_ should be allocated
  // from outHeap_ which is passed in from constructor.
  void* takeData() { void* ret = data_; data_ = 0; return ret; }
  void allocateData(ULng32 size);

private:
  // private members
  CollHeap* outHeap_; // the heap to allocate output data_ from, since this data_ pointer might
                     // be taken away for performance reason. The caller should pass in the 
                     // CollHeap* to allocate data from.

  CmpSPExecDataItemInput* inputData_; // pointer to the input data that passed in to generate
										// this reply data.
  ULng32 rowLength_; // the length of each row, either calculated from expression, 
							  // or passed in from executor.
  // in the processing routines, open(), fetch() and close() the following output buffer
  // and diags_ are passed in to the built-in SP for output and being moved into the data_
  // fields sent back to executor.
  // In the future, for efficiency, we might want to use the pointer in data_ to directly
  // move in data instead of move it into the data_ buffer. 
  void* rowBuffer_; // the temperary buffer for each row, allocated in the constructor, reused
					  // for each fetch.
  ComDiagsArea diags_; // the buffer for ComDiagsArea for processing routines

  // The following flags identify whethere this object contains data to be sent back to 
  // executor. They are
  // . FALSE in the beginning
  // . TRUE/FALSE accordingly after interface with SP execution implementation.
  // . FALSE after packed into data_ buffer successfully.
 
  NABoolean rowExist_; // flag to identify whether rowBuffer_ contains data to be sent 
						 // back to executor.
  NABoolean diagsExist_; // flag to identify whether diags_ contains info to be sent back
						   // to executor.
  NABoolean EORExist_; // needs to send an EOR(End Of a Row) to the data_

  CmpSPExecDataItemReply(const CmpSPExecDataItemReply&);
  const CmpSPExecDataItemReply& operator=(const CmpSPExecDataItemReply&);

}; // CmpSPExecDataItemReply

// -----------------------------------------------------------------------
// CmpSPDataObject class contains the actual data(input/output/key) for 
// execution of the stored procedure. It is to be passed into the 
// processing routines, i.e. open(), fetch() and close() routines. 
// -----------------------------------------------------------------------

class CmpSPDataObject : public NABasicObject
{
public:
  CmpSPDataObject() {}
  virtual ~CmpSPDataObject() {}

private:
  CmpSPDataObject(const CmpSPDataObject&);
  const CmpSPDataObject& operator=(const CmpSPDataObject&);

}; // CmpSPDataObject


// -----------------------------------------------------------------------
// CmpISPDataObject class contains the actual data/expression for execution of the
// stored procedure. It is to be passed into the processing routines, i.e.
// open(), fetch() and close() routines. 
// ----------------------------------------------------------------------- 

class CmpISPDataObject : public CmpSPDataObject
{
public: 
  CmpISPDataObject(CmpMessageISPRequest* ,
    CmpInternalSP* isp,
    CollHeap* outHeap,
    CmpContext* context);
  virtual ~CmpISPDataObject();

  CmpSPExecDataItemInput* input() { return &input_; }
  CmpSPExecDataItem* key() { return &key_; }
  CmpSPExecDataItemReply* output() { return &output_; }

  // returns TRUE, if there is more data to be retrieved for output.
  NABoolean moreData() const { return output_.moreData(); }

private:
  // members
  // ispRequest from executor is kept in this object, since it owns the data
  // member inside this ispRequest for performance reason. It will be 
  // deleted when this object is out of scope.
  CmpMessageISPRequest* ispRequest_;
  CmpSPExecDataItemInput input_;
  CmpSPExecDataItem key_; // ???? TODO
  CmpSPExecDataItemReply output_;

  CmpContext* context_;
  // the CollHeap to allocate output data from
  CollHeap* outHeap_;

  CmpISPDataObject(const CmpISPDataObject&);
  const CmpISPDataObject& operator=(const CmpISPDataObject&);

}; // CmpISPDataObject

// -----------------------------------------------------------------------
// CmpStoredProc class is used by arkcmp ( compiler main program ) to 
// compile and execute stored procedure. ( currently only internal stored
// procedures ) 
// The input/output data of the stored procedures are treated as rows of data
// from virtual tables. The format of the data will be provided in the virtual
// functions. These functions are used in compilation of the SQL query of
// executing the stored procedure. 
// To actually execute the stored procedure, executor calls arkcmp for 
// execution. The execution mechanism is similiar to cursor, i.e. 
// for each input data
//    call open virtual method to start the process of input data.
//    call fetch virtual method to fetch each row of data. 
//    call close virtual method when the there is no more output data.
// 
// -----------------------------------------------------------------------

class CmpStoredProc : public NABasicObject 
{
public:
  CmpStoredProc(const NAString& procName, CmpContext* cmpContext);
  virtual ~CmpStoredProc();

  // virtual function to retrieve the input data format
  // return TRUE if success, FALSE otherwise
  // num is the number of input parameters expected, it is returned from
  // parser.
  virtual NABoolean InputFormat(Lng32 num, CmpSPInputFormat&);

  // virtual function to retrieve the output data format
  //return TRUE if success, FALSE otherwise
  virtual NABoolean OutputFormat(CmpSPOutputFormat&);  

  // return status from open, fetch and close methods
  enum ExecStatus { SUCCESS, MOREDATA, FAIL };

  // virtual function to process input data
  // return SUCESS if no error, return FAIL if there is error. The error information
  // is put into the diags area in cmpContext_
  virtual ExecStatus open(CmpSPDataObject&);  

  // virtual function to fetch output data
  // return SUCESS if there is no more data, return MOREDATA if there are more data
  // to be fetched, return FAIL if there is error. The error information is put 
  // into the diags area in CmpContext_
  virtual ExecStatus fetch(CmpSPDataObject&);  

  // virtual function to end a process of one input data
  // return SUCCESS if no error. return FAIL is there is error. The error information
  // is put into the diags area in cmpContext_
  virtual ExecStatus close(CmpSPDataObject&);  

  CmpContext* cmpContext() { return cmpContext_; }
  const NAString& procName()  const  { return procName_; }
  
private:

  NAString procName_;  
  CmpContext* cmpContext_;

  CmpStoredProc(const CmpStoredProc&);
  const CmpStoredProc& operator=(const CmpStoredProc&);
}; // CmpStoredProc

// -----------------------------------------------------------------------
// CmpInternalSP is the stored procedure class that runs in the mechanism 
// as described in the 'Internal Stored Procedure ES'.
// These stored procedures are defined by the internal developer, the
// interfaces are 
// from arkcmp : CmpISPStd.h 
// from user defined internal SP : an xxxx.h file for declaration of 
//            interface routines. A DLL/LIB for the implementation.
// The user defined SP will be linked into arkcmp.
// The interface procedures are defined as C interface to accomidate C, C++
// routines. The basic routines are 
//   - routine to get the input data format. 
//   - routines to get the output data format, number of fields and format.
//     The output data is treated as a virtual table.
//   - routine to process the data, the processing is treated as in cursor, 
//     for each input row of data, OPEN, FETCH and CLOSE are called 
//     accordingly.
// -----------------------------------------------------------------------

//------------------------------------------------------------------------
// CmpISPFuncs is the class to maintain the index to built-in stored procedure
// implementation being linked in. 
// Currently ( 05/20/97 ) a static array is maintained, being populated by 
// in the constructor by calling some entry routines that built-in stored
// procedures provide. 
//------------------------------------------------------------------------ 

class CmpISPFuncs
{
public:
  // The structure to keep references to a built-in stored procedure, 
  // it is indexed by the procName_.
  struct ProcFuncsStruct 
  {
    char procName_[SP_STRING_MAX_LENGTH]; // null terminated
    SP_COMPILE_FUNCPTR compileFunc_;
    SP_INPUTFORMAT_FUNCPTR inFormatFunc_;
    SP_PARSE_FUNCPTR parseFunc_;
    SP_NUM_OUTPUTFIELDS_FUNCPTR outNumFormatFunc_;
    SP_OUTPUTFORMAT_FUNCPTR outFormatFunc_;
    SP_PROCESS_FUNCPTR procFunc_;
    SP_HANDLE spHandle_;

    ProcFuncsStruct(const char* procName=0, 
      SP_COMPILE_FUNCPTR compileFunc=0,
      SP_INPUTFORMAT_FUNCPTR inFormatFunc=0,
      SP_PARSE_FUNCPTR parseFunc=0,
      SP_NUM_OUTPUTFIELDS_FUNCPTR outNumFormatFunc=0,
      SP_OUTPUTFORMAT_FUNCPTR outFormatFunc=0,
      SP_PROCESS_FUNCPTR procFunc=0,
      SP_HANDLE spHandle=0)
    {
      if (procName)
        strcpy(procName_, procName);
      else
	procName_[0] = 0;
      compileFunc_ = compileFunc;
      inFormatFunc_ = inFormatFunc;
      parseFunc_ = parseFunc;
      outNumFormatFunc_ = outNumFormatFunc;
      outFormatFunc_ = outFormatFunc;
      procFunc_ = procFunc;
      spHandle_ = spHandle;
    }
    NABoolean operator == (const ProcFuncsStruct& other)
    {
      return(strcmp(procName_, other.procName_)== 0 );
    }
  };
    
  // the static array for built-in stored procedure implementation
  // reference. This should be a process wide information. 
  static NAList<ProcFuncsStruct> procFuncsArray_;

  CmpISPFuncs();
  virtual ~CmpISPFuncs();

  // methods to access the interface function pointer
  inline const ProcFuncsStruct& operator[](const NAString& name) const;
  NABoolean ValidPFuncs(const ProcFuncsStruct& pFuncs) const;

  // method to register function pointers for a built-in stored procedures, 
  // indexed by name. This will be the callback function for built-in 
  // stored procedures to register the function pointers.  
  static Int32 RegFuncs(const char* procName,
    SP_COMPILE_FUNCPTR compileFunc,
    SP_INPUTFORMAT_FUNCPTR inFormatFunc,
    SP_PARSE_FUNCPTR parseFunc,
    SP_NUM_OUTPUTFIELDS_FUNCPTR outNumFormatFunc,
    SP_OUTPUTFORMAT_FUNCPTR outFormatFunc,
    SP_PROCESS_FUNCPTR procFunc,
    SP_HANDLE spHandle,
    const char* version);
  
  private:
    CmpISPFuncs(const CmpISPFuncs&);
    const CmpISPFuncs& operator = (const CmpISPFuncs&);
}; // end of CmpISPFuncs

class CmpInternalSP : public CmpStoredProc 
{
public:  
  CmpInternalSP(const NAString& procName, CmpContext* cmpContext);
  virtual ~CmpInternalSP();  

  // return the table name from procName
  NAString OutTableName();

  virtual NABoolean InputFormat(Lng32 num, CmpSPInputFormat&);
  virtual NABoolean OutputFormat(CmpSPOutputFormat& );  

  // used only in ISP_UTIL command
  // return TRUE if success, FALSE if fail.
  NABoolean ParseInput(const NAString& param);
  
  // virtual function to process input data
  ExecStatus open(CmpISPDataObject&);  

  // virtual function to fetch output data
  ExecStatus fetch(CmpISPDataObject&);  

  // virtual function to end a process of one input data
  ExecStatus close(CmpISPDataObject&);  

  CmpISPDataObject* ispData() const { return ispData_; }
  void SetCmpISPDataObject(CmpISPDataObject* ispData) { ispData_ = ispData; }

private:
  // The Lookup Table for all the built-in stored procedured 
  CmpISPFuncs procFuncsLookupTable_;

  // The entry of the built-in stored procedure interface
  // routines for this instance of CmpInternalSP
  CmpISPFuncs::ProcFuncsStruct procFuncs_;

  // members for handles to interface routines
  SP_COMPILE_HANDLE compHandle_;
  SP_PROCESS_HANDLE procHandle_;
  // SP_ERROR_STRUCT used for the interface routines to ISP
  SP_ERROR_STRUCT spError_[SP_MAX_ERROR_STRUCTS];  

  // CmpISPDataObject contains data for execution of ISP
  CmpISPDataObject* ispData_;

  // state of the internalSP, the state of the CmpInternalSP could be
  // NONE -> COMPILE -> NONE for compilation.
  // NONE -> EXECUTE -> NONE for execution.
  enum InterfaceState 
  { 
    COMPILE, // in compile of the SP query
    PROCESS, // in execution of the SP query 
    NONE     // none of the above
  } state_;

  // helper routines for compiler related tasks.

  // method to start Compile cycle.
  NABoolean startCompileCycle();

  // methods for SP interface structure allocation
  SP_FIELDDESC_STRUCT* allocSP_FIELDDESC_STRUCT(Lng32 num);
  void deleteSP_FIELDDESC_STRUCT(SP_FIELDDESC_STRUCT*);
  SP_KEYDESC_STRUCT* allocSP_KEYDESC_STRUCT(Lng32 num);
  void deleteSP_KEYDESC_STRUCT(SP_KEYDESC_STRUCT*);
    
  // private methods for error handling.
  // initialize spError_ before each interface routine.
  void initSP_ERROR_STRUCT();
  // append the error info into cmpContext()->diags, sent back to arkcmp
  void appendDiags(); 

  CmpInternalSP(const CmpInternalSP&);
  const CmpInternalSP& operator=(const CmpInternalSP&);
}; // CmpInternalSP

inline const CmpISPFuncs::ProcFuncsStruct& 
	CmpISPFuncs::operator[](const NAString& name) const
{
  Int32 i;
  for ( i=0; i < (Int32)(procFuncsArray_.entries()-1); i++)
    if (name == procFuncsArray_[i].procName_)
      return procFuncsArray_[i];
  return procFuncsArray_[i];
}

// helper routines CmpInternalSP provides for the SP interface routines.

#ifndef CMPISPUTILS__C
extern "C"
{
extern SP_EXTRACT_FUNCPTR CmpSPExtractFunc;
extern SP_FORMAT_FUNCPTR CmpSPFormatFunc;
extern SP_KEYVALUE_FUNCPTR CmpSPKeyValueFunc;
}
#endif

#endif

