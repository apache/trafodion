#######################################################################
# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

CSRC := bio_ssl.c \
	d1_both.c \
	d1_clnt.c \
	d1_enc.c \
	d1_lib.c \
	d1_meth.c \
	d1_pkt.c \
	d1_srvr.c \
	encrypt.c \
	kssl.c \
	s23_clnt.c \
	s23_lib.c \
	s23_meth.c \
	s23_pkt.c \
	s23_srvr.c \
	s2_clnt.c \
	s2_enc.c \
	s2_lib.c \
	s2_meth.c \
	s2_pkt.c \
	s2_srvr.c \
	s3_both.c \
	s3_clnt.c \
	s3_enc.c \
	s3_lib.c \
	s3_meth.c \
	s3_pkt.c \
	s3_srvr.c \
	ssl_algs.c \
	ssl_asn1.c \
	ssl_cert.c \
	ssl_ciph.c \
	ssl_err.c \
	ssl_err2.c \
	ssl_lib.c \
	ssl_rsa.c \
	ssl_sess.c \
	ssl_stat.c \
	ssl_txt.c \
	t1_clnt.c \
	t1_enc.c \
	t1_lib.c \
	t1_meth.c \
	t1_srvr.c

CPPSRC := vprocsafdll.cpp

ifeq ($(FLAVOR),debug)
    CXXFLAGS := -Woverflow_traps
endif

OPENSSLDIR := security/openssl
SSLDIR := security/openssl/ssl

SRCPATH := $(SSLDIR) bin

INCLUDE_DIRS := $(OPENSSLDIR)/include bin
