#ifndef EXP_IEEE_H
#define EXP_IEEE_H
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


// Encapsulate IEEE floating point overflow checks

#ifdef __cplusplus
extern "C" {
#endif
double MathReal64Add(double x, double y, short * ov);
double MathReal64Sub(double x, double y, short * ov);
double MathReal64Mul(double x, double y, short * ov);
double MathReal64Div(double x, double y, short * ov);
float  MathConvReal64ToReal32(double x, short * ov);
double MathConvReal64ToReal64(double x, short * ov);

#ifdef __cplusplus
}
#endif

void MathEvalException(double result, unsigned long exc, short * ov); // so exp_eval.cpp can get it

#endif
