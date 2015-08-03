#ifndef STFS_ERROR_H
#define STFS_ERROR_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_error.h
///  \brief   STFS_Error class
///    
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
///////////////////////////////////////////////////////////////////////////////

#include "stfs_defs.h"
#include "stfs_root.h"

#include <vector>

namespace STFS {


  class STFS_Error;
  class STFSError_ReportableError;
  class STFSError_Context;

  //////////////////////////////////////////////////////////////////////////
  //
  // STFSERROR_MAXERRS is an upper bound on the number of errors.  I don't
  // expect that we'll ever get close to it, but we need a limit for
  // packing and unpacking, if only to check for corruption...
  //
  //////////////////////////////////////////////////////////////////////////
#define STFSERROR_MAXERRS 10000

  //////////////////////////////////////////////////////////////////////////
  //
  // STFSERROR_CONTEXTLENMAX is the largest context buffer size accepted
  //
  //////////////////////////////////////////////////////////////////////////
#define STFSERROR_CONTEXTLENMAX 4096

  //////////////////////////////////////////////////////////////////////////
  //
  // Condition significance indicates the importance of the
  // condition being added.  The highest significance condition is
  // always the one that gets reported.  Typically, this is the
  // first condition received.  Other conditions, of whatever
  // significance are automatically added to the stack below the
  // first one.  If a later condition occurs that should be the
  // condition reported by the STFS library call,the ReplaceHighest
  // significance is used to replace the current reported condition.
  // The final Replace Highest condition is the one that gets
  // returned to a client.  If no ReplaceHighest condition is
  // recorded, then the first Highest condition is reported.  If no
  // Highest conditions are reported, then the first
  // lower-significance condition reported in in order received
  // by category.
  //
  /////////////////////////////////////////////////////////////////////////


  enum Enum_ConditionSignificance {
    CondSig_ReplaceHighest = 0,
    CondSig_Highest,
    CondSig_Moderate,
    CondSig_Low,
    CondSig_Last
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFS_Error
  /// 
  /// \brief  The error class for a single STFS operation
  ///
  /// This class encapsulates a the error handling/reporting for a
  /// single STFS logical operation.  A single operation such as a
  /// read or write might incorporate many steps, which might
  /// encounter one or more reportable conditions along the way.  The
  /// STFS_Error class allows for this, while preserving the highest
  /// reportable error as the main summary error code returned from
  /// the operation.
  /// \n
  /// Right now, only the top error is returned.  EvLog facilities,
  /// special logging or trace functions based on environment
  /// variables, or other diagnostic methods could be used to iterate
  /// through the other errors, especially for debugging.  This class
  /// is implemented early on so that we can dump errors into it;
  /// making things fancy comes later.
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFS_Error : public STFS_Root {
  public:

    // Constructor, destructor
    STFS_Error();
    ~STFS_Error();
      

    // Methods for manipulating condition sections
    bool PreserveCondition ( Enum_ConditionSignificance pv_CondSig,
                             int pv_errNo,
                             int pv_additionalErr,
                             char *pp_context,
                             size_t pv_contextLen
                             );

    void ReleaseAllConditions (void);


    bool ExtractCondition ( Enum_ConditionSignificance *pv_CondSig,
                            int *pv_errNo,
                            int *pv_additionErr,
                            char *pp_context,
                            size_t pv_contextLenMax,
                            size_t *pv_contextLenOut
                            );

    void ResetExtractPosition (void);  // resets extract to highest

    int GetHighestReportedErrNo (void); // gets the first erro no,
                                        // without resetting the
                                        // extract position

    size_t GetNumberOfReportableErrors (void); // Returns the number of reportable errors

    bool Pack (char *pp_buf, size_t pv_bufLen, size_t *pv_packedLen);
    bool Unpack (char *pp_buf, size_t pv_bufLen);

  protected:

  private:
    ////////////////////
    /// private data
    ////////////////////

    typedef std::vector<STFSError_ReportableError *>  errStructVector_Def;
    errStructVector_Def errVector_;

    int currentExtractOffset_;    // for iterative extraction, the current location
    int highCondCount_;           // number of high conditions stored in the vector
    int modCondCount_;            // number of mod conditions stored in the vector
    int lowCondCount_;            // number of low conditions stored in the vector


    ////////////////////
    // private methods
    ////////////////////

    bool InsertReportableError (STFSError_ReportableError *pp_errToInsert);

    void DecrementSignificanceCounter (Enum_ConditionSignificance pv_condSig);
    void IncrementSignificanceCounter (Enum_ConditionSignificance pv_condSig);

    int * FindConditionSignificanceCounter (Enum_ConditionSignificance pv_condSig);

    virtual bool IsEyeCatcherValid();

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSError_ReportableError
  /// 
  /// \brief  Each instance is a single reportable STFS error
  ///
  /// This class encapsulates a single error condition that is to be
  /// reported by STFS.  An error condition is represented by its
  /// error number, any additional error number, and any context.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSError_ReportableError : public STFS_Root {
  public:

    STFSError_ReportableError ();  // default constructor
    STFSError_ReportableError(Enum_ConditionSignificance pv_condSig, 
                            int pv_errNo, int pv_additionalErr,
                            STFSError_Context *pp_Context);

    ~STFSError_ReportableError () {}

    bool SetErrInfo (Enum_ConditionSignificance pv_CondSig, 
                     int pv_errNo, int pv_additionalErr,
                     STFSError_Context *pp_context);

    bool GetErrInfo (Enum_ConditionSignificance *pv_condSig, 
                     int *pv_errNo, int *pv_additionalErr, 
                     STFSError_Context **pp_context);

    virtual bool IsEyeCatcherValid(void);

    Enum_ConditionSignificance GetCondSig(void);


  protected:

  private:

    ///////////////////////////
    ///  Private Data Members
    //////////////////////////


    Enum_ConditionSignificance  significance_;

    int errNo_;
    int additionalErr_;

    STFSError_Context *context_;

    //////////////////////////
    ///  Private Methods
    //////////////////////////


  };


  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSError_Context
  /// 
  /// \brief  Error Context Class
  ///
  /// This class abstracts the error context buffer that is associated
  /// with some STFS errors.  Right now, this is a very basic
  /// abstraction as a simple character buffer type.  We might want to
  /// get fancier later, so I created a separate class for it.
  ///\n
  /// The Context class allocates its own context buffer.  The calling
  /// client can release the context buffer or reuse it after the
  /// SetContext method returns. If SetContext is unable to get enough
  /// memory, it returns FALSE, indicating an ENOMEM.  Since this class 
  /// is already in the error module, we can't report another error...
  ///\n
  /// WARNING: The contents and interpretation of the context buffers
  /// are error-specific.  If you add an error that makes use of the
  /// context buffer, please add an appropriate method for formatting
  /// its contents, if needed.  Otherwise, the contents are presumed
  /// to be string data, suitable for direct interpretation.  This method
  /// just copies things around while paying no attention to formatting
  /// for readability.
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSError_Context {
  public:

    //////////////////////////
    //  Public Methods
    //////////////////////////


    STFSError_Context ();
    STFSError_Context (char *pp_context, size_t pv_contextLen);
    ~STFSError_Context ();


    bool SetContext (char *pp_context, size_t pv_contextLen);

    bool GetContext (char *pp_tgtContext, 
                     size_t  pv_contextLenMax, 
                     size_t *pv_contextLen);

    size_t GetContextLen (void);

    void DeleteContext(void);    // deletes any existing context, preserves instance

    bool IsEmpty(void);          // checks to see if there's actually context

  protected:

  private:

    //////////////////////////
    //  Private Data 
    //////////////////////////

    char  *context_;  // local context, allocated here!
    size_t contextLen_;  // size of our local context

    // for now, we just have local copies of these items.  We might
    // get more fancy later, so I made it a separate class.

    // Failures to allocate or copy are ENOMEN.  But we can't allocate
    // space to actually report them, so just make these booleans.

    //////////////////////////
    //  Private Methods
    //////////////////////////

  };

}
#endif //STFS_ERROR_HANDLE
