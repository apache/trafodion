/*
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

package org.trafodion.rest.provider;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import javax.ws.rs.ext.ContextResolver;
import javax.ws.rs.ext.Provider;
import javax.xml.bind.JAXBContext;

import org.trafodion.rest.model.VersionModel;

import com.sun.jersey.api.json.JSONConfiguration;
import com.sun.jersey.api.json.JSONJAXBContext;

/**
 * Plumbing for hooking up Jersey's JSON entity body encoding and decoding
 * support to JAXB. Modify how the context is created (by using e.g. a 
 * different configuration builder) to control how JSON is processed and
 * created.
 */
@Provider
public class JAXBContextResolver implements ContextResolver<JAXBContext> {

	private final JAXBContext context;

	private final Set<Class<?>> types;

	private final Class<?>[] cTypes = {
 	  VersionModel.class
	};

	@SuppressWarnings("unchecked")
  public JAXBContextResolver() throws Exception {
		this.types = new HashSet<Class<?>>(Arrays.asList(cTypes));
		this.context = new JSONJAXBContext(JSONConfiguration.natural().build(),
		  cTypes);
	}

	@Override
	public JAXBContext getContext(Class<?> objectType) {
		return (types.contains(objectType)) ? context : null;
  }
}
