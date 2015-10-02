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

package org.trafodion.wms.server.stats;

import java.io.IOException;
public class PStatsMemoryUsage {
    /**
     * Total physical memory of the system, in bytes.
     */
    public final long totalPhysicalMemory;
    /**
     * Of the total physical memory of the system, available bytes.
     */
    public final long availablePhysicalMemory;
    /**
     * Of the total buffers memory available bytes.
     */
    public final long memoryBuffers;
    /**
     * Of the total cached memory available bytes.
     */
    public final long memoryCached;

    public PStatsMemoryUsage(long totalPhysicalMemory, long availablePhysicalMemory,  long memoryBuffers, long memoryCached) {
        this.totalPhysicalMemory = totalPhysicalMemory;
        this.availablePhysicalMemory = availablePhysicalMemory;
        this.memoryBuffers = memoryBuffers;
        this.memoryCached = memoryCached;
    }
    PStatsMemoryUsage(long[] v) throws IOException {
        this(v[0],v[1],v[2],v[3]);
        if(!hasData(v))
            throw new IOException("No data available");
    }
    public String toString() {
        return String.format("Memory:%d/%d/%d/%dMB",
            toMB(availablePhysicalMemory),
            toMB(totalPhysicalMemory),
            toMB(memoryBuffers),
            toMB(memoryCached));
    }
    private static long toMB(long l) {
        return l/(1024*1024);
    }
    private static long toKB(long l) {
        return l/(1024);
    }

    /*package*/ static boolean hasData(long[] values) {
        for (long v : values)
            if(v!=-1)   return true;
        return false;
    }
    public float getMemoryUsage() {
    	float kb_main_total = toKB(totalPhysicalMemory);
    	float kb_main_used = kb_main_total - toKB(availablePhysicalMemory);
    	float kb_buffers = toKB(memoryBuffers);
    	float kb_cached = toKB(memoryCached);
    	return (kb_main_used - kb_buffers - kb_cached) * 100.0f / (kb_main_total);
    }
}
