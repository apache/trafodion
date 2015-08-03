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
#ifndef EX_IO_CONTROL_H
#define EX_IO_CONTROL_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ex_queue.h"

class ControlInfo 
{
public:
NA_EIDPROC
  ControlInfo(): ciFlags_(HAS_NOTHING) {};
NA_EIDPROC
  ~ControlInfo(){};
  
NA_EIDPROC
  inline up_state &getUpState();
NA_EIDPROC
  inline down_state &getDownState();
NA_EIDPROC 
  inline ULng32 getBufferSequenceNumber();
NA_EIDPROC 
  inline void setBufferSequenceNumber(ULng32 snum);
NA_EIDPROC
  inline NABoolean getIsDiagsAreaPresent() const;
NA_EIDPROC
  inline void setIsDiagsAreaPresent(NABoolean);
NA_EIDPROC
  inline NABoolean getIsDiagsAreaUnpacked() const;
NA_EIDPROC
  inline void setIsDiagsAreaUnpacked(NABoolean);
// an external diags area is one that gets sent outside of the sql_buffer
NA_EIDPROC
  inline NABoolean getIsExtDiagsAreaPresent() const;
NA_EIDPROC
  inline void setIsExtDiagsAreaPresent(NABoolean);
NA_EIDPROC
  inline NABoolean getIsDataRowPresent() const;
NA_EIDPROC
  inline void setIsDataRowPresent(NABoolean);
NA_EIDPROC
  inline NABoolean getIsStatsAreaPresent() const;
NA_EIDPROC
  inline void setIsStatsAreaPresent(NABoolean);
// an external stats area is one that gets sent outside of the sql_buffer
NA_EIDPROC
  inline NABoolean getIsExtStatsAreaPresent() const;
NA_EIDPROC
  inline void setIsExtStatsAreaPresent(NABoolean);
NA_EIDPROC
  inline void resetFlags();
NA_EIDPROC
  inline void copyFlags(const ControlInfo &other);

private:
  union   // anonymous
    {
      up_state    upState_;
      down_state  downState_;
    };
   enum ciFlagsBits {HAS_NOTHING=0x0,
		     HAS_DATA=0x1,
		     HAS_DIAGSAREA=0x02,
                     HAS_STATSAREA=0x04,
		     HAS_EXT_DIAGSAREA=0x08,
		     HAS_EXT_STATSAREA=0x10,
                     IS_DIAGSAREA_UNPACKED=0x20};
   ULng32 ciFlags_;
};


//
// In-line methods.
//

inline up_state &ControlInfo::getUpState()
{
  return upState_;
};

inline down_state &ControlInfo::getDownState()
{
  return downState_;
};

inline ULng32 ControlInfo::getBufferSequenceNumber()
{
  return downState_.requestValue;
}

inline void ControlInfo::setBufferSequenceNumber(ULng32 snum)
{
  downState_.requestValue = (Lng32) snum;
}

inline NABoolean ControlInfo::getIsDiagsAreaPresent() const
{
#pragma nowarn(1506)   // warning elimination 
  return (ciFlags_ & HAS_DIAGSAREA);
#pragma warn(1506)  // warning elimination 
}

inline void ControlInfo::setIsDiagsAreaPresent(NABoolean val)
{
  if (val)
    ciFlags_ |= HAS_DIAGSAREA;
  else
    ciFlags_ &= ~HAS_DIAGSAREA;
}

inline NABoolean ControlInfo::getIsDiagsAreaUnpacked() const
{
  return ((ciFlags_ & IS_DIAGSAREA_UNPACKED) != 0);
}

inline void ControlInfo::setIsDiagsAreaUnpacked(NABoolean val)
{
  if (val)
    ciFlags_ |= IS_DIAGSAREA_UNPACKED;
  else
    ciFlags_ &= ~IS_DIAGSAREA_UNPACKED;
}

inline NABoolean ControlInfo::getIsExtDiagsAreaPresent() const
{
#pragma nowarn(1506)   // warning elimination 
  return (ciFlags_ & HAS_EXT_DIAGSAREA);
#pragma warn(1506)  // warning elimination 
}

inline void ControlInfo::setIsExtDiagsAreaPresent(NABoolean val)
{
  if (val)
    ciFlags_ |= HAS_EXT_DIAGSAREA;
  else
    ciFlags_ &= ~HAS_EXT_DIAGSAREA;
}

inline NABoolean ControlInfo::getIsDataRowPresent() const
{
#pragma nowarn(1506)   // warning elimination 
  return (ciFlags_ & HAS_DATA);
#pragma warn(1506)  // warning elimination 
}

inline void ControlInfo::setIsDataRowPresent(NABoolean val)
{
  if (val)
    ciFlags_ |= HAS_DATA;
  else
    ciFlags_ &= ~HAS_DATA;
}

inline NABoolean ControlInfo::getIsStatsAreaPresent() const
{
#pragma nowarn(1506)   // warning elimination 
  return (ciFlags_ & HAS_STATSAREA);
#pragma warn(1506)  // warning elimination 
}

inline void ControlInfo::setIsStatsAreaPresent(NABoolean val)
{
  if (val)
    ciFlags_ |= HAS_STATSAREA;
  else
    ciFlags_ &= ~HAS_STATSAREA;
}

inline NABoolean ControlInfo::getIsExtStatsAreaPresent() const
{
#pragma nowarn(1506)   // warning elimination 
  return (ciFlags_ & HAS_EXT_STATSAREA);
#pragma warn(1506)  // warning elimination 
}

inline void ControlInfo::setIsExtStatsAreaPresent(NABoolean val)
{
  if (val)
    ciFlags_ |= HAS_EXT_STATSAREA;
  else
    ciFlags_ &= ~HAS_EXT_STATSAREA;
}

inline void ControlInfo::resetFlags()
{
  ciFlags_ = 0;
}

inline void ControlInfo::copyFlags(const ControlInfo &other)
{
  ciFlags_ = other.ciFlags_;
}

#endif




