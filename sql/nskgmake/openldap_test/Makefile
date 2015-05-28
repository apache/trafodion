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

OPENLDAPDIR := security/openldap
LDAPLIBSDIR := $(OPENLDAPDIR)/libraries

LDAPCSRC := ldapmodify ldapsearch ldapwhoami
SLDAPCSRC := slapd-bind slapd-read slapd-search slapd-tester
LIBUTILCSRC := liblutil_ptest liblutil_testavl liblutil_md5 liblutil_uuid
LIBLDAPCSRC := libldap_test

ifneq (,$(filter $(DRIVER),$(LDAPCSRC)))
	CSRC := $(DRIVER).c common.c
	TESTSRCDIR := $(OPENLDAPDIR)/clients/tools
else
ifneq (,$(filter $(DRIVER),$(SLDAPCSRC)))
	CSRC := $(DRIVER).c
	TESTSRCDIR := $(OPENLDAPDIR)/tests/progs
else
ifneq (,$(filter $(DRIVER),$(LIBUTILCSRC)))
	CSRC := $(patsubst liblutil_%,%.c,$(DRIVER))
	TESTSRCDIR= $(OPENLDAPDIR)/libraries/liblutil
else
ifneq (,$(filter $(DRIVER),$(LIBLDAPCSRC)))
	CSRC := $(patsubst libldap_%,%.c,$(DRIVER))
	TESTSRCDIR= $(OPENLDAPDIR)/libraries/libldap
else
	CSRC := openldap_test.c
	TESTSRCDIR := $(OPENLDAPDIR)/tests/progs
endif
endif
endif
endif

# JJM - If $(DRIVER) string is empty
ifeq (,$(DRIVER))
	TESTSRCDIR := $(OPENLDAPDIR)/libraries
endif

SRCPATH := $(TESTSRCDIR)

INCLUDE_DIRS += $(OPENLDAPDIR)/include

DEP_LIBS := openldap_liblber \
	openldap_libldap \
	openldap_liblutil \
	openldap_liblunicode \
	openldap_librewrite \
	openssl_client \
	openssl_crypto \
	openssl_crypto1 \
	openssl_crypto2 \
	openssl_crypto3

ifeq ($(TARGTYPE),yos)

ifneq (, $(filter $(DRIVER), liblutil_md5 liblutil_uuid))
	DEFS += -DTEST
endif

endif
