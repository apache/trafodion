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


package org.trafodion.wms.util;


import org.apache.commons.logging.LogFactory;
import java.io.PrintWriter;

import org.trafodion.wms.VersionAnnotation;
import org.trafodion.wms.server.WmsServer;
import org.apache.commons.logging.Log;


/**
 * This class finds the package info for wms and the VersionAnnotation
 * information.  Taken from hadoop.  Only name of annotation is different.
 */
public class VersionInfo {
  private static final Log LOG = LogFactory.getLog(VersionInfo.class.getName());
  private static Package myPackage;
  private static VersionAnnotation version;

  static {
    myPackage = VersionAnnotation.class.getPackage();
    version = myPackage.getAnnotation(VersionAnnotation.class);
  }

  /**
   * Get the meta-data for the wmspackage.
   * @return package
   */
  static Package getPackage() {
    return myPackage;
  }

  /**
   * Get the wms version.
   * @return the wms version string, eg. "0.6.3-dev"
   */
  public static String getVersion() {
    return version != null ? version.version() : "Unknown";
  }

  /**
   * Get the subversion revision number for the root directory
   * @return the revision number, eg. "451451"
   */
  public static String getRevision() {
    return version != null ? version.revision() : "Unknown";
  }

  /**
   * The date that wms was compiled.
   * @return the compilation date in unix date format
   */
  public static String getDate() {
    return version != null ? version.date() : "Unknown";
  }

  /**
   * The user that compiled wms.
   * @return the username of the user
   */
  public static String getUser() {
    return version != null ? version.user() : "Unknown";
  }

  /**
   * Get the subversion URL for the root wms directory.
   * @return the url
   */
  public static String getUrl() {
    return version != null ? version.url() : "Unknown";
  }
  
  static String[] versionReport() {
    return new String[] {
      "Wms " + getVersion(),
      "Subversion " + getUrl() + " -r " + getRevision(),
      "Compiled by " + getUser() + " on " + getDate()
      };
  }

  public static void writeTo(PrintWriter out) {
    for (String line : versionReport()) {
      out.println(line);
    }
  }

  public static void logVersion() {
    for (String line : versionReport()) {
      LOG.info(line);
    }
  }

  public static void main(String[] args) {
    logVersion();
  }
}
