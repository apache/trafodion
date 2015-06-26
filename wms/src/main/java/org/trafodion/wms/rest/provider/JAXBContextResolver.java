/*
 * Copyright 2010 The Apache Software Foundation
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.trafodion.wms.rest.provider;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import javax.ws.rs.ext.ContextResolver;
import javax.ws.rs.ext.Provider;
import javax.xml.bind.JAXBContext;

import org.trafodion.wms.rest.model.ServerListModel;
import org.trafodion.wms.rest.model.ServerModel;
import org.trafodion.wms.rest.model.WorkloadListModel;
import org.trafodion.wms.rest.model.WorkloadModel;
import org.trafodion.wms.rest.model.StreamListModel;
import org.trafodion.wms.rest.model.StreamModel;
import org.trafodion.wms.rest.model.RuleListModel;
import org.trafodion.wms.rest.model.RuleModel;
import org.trafodion.wms.rest.model.VersionModel;

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
      ServerListModel.class,
      ServerModel.class,
 	  WorkloadListModel.class,
 	  WorkloadModel.class,
	  VersionModel.class,
	  StreamListModel.class,
	  StreamModel.class,
	  RuleListModel.class,
	  RuleModel.class
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
