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

CSRC := 	bio_b64.c \
	bio_enc.c \
	bio_md.c \
	bio_ok.c \
	c_all.c \
	c_allc.c \
	c_alld.c \
	cbc_cksm.c \
	cbc_enc.c \
	cfb64ede.c \
	cfb64enc.c \
	cfb_enc.c \
	des_enc.c \
	des_old.c \
	des_old2.c \
	dh_asn1.c \
	dh_check.c \
	dh_depr.c \
	dh_err.c \
	dh_gen.c \
	dh_key.c \
	dh_lib.c \
	digest.c \
	dsa_asn1.c \
	dsa_depr.c \
	dsa_err.c \
	dsa_gen.c \
	dsa_key.c \
	dsa_lib.c \
	dsa_ossl.c \
	dsa_sign.c \
	dsa_vrf.c \
	dso_dl.c \
	dso_dlfcn.c \
	dso_err.c \
	dso_lib.c \
	dso_null.c \
	dso_openssl.c \
	dso_vms.c \
	dso_win32.c \
	e_aes.c \
	e_bf.c \
	e_cast.c \
	e_des.c \
	e_des3.c \
	e_null.c \
	e_old.c \
	e_rc2.c \
	e_rc4.c \
	e_xcbc_d.c \
	ec2_mult.c \
	ec2_smpl.c \
	ec_asn1.c \
	ec_check.c \
	ec_curve.c \
	ec_cvt.c \
	ec_err.c \
	ec_key.c \
	ec_lib.c \
	ec_mult.c \
	ec_print.c \
	ecb3_enc.c \
	ecb_enc.c \
	ech_err.c \
	ech_key.c \
	ech_lib.c \
	ech_ossl.c \
	ecp_mont.c \
	ecp_nist.c \
	ecp_smpl.c \
	ecs_asn1.c \
	ecs_err.c \
	ecs_lib.c \
	ecs_ossl.c \
	ecs_sign.c \
	ecs_vrf.c \
	ede_cbcm_enc.c \
	enc_read.c \
	enc_writ.c \
	encode.c \
	eng_all.c \
	eng_cnf.c \
	eng_cryptodev.c \
	eng_ctrl.c \
	eng_dyn.c \
	eng_err.c \
	eng_fat.c \
	eng_init.c \
	eng_lib.c \
	eng_list.c \
	eng_openssl.c \
	eng_padlock.c \
	eng_pkey.c \
	eng_table.c \
	err.c \
	err_all.c \
	err_prn.c \
	evp_acnf.c \
	evp_enc.c \
	evp_err.c \
	evp_key.c \
	evp_lib.c \
	evp_pbe.c \
	evp_pkey.c \
	fcrypt.c \
	fcrypt_b.c \
	m_dss.c \
	m_dss1.c \
	m_ecdsa.c \
	m_md2.c \
	m_md4.c \
	m_md5.c \
	m_null.c \
	m_ripemd.c \
	m_sha.c \
	m_sha1.c \
	names.c \
	ofb64ede.c \
	ofb64enc.c \
	ofb_enc.c \
	p5_crpt.c \
	p5_crpt2.c \
	p_dec.c \
	p_enc.c \
	p_lib.c \
	p_open.c \
	p_seal.c \
	p_sign.c \
	p_verify.c \
	pcbc_enc.c \
	qud_cksm.c \
	rand_key.c \
	read2pwd.c \
	rpc_enc.c \
	set_key.c \
	str2key.c \
	tb_cipher.c \
	tb_dh.c \
	tb_digest.c \
	tb_dsa.c \
	tb_ecdh.c \
	tb_ecdsa.c \
	tb_rand.c \
	tb_rsa.c \
	tb_store.c \
	xcbc_enc.c


OPENSSLDIR := security/openssl
SSLDIR := $(OPENSSLDIR)/ssl
CRYPTODIR := $(OPENSSLDIR)/crypto

SRCPATH := $(CRYPTODIR)/des $(CRYPTODIR)/dh $(CRYPTODIR)/dsa $(CRYPTODIR)/dso $(CRYPTODIR)/ec \
	$(CRYPTODIR)/ecdh $(CRYPTODIR)/ecdsa $(CRYPTODIR)/engine $(CRYPTODIR)/err $(CRYPTODIR)/evp

INCLUDE_DIRS := $(OPENSSLDIR)/include $(CRYPTODIR) $(SSLDIR)

