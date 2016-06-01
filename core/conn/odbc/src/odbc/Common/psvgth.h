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
/* This header file must be kept in sync with the SUTVER_GET_ source */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_tal _extensible short SUTVER_GET_(
    short _far *   /* OUT OPTIONAL ERROR_DETAIL */
   ,short _far *   /* OUT OPTIONAL SVGT_VER     */
   ,short _far *   /* OUT OPTIONAL TOSV         */
   ,short _far *   /* OUT OPTIONAL MINTOSV      */
   ,short _far *   /* OUT OPTIONAL P_C_VER      */
   ,char _far *    /* OUT OPTIONAL SUTVER       */
   ,short          /* IN OPTIONAL SUTVER_MAXLEN */
   ,short _far *   /* OUT OPTIONAL SUTVER_LEN   */
   ,char _far *    /* OUT OPTIONAL RLSEID       */
   ,short          /* IN OPTIONAL RLSEID_MAXLEN */
   ,short _far *   /* OUT OPTIONAL RLSEID_LEN   */
   );

#define SUTVER_GET_VER_13MAR2001 100 /* Corresponds to version 1.00 */
#define SUTVER_GET_VERSION       SUTVER_GET_VER_13MAR2001

#ifdef __cplusplus
}
#endif /* __cplusplus */
