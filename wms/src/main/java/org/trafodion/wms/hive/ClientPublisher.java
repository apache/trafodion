package org.trafodion.wms.hive;

import java.io.*;
import java.util.*;
import org.apache.hadoop.hive.ql.stats.*;

public class ClientPublisher implements ClientStatsPublisher {

  @Override
  public void run(Map<String,Double> counterValues, String jobID) {
    this.run_(counterValues,jobID);
  }

  public void run_(Map<String,Double> counterValues, String jobID){
	  //PreExecute.printUpdateWms(counterValues, jobID);
 }
}

