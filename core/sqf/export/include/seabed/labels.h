//------------------------------------------------------------------
//
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

//
// Labels module
//
#ifndef __SB_LABELS_H_
#define __SB_LABELS_H_

#include "int/exp.h"

typedef struct SB_Label_Map {
    int          iv_low_inx;  // low index
    int          iv_max;      // max label
    const char  *ip_unknown;  // unknown label
    const char **ipp_labels;  // list of labels [zero-based, monotonically increasing]
} SB_Label_Map;

#define SB_LABEL_END NULL

//
// Purpose:
//   Return label from map given a value.
//
// Usage example:
//   #include "int/assert.h" // need for SB_util_static_assert
//
//   // define array of labels with SB_LABEL_END as last entry
//   static const char *fserror_labels[] = {
//       "FEOK",
//       "FEEOF",
//       "FEINVALOP",
//       "FEPARTFAIL",
//       "FEKEYFAIL",
//       "FESEQFAIL",
//       "FESYSMESS",
//       SB_LABEL_END
//   };
//
//   enum {
//       LABEL_LIMIT_FSERROR_LO = FEOK,
//       LABEL_LIMIT_FSERROR_HI = FESYSMESS
//   };
//   SB_Label_Map fserror_label_map = {
//       LABEL_LIMIT_FSERROR_LO, // low-limit
//       LABEL_LIMIT_FSERROR_HI, // high-limit
//       "<unknown>",            // unknown label
//       fserror_labels          // labels
//   };
//
//   #define LABEL_CHK(name, low, high) 
//   SB_util_static_assert((sizeof(name)/sizeof(const char *)) == (high - low + 2));
//
//   // Add a 'dummy' function to call LABEL_CHK
//   // this will make sure that the size of the label array
//   // is the correct size.
//   // Note that if there is a mismatch, the compiler will emit an error.
//   void label_map_init() {
//       LABEL_CHK(fserror_labels,
//                 LABEL_LIMIT_FSERROR_TYPE_LO,
//                 LABEL_LIMIT_FSERROR_TYPE_HI)
//   }
//
//   int fserror = 0;
//   const char *fserror_label = SB_get_label(&fserror_label_map, 0);
//   printf("fserror=%d(%s)\n", fserror, fserror_label);
//   // should print 'fserror=0(FEOK)'
//
// Usage notes:
//   Label access is very fast.
//   Label access is available from gdb scripts.
//
SB_Export const char *SB_get_label(SB_Label_Map *map, int value);

//
// Purpose:
//   Return label from array-of-maps given a value.
//
// Usage example:
//   // Contains an array-of-maps
//   SB_Label_Map fserror_label_maps[] = {
//       &fserror_label_map,
//       &fserror1_label_map,
//       NULL
//   };
// 
//   const char *fserror_label = SB_get_label_maps(&fserror_label_maps, 0);
//
// Usage notes:
//   The array-of-maps will be searched until either the value is
//   found or the end-of-array is found.
//
//   This usage allows for specifying sparse tables.
//   Note that the maps are searched in order.
//
SB_Export const char *SB_get_label_maps(SB_Label_Map **map, int value);

#endif // !__SB_LABELS_H_
