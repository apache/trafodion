#
# @@@ START COPYRIGHT @@@
#
#Licensed to the Apache Software Foundation (ASF) under one
#or more contributor license agreements.  See the NOTICE file
#distributed with this work for additional information
#regarding copyright ownership.  The ASF licenses this file
#to you under the Apache License, Version 2.0 (the
#"License"); you may not use this file except in compliance
#with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing,
#software distributed under the License is distributed on an
#"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#KIND, either express or implied.  See the License for the
#specific language governing permissions and limitations
#under the License.
#
# @@@ END COPYRIGHT @@@
#
#
import os
from xml.dom import minidom

dcsconfig_dir = os.environ.get('DCS_CONF_DIR')
if not dcsconfig_dir:
   name = os.environ.get('DCS_INSTALL_DIR')
   dcsconfig_dir=name+"/conf"   
doc = minidom.parse(dcsconfig_dir+"/dcs-site.xml")
props = doc.getElementsByTagName("property")
for prop in props:
        pname = prop.getElementsByTagName("name")[0]
        if (pname.firstChild.data == "dcs.master.port"):
           pvalue = prop.getElementsByTagName("value")[0]
           dcsPort=pvalue.firstChild.data
           print("%s" % (dcsPort))
        if (pname.firstChild.data == "dcs.master.floating.ip.external.ip.address"):
           pvalue = prop.getElementsByTagName("value")[0]
           float_ipaddress=pvalue.firstChild.data
           print("%s" % (float_ipaddress))
        if (pname.firstChild.data == "dcs.master.floating.ip.external.interface"):
           pvalue = prop.getElementsByTagName("value")[0]
           float_interface=pvalue.firstChild.data
           print("%s" % (float_interface))
