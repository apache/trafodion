package org.trafodion.wms.client;

import java.io.*;
import java.net.*;
import java.nio.charset.Charset;
import java.util.List;
import java.util.Iterator;
import java.util.StringTokenizer;
import java.util.HashMap;
import java.util.Map;
import java.util.Date;

import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.CreateMode;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.thrift.TException;
import org.apache.thrift.transport.TSSLTransportFactory;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSSLTransportFactory.TSSLTransportParameters;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.hadoop.conf.Configuration;

import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.Constants;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.thrift.generated.*;

public class ClientData extends Data{
    private static final Log LOG = LogFactory.getLog(ClientData.class);

    public ClientData(){
    }
    public ClientData(Data data){
        super(data);
    }
    public void putKeyValue(String key, Operation value){
        getKeyValues().put(key,new KeyValue().setIntValue(value.getValue()));
    }
    public void putKeyValue(String key, String value){
        getKeyValues().put(key,new KeyValue().setStringValue(value));
    }
    public void putKeyValue(String key, Long value){
        getKeyValues().put(key,new KeyValue().setLongValue(value));
    }
    public String getKeyValueAsString(String key) {
        KeyValue kvl = getKeyValues().get(key);
        if (kvl.isSetStringValue()) {
            return kvl.getStringValue();
        }
        else throw new IllegalStateException();
    }
    public long getKeyValueAsLong(String key) {
        long value = 0;
        KeyValue kvl = getKeyValues().get(key);
        if(kvl.isSetBoolValue()){
             if(true == kvl.boolValue)
                 value = 1;
        }
        else if(kvl.isSetByteValue()){
            value = Byte.valueOf(kvl.getByteValue()).longValue();
        }
        else if(kvl.isSetShortValue()){
            value = Short.valueOf(kvl.getShortValue()).longValue();
        }
        else if(kvl.isSetIntValue()){
            value = Integer.valueOf(kvl.getIntValue()).longValue();
        }
        else if(kvl.isSetLongValue()){
            value = kvl.getLongValue();
        }
        else if(kvl.isSetFloatValue()){
            value = Double.doubleToLongBits(kvl.getFloatValue());
        }
        else throw new IllegalStateException();

        return value;
    }
    
    public Action getKeyValueAction(){
        return Action.findByValue(keyValues.get("action").getIntValue());
    }
}
