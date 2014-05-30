package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.regionserver.HRegion;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.regionserver.wal.HLog.Entry;
import org.apache.hadoop.hbase.regionserver.wal.HLog.Reader;
import org.apache.hadoop.hbase.regionserver.wal.HLog.Writer;
import org.apache.hadoop.hbase.regionserver.wal.HLogSplitter;
import org.apache.hadoop.hbase.util.CancelableProgressable;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.regionserver.transactional.THLog;

/**
 * Extend core THLog splitter to make it also split the transactional logs.
 * 
 * @author clint.morgan
 */
public class THLogSplitter extends HLogSplitter {
	
	private static final Log LOG = LogFactory.getLog(THLogSplitter.class);

    public THLogSplitter(final Configuration conf, final Path rootDir, final Path srcDir, final Path oldLogDir,
            final FileSystem fs) {
        super(conf, rootDir, srcDir, oldLogDir, fs);
        LOG.trace("THLogSplitter -- CONSTRUCT");
    }

    private boolean isTrxLog(final Path logfile) {
    	LOG.trace("isTrxLog, Path: " + logfile.toString());
        return logfile.getName().contains(THLog.THLOG_DATFILE);
    }

    @Override
    protected Writer createWriter(final FileSystem fs, final Path logfile, final Configuration conf) throws IOException {
    	LOG.trace("createWriter, Path: " + logfile.toString());
    	return new DualWriter(fs, logfile, conf);
    }

    // The following getReader method is for use with the Cloudera HBase distro (part of CDH 4.4)
    //@Override
    protected Reader getReader(FileSystem fs, FileStatus file, Configuration conf,
                                                       boolean skipErrors, CancelableProgressable reporter) throws IOException {

        Path logfile = file.getPath();
    	if (isTrxLog(logfile)) {
    		LOG.trace("AAA getReader, Path: " + logfile.toString());
            return THLog.getReader(fs, logfile, conf);
        }
    	LOG.trace("AAA getReader, Path: " + logfile.toString());
        return HLog.getReader(fs, logfile, conf);
    }

    // This is used with Apache HBase 0.94.6, MapR HBase 0.94.13 and others
    //@Override
    protected Reader getReader(final FileSystem fs, final Path logfile, final Configuration conf) throws IOException {
    	if (isTrxLog(logfile)) {
    		LOG.trace("getReader, Path: " + logfile.toString());
            return THLog.getReader(fs, logfile, conf);
        }
    	LOG.trace("getReader, Path: " + logfile.toString());
        return HLog.getReader(fs, logfile, conf);
    }

    private static class DualWriter implements Writer {
    	private static final Log LOG = LogFactory.getLog(DualWriter.class);

        private final FileSystem fs;
        private final Path logfile;
        private final Configuration conf;

        private Writer coreWriter = null;
        private Writer trxWriter = null;

        public DualWriter(final FileSystem fs, final Path logfile, final Configuration conf) {
        	LOG.trace("DualWriter -- CONSTRUCT");
            this.fs = fs;
            this.logfile = logfile;
            this.conf = conf;
        }

        private boolean isTrxEntry(final Entry entry) {
        	LOG.trace("isTrxEntry -- " + entry.getKey().toString());
            return entry.getKey() instanceof THLogKey;
        }

        private synchronized Writer getCoreWriter() throws IOException {
            if (coreWriter == null) {
                coreWriter = HLog.createWriter(fs, logfile, conf);
            }
            LOG.trace("getCoreWriter");
            return coreWriter;
        }

        private synchronized Writer getTrxWriter() throws IOException {
            if (trxWriter == null) {
                Path trxPath = new Path(logfile.getParent(), THLog.HREGION_OLD_THLOGFILE_NAME);
                trxWriter = THLog.createWriter(fs, trxPath, conf);
            }
            LOG.trace("getTrxWriter");
            return trxWriter;
        }

        @Override
        public void append(final Entry entry) throws IOException {
        	LOG.trace("append -- ENTRY, " + entry.getKey().toString());
            if (isTrxEntry(entry)) {
                getTrxWriter().append(entry);
            } else {
                getCoreWriter().append(entry);
            }
            LOG.trace("append -- EXIT");

        }

        @Override
        public void close() throws IOException {
        	LOG.trace("close -- ENTRY");
            if (coreWriter != null) {
                coreWriter.close();
            }
            if (trxWriter != null) {
                trxWriter.close();
            }
            LOG.trace("close -- EXIT");
        }

        @Override
        public long getLength() throws IOException {
        	LOG.trace("getLength");
            throw new UnsupportedOperationException("Not expected to be called");
        }

        @Override
        public void init(final FileSystem fs, final Path path, final Configuration c) throws IOException {
        	LOG.trace("init");
            throw new UnsupportedOperationException("Not expected to be called");
        }

        @Override
        public void sync() throws IOException {        
        	LOG.trace("sync -- ENTRY");
            if (coreWriter != null) {
                coreWriter.sync();
            }
            if (trxWriter != null) {
                trxWriter.sync();
            }
            LOG.trace("sync -- EXIT");
        }

    }
}
