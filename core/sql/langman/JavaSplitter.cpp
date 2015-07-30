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
 * File:         JavaSplitter.cpp
 * Description:  Splitter functions to support different code path for
 *               NSJ 3.0 IEEE float and NSJ2.0 Tandem float
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
#include <jni.h>
#include "JavaSplitter.h"
#include "Platform.h"
#include <iostream>
#include <string.h>

#include "ComDiags.h"
#include "LmError.h"


static JavaNormal normalJava;
static JavaVirtual *splitter = &normalJava;

// class JavaSplitter function definitions

Lng32 JavaSplitter::setupVersion(JNIEnv *jni, ComDiagsArea *diags)
{
  return 0;
}

jfloat JavaSplitter::CallStaticFloatMethod(JNIEnv *env, jclass clazz,
					   jmethodID methodID, ...)
{
  va_list args;
  va_start(args, methodID);
  return splitter->CallStaticFloatMethodV(env, clazz, methodID, args);
}

jdouble JavaSplitter::CallStaticDoubleMethod(JNIEnv *env, jclass clazz,
					     jmethodID methodID, ...)
{
  va_list args;
  va_start(args, methodID);
  return splitter->CallStaticDoubleMethodV(env, clazz, methodID, args);
}

jfloat JavaSplitter::CallStaticFloatMethodA(JNIEnv *env, jclass clazz,
					    jmethodID methodID, jvalue *args)
{
  return splitter->CallStaticFloatMethodA(env, clazz, methodID, args);
}

jdouble JavaSplitter::CallStaticDoubleMethodA(JNIEnv *env, jclass clazz,
					      jmethodID methodID, jvalue *args)
{
  return splitter->CallStaticDoubleMethodA(env, clazz, methodID, args);
}

jfloat JavaSplitter::CallStaticFloatMethodV(JNIEnv *env, jclass clazz,
					    jmethodID methodID, va_list args)
{
  return splitter->CallStaticFloatMethodV(env, clazz, methodID, args);
}

jdouble JavaSplitter::CallStaticDoubleMethodV(JNIEnv *env, jclass clazz,
					      jmethodID methodID, va_list args)
{
  return splitter->CallStaticDoubleMethodV(env, clazz, methodID, args);
}
  
jfloat JavaSplitter::CallFloatMethod(JNIEnv *env, jobject obj,
				     jmethodID methodID, ...)
{
  va_list args;
  va_start(args, methodID);
  return splitter->CallFloatMethodV(env, obj, methodID, args);
}

jdouble JavaSplitter::CallDoubleMethod(JNIEnv *env, jobject obj,
				       jmethodID methodID, ...)
{
  va_list args;
  va_start(args, methodID);
  return splitter->CallDoubleMethodV(env, obj, methodID, args);
}

jfloat JavaSplitter::CallFloatMethodA(JNIEnv *env, jobject obj,
				      jmethodID methodID, jvalue *args)
{
  return splitter->CallFloatMethodA(env, obj, methodID, args);
}

jdouble JavaSplitter::CallDoubleMethodA(JNIEnv *env, jobject obj,
					jmethodID methodID, jvalue *args)
{
  return splitter->CallDoubleMethodA(env, obj, methodID, args);
}

jfloat JavaSplitter::CallFloatMethodV(JNIEnv *env, jobject obj,
				      jmethodID methodID, va_list args)
{
  return splitter->CallFloatMethodV(env, obj, methodID, args);
}

jdouble JavaSplitter::CallDoubleMethodV(JNIEnv *env, jobject obj,
					jmethodID methodID, va_list args)
{
  return splitter->CallDoubleMethodV(env, obj, methodID, args);
}

jfloat JavaSplitter::CallNonvirtualFloatMethod(JNIEnv *env, jobject obj,
					       jclass clazz,
					       jmethodID methodID, ...)
{
  va_list args;
  va_start(args, methodID);
  return splitter->CallNonvirtualFloatMethodV(env, obj, clazz, methodID, args);
}

jdouble JavaSplitter::CallNonvirtualDoubleMethod(JNIEnv *env, jobject obj,
						 jclass clazz,
						 jmethodID methodID, ...)
{
  va_list args;
  va_start(args, methodID);
  return splitter->CallNonvirtualDoubleMethodV(env, obj, clazz, methodID,
					       args);
}

jfloat JavaSplitter::CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj,
						jclass clazz,
						jmethodID methodID,
						jvalue *args)
{
  return splitter->CallNonvirtualFloatMethodA(env, obj, clazz, methodID,
					      args);
}

jdouble JavaSplitter::CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj,
						  jclass clazz,
						  jmethodID methodID,
						  jvalue *args)
{
  return splitter->CallNonvirtualDoubleMethodA(env, obj, clazz, methodID,
					       args);
}

jfloat JavaSplitter::CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj,
						jclass clazz,
						jmethodID methodID,
						va_list args)
{
  return splitter->CallNonvirtualFloatMethodV(env, obj, clazz, methodID,
					      args);
}

jdouble JavaSplitter::CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj,
						  jclass clazz,
						  jmethodID methodID,
						  va_list args)
{
  return splitter->CallNonvirtualDoubleMethodV(env, obj, clazz, methodID,
					       args);
}



// class JavaNormal function definitions

jfloat JavaNormal::CallStaticFloatMethodA(JNIEnv *env, jclass clazz,
					  jmethodID methodID, jvalue *args)
{
  return env->CallStaticFloatMethodA(clazz, methodID, args);
}

jdouble JavaNormal::CallStaticDoubleMethodA(JNIEnv *env, jclass clazz,
					    jmethodID methodID, jvalue *args)
{
  return env->CallStaticDoubleMethodA(clazz, methodID, args);
}

jfloat JavaNormal::CallStaticFloatMethodV(JNIEnv *env, jclass clazz,
					  jmethodID methodID, va_list args)
{
  return env->CallStaticFloatMethodV(clazz, methodID, args);
}

jdouble JavaNormal::CallStaticDoubleMethodV(JNIEnv *env, jclass clazz,
					    jmethodID methodID, va_list args)
{
  return env->CallStaticDoubleMethodV(clazz, methodID, args);
}
  
jfloat JavaNormal::CallFloatMethodA(JNIEnv *env, jobject obj,
				    jmethodID methodID, jvalue *args)
{
  return env->CallFloatMethodA(obj, methodID, args);
}

jdouble JavaNormal::CallDoubleMethodA(JNIEnv *env, jobject obj,
				      jmethodID methodID, jvalue *args)
{
  return env->CallDoubleMethodA(obj, methodID, args);
}

jfloat JavaNormal::CallFloatMethodV(JNIEnv *env, jobject obj,
				    jmethodID methodID, va_list args)
{
  return env->CallFloatMethodV(obj, methodID, args);
}

jdouble JavaNormal::CallDoubleMethodV(JNIEnv *env, jobject obj,
				      jmethodID methodID, va_list args)
{
  return env->CallDoubleMethodV(obj, methodID, args);
}

jfloat JavaNormal::CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj,
					      jclass clazz, jmethodID methodID,
					      jvalue *args)
{
  return env->CallNonvirtualFloatMethodA(obj, clazz, methodID, args);
}

jdouble JavaNormal::CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj,
						jclass clazz,
						jmethodID methodID,
						jvalue *args)
{
  return env->CallNonvirtualDoubleMethodA(obj, clazz, methodID, args);
}

jfloat JavaNormal::CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj,
					      jclass clazz, jmethodID methodID,
					      va_list args)
{
  return env->CallNonvirtualFloatMethodV(obj, clazz, methodID, args);
}

jdouble JavaNormal::CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj,
						jclass clazz,
						jmethodID methodID,
						va_list args)
{
  return env->CallNonvirtualDoubleMethodV(obj, clazz, methodID, args);
}


// class JavaI3EAdaptor function definitions
