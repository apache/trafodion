/**
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
package org.trafodion.dcs.util;

import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.TimeUnit;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class RetryCounter {
    private static final Log LOG = LogFactory.getLog(RetryCounter.class);
    private final int maxRetries;
    private int retriesRemaining;
    private int retryInterval;
    private Queue<Long> queue;
    private TimeUnit timeUnit;

    public RetryCounter(int maxRetries, int retryInterval, TimeUnit timeUnit) {
        this.maxRetries = maxRetries;
        this.retriesRemaining = maxRetries;
        this.retryInterval = retryInterval;
        this.queue = new LinkedList<Long>();
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
        long sleepTime = (long) (retryInterval * Math.log(attempts + 15));
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
        return maxRetries - retriesRemaining + 1;
    }

    public void resetAttemptTimes() {
        this.retriesRemaining = maxRetries;
    }

    //this retry is in minutes level
    public boolean shouldRetryInnerMinutes() {
        if (LOG.isDebugEnabled()) {
            LOG.debug("retryInterval = [" + retryInterval + "]. queue size = [" + queue.size() + "]. max retries = ["
                    + getMaxRetries() + "]. ");
        }
        if (retryInterval == 0) {
            return true;
        }
        if (queue.size() < getMaxRetries()) {
            queue.offer(System.currentTimeMillis());
            return true;
        } else {
            long currentTime = System.currentTimeMillis();
            Long firstRetryTime = queue.peek();
            long delta = calcDelta(currentTime - firstRetryTime);
            if (delta < 0) {
                LOG.error("reject!!! attempt to restart mxosrvr in [ "
                        + TimeUnit.MILLISECONDS.toMinutes(currentTime - firstRetryTime)
                        + " ] minutes...can't restart mxosrvr large than [ " + getMaxRetries() + " ] times in [ "
                        + this.retryInterval + " ] minutes");
                return false;
            }
            queue.poll();
            queue.offer(currentTime);
            return true;
        }
    }

    //give the real millisecond as parameter. use this parameter minus given retryInterval.
    private long calcDelta(long realMillis) {
        if (LOG.isDebugEnabled()) {
            LOG.debug("realMillis = [" + realMillis + "]. retryInterval = [" + timeUnit.toMillis(retryInterval) + "]");
        }
        return realMillis - timeUnit.toMillis(retryInterval);
    }
}
