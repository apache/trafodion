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
#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else


#include "sqtypes.h"

#endif

typedef DWORD MeasCId_t;      // Measure Counter ID type.
typedef DWORD MeasCType_t;    // Measure Counter Type type.
typedef DWORD MeasVrsn_t;     // Measure Counter Version type.

const MeasCType_t   MEASURE_NULLCTYPE       = 0;  // "Null" type. Do not use.
const MeasCType_t   MEASURE_TDMACTYPE       = 1;  // Accumulating Counter.
const MeasCType_t   MEASURE_TDMBCTYPE       = 2;  // Busy Counter.
const MeasCType_t   MEASURE_TDMICTYPE       = 3;  // Incrementing Counter.
const MeasCType_t   MEASURE_TDMQCTYPE       = 4;  // Queue Counter.
const MeasCType_t   MEASURE_TDMSCTYPE       = 5;  // Snapshot Counter.
const MeasCType_t   MEASURE_TDMTCTYPE       = 6;  // Time Counter.

class MeasureCounter;

class DLLEXPORT MeasureEntity
{
public:
  MeasureEntity( const MeasVrsn_t counterVersion )
  {
    dwGetMeasureWareDynamicStatus();
  }

  virtual ~MeasureEntity()
  {}

  void        disable() 
  { return; }

  void        enable         ();

  DWORD enable ( LPCWSTR   pInstanceName,
			 MeasCId_t counterId,
			 LPVOID    counterDataAddress
		 );

  BOOL inline enabled        ( void ) 
  { return false; }

  DWORD       instantiate    ( LPCSTR objectName,
			       LPCSTR instanceName,
			       DWORD  timeOut
			       )
  { return 0; }

  DWORD       instantiate    ( LPCSTR objectName,
			       LPCSTR instanceName,
			       DWORD  timeOut,
			       DWORD  dwCurrentMeasureWareStatus
			       )
  { return 0; }

  DWORD       instantiate    ( LPCSTR objectName,
			       LPCSTR instanceName,
			       LPCSTR subInstanceName,
			       DWORD  timeOut
			       )
  { return 0; }

  DWORD       registerCounter( MeasCId_t        counterId,
			       MeasureCounter * pCounter
			       )
  { return 0; }

  DWORD       dwGetMeasureWareDynamicStatus ( )
  { return 0; }


  DWORD       dwInstantiateAvecSaved ( )
  { return 0; }



protected:
  void   vFreeBuffers ( void )
  { return; }


  DWORD  checkType            ( MeasCId_t        counterId,
				MeasureCounter * pCounter
				)
  { return 0; }

  DWORD  initializeCounter    ( MeasCId_t        counterId,
				MeasureCounter * pCounter
				)
  { return 0; }

private:
  MeasureEntity()                                         // No default constructor.
  { return; }

  MeasureEntity ( const MeasureEntity &ME )               // No copying.
  { return; }


#if 0
  MeasureEntity& operator = ( const MeasureEntity &ME )  // No assignment.
  { return; }

protected:
  //
  // Following set of flags are added to support the Dynamic enable
  // and disable of MeasureWare.
  //
  DWORD                   m_dwMeasureWareEnableStatus_;
  char                    *m_pObjectName_;
  char                    *m_pInstanceName_;
  DWORD                   m_dwTimeOut_;

  DWORD
  instanceHandle_;

  LPBOOL
  pEntityEnabled_;

  LPVOID
  pCounterBlock_;

  LPVOID
  pCounterDescriptors_;

  LPWSTR
  pInstanceNTNamesPrefix_;

  LPVOID
  pObjectControlBlock_;

  MeasCId_t
  maxCounterId_;

  MeasureSharedMemoryManager
  *pSharedMemoryMgr_;

  MeasVrsn_t
  counterBlockVersion_;
#endif

};

void        MeasureEntity::enable         ()
  { return; }

  DWORD MeasureEntity::enable ( LPCWSTR   pInstanceName,
			 MeasCId_t counterId,
			 LPVOID    counterDataAddress
			 )
  { return 0; }


class DLLEXPORT MeasureCounter
{
public:
  MeasureCounter() 
  { 
    dwRegisterNow(1);
    dwRegisterMyself();
  }
  virtual ~MeasureCounter();

  virtual void        disable ( void )
  { return; }

  virtual DWORD       enable  ( LPCWSTR   pInstanceName,
				MeasCId_t counterId,
				LPVOID    counterDataAddress
				) ;

  virtual BOOL bIsEnabled ( void )
  { return false; }

  //NGG  virtual MeasCType_t type    ( void ) const = 0;


private:
  MeasureCounter( const MeasureCounter &counter );         // No copying.
  MeasureCounter& operator = ( const MeasureCounter &MC ); // No assigning.

protected:
  //NGG  LPVOID pCounter_;

  //
  // Fast Foreign counter's Related functions and counters
  //

public:
  DWORD SaveFastCounterData ( LPCSTR pszObjectName_i,
			      LPCSTR pszInstanceName_i,
			      DWORD dwTimeOut_i,
			      const MeasVrsn_t constCounterVersion_i,
			      MeasCId_t counterId,
			      void  *pHashDictionary_i ) 
  { return 0; }

  void vSaveMeasEntyPtr ( MeasureEntity *pMeasureEntity,
			  MeasCId_t      CounterId,
			  MeasureCounter * pCounter,
			  DWORD                   dwCurrentMeasureWareStatus
			  )
  { return; }

  DWORD dwRegisterNow ( DWORD dwCurrentMeasureWareStatus )
  { return 0; }

  DWORD dwRegisterMyself ( void ) 
  { return 0; }

#if 0
  //
  // Saved Variables for Dynamic Enable and Disabling
  //
  MeasureEntity           *m_pSavedMeasureEntity_;
  DWORD                           m_dwMeasureWareEnableStatus_;
  MeasCId_t                       m_CounterId_;
  MeasureCounter          *m_pMeasureCounter_;



  BOOL m_bFastForeignCounter_;

  LPSTR                           m_pszObjectName_;
  LPSTR                           m_pszInstanceName_;
  DWORD                           m_dwTimeOut_;
  const MeasVrsn_t        m_constCounterVersion_;
  void                *m_pHashDictionary_;
#endif

};

MeasureCounter::~MeasureCounter()
{}

DWORD       MeasureCounter::enable  ( LPCWSTR   pInstanceName,
					      MeasCId_t counterId,
					      LPVOID    counterDataAddress
					      ) 
  { return 0; }



class DLLEXPORT CFastForeignCounter
{
public:
  CFastForeignCounter( int nMaxEntries ) 
  {}
  ~CFastForeignCounter() 
  {}

  DWORD GetFFCPointer ( LPCSTR pszObjectName_i,
			LPCSTR pszInstanceName_i,
    void  **ppHashElement_o ) 
  { return 0; }

  DWORD dwInstantiate ( LPCSTR pszObjectName_i,
			LPCSTR pszInstanceName_i,
			DWORD dwTimeOut_i,
			const MeasVrsn_t constCounterVersion_i,
			MeasCId_t CounterId_i,
			MeasureEntity **ppMeasureEntity_o )
  { return 0; }


  DWORD dwRemoveLeastUsedEntry ( void ) 
  { return 0; }

  //
  // Following are the functions to be used by NSK Lite for implementing FFC.
  // Instantiate the measure entity by calling the dwInstantiate function
  // from the FFC class.
  //
  // Once the entity is instantiated, a measure counter needs to be reigstered
  // before it can be used.
  //
  DWORD dwInstantiate ( LPCSTR pszObjectName_i,
			LPCSTR pszInstanceName_i,
			DWORD dwTimeOut_i,
			const MeasVrsn_t constCounterVersion_i,
			MeasureEntity **ppMeasureEntity_o )
  { return 0; }


  DWORD dwDeleteInstance ( LPCSTR pszObjectName_i,
			   LPCSTR pszInstanceName_i )
  { return 0; }

  DWORD dwRegisterCounter ( LPCSTR pszObjectName_i,
			    LPCSTR pszInstanceName_i,
			    DWORD dwTimeOut_i,
			    const MeasVrsn_t constCounterVersion_i,
			    MeasCId_t CounterId,
			    MeasureCounter *pMeasCounter )
  { return 0; }

  void vTraceList ( void )
  { return; }



public:
#if 0
  int             m_nMaxEntries_;
  time_t          m_lTimeFastCounterCreated_;
  void        *pvHashDictionary;
  BOOL        m_bMeasureWareDisabled;
  FILE            *m_fpTraceFile;
  DWORD       m_dwTraceMeasureWare;
#endif

};




class DLLEXPORT ProcessSafeComplexMeasureCounter : public MeasureCounter
{
public:
  ProcessSafeComplexMeasureCounter()
  {  }

  ~ProcessSafeComplexMeasureCounter();

#if 0
private:
  ProcessSafeComplexMeasureCounter( const ProcessSafeComplexMeasureCounter &PSC );   // No copying.

protected:
  BOOL
  initialized_;

  MeasureCriticalSection
  CS_;
#endif

};

ProcessSafeComplexMeasureCounter::~ProcessSafeComplexMeasureCounter()
{}


class DLLEXPORT ProcessSafeBusyCounter : public ProcessSafeComplexMeasureCounter
{
public:
  virtual DWORD       dataSize ( void ) const
  { return 0; }

  virtual MeasCType_t type     ( void ) const {return( MEASURE_TDMBCTYPE );}

  void inline         decrement( void )
  { return; }

  void inline         increment( void )
  { return; }

  DWORD               enable   ( LPCWSTR   pInstanceName,
				 MeasCId_t counterId,
				 LPVOID    counterDataAddress
				 )
  { return 0; }

};

class DLLEXPORT ProcessSafeQueueCounter : public ProcessSafeComplexMeasureCounter
{
public:
  virtual DWORD       dataSize ( void ) const
  {
    return 0;
  }

  virtual MeasCType_t type     ( void ) const {return( MEASURE_TDMQCTYPE );}

  void inline         decrement( void )
  { return; }

  void inline         increment( void )
  { return; }

  DWORD enable                 ( LPCWSTR   pInstanceName,
				 MeasCId_t counterId,
				 LPVOID    counterDataAddress
				 )
  { return 0; }

};

class DLLEXPORT ProcessSafeTimeCounter : public ProcessSafeComplexMeasureCounter
{
public:
  virtual DWORD       dataSize( void ) const
  {
    return 0;
  }

  virtual MeasCType_t type    ( void ) const {return( MEASURE_TDMTCTYPE );}

  void inline         add     ( _int64 addend )
  { return; }

  void inline         set     ( _int64 value )
  { return; }

  void inline         subtract( _int64 subtrahend )
  { return; }

  DWORD enable                ( LPCWSTR   pInstanceName,
				MeasCId_t counterId,
				LPVOID    counterDataAddress
				)
  { return 0; }

};

ProcessSafeTimeCounter _stub_stc;

  
ProcessSafeBusyCounter _stub_sbc;

MeasureEntity __stub_me(MEASURE_NULLCTYPE);
