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
#ifndef __COMAES_H__
#define __COMAES_H__
#include <openssl/evp.h>
#include <openssl/err.h>

#include <string.h>
#include <stdint.h>

#include "Platform.h"
/**
 *  some helper struct and function for AES
 *
 */

const EVP_CIPHER * aes_algorithm_type[] = {
                            /* CQD value   Algorithm type   */
  EVP_aes_128_ecb(),        /*    0        aes-128-ecb      */
  EVP_aes_192_ecb(),        /*    1        aes_192_ecb      */
  EVP_aes_256_ecb(),        /*    2        aes_256_ecb      */
  EVP_aes_128_cbc(),        /*    3        aes_128_cbc      */
  EVP_aes_192_cbc(),        /*    4        aes_192_cbc      */
  EVP_aes_256_cbc(),        /*    5        aes_256_cbc      */
  EVP_aes_128_cfb1(),       /*    6        aes_128_cfb1     */
  EVP_aes_192_cfb1(),       /*    7        aes_192_cfb1     */
  EVP_aes_256_cfb1(),       /*    8        aes_256_cfb1     */
  EVP_aes_128_cfb8(),       /*    9        aes_128_cfb8     */
  EVP_aes_192_cfb8(),       /*    10       aes_192_cfb8     */
  EVP_aes_256_cfb8(),       /*    11       aes_256_cfb8     */
  EVP_aes_128_cfb128(),     /*    12       aes_128_cfb128   */
  EVP_aes_192_cfb128(),     /*    13       aes_192_cfb128   */
  EVP_aes_256_cfb128(),     /*    14       aes_256_cfb128   */
  EVP_aes_128_ofb(),        /*    15       aes_128_ofb      */
  EVP_aes_192_ofb(),        /*    16       aes_192_ofb      */
  EVP_aes_256_ofb(),        /*    17       aes_256_ofb      */
};

void aes_create_key(const unsigned char * input,
                    Lng32 input_len,
                    unsigned char * key,
                    Int32 aes_mode);

#endif

