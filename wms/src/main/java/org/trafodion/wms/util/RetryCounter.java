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

/**
 * Copyright 2011 The Apache Software Foundation
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.trafodion.wms.util;

import java.util.concurrent.TimeUnit;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class RetryCounter {
  private static final Log LOG = LogFactory.getLog(RetryCounter.class);
  private final int maxRetries;
  private int retriesRemaining;
  private final int retryIntervalMillis;
  private final TimeUnit timeUnit;

  public RetryCounter(int maxRetries, 
  int retryIntervalMillis, TimeUnit timeUnit) {
    this.maxRetries = maxRetries;
    this.retriesRemaining = maxRetries;
    this.retryIntervalMillis = retryIntervalMillis;
    this.timeUnit = timeUnit;
  }

  public int getMaxRetries() {
    return maxRetries;
  }

  /**
   * Sleep for a exponentially back off time
   * @throws InterruptedException
   */
  public void sleepUntilNextRetry() throws InterruptedException {
    int attempts = getAttemptTimes();
    long sleepTime = (long) (retryIntervalMillis * Math.pow(2, attempts));
    LOG.info("Sleeping " + sleepTime + "ms before retry #" + attempts + "...");
    timeUnit.sleep(sleepTime);
  }

  public boolean shouldRetry() {
    return retriesRemaining > 0;
  }

  public void useRetry() {
    retriesRemaining--;
  }
  
  public int getAttemptTimes() {
    return maxRetries-retriesRemaining+1;
  }
}
