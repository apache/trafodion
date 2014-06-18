package org.trafodion.wms.util;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.wms.util.WmsConfiguration;

/**
 * Tool that prints out a configuration.
 * Pass the configuration key on the command-line.
 */
public class WmsConfTool {
  public static void main(String args[]) {
    if (args.length < 1) {
      System.err.println("Usage: WmsConfTool <CONFIGURATION_KEY>");
      System.exit(1);
      return;
    }

    Configuration conf = WmsConfiguration.create();
    System.out.println(conf.get(args[0]));
  }
}