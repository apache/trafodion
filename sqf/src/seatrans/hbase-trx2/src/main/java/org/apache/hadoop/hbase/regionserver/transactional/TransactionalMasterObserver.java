package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HConnectionManager;
import org.apache.hadoop.hbase.coprocessor.BaseMasterObserver;
import org.apache.hadoop.hbase.coprocessor.MasterCoprocessorEnvironment;
import org.apache.hadoop.hbase.coprocessor.ObserverContext;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
import org.apache.hadoop.hbase.master.HMaster;

public class TransactionalMasterObserver extends BaseMasterObserver{
        
        private static final Log LOG = LogFactory.getLog(TransactionalMasterObserver.class.getName());
        private HConnection connection;
                
        @Override
        public void preMove(ObserverContext<MasterCoprocessorEnvironment> ctx,
            HRegionInfo region, ServerName srcServer, ServerName destServer)
        throws IOException {
                LOG.trace("preMove -- ENTRY -- region: " + region.toString());
                 
                TransactionalRegionInterface tri = null;
                
                if (this.connection == null)
                   this.connection = HConnectionManager.getConnection(ctx.getEnvironment().getConfiguration());

                try {
                    tri = (TransactionalRegionInterface)this.connection.getHRegionConnection(
                                                                               srcServer.getHostname(),
                                                                               srcServer.getPort());                
                } catch (Exception e) {
                    LOG.error("Error occurred while obtaining TransactionalRegionInterface");
                }
  
                if(!tri.isMoveable(region.getRegionName())) {
                    LOG.debug("Unable to balance region, transactions present in region: " +
                    region.toString());
                    ctx.bypass();
                }
                LOG.trace("preMove -- EXIT -- region: " + region.toString()); 
        }
}
