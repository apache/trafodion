package org.trafodion.wms.cep; 

import java.util.*;

public interface CepSubject {       
	public void register(CepObserver obj);    
	public void unregister(CepObserver obj);           
	public void notifyObservers();           
	public Object getUpdate(CepObserver obj);       
}
