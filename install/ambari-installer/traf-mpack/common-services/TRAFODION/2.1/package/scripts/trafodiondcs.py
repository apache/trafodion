# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
import subprocess
from resource_management import *
from tempfile import TemporaryFile

class DCS(Script):
  def install(self, env):
  
    # Install packages listed in metainfo.xml
    self.install_packages(env)

  def configure(self, env):
    return True

  def stop(self, env):
    import params
    Execute('source ~/.bashrc ; dcsstop',user=params.traf_user)

  # REST should run on all DCS backup and master nodes
  def start(self, env):
    import params
    Execute('source ~/.bashrc ; sqcheck -f -c rest || reststart',user=params.traf_user)
    Execute('source ~/.bashrc ; dcsstart',user=params.traf_user)
	
  # Check master pidfile
  def status(self, env):
    import status_params
    cmd = "source ~%s/.bashrc >/dev/null 2>&1; ls $DCS_INSTALL_DIR/tmp/dcs*master.pid" % status_params.traf_user
    ofile = TemporaryFile()
    try:
      Execute(cmd,stdout=ofile) # cannot switch user in status mode for some reason
    except:
      ofile.close()
      raise ComponentIsNotRunning()
    ofile.seek(0) # read from beginning
    pidfile = ofile.read().rstrip()
    ofile.close()
    check_process_status(pidfile)

if __name__ == "__main__":
  DCS().execute()
