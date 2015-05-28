/**
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.trafodion.rest.script;

import java.util.List;
import java.util.HashMap; 
import java.util.Map;
import java.io.File; 
import java.io.FileNotFoundException; 
import java.io.FileReader; 
import java.io.FilenameFilter;
import javax.script.ScriptEngine; 
import javax.script.ScriptEngineFactory; 
import javax.script.ScriptEngineManager; 
import javax.script.ScriptException; 
import javax.script.CompiledScript;
import javax.script.Compilable;
import javax.script.Bindings;
import org.apache.log4j.Logger;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public final class ScriptManager {
	private static ScriptManager instance = null;
	private static final Log LOG = LogFactory.getLog(ScriptManager.class.getName());
	private ScriptEngineManager manager = new ScriptEngineManager();
	private Map<String, CompiledScript> m = new HashMap<String, CompiledScript>();
	private ScriptManagerWatcher watcherWorker = null;
	private static final String PYTHON_SUFFIX = ".py";
	private static final String DEFAULT_SCRIPT_NAME = "sys_shell" + PYTHON_SUFFIX;
	private static String restHome = null;
	
	public synchronized static ScriptManager getInstance() {
		if(instance == null) {
			instance = new ScriptManager();
		}
		return instance;
	}
	
	private ScriptManager() {
		List<ScriptEngineFactory> engines = manager.getEngineFactories();
		if (engines.isEmpty()) {
			LOG.warn("No scripting engines were found");
			return;         
		}

		StringBuffer sb = new StringBuffer();
		sb.append("\nThe following " + engines.size() + " scripting engine(s) were found");

		for (ScriptEngineFactory engine : engines) {
			sb.append("\nEngine name: " + engine.getEngineName() + "\nVersion: " + engine.getEngineVersion()+ "\nLanguage: " + engine.getLanguageName());
			List<String> extensions = engine.getExtensions();             
			if (extensions.size() > 0) {                 
				sb.append("\n\tEngine supports the following extensions:");
				for (String e : extensions) {
					sb.append("\n\t\t" + e);
				}             
			}
			List<String> shortNames = engine.getNames();
			if (shortNames.size() > 0) {
				sb.append("\n\tEngine has the following short names:");
				for (String n : engine.getNames()) {
					sb.append("\n\t\t" + n);                 
				}             
			} 
			
			String [] params =
			{
					ScriptEngine.ENGINE,
					ScriptEngine.ENGINE_VERSION,
					ScriptEngine.LANGUAGE,
					ScriptEngine.LANGUAGE_VERSION,
					ScriptEngine.NAME,
					"THREADING"
			};
			
			sb.append("\n\tEngine has the following parameters:");
			for (String param: params){
				sb.append("\n\t\t" + param + " = " + engine.getParameter(param));
			}
			sb.append("\n=========================");
		}
		LOG.debug(sb.toString());
		
		//Get -Drest.home.dir
		restHome = System.getProperty("rest.home.dir");	
		
		//Start the scripts directory watcher
		watcherWorker = new ScriptManagerWatcher ("ScriptManagerWatcher",restHome + "/bin/scripts");
	}

 	public void runScript(ScriptContext ctx) {
		String scriptName;
		
		if(ctx.getScriptName().length() == 0)
			scriptName = DEFAULT_SCRIPT_NAME;
		else if(! ctx.getScriptName().endsWith(".py"))
			scriptName = ctx.getScriptName() + PYTHON_SUFFIX;
		else
			scriptName = ctx.getScriptName();			
		
		try {
			ScriptEngine engine = manager.getEngineByName("python");
			Bindings bindings = engine.createBindings();
			bindings.put("scriptcontext", ctx); 
			if(engine instanceof Compilable) {
				CompiledScript script = m.get(scriptName);
				if(script == null) {
					LOG.info("Compiling script " + scriptName);
					Compilable compilingEngine = (Compilable)engine;
					try {
						script = compilingEngine.compile(new FileReader(restHome + "/bin/scripts/" + scriptName));
					} catch (Exception e) {
						LOG.warn(e.getMessage());
					}
					m.put(scriptName, script);
				}
				script.eval(bindings);
			} else {
				try {
					engine.eval(new FileReader(restHome + "/bin/scripts/" + scriptName), bindings);
				} catch (Exception e) {
					LOG.warn(e.getMessage());
				}
			}
		} catch (javax.script.ScriptException se) {
			LOG.warn(se.getMessage());
		}
	}

	public synchronized void removeScript(String name) {
		m.remove(name);
	}
}






 
