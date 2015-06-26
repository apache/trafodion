package org.trafodion.wms.server.stats;

import java.io.IOException;
import java.io.RandomAccessFile;

public final class PStatsCpuBusy {

    public String readSystemStat() {

        RandomAccessFile reader = null;
        String load = null;

        try {
            reader = new RandomAccessFile("/proc/stat", "r");
            load = reader.readLine();
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
        	try{
        		reader.close();
        	} catch (IOException ex) {}
        }
        return load;
    }

    public float getSystemCpuUsage(String start, String end) {
        String[] stat = start.split(" ");
        long idle1 = getSystemIdleTime(stat);
        long up1 = getSystemUptime(stat);

        stat = end.split(" ");
        long idle2 = getSystemIdleTime(stat);
        long up2 = getSystemUptime(stat);

       float cpu = -1f;
        if (idle1 >= 0 && up1 >= 0 && idle2 >= 0 && up2 >= 0) {
            if ((up2 + idle2) > (up1 + idle1) && up2 >= up1) {
                cpu = (up2 - up1) / (float) ((up2 + idle2) - (up1 + idle1));
                cpu *= 100.0f;
            }
        }
        return cpu;
    }

    public long getSystemUptime(String[] stat) {
        long l = 0L;
        for (int i = 2; i < stat.length; i++) {
            if (i != 5) {
                try {
                    l += Long.parseLong(stat[i]);
                } catch (NumberFormatException ex) {
                    ex.printStackTrace();
                    return -1L;
                }
            }
        }
        return l;
    }

    public long getSystemIdleTime(String[] stat) {
        try {
            return Long.parseLong(stat[5]);
        } catch (NumberFormatException ex) {
            ex.printStackTrace();
        }
        return -1L;
    }

    public float syncGetSystemCpuUsage(long elapse) {

        String stat1 = readSystemStat();
        if (stat1 == null) {
            return -1.f;
        }

        try {
            Thread.sleep(elapse);
        } catch (Exception e) {
        }

        String stat2 = readSystemStat();
        if (stat2 == null) {
            return -1.f;
        }
        return getSystemCpuUsage(stat1, stat2);
    }
}
