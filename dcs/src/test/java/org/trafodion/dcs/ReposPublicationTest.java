/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */

package org.trafodion.dcs;
 
import static org.junit.Assert.*;
import static org.junit.Assert.assertEquals;

import java.util.List;
import java.util.ArrayList;
 
import org.junit.Test;
import org.junit.experimental.categories.Category;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.dcs.*;


@Category(SmallTests.class)
public class ReposPublicationTest
{
	private static final Log LOG = LogFactory.getLog(ReposPublicationTest.class);
	
    @Test
    public void test01()
    {
        assertTrue( true );
    }
    
    @org.junit.Rule
    public org.trafodion.dcs.ResourceCheckerJUnitRule cu =
      new org.trafodion.dcs.ResourceCheckerJUnitRule();
}
