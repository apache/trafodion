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

CSRC := a_bitstr.c \
	a_bool.c \
	a_bytes.c \
	a_d2i_fp.c \
	a_digest.c \
	a_dup.c \
	a_enum.c \
	a_gentm.c \
	a_hdr.c \
	a_i2d_fp.c \
	a_int.c \
	a_mbstr.c \
	a_meth.c \
	a_object.c \
	a_octet.c \
	a_print.c \
	a_set.c \
	a_sign.c \
	a_strex.c \
	a_strnid.c \
	a_time.c \
	a_type.c \
	a_utctm.c \
	a_utf8.c \
	a_verify.c \
	aes_cbc.c \
	aes_cfb.c \
	aes_core.c \
	aes_ctr.c \
	aes_ecb.c \
	aes_ige.c \
	aes_misc.c \
	aes_ofb.c \
	asn1_err.c \
	asn1_gen.c \
	asn1_lib.c \
	asn1_par.c \
	asn_moid.c \
	asn_pack.c \
	b_dump.c \
	b_print.c \
	b_sock.c \
	bf_buff.c \
	bf_cfb64.c \
	bf_ecb.c \
	bf_enc.c \
	bf_nbio.c \
	bf_null.c \
	bf_ofb64.c \
	bf_skey.c \
	bio_cb.c \
	bio_err.c \
	bio_lib.c \
	bn_add.c \
	bn_asm.c \
	bn_blind.c \
	bn_const.c \
	bn_ctx.c \
	bn_depr.c \
	bn_div.c \
	bn_err.c \
	bn_exp.c \
	bn_exp2.c \
	bn_gcd.c \
	bn_gf2m.c \
	bn_kron.c \
	bn_lib.c \
	bn_mod.c \
	bn_mont.c \
	bn_mpi.c \
	bn_mul.c \
	bn_nist.c \
	bn_prime.c \
	bn_print.c \
	bn_rand.c \
	bn_recp.c \
	bn_shift.c \
	bn_sqr.c \
	bn_sqrt.c \
	bn_word.c \
	bss_acpt.c \
	bss_bio.c \
	bss_conn.c \
	bss_dgram.c \
	bss_fd.c \
	bss_file.c \
	bss_log.c \
	bss_mem.c \
	bss_null.c \
	bss_sock.c \
	buf_err.c \
	buffer.c \
	c_cfb64.c \
	c_ecb.c \
	c_enc.c \
	c_ofb64.c \
	c_rle.c \
	c_skey.c \
	c_zlib.c \
	comp_err.c \
	comp_lib.c \
	conf_api.c \
	conf_def.c \
	conf_err.c \
	conf_lib.c \
	conf_mall.c \
	conf_mod.c \
	conf_sap.c \
	d2i_pr.c \
	d2i_pu.c \
	evp_asn1.c \
	f_enum.c \
	f_int.c \
	f_string.c \
	i2d_pr.c \
	i2d_pu.c \
	n_pkey.c \
	nsseq.c \
	p5_pbe.c \
	p5_pbev2.c \
	p8_pkey.c \
	t_bitst.c \
	t_crl.c \
	t_pkey.c \
	t_req.c \
	t_spki.c \
	t_x509.c \
	t_x509a.c \
	tasn_dec.c \
	tasn_enc.c \
	tasn_fre.c \
	tasn_new.c \
	tasn_typ.c \
	tasn_utl.c \
	x_algor.c \
	x_attrib.c \
	x_bignum.c \
	x_crl.c \
	x_exten.c \
	x_info.c \
	x_long.c \
	x_name.c \
	x_pkey.c \
	x_pubkey.c \
	x_req.c \
	x_sig.c \
	x_spki.c \
	x_val.c \
	x_x509.c \
	x_x509a.c

OPENSSLDIR := security/openssl
SSLDIR := $(OPENSSLDIR)/ssl
CRYPTODIR := $(OPENSSLDIR)/crypto

SRCPATH := $(CRYPTODIR)/aes $(CRYPTODIR)/asn1 $(CRYPTODIR)/bf $(CRYPTODIR)/bio \
	$(CRYPTODIR)/bn $(CRYPTODIR)/buffer $(CRYPTODIR)/cast \
	$(CRYPTODIR)/comp $(CRYPTODIR)/conf

INCLUDE_DIRS := $(OPENSSLDIR)/include $(CRYPTODIR) $(SSLDIR)
