#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "jni.h"

JavaVM* gp_jvm  = NULL;
__thread JNIEnv* _tlp_jenv = 0;
__thread bool  _tlv_jenv_set = false;

struct TestJavaMethodInit {
  std::string   jm_name;       // The method name.
  std::string   jm_signature;  // The method signature.
  jmethodID     methodID;      // The JNI methodID
};

enum JAVA_METHODS {
  JM_ISHBASEAVAILABLE=0,
  JM_LAST
};

TestJavaMethodInit CheckHBJavaMethods_[JM_LAST];

typedef enum {
  TestJOI_OK = 0
 ,TestJOI_ERROR_CHECK_JVM           // Cannot check existing JVMs
 ,TestJOI_ERROR_JVM_VERSION         // Attaching to JVM of wrong version.
 ,TestJOI_ERROR_ATTACH_JVM          // Cannot attach to an existing JVM
 ,TestJOI_ERROR_CREATE_JVM          // Cannot create JVM
 ,TestJOI_ERROR_FINDCLASS           // JNI FindClass() failed
 ,TestJOI_ERROR_GETMETHOD           // JNI GetMethodID() failed
 ,TestJOI_ERROR_NEWOBJ              // JNI NewObject() failed
 ,TestJOI_LAST
} TestJOI_RetCode;

char* test_buildClassPath()
{
  char* classPath = getenv("CLASSPATH");
  int   size = strlen(classPath) + 4096;
  char* classPathBuffer = (char*)malloc(size);
  
  strcpy(classPathBuffer, "-Djava.class.path=");
  strcat(classPathBuffer, classPath);
  return classPathBuffer;
}

int test_createJVM()
{
  JavaVMInitArgs jvm_args;
  JavaVMOption jvm_options[4];

  char* classPathArg = test_buildClassPath();
  int numJVMOptions = 0;
  jvm_options[numJVMOptions].optionString = classPathArg;
  numJVMOptions++;

  char maxHeapOptions[64];
  bool passMaxHeapToJVM = true;
  int maxHeapEnvvarMB = 4096;
  jvm_options[numJVMOptions].optionString = (char *) "-Xmx1024m";
  numJVMOptions++;

  jvm_args.version            = JNI_VERSION_1_6;
  jvm_args.options            = jvm_options;
  jvm_args.nOptions           = numJVMOptions;
  jvm_args.ignoreUnrecognized = 1;

  int ret = JNI_CreateJavaVM(&gp_jvm, (void**)&_tlp_jenv, &jvm_args);
  free(classPathArg);
  return ret;
}

short initJVM()
{
  jint result;

  if ((_tlp_jenv != 0) && (_tlv_jenv_set)) {
    return TestJOI_OK;
  }

  if (gp_jvm == NULL)
  {
    jsize jvm_count = 0;
    // Is there an existing JVM?
    result = JNI_GetCreatedJavaVMs (&gp_jvm, 1, &jvm_count);
    if (result != JNI_OK)
      return TestJOI_ERROR_CHECK_JVM;
      
    if (jvm_count == 0)
    {
      test_createJVM();
      return 0;
    }
  }

  // We found a JVM, can we use it?
  result = gp_jvm->GetEnv((void**) &_tlp_jenv, JNI_VERSION_1_6);
  switch (result)
  {
    case JNI_OK:
      break;
    
    case JNI_EDETACHED:
      printf("initJVM: Detached, Try 2 attach\n");
      result = gp_jvm->AttachCurrentThread((void**) &_tlp_jenv, NULL);   
      if (result != JNI_OK)
	{
	  printf("initJVM: Error in attaching\n");
	  return TestJOI_ERROR_ATTACH_JVM;
	}
      
      break;
       
    case JNI_EVERSION:
      return TestJOI_ERROR_JVM_VERSION;
      break;
      
    default:
      return TestJOI_ERROR_ATTACH_JVM;
      break;
  }

  _tlv_jenv_set = true;

  return TestJOI_OK;
}

bool isHBaseAvailable()
{
  static bool sv_class_initialized = false;
  static bool sv_methods_initialized = false;
  static jclass sv_jclass_class;

  char lv_class_name[] = "CheckHBase";
  jthrowable jexception;

  if (initJVM() != TestJOI_OK) {
    exit(1);
  }
  
  if (! sv_class_initialized) {

    sv_jclass_class = _tlp_jenv->FindClass(lv_class_name); 

    if (_tlp_jenv->ExceptionCheck()) {
      _tlp_jenv->ExceptionDescribe();
      _tlp_jenv->ExceptionClear();
      exit(1);
    }
  
    if (sv_jclass_class == 0) {
      exit(1);
    }

    sv_class_initialized = true;

  }

  if (! sv_methods_initialized) {

    CheckHBJavaMethods_[JM_ISHBASEAVAILABLE].jm_name      = "isHBaseAvailable";
    CheckHBJavaMethods_[JM_ISHBASEAVAILABLE].jm_signature = "()Z";
    CheckHBJavaMethods_[JM_ISHBASEAVAILABLE].methodID     = 
      _tlp_jenv->GetStaticMethodID(sv_jclass_class,
				   CheckHBJavaMethods_[JM_ISHBASEAVAILABLE].jm_name.data(),
				   CheckHBJavaMethods_[JM_ISHBASEAVAILABLE].jm_signature.data());

    if (CheckHBJavaMethods_[JM_ISHBASEAVAILABLE].methodID == 0 || _tlp_jenv->ExceptionCheck()) { 
      _tlp_jenv->ExceptionDescribe();
      _tlp_jenv->ExceptionClear();
      exit(1);
    }      

    sv_methods_initialized = true;
  }

  jlong j_tid = 100;         ;
  jboolean jv_ret = _tlp_jenv->CallStaticBooleanMethod(sv_jclass_class,
						       CheckHBJavaMethods_[JM_ISHBASEAVAILABLE].methodID);

  jexception = _tlp_jenv->ExceptionOccurred();
  if(jexception) {
    _tlp_jenv->ExceptionDescribe();
    _tlp_jenv->ExceptionClear();
    exit(1);
  }

  return jv_ret;
}

main ()
{
  if (isHBaseAvailable()) {
    printf("HBase is available!\n");
    exit(0);
  }

  exit(1);
}
