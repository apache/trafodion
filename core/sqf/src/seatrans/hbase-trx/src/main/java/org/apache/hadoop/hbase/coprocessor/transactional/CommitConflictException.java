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
**/

package org.apache.hadoop.hbase.coprocessor.transactional;
import java.io.IOException;

/** Thrown when a transaction conflicts with another transaction. 
 * 
 */
public class CommitConflictException extends IOException {

  private static final long serialVersionUID = 7062921444531109202L;

  /** Default Constructor */
  public CommitConflictException() {
    super();
  }

  /**
   * @param arg0 message
   * @param arg1 cause
   */
  public CommitConflictException(String arg0, Throwable arg1) {
    super(arg0, arg1);
  }

  /**
   * @param arg0 message
   */
  public CommitConflictException(String arg0) {
    super(arg0);
  }

  /**
   * @param arg0 cause
   */
  public CommitConflictException(Throwable arg0) {
    super(arg0);
  }

}
