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
#ifndef _JAVA_SPLITTER_H_
#define _JAVA_SPLITTER_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         JavaSplitter.h
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
#include <Platform.h>

class ComDiagsArea;

class JavaSplitter{
public:
  static jfloat CallStaticFloatMethod(JNIEnv *env, jclass clazz,
				      jmethodID methodID, ...);
  static jdouble CallStaticDoubleMethod(JNIEnv *env, jclass clazz,
					jmethodID methodID, ...);
  static jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz,
				       jmethodID methodID, jvalue *args);
  static jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz,
					 jmethodID methodID, jvalue *args);
  static jfloat CallStaticFloatMethodV(JNIEnv *env, jclass clazz,
				       jmethodID methodID, va_list args);
  static jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass clazz,
					 jmethodID methodID, va_list args);
  
  static jfloat CallFloatMethod(JNIEnv *env, jobject obj,
				jmethodID methodID, ...);
  static jdouble CallDoubleMethod(JNIEnv *env, jobject obj,
				  jmethodID methodID, ...);
  static jfloat CallFloatMethodA(JNIEnv *env, jobject obj,
				 jmethodID methodID, jvalue *args);
  static jdouble CallDoubleMethodA(JNIEnv *env, jobject obj,
				   jmethodID methodID, jvalue *args);
  static jfloat CallFloatMethodV(JNIEnv *env, jobject obj,
				 jmethodID methodID, va_list args);
  static jdouble CallDoubleMethodV(JNIEnv *env, jobject obj,
				   jmethodID methodID, va_list args);

  static jfloat CallNonvirtualFloatMethod(JNIEnv *env, jobject obj,
					  jclass clazz,
					  jmethodID methodID, ...);
  static jdouble CallNonvirtualDoubleMethod(JNIEnv *env, jobject obj,
					    jclass clazz,
					    jmethodID methodID, ...);
  static jfloat CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj,
					   jclass clazz,
					   jmethodID methodID, jvalue *args);
  static jdouble CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj,
					     jclass clazz,
					     jmethodID methodID, jvalue *args);
  static jfloat CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj,
					   jclass clazz,
					   jmethodID methodID, va_list args);
  static jdouble CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj,
					     jclass clazz,
					     jmethodID methodID, va_list args);

  static Lng32 setupVersion(JNIEnv *env, ComDiagsArea *diags);
};

class JavaVirtual{
public:
  virtual jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz,
					jmethodID methodID, jvalue *args) =0;
  virtual jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz,
					  jmethodID methodID, jvalue *args) =0;
  virtual jfloat CallStaticFloatMethodV(JNIEnv *env, jclass clazz,
					jmethodID methodID, va_list args) =0;
  virtual jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass clazz,
					 jmethodID methodID, va_list args) =0;
  
  virtual jfloat CallFloatMethodA(JNIEnv *env, jobject obj,
				 jmethodID methodID, jvalue *args) =0;
  virtual jdouble CallDoubleMethodA(JNIEnv *env, jobject obj,
				   jmethodID methodID, jvalue *args) =0;
  virtual jfloat CallFloatMethodV(JNIEnv *env, jobject obj,
				 jmethodID methodID, va_list args) =0;
  virtual jdouble CallDoubleMethodV(JNIEnv *env, jobject obj,
				   jmethodID methodID, va_list args) =0;

  virtual jfloat CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj,
					    jclass clazz, jmethodID methodID,
					    jvalue *args) =0;
  virtual jdouble CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj,
					      jclass clazz, jmethodID methodID,
					      jvalue *args) =0;
  virtual jfloat CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj,
					    jclass clazz, jmethodID methodID,
					    va_list args) =0;
  virtual jdouble CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj,
					      jclass clazz, jmethodID methodID,
					      va_list args) =0;
};

class JavaNormal : public JavaVirtual{
public:
  virtual jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz,
					jmethodID methodID, jvalue *args);
  virtual jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz,
					  jmethodID methodID, jvalue *args);
  virtual jfloat CallStaticFloatMethodV(JNIEnv *env, jclass clazz,
					jmethodID methodID, va_list args);
  virtual jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass clazz,
					 jmethodID methodID, va_list args);
  
  virtual jfloat CallFloatMethodA(JNIEnv *env, jobject obj,
				 jmethodID methodID, jvalue *args);
  virtual jdouble CallDoubleMethodA(JNIEnv *env, jobject obj,
				   jmethodID methodID, jvalue *args);
  virtual jfloat CallFloatMethodV(JNIEnv *env, jobject obj,
				 jmethodID methodID, va_list args);
  virtual jdouble CallDoubleMethodV(JNIEnv *env, jobject obj,
				   jmethodID methodID, va_list args);

  virtual jfloat CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj,
					    jclass clazz, jmethodID methodID,
					    jvalue *args);
  virtual jdouble CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj,
					      jclass clazz, jmethodID methodID,
					      jvalue *args);
  virtual jfloat CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj,
					    jclass clazz, jmethodID methodID,
					    va_list args);
  virtual jdouble CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj,
					      jclass clazz, jmethodID methodID,
					      va_list args);
};


#endif
