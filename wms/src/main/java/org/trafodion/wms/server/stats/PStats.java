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

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.charset.Charset;
import java.io.ByteArrayOutputStream;
import java.util.List;

import org.apache.commons.io.IOUtils;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;

import org.apache.avro.Schema;
import org.apache.avro.generic.GenericData;
import org.apache.avro.generic.GenericDatumReader;
import org.apache.avro.generic.GenericDatumWriter;
import org.apache.avro.generic.GenericRecord;
import org.apache.avro.io.DatumReader;
import org.apache.avro.io.Decoder;
import org.apache.avro.io.DecoderFactory;
import org.apache.avro.io.Encoder;
import org.apache.avro.io.EncoderFactory;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.KeeperException;
import org.trafodion.wms.Constants;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.util.Bytes;
import org.trafodion.wms.zookeeper.ZkClient;

public class PStats implements Runnable {
    private static final Log LOG = LogFactory.getLog(PStats.class.getName());
    private Configuration conf;
    private Thread thrd;
    private String[] args;
    private String instance;

    private String zkhost;
    private int zkport;

    public PStats(String[] args) {
        this.args = args;
        conf = WmsConfiguration.create();
        thrd = new Thread(this);
        thrd.setDaemon(true);
        thrd.start();
    }

    public PStats(Configuration conf, String instance) {
        this.conf = conf;
        this.instance = instance;
        thrd = new Thread(this);
        thrd.setDaemon(true);
        thrd.start();
    }

    public void run() {
        /*
         * Options opt = new Options();
         * opt.addOption("i",true,"zookeeper ip net addres");
         * opt.addOption("p",true,"zookeeper port number"); CommandLine cmd; try
         * { cmd = new GnuParser().parse(opt, args); zkhost =
         * cmd.getOptionValue("i", Constants.LOCALHOST); try { zkport =
         * Integer.parseInt(cmd.getOptionValue("p",
         * Integer.toString(Constants.DEFAULT_ZOOKEEPER_CLIENT_PORT))); } catch
         * (NumberFormatException e){ LOG.error("Could not parse: ", e); zkport
         * = Constants.DEFAULT_ZOOKEEPER_CLIENT_PORT; } LOG.debug("Command=" +
         * cmd.toString()); LOG.debug("Options: " + zkhost + " " + zkport);
         * System.out.println("Options: " + zkhost + " " + zkport); } catch
         * (ParseException e) { LOG.error("Could not parse: ", e); zkhost =
         * Constants.LOCALHOST; zkport =
         * Constants.DEFAULT_ZOOKEEPER_CLIENT_PORT; }
         */
        try {
            // ZkClient zkc = new ZkClient(zkhost, zkport);
            ZkClient zkc = new ZkClient();
            zkc.connect();
            LOG.info("Connected to ZooKeeper");

            String parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,
                    Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);
            Stat stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS, false);
            if (stat == null) {
                LOG.error(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS
                        + " does not exist");
                throw new IOException(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS
                        + " does not exist");
            }
            String schema = Bytes.toString(zkc.getData(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS, false, stat));

            InetAddress ip = InetAddress.getLocalHost();
            LOG.info("hostname " + ip.getCanonicalHostName());

            String znode_stats = parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS + "/"
                    + ip.getCanonicalHostName() + ":" + instance;
            stat = zkc.exists(znode_stats, false);
            if (stat != null)
                zkc.delete(znode_stats, -1);
            zkc.create(znode_stats, new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                    CreateMode.EPHEMERAL);
            LOG.info("Created " + znode_stats);
            // serialize data
            Schema s = new Schema.Parser().parse(schema);
            ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
            Encoder en = EncoderFactory.get().binaryEncoder(outputStream, null);
            GenericRecord record = new GenericData.Record(s);
            GenericDatumWriter<GenericRecord> w = new GenericDatumWriter<GenericRecord>(
                    s);
            //
            DatumReader<GenericRecord> reader = new GenericDatumReader<GenericRecord>(
                    s);

            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec("uname -n");
            process.waitFor();
            BufferedReader buff = new BufferedReader(new InputStreamReader(
                    process.getInputStream()));
            String nodename = buff.readLine();
            LOG.info("nodename " + nodename);

            while (true) {
                PStatsCpuBusy pstatscpubusy = new PStatsCpuBusy();
                String cpuStat1 = pstatscpubusy.readSystemStat();

                try {
                    Thread.sleep(Constants.CPU_WINDOW);
                } catch (Exception e) {
                    LOG.info(e);
                }
                String cpuStat2 = pstatscpubusy.readSystemStat();

                float cpu = pstatscpubusy.getSystemCpuUsage(cpuStat1, cpuStat2);
                PStatsMemoryMonitor pm = PStatsMemoryMonitor.get();
                PStatsMemoryUsage mu = pm.monitor();
                // --------------- encoding
                // ------------------------------------------------
                Float cpubusy = cpu;
                Float memusage = mu.getMemoryUsage();
                LOG.debug("get cpubusy " + Float.toString(cpubusy));
                LOG.debug("get memory usage " + Float.toString(memusage));

                outputStream.reset();
                record.put("nodename", nodename);
                record.put("cpubusy", cpubusy.floatValue());
                record.put("memusage", memusage.floatValue());
                LOG.debug("nodename " + record.get("nodename"));
                LOG.debug("cpubusy " + record.get("cpubusy"));
                LOG.debug("memusage " + record.get("memusage"));
                // Encode
                w.write(record, en);
                en.flush();
                outputStream.close();
                stat = zkc.setData(znode_stats, outputStream.toByteArray(), -1);

                try {
                    Thread.sleep(Constants.PLATFORM_STATS_DELAY);
                } catch (Exception e) {
                    LOG.info(e);
                }
                /*-------------- test -------------------------------
                			LOG.debug(schema);
                			String encodedString = outputStream.toString();
                			LOG.debug("encodedString: "+encodedString);

                			byte[] b = zkc.getData(znode_stats, false, stat);
                			Decoder decoder = DecoderFactory.get().binaryDecoder(b, null);
                			GenericRecord result = reader.read(null, decoder);
                			LOG.debug("nodename " + result.get("nodename").toString());
                			LOG.debug("cpubusy " + result.get("cpubusy").toString());
                			LOG.debug("memusage " + result.get("memusage").toString());
                 */
            }
        } catch (IOException ioe) {
            LOG.info(ioe);
        } catch (InterruptedException ie) {
            LOG.info(ie);
        } catch (KeeperException ke) {
            LOG.info(ke);
        }
    }

    public static void main(String[] args) {
        PStats ps = new PStats(args);
    }
}
