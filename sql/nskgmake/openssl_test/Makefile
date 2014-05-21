#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################

A_SRC := apps.c
S_SRC := s_cb.c s_socket.c
RAND_SRC := app_rand.c

OPENSSL_SRC := $(A_SRC) \
	$(RAND_SRC) \
	$(S_SRC) \
	asn1pars.c \
	ca.c \
	ciphers.c \
	crl.c \
	crl2p7.c \
	dgst.c \
	dh.c \
	dhparam.c \
	dsa.c \
	dsaparam.c \
	ec.c \
	ecparam.c \
	enc.c \
	engine.c \
	errstr.c \
	gendh.c \
	gendsa.c \
	genrsa.c \
	nseq.c \
	ocsp.c \
	openssl.c \
	passwd.c \
	pkcs12.c \
	pkcs7.c \
	pkcs8.c \
	prime.c \
	rand.c \
	req.c \
	rsa.c \
	rsautl.c \
	s_client.c \
	s_server.c \
	s_time.c \
	sess_id.c \
	smime.c \
	speed.c \
	spkac.c \
	verify.c \
	version.c \
	x509.c

OPENSSLDIR := security/openssl
CRYPTODIR := $(OPENSSLDIR)/crypto
SSLDIR := $(OPENSSLDIR)/ssl
DEMOSDIR := $(OPENSSLDIR)/demos
APPSDIR := $(OPENSSLDIR)/apps

TESTSRCDIR := bf bn cast des dh dsa ec ecdh ecdsa engine hmac md2 md4 md5 rand rc2 rc4 rc5 sha ssl
TEST_SRCDIR := evp rsa
ifneq (,$(filter $(DRIVER), $(TESTSRCDIR)))
CSRC := $(patsubst %,%test.c, $(DRIVER))
else 
ifneq (,$(filter $(DRIVER), $(TEST_SRCDIR)))
CSRC := $(patsubst %,%_test.c, $(DRIVER))
else 
ifeq ($(DRIVER), cli)
CSRC := cli.c
else 
ifeq ($(DRIVER), openssl)
CSRC := $(OPENSSL_SRC)
endif
endif
endif
endif

ifeq ($(DRIVER), sha)
CSRC +=sha1test.c sha256t.c sha512t.c
endif
ifeq ($(DRIVER), bn)
CSRC += exptest.c
endif

DRIVER_DIRS := bf bn cast des dh dsa ec ecdsa engine evp hmac md2 md4 md5 rand rc2 rc4 rc5 ripemd rsa sha
DEMO_DIRS := serv cli

ifneq (,$(filter $(DRIVER), $(DRIVER_DIRS)))
TESTSRCDIR := $(CRYPTODIR)/$(DRIVER)
else 
ifneq (,$(filter $(DRIVER), $(DEMO_DIRS)))
TESTSRCDIR := $(DEMOSDIR)/ssl
else 
ifeq ($(DRIVER), ssl)
TESTSRCDIR := $(APPSDIR)
endif
endif
endif

INCLUDE_DIRS := $(OPENSSLDIR)/include $(CRYPTODIR) $(SSLDIR)

DEP_LIBS := openssl_client \
	openssl_crypto\
	openssl_crypto1 \
	openssl_crypto2 \
	openssl_crypto3
