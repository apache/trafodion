/**
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.trafodion.rest.zookeeper;

import java.io.*;
import java.util.List;
import java.util.Scanner;
import java.nio.charset.Charset;
import org.apache.zookeeper.data.Stat;
import org.apache.commons.io.IOUtils;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.rest.Constants;
import org.trafodion.rest.util.Bytes;

public class ZkUtil {
	private static final Log LOG = LogFactory.getLog(ZkUtil.class);
	
	public static void main(String [] args) throws Exception {

		if (args.length < 1) {
			System.err.println("Usage: ZkUtil {command}");
			System.exit(1);
		}

		Options opt = new Options();
		CommandLine cmd = null;

		try {
			cmd = new GnuParser().parse(opt, args);
		} catch (NullPointerException e) {
			System.err.println("No args found: " + e);
			System.exit(1);
		} catch (ParseException e) {
			System.err.println("Could not parse: " + e);
			System.exit(1);
		}

		try {
			String znode = cmd.getArgList().get(0).toString();
		    
			ZkClient zkc = new ZkClient();
			zkc.connect();
			Stat stat = zkc.exists(znode,false);
			if(stat == null) {
				System.out.println("");
			} else {
				List<String> znodes = zkc.getChildren(znode,null);
				zkc.close();
				if(znodes.isEmpty()) {
					System.out.println("");
				} else {
					Scanner scn = new Scanner(znodes.get(0));
					scn.useDelimiter(":");
					String hostName = scn.next();//host name
					scn.close();
					System.out.println(hostName);
				}
			}
		} catch (Exception e) {
			System.err.println(e);
			e.printStackTrace();
			System.exit(1);
		}			
	}
}