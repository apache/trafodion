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

#include "ComSSL.h"
void aes_create_key(const unsigned char * input,
                    Lng32 input_len,
                    unsigned char * key,
                    Int32 aes_mode)
{
  const Lng32 key_len = EVP_CIPHER_key_length(aes_algorithm_type[aes_mode]);

  memset(key, 0, key_len);

  const unsigned char * source;
  unsigned char * ptr;
  const unsigned char * input_end = input + input_len;
  const unsigned char * key_end = key + key_len;

  // loop through all the characters of the input string, and does an assignment
  // with bitwise XOR between the input string and key string. If it iterate until
  // hit the end of key_len byte buffer, just start over from begining of the key
  // and continue doing ^=
  // If the length of input string shorter than key_len, it will stop and the end of
  // the input string.
  for (ptr = key, source = input; source < input_end; source++, ptr++)
  {
    if (ptr == key_end)
      ptr = key;
    *ptr ^= *source;
  }
}
