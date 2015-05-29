package org.trafodion.wms.server.stats;

import java.io.IOException;
import java.io.File;

public abstract class PStatsMemoryMonitor {
    public abstract PStatsMemoryUsage monitor() throws IOException;
	public static PStatsMemoryMonitor get() throws IOException {

        if(new File("/proc/meminfo").exists())
            return new PStatsMemInfo();

         throw new IOException("No suitable implementation found");
    }

}
