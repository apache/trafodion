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

CSRC := 	hmac.c \
	krb5_asn.c \
	lh_stats.c \
	lhash.c \
	md2_dgst.c \
	md2_one.c \
	md4_dgst.c \
	md4_one.c \
	md5_dgst.c \
	md5_one.c \
	md_rand.c \
	o_names.c \
	obj_dat.c \
	obj_err.c \
	obj_lib.c \
	ocsp_asn.c \
	ocsp_cl.c \
	ocsp_err.c \
	ocsp_ext.c \
	ocsp_ht.c \
	ocsp_lib.c \
	ocsp_prn.c \
	ocsp_srv.c \
	ocsp_vfy.c \
	p12_add.c \
	p12_asn.c \
	p12_attr.c \
	p12_crpt.c \
	p12_crt.c \
	p12_decr.c \
	p12_init.c \
	p12_key.c \
	p12_kiss.c \
	p12_mutl.c \
	p12_npas.c \
	p12_p8d.c \
	p12_p8e.c \
	p12_utl.c \
	pem_all.c \
	pem_err.c \
	pem_info.c \
	pem_lib.c \
	pem_oth.c \
	pem_pk8.c \
	pem_pkey.c \
	pem_seal.c \
	pem_sign.c \
	pem_x509.c \
	pem_xaux.c \
	pk12err.c \
	pk7_asn1.c \
	pk7_attr.c \
	pk7_doit.c \
	pk7_lib.c \
	pk7_mime.c \
	pk7_smime.c \
	pkcs7err.c \
	pqueue.c \
	rand_egd.c \
	rand_err.c \
	rand_lib.c \
	rand_nw.c \
	rand_os2.c \
	rand_unix.c \
	rand_win.c \
	randfile.c \
	rc2_cbc.c \
	rc2_ecb.c \
	rc2_skey.c \
	rc2cfb64.c \
	rc2ofb64.c \
	rc4_enc.c \
	rc4_skey.c \
	rmd_dgst.c \
	rmd_one.c \
	rsa_asn1.c \
	rsa_chk.c \
	rsa_depr.c \
	rsa_eay.c \
	rsa_err.c \
	rsa_gen.c \
	rsa_lib.c \
	rsa_none.c \
	rsa_null.c \
	rsa_oaep.c \
	rsa_pk1.c \
	rsa_pss.c \
	rsa_saos.c \
	rsa_sign.c \
	rsa_ssl.c \
	rsa_x931.c \
	sha1_one.c \
	sha1dgst.c \
	sha256.c \
	sha512.c \
	sha_dgst.c \
	sha_one.c \
	stack.c \
	str_err.c \
	str_lib.c \
	str_mem.c \
	str_meth.c \
	txt_db.c \
	ui_compat.c \
	ui_err.c \
	ui_lib.c \
	ui_openssl.c \
	ui_util.c

OPENSSLDIR := security/openssl
SSLDIR := $(OPENSSLDIR)/ssl
CRYPTODIR := $(OPENSSLDIR)/crypto

SRCPATH := .SOURCE : $(CRYPTODIR)/hmac $(CRYPTODIR)/krb5 $(CRYPTODIR)/lhash $(CRYPTODIR)/md2 $(CRYPTODIR)/md4 \
	$(CRYPTODIR)/md5 $(CRYPTODIR)/objects $(CRYPTODIR)/ocsp $(CRYPTODIR)/pem $(CRYPTODIR)/pkcs7 \
	$(CRYPTODIR)/pkcs12 $(CRYPTODIR)/pqueue $(CRYPTODIR)/rand $(CRYPTODIR)/rc2 \
	$(CRYPTODIR)/rc4 $(CRYPTODIR)/rc5 $(CRYPTODIR)/ripemd $(CRYPTODIR)/rsa \
	$(CRYPTODIR)/sha $(CRYPTODIR)/stack $(CRYPTODIR)/store $(CRYPTODIR)/threads \
	$(CRYPTODIR)/txt_db $(CRYPTODIR)/ui

INCLUDE_DIRS := $(OPENSSLDIR)/include $(CRYPTODIR) $(SSLDIR)

