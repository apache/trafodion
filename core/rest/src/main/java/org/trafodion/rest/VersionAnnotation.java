/**
* @@@ START COPYRIGHT @@@                                                       
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
*/

package org.trafodion.rest;

import java.lang.annotation.*;

@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.PACKAGE)
public @interface VersionAnnotation {

  /**
   * Get the Hadoop version
   * @return the version string "0.6.3-dev"
   */
  String version();

  /**
   * Get the username that compiled Hadoop.
   */
  String user();

  /**
   * Get the date when Hadoop was compiled.
   * @return the date in unix 'date' format
   */
  String date();

  /**
   * Get the url for the repository.
   */
  String url();

  /**
   * Get the revision.
   * @return the revision number as a string (eg. "451451")
   */
  String revision();
}
