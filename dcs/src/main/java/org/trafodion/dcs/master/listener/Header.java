/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.master.listener;

import java.sql.SQLException;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.net.*;
import java.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

class Header {
	private static  final Log LOG = LogFactory.getLog(Header.class);
	//
	// The Java version of the HEADER structure taken from TransportBase.h
	//
	short operation_id_;
	// + 2 filler
	int dialogueId_;
	int total_length_;
	int cmp_length_;
	char compress_ind_;
	char compress_type_;
	// + 2 filler
	int hdr_type_;
	int signature_;
	int version_;
	char platform_;
	char transport_;
	char swap_;
	// + 1 filler
	short error_;
	short error_detail_;

	Header() {
		operation_id_=0;
		dialogueId_=0;
		total_length_=0;
		cmp_length_=0;
		compress_ind_=' ';
		compress_type_=' ';
		hdr_type_=0;
		signature_=0;
		version_=0;
		platform_=' ';
		transport_=' ';
		swap_=' ';
		error_=0;
		error_detail_=0;
	}
	static int sizeOf() {
		return 40;
	}
	short getOperationId(){
		return operation_id_;
	}
	int getTotalLength(){
		return total_length_;
	}
	char getSwap(){
		return swap_;
	}
	int getVersion(){
		return version_;
	}
	void setOperationId(short value){
		operation_id_ = value;
	}
	void setSwap(char swap ){
		swap_ = swap;
	}
	void setTotalLength(int total_length ){
		total_length_ = total_length;
	}
	void setVersion(int version){
		version_ = version;
	}
	void debugHeader(String function){
		LOG.debug("Function :" + function);
		LOG.debug("operation_id :"+operation_id_);
		LOG.debug("dialogueId :"+dialogueId_);
		LOG.debug("total_length :"+total_length_);
		LOG.debug("cmp_length :"+cmp_length_);
		LOG.debug("compress_ind :"+compress_ind_);
		LOG.debug("compress_type :"+compress_type_);
		LOG.debug("hdr_type :"+hdr_type_);
		LOG.debug("signature :"+signature_);
		LOG.debug("version :"+version_);
		LOG.debug("platform :"+platform_);
		LOG.debug("transport :"+transport_);
		LOG.debug("swap :"+swap_);
		LOG.debug("error :"+error_);
		LOG.debug("error_detail :"+error_detail_);
	}

	void insertIntoByteBuffer(ByteBuffer buf) {
		debugHeader("insert");
		buf.putShort(operation_id_);
		buf.put((byte)0); // + filler
		buf.put((byte)0); // + filler
		buf.putInt(dialogueId_);
		buf.putInt(total_length_);
		buf.putInt(cmp_length_);
		buf.put((byte)compress_ind_);
		buf.put((byte)compress_type_);
		buf.put((byte)0); // + filler
		buf.put((byte)0); // + filler
		buf.putInt(hdr_type_);
		buf.putInt(signature_);
		buf.putInt(version_);
		buf.put((byte)platform_);
		buf.put((byte)transport_);
		buf.put((byte)swap_);
		buf.put((byte)0); // + filler
		buf.putShort(error_);
		buf.putShort(error_detail_);
	}

	void extractFromByteArray(ByteBuffer buf) {
		buf.rewind();
		operation_id_ = buf.getShort();
		buf.get(); // + filler
		buf.get(); // + filler
		dialogueId_ = buf.getInt();
		total_length_ = buf.getInt();
		cmp_length_ = buf.getInt();
		compress_ind_ = (char)buf.get();
		compress_type_ = (char)buf.get();
		buf.get(); // + filler
		buf.get(); // + filler
		hdr_type_ = buf.getInt();
		signature_ = buf.getInt();
		version_ = buf.getInt();
		platform_ = (char)buf.get();
		transport_ = (char)buf.get();
		swap_ = (char)buf.get();
		buf.get(); // + filler
		error_ = buf.getShort();
		error_detail_ = buf.getShort();
		debugHeader("extract");
	}
}
