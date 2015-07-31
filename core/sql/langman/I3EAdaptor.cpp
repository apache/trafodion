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
 * File:         I3EAdaptor.cpp
 * Description:  Adaptor functions to handle NSJ 3.0 IEEE float
 *               
 *               
 * Created:      8/15/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// This Adaptor module has to be compiled with -WIEEE_float flag in c89.
// The current NT-to-NSK c89 we are using does not support this flag.
// Therefore, this module is compiled on NSK and the produced object file
// I3EAdaptor.o is checked into w:/langman
#include <jni.h>
#include "IntType.h"
#include "I3EAdaptor.h"

Int32 I3EAdaptor::CallStaticFloatMethodA(JNIEnv *env, jclass clazz,
				       jmethodID methodID, jvalue *args)
{
  jfloat result = env->CallStaticFloatMethodA(clazz, methodID, args);
  return *((Int32 *)&result);
}

Int64 I3EAdaptor::CallStaticDoubleMethodA(JNIEnv *env, jclass clazz,
					 jmethodID methodID, jvalue *args)
{
  jdouble result = env->CallStaticDoubleMethodA(clazz, methodID, args);
  return *((Int64 *)&result);
}

Int32 I3EAdaptor::CallStaticFloatMethodV(JNIEnv *env, jclass clazz,
				       jmethodID methodID, va_list args)
{
  jfloat result = env->CallStaticFloatMethodV(clazz, methodID, args);
  return *((Int32 *)&result);
}

Int64 I3EAdaptor::CallStaticDoubleMethodV(JNIEnv *env, jclass clazz,
					 jmethodID methodID, va_list args)
{
  jdouble result = env->CallStaticDoubleMethodV(clazz, methodID, args);
  return *((Int64 *)&result);
}
  
Int32 I3EAdaptor::CallFloatMethodA(JNIEnv *env, jobject obj,
				 jmethodID methodID, jvalue *args)
{
  jfloat result = env->CallFloatMethodA(obj, methodID, args);
  return *((Int32 *)&result);
}

Int64 I3EAdaptor::CallDoubleMethodA(JNIEnv *env, jobject obj,
				   jmethodID methodID, jvalue *args)
{
  jdouble result = env->CallDoubleMethodA(obj, methodID, args);
  return *((Int64 *)&result);
}

Int32 I3EAdaptor::CallFloatMethodV(JNIEnv *env, jobject obj,
				 jmethodID methodID, va_list args)
{
  jfloat result = env->CallFloatMethodV(obj, methodID, args);
  return *((Int32 *)&result);
}

Int64 I3EAdaptor::CallDoubleMethodV(JNIEnv *env, jobject obj,
				   jmethodID methodID, va_list args)
{
  jdouble result = env->CallDoubleMethodV(obj, methodID, args);
  return *((Int64 *)&result);
}

Int32 I3EAdaptor::CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj,
					   jclass clazz, jmethodID methodID,
					   jvalue *args)
{
  jfloat result = env->CallNonvirtualFloatMethodA(obj, clazz, methodID, args);
  return *((Int32 *)&result);
}

Int64 I3EAdaptor::CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj,
					     jclass clazz, jmethodID methodID,
					     jvalue *args)
{
  jdouble result =
    env->CallNonvirtualDoubleMethodA(obj, clazz, methodID, args);
  return *((Int64 *)&result);
}

Int32 I3EAdaptor::CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj,
					   jclass clazz, jmethodID methodID,
					   va_list args)
{
  jfloat result = env->CallNonvirtualFloatMethodV(obj, clazz, methodID, args);
  return *((Int32 *)&result);
}

Int64 I3EAdaptor::CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj,
					     jclass clazz, jmethodID methodID,
					     va_list args)
{
  jdouble result =
    env->CallNonvirtualDoubleMethodV(obj, clazz, methodID, args);
  return *((Int64 *)&result);
}
