package org.trafodion.wms.cep; 

import java.util.*;

public interface CepObserver {           
	public void update();           
	public void setSubject(CepSubject sub);
}
