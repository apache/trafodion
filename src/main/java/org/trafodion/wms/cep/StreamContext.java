package org.trafodion.wms.cep;

import java.util.*; 

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.wms.Constants;
import org.trafodion.wms.thrift.generated.*;

public class StreamContext {
	static  final Log LOG = LogFactory.getLog(StreamContext.class);
	String name;
	Map<String,Integer> keyOrderMap = new HashMap<String,Integer>();
	StringBuilder defineString = new StringBuilder();
	
	public StreamContext(String name,String keys) throws Exception {
		this.name = name;
		buildDefineStreamString(name,keys);
		LOG.debug("StreamContext [" + name + "], keyOrderMap [" + keyOrderMap + "]");
	}
	
	public String getDefineStreamString(){
		return defineString.toString();
	}
	
	void buildDefineStreamString(String name,String keys) {
		if(keys != null && name != null){
			keys = keys.replaceAll("[\\n\\r\\t]", "");
			defineString.append("define stream " + name + "(");
			boolean isFirst = true;
			Scanner scn = new Scanner(keys).useDelimiter(",");
			Integer index = 0;
			String key,value;
			while(scn.hasNext()){
				if(isFirst) {
					key = scn.next();
					key = key.replaceAll(" ", "");
					value = scn.next();
					value = value.replaceAll(" ", "");
					defineString.append(key + " " + value);
					isFirst = false;
				} else {
					key = scn.next();
					key = key.replaceAll(" ", "");
					value = scn.next();
					value = value.replaceAll(" ", "");
					defineString.append(", " + key + " " + value);
				}
				LOG.debug("key [" + key + "],index [" + index + "]");
				keyOrderMap.put(key,index);
				index++;
			}
			scn.close();
			defineString.append(")");
		}
		LOG.debug("buildDefineStreamString [" + name + "," + defineString.toString() + "]");
	}
	
	public Object[] keyValuesToArray(HashMap<String,KeyValue> keyValues){
		Object[] values = new Object[keyOrderMap.size()];
		LOG.debug("keyValuesToArray");
 
		if(keyValues.keySet() != null){
			Integer index = 0;
			for (String key: keyValues.keySet()) {
				index = keyOrderMap.get(key);
				LOG.debug("key" + "[" + key + ", index [" + index + "]");
				if(keyValues.get(key).isSetByteValue()) {
					values[index] = keyValues.get(key).getByteValue();
				} else if(keyValues.get(key).isSetShortValue()) {
					values[index] = keyValues.get(key).getShortValue();
				} else if(keyValues.get(key).isSetIntValue()) {
					values[index] = keyValues.get(key).getIntValue();
				} else if(keyValues.get(key).isSetLongValue()) {
					values[index] = keyValues.get(key).getLongValue();
				} else if(keyValues.get(key).isSetFloatValue()) {
					values[index] = keyValues.get(key).getFloatValue();
				} else if(keyValues.get(key).isSetStringValue()) {
					values[index] = keyValues.get(key).getStringValue();
				}
				index = 0;
			} 
		}
		return values;
	}
	
}