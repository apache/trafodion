package org.trafodion.wms.cep; 

import java.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.log4j.Logger;

public class CepSubjectImpl implements CepSubject {   
	private static final Log LOG = LogFactory.getLog(CepSubjectImpl.class.getName());
	private List<CepObserver> observers;    
	private String message;     
	private boolean changed;    
	private final Object MUTEX = new Object(); 
	
	public CepSubjectImpl(){         
		this.observers = new ArrayList<CepObserver>();     
	}     
	
	@Override    
	public void register(CepObserver obj) { 
		LOG.debug("register " + obj);   
		if(obj == null) throw new NullPointerException("Null Observer");         
		synchronized (MUTEX) {         
			if(!observers.contains(obj)) observers.add(obj);         
		}     
	}   
	
	@Override   
	public void unregister(CepObserver obj) {    
		LOG.debug("unregister " + obj);   
		synchronized (MUTEX) {         
			observers.remove(obj);         
		}     
	} 
	
	@Override    
	public void notifyObservers() 
	{       
		LOG.debug("notifyObservers");      
		List<CepObserver> observersLocal = null;   
		
		synchronized (MUTEX) {             
			if (!changed)                 
				return;             
			observersLocal = new ArrayList<CepObserver>(this.observers);             
			this.changed=false;         
		} 
		
		for (CepObserver obj : observersLocal) {             
			obj.update();         
		}       
	} 
	
	@Override   
	public Object getUpdate(CepObserver obj) {  
		LOG.debug("getUpdate " + obj);   
		return this.message;     
	}

	public void post(String value) {         
		LOG.debug("post " + value);         
		this.message = value;         
		this.changed = true;         
		notifyObservers();    
	}       
}
