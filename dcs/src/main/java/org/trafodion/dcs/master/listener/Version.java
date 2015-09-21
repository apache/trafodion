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

class Version {
	private static  final Log LOG = LogFactory.getLog(Version.class);

	short componentId;
	short majorVersion;
	short minorVersion;
	int buildId;

	Version(){
		componentId = 0;
		majorVersion = 0;
		minorVersion = 0;
		buildId = 0;
	}

	void extractFromByteBuffer(ByteBuffer buf) {
		componentId = buf.getShort();
		majorVersion = buf.getShort();
		minorVersion = buf.getShort();
		buildId = buf.getInt();
	}
	void insertIntoByteBuffer(ByteBuffer buf) {
		buf.putShort(componentId);
		buf.putShort(majorVersion);
		buf.putShort(minorVersion);
		buf.putInt(buildId);
	}
}
