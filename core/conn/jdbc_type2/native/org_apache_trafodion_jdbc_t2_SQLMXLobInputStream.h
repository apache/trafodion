/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_apache_trafodion_jdbc_t2_SQLMXLobInputStream */

#ifndef _Included_org_apache_trafodion_jdbc_t2_SQLMXLobInputStream
#define _Included_org_apache_trafodion_jdbc_t2_SQLMXLobInputStream
#ifdef __cplusplus
extern "C" {
#endif
#undef org_apache_trafodion_jdbc_t2_SQLMXLobInputStream_MAX_SKIP_BUFFER_SIZE
#define org_apache_trafodion_jdbc_t2_SQLMXLobInputStream_MAX_SKIP_BUFFER_SIZE 2048L
/*
 * Class:     org_apache_trafodion_jdbc_t2_SQLMXLobInputStream
 * Method:    readChunk
 * Signature: (Ljava/lang/String;JJILjava/lang/String;Ljava/nio/ByteBuffer;)I
 */
JNIEXPORT jint JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXLobInputStream_readChunk
  (JNIEnv *, jobject, jstring, jlong, jlong, jint, jstring, jobject);

#ifdef __cplusplus
}
#endif
#endif