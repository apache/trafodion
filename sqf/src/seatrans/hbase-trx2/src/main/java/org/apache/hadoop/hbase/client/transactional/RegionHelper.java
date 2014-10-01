package org.apache.hadoop.hbase.client.transactional;

import org.apache.hadoop.hbase.regionserver.HRegion;

public class RegionHelper {
	static {
		   System.loadLibrary("stmlib"); 
		}
	
    
    private native short REGISTERREGION(String region);
    
    public static void RegisterRegion(HRegion region) {   
    	if (region != null) {
    		
    	}
    	//short returnVal = new RegionHelper().REGISTERREGION(region.getRegionNameAsString());
    	short returnVal = new RegionHelper().REGISTERREGION("This is the string");
    	if (returnVal != 0) {
    		// log error 
    	}
    }
}