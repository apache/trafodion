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

#ifndef _QREXPRELEMENT_H_
#define _QREXPRELEMENT_H_

/**
 * \file
 * Contains the \c ExprElement enum, which is used in several optimizer header
 * files for the \c ItemExpr hierarchy. A change to those headers causes a long
 * compile, so this enum is placed in its own file instead of QRDescriptor.h,
 * thus avoiding a dependency on that file and a big recompilation whenever
 * it changes.
 */

// This is an extension of the QR namespace, some elements of which are also
// defined in QRMessage.h.
namespace QR
{
  /**
   * Enumeration representing element types that can appear as subelements in
   * an &lt;Expr&gt; element. This is used in a virtual function in \c ItemExpr
   * to indicate what element we should generate when a node of an item
   * expression is visited.
   */
  enum ExprElement 
    { 
      QRNoElem,
      QRFunctionElem,
      QRFunctionWithParameters,
      QRBinaryOperElem,
      QRUnaryOperElem,
      QRColumnElem,
      QRScalarValueElem
    };
};  // namespace QR

#endif  /* _QREXPRELEMENT_H_ */
